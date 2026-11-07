#!/usr/bin/env python3
"""Offline MVC1 M68000 pose-footer interpreter leads, not a timing decoder.

Correlates hash-verified original ROM pose records with aligned pointer values
in the original READ image and exact opcode motifs in the decrypted FETCH
image. An aligned word sequence, even in FETCH memory, is NOT proof of an
executed instruction. No ROM bytes or instruction windows leave the machine.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import sys
import zipfile

import TRACE_MVC_STAGING as staging

OUTPUTS = (
    'pose_footer_address_references.tsv',
    'pose_footer_pointer_blocks.tsv',
    'pose_footer_selector_motifs.tsv',
    'pose_footer_dispatch_leads.tsv',
    'pose_footer_interpreter_manifest.json',
    'pose_footer_interpreter_readme.txt',
    'pose_footer_two_level_index.tsv',
)
SELECTORS = frozenset(range(0x35, 0x3A))


class InterpreterError(ValueError):
    pass


def word(data: bytes, address: int) -> int:
    if address < 0 or address + 2 > len(data) or address & 1:
        raise InterpreterError('unaligned or out-of-range host-word address')
    return data[address] | (data[address + 1] << 8)


def signed8(value: int) -> int:
    return value - 256 if value & 0x80 else value


def tsv(path: Path, fields, rows) -> None:
    with path.open('w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f, delimiter='\t')
        writer.writerow(fields)
        writer.writerows(rows)


def read_records(path: Path, data: bytes) -> tuple[dict, dict]:
    manifest_path = path.with_name('rom_pose_manifest.json')
    if not path.is_file() or not manifest_path.is_file():
        raise InterpreterError('existing pose TSV and manifest required; run RUN_TEST.bat poses')
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest.get('format') != 'MVC1_68000_CANDIDATE_POSE_RECORDS_V1':
        raise InterpreterError('unexpected pose-record manifest format')
    if manifest.get('private_rom_sha256_for_note_matching') != hashlib.sha256(data).hexdigest():
        raise InterpreterError('pose records belong to a DIFFERENT program image; refuse stale joins')
    addresses = {}
    records = {}
    with path.open('r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        needed = {'record_offset_hex', 'chain_start_hex', 'tile_attribute_pairs',
                  'footer_offset_hex', *('control_%d_hex' % i for i in range(5))}
        if not reader.fieldnames or not needed.issubset(reader.fieldnames):
            raise InterpreterError('missing original pose-record address/control columns')
        for row in reader:
            try:
                start = int(row['record_offset_hex'], 16)
                chain = int(row['chain_start_hex'], 16)
                count = int(row['tile_attribute_pairs'])
                footer_at = int(row['footer_offset_hex'], 16)
                values = tuple(int(row['control_%d_hex' % i], 16) for i in range(5))
            except (ValueError, TypeError, KeyError) as e:
                raise InterpreterError('invalid pose metadata fields') from e
            if start & 1 or start < 0 or count < 2 or footer_at != start + 4 * count:
                raise InterpreterError('invalid pose record boundaries')
            if footer_at + 10 > len(data):
                raise InterpreterError('pose record exceeds original data')
            if values != tuple(word(data, footer_at + i * 2) for i in range(5)):
                raise InterpreterError('pose footer does not match original ROM')
            if values[0] != 2 or values[3] not in SELECTORS:
                raise InterpreterError('pose footer grammar changed')
            if start in records or start in addresses or footer_at in addresses:
                raise InterpreterError('duplicate or overlapping pose address')
            records[start] = (chain, footer_at, values[3])
            addresses[start] = (start, chain, 'RECORD_START')
            addresses[footer_at] = (start, chain, 'FIVE_WORD_FOOTER')
    if len(records) != manifest.get('records_with_repeating_control_footers'):
        raise InterpreterError('pose manifest record count does not match TSV')
    return records, addresses


def instruction_address_kind(op: int) -> str | None:
    # These are exact Motorola 68000 opcodes, not a guessed MOVEM/register
    # interpretation. The following TWO words carry a 32-bit operand.
    for reg in range(8):
        if op == 0x207C + (reg << 9):
            return 'MOVEA_L_IMMEDIATE_A%d' % reg
        if op == 0x41F9 + (reg << 9):
            return 'LEA_ABSOLUTE_L_A%d' % reg
        if op == 0x203C + (reg << 9):
            return 'MOVE_L_IMMEDIATE_D%d' % reg
    return {0x4879: 'PEA_ABSOLUTE_L', 0x4EB9: 'JSR_ABSOLUTE_L',
            0x4EF9: 'JMP_ABSOLUTE_L'}.get(op)


def exact_pointer_sites(data: bytes, code: bytes, addresses: dict):
    """Return only exact aligned 32-bit values resolving to known ROM records.

    READ values are untyped data; FETCH instruction decoding accepts only
    specific immediate/absolute-long forms. Both remain static candidates.
    """
    result = []
    for name, blob in (('READ', data), ('FETCH', code)):
        for at in range(0, len(blob) - 3, 2):
            address = (word(blob, at) << 16) | word(blob, at + 2)
            if address not in addresses:
                continue
            record, chain, role = addresses[address]
            opcode_pc = at - 2
            kind = instruction_address_kind(word(blob, opcode_pc)) if name == 'FETCH' and opcode_pc >= 0 else None
            result.append({
                'source': name, 'offset': at, 'target': address,
                'record': record, 'chain': chain, 'role': role,
                'instruction_pc': opcode_pc if kind else None,
                'kind': kind or ('ALIGNED_LONG_IN_READ_DATA' if name == 'READ'
                                else 'UNCLASSIFIED_FETCH_WORDS'),
            })
    return result


def read_pointer_blocks(pointers: list[dict], records: dict):
    """Find adjacent exact READ-address references at four-byte intervals.

    The observed run may start/end in the MIDDLE of a larger ROM table:
    unknown pointers on either side are not declared table delimiters.
    Duplicates prove repeated ordered pointer values, not shared execution.
    """
    hits = [p for p in pointers if p['source'] == 'READ']
    runs = []
    current = []
    for hit in hits:
        if current and hit['offset'] != current[-1]['offset'] + 4:
            if len(current) >= 4:
                runs.append(current)
            current = []
        current.append(hit)
    if len(current) >= 4:
        runs.append(current)
    same_order = {}
    for run in runs:
        signature = tuple(hit['target'] for hit in run)
        same_order.setdefault(signature, []).append(run[0]['offset'])
    rows = []
    for run in runs:
        sequence = tuple(hit['target'] for hit in run)
        other = [f'{address:06X}' for address in same_order[sequence]
                 if address != run[0]['offset']]
        controls = sorted({records[hit['record']][2] for hit in run})
        rows.append((f"{run[0]['offset']:06X}",
                     f"{run[-1]['offset']:06X}", len(run),
                     len(set(sequence)),
                     sum(hit['role'] == 'FIVE_WORD_FOOTER' for hit in run),
                     sum(hit['role'] == 'RECORD_START' for hit in run),
                     len({hit['chain'] for hit in run}),
                     ','.join(f'{v:04X}' for v in controls),
                     ','.join(other),
                     'OBSERVED_ALIGNED_POINTER_SUBSEQUENCE_NOT_PROVEN_TABLE_BOUNDS'))
    return runs, rows


def two_level_pointer_indices(data: bytes, code: bytes, addresses: dict):
    """Source-aligned candidate MOVEA.L #table,A1 / indexed MOVEA.L trail.

    Only the exact mirrored opcode prefix is classified. A table is accepted
    when its FIRST pointer determines a bounded top-level slot count, its
    second-level list starts are regularly spaced, and most leaf pointers
    match catalogued five-word blocks OR the original footer grammar. Neither
    opcode presence nor table topology proves this branch executes in MVC1.
    """
    def long(blob: bytes, at: int) -> int:
        return (word(blob, at) << 16) | word(blob, at + 2)

    sources = []
    rows = []
    for pc in range(0, len(code) - 10, 2):
        if (word(code, pc) != 0x227C or word(code, pc + 6) != 0x2271
                or word(code, pc + 8) != 0x2000):
            continue
        table = long(code, pc + 2)
        if table < 0 or table + 32 > len(data) or table & 1:
            continue
        first = long(data, table)
        span = first - table
        if span < 16 or span > 512 or span % 4:
            continue
        num_slots = span // 4
        if table + num_slots * 4 > len(data):
            continue
        lists = [long(data, table + 4 * i) for i in range(num_slots)]
        unique = sorted(set(lists))
        if (len(unique) < 4 or unique[0] != first or
                any(a & 1 or a < first or a + 16 > len(data) for a in unique)):
            continue
        steps = [b - a for a, b in zip(unique, unique[1:])]
        if len(set(steps)) != 1 or not 16 <= steps[0] <= 128 or steps[0] % 4:
            continue
        stride = steps[0]
        if unique[-1] + stride > len(data):
            continue
        candidate_rows = []
        accepted = 0
        known = 0
        grammar_only = 0
        unknown = 0
        for slot, second in enumerate(lists):
            for leaf_slot in range(stride // 4):
                ref_at = second + leaf_slot * 4
                target = long(data, ref_at)
                entry = addresses.get(target)
                is_known = bool(entry and entry[2] == 'FIVE_WORD_FOOTER')
                is_grammar = bool(target % 2 == 0 and 0 <= target <= len(data) - 10
                                  and word(data, target) == 2
                                  and word(data, target + 6) in SELECTORS)
                if is_known:
                    status = 'CATALOGUED_EXACT_FIVE_WORD_FOOTER'
                    known += 1
                    accepted += 1
                elif is_grammar:
                    status = 'ORIGINAL_FOOTER_GRAMMAR_NOT_IN_POSE_CATALOG'
                    grammar_only += 1
                    accepted += 1
                else:
                    status = 'NO_MATCH_UNVERIFIED_REFERENCE'
                    unknown += 1
                words = tuple(word(data, target + 2 * k) for k in range(5)) if is_grammar else None
                candidate_rows.append((f'{pc:06X}', f'{table:06X}', slot,
                    f'{table + 4 * slot:06X}', f'{second:06X}', leaf_slot,
                    f'{ref_at:06X}', f'{target:06X}',
                    *(tuple(f'{w:04X}' for w in words) if words else ('', '', '', '', '')),
                    status))
        if not candidate_rows or accepted * 4 < len(candidate_rows) * 3:
            continue
        sources.append({'fetch_pc': pc, 'table': table, 'first_level_slots': num_slots,
                        'unique_second_level_lists': len(unique),
                        'second_level_stride': stride, 'leaf_pointers': len(candidate_rows),
                        'catalogued_footers': known, 'grammar_only_footers': grammar_only,
                        'unresolved_leaf_pointers': unknown})
        rows.extend(candidate_rows)
    return sources, rows


def selector_sites(code: bytes):
    """Find exact immediate selector comparisons / normalization motifs.

    Scan is intentionally NOT a control-flow disassembler. Decrypted opcode
    regions may contain extension words or inline data which mimic opcodes.
    """
    selectors = []
    guards = []
    jumps = []
    for pc in range(0, len(code) - 3, 2):
        op = word(code, pc)
        imm = word(code, pc + 2)
        reg = op & 7
        kind = None
        if (op & 0xFFF8) == 0x0C40:
            kind = 'CMPI_W_D%d' % reg
            if imm == 4:
                guards.append((pc, reg))
        elif (op & 0xFFF8) == 0x0440:
            kind = 'SUBI_W_D%d' % reg
        elif (op & 0xFFF8) == 0x0640:
            kind = 'ADDI_W_D%d' % reg
        elif op in tuple(0x303C + (r << 9) for r in range(8)):
            reg = (op - 0x303C) >> 9
            kind = 'MOVE_W_IMMEDIATE_D%d' % reg
        elif (op & 0xF100) == 0x7000:
            # MOVEQ embeds the signed 8-bit immediate in the opcode.
            # Keep it as a static motif, not a verified selector consumer.
            reg = (op >> 9) & 7
            kind = 'MOVEQ_IMMEDIATE_D%d' % reg
            imm = op & 0xFF
        if kind and imm in SELECTORS:
            selectors.append({'pc': pc, 'reg': reg, 'value': imm, 'kind': kind})
        # JMP/JSR (d8,PC,Xn). Index size/scale and the pointed-to bytes
        # require further inspection; only the static opcode form is retained.
        if op in (0x4EFB, 0x4EBB):
            ext = imm
            jumps.append({'pc': pc,
                          'kind': 'JMP_PC_INDEXED' if op == 0x4EFB else 'JSR_PC_INDEXED',
                          'table_base': pc + 2 + signed8(ext & 0xFF),
                          'register': ('A' if ext & 0x8000 else 'D') + str((ext >> 12) & 7),
                          'index_long': bool(ext & 0x0800)})
    return selectors, guards, jumps


def inspect(data: bytes, code: bytes, records: dict, addresses: dict):
    pointers = exact_pointer_sites(data, code, addresses)
    selectors, guards, jumps = selector_sites(code)
    # Keep computed-jump leads near selector comparisons. Never claim
    # branch reachability or that a selector operand came from the footer.
    leads = []
    for s in selectors:
        near_jump = min((j for j in jumps if 0 <= j['pc'] - s['pc'] <= 128),
                        key=lambda j: j['pc'] - s['pc'], default=None)
        near_ptr = min((p for p in pointers
                        if p['source'] == 'FETCH' and p['instruction_pc'] is not None
                        and abs(p['instruction_pc'] - s['pc']) <= 512),
                       key=lambda p: abs(p['instruction_pc'] - s['pc']), default=None)
        guard = min((pc for pc, reg in guards if reg == s['reg']
                     and 0 < pc - s['pc'] <= 32),
                    default=None)
        leads.append({
            'selector': s, 'jump': near_jump, 'pointer': near_ptr, 'guard': guard,
            'classification': 'STATIC_SELECTOR_AND_INDEXED_DISPATCH_LEAD'
                if near_jump and s['kind'].startswith(('CMPI', 'SUBI'))
                else 'UNLINKED_SELECTOR_IMMEDIATE_MOTIF',
        })
    return pointers, selectors, leads


def report(trace_root: Path, pose_records: Path, output: Path) -> dict:
    root = trace_root.resolve()
    poses = pose_records.resolve()
    dest = output.resolve()
    for source in (root, poses, poses.parent):
        if dest == source or dest in source.parents or source in dest.parents:
            raise InterpreterError('output overlaps existing private or pose source')
    run = staging.latest_run(root)
    if run is None or not (run / staging.BIN_DATA).is_file():
        raise InterpreterError('decrypted FETCH and original READ images required locally')
    code_path, data_path = run / staging.BIN_CODE, run / staging.BIN_DATA
    if not (0 < code_path.stat().st_size <= 16 * 1024 * 1024 and
            0 < data_path.stat().st_size <= 16 * 1024 * 1024):
        raise InterpreterError('invalid source image sizes')
    code, data = code_path.read_bytes(), data_path.read_bytes()
    if len(code) & 1 or len(data) & 1:
        raise InterpreterError('invalid host-word-swapped source alignment')
    records, addresses = read_records(poses, data)
    pointers, selectors, leads = inspect(data, code, records, addresses)
    blocks, block_rows = read_pointer_blocks(pointers, records)
    indexed_sources, indexed_rows = two_level_pointer_indices(data, code, addresses)
    dest.mkdir(parents=True, exist_ok=True)
    tsv(dest / OUTPUTS[0],
        ('source_view', 'aligned_long_offset_hex', 'referenced_rom_address_hex',
         'pose_record_hex', 'chain_start_hex', 'target_kind',
         'candidate_instruction_pc_hex', 'exact_opcode_form_or_unclassified',
         'interpretation'),
        ((p['source'], f"{p['offset']:06X}", f"{p['target']:06X}",
          f"{p['record']:06X}", f"{p['chain']:06X}", p['role'],
          f"{p['instruction_pc']:06X}" if p['instruction_pc'] is not None else '',
          p['kind'], 'STATIC_ADDRESS_MATCH_NOT_EXECUTION') for p in pointers))
    tsv(dest / OUTPUTS[1],
        ('first_reference_offset_hex', 'last_reference_offset_hex',
         'consecutive_32bit_references', 'distinct_target_addresses',
         'control_footer_targets', 'tile_pair_start_targets',
         'candidate_chains_referenced', 'raw_footer_selectors',
         'same_order_at_other_offsets_hex', 'boundary_status'), block_rows)
    tsv(dest / OUTPUTS[2],
        ('fetch_pc_hex', 'literal_selector_hex', 'register', 'opcode_motif',
         'interpretation'),
        ((f"{s['pc']:06X}", f"{s['value']:04X}", 'D%d' % s['reg'],
          s['kind'], 'STATIC_DECRYPTED_FETCH_WORDS_NOT_CONFIRMED_INSTRUCTION')
         for s in selectors))
    tsv(dest / OUTPUTS[3],
        ('selector_motif_pc_hex', 'selector_value_hex', 'selector_opcode_form',
         'nearby_indexed_jump_pc_hex', 'nearby_indexed_jump_form',
         'computed_table_base_hex', 'same_register_cmpi_4_guard_pc_hex',
         'nearby_record_address_load_pc_hex', 'nearby_record_hex',
         'classification', 'verification_status'),
        ((f"{l['selector']['pc']:06X}", f"{l['selector']['value']:04X}",
          l['selector']['kind'],
          f"{l['jump']['pc']:06X}" if l['jump'] else '',
          l['jump']['kind'] if l['jump'] else '',
          f"{l['jump']['table_base']:06X}" if l['jump'] else '',
          f"{l['guard']:06X}" if l['guard'] is not None else '',
          f"{l['pointer']['instruction_pc']:06X}" if l['pointer'] else '',
          f"{l['pointer']['record']:06X}" if l['pointer'] else '',
          l['classification'], 'UNVERIFIED_STATIC_MOTIFS_ONLY')
         for l in leads))
    tsv(dest / OUTPUTS[6],
        ('fetch_motif_pc_hex', 'first_table_base_hex', 'first_level_slot',
         'first_level_entry_offset_hex', 'second_level_list_base_hex',
         'second_level_slot', 'second_level_entry_offset_hex',
         'target_original_data_hex', 'footer_word_0_hex', 'footer_word_1_hex',
         'footer_word_2_hex', 'footer_word_3_hex', 'footer_word_4_hex',
         'target_provenance'), indexed_rows)
    manifest = {
        'format': 'MVC1_M68K_POSE_FOOTER_INTERPRETER_LEADS_V1',
        'original_rom_sha256': hashlib.sha256(data).hexdigest(),
        'decrypted_fetch_sha256': hashlib.sha256(code).hexdigest(),
        'pose_records_verified_against_original_rom': len(records),
        'exact_aligned_address_matches': len(pointers),
        'two_level_index_code_motifs': len(indexed_sources),
        'two_level_index_evidence': indexed_sources,
        'two_level_index_leaf_rows': len(indexed_rows),
        'two_level_index_execution_verified': False,
        'read_pointer_blocks_min_four_refs': len(blocks),
        'read_pointer_block_references': sum(map(len, blocks)),
        'longest_read_pointer_block': max(map(len, blocks), default=0),
        'references_to_control_footer': sum(p['role'] == 'FIVE_WORD_FOOTER' for p in pointers),
        'references_to_tile_pair_start': sum(p['role'] == 'RECORD_START' for p in pointers),
        'distinct_control_footer_addresses_referenced': len({p['target'] for p in pointers if p['role'] == 'FIVE_WORD_FOOTER'}),
        'read_data_address_matches': sum(p['source'] == 'READ' for p in pointers),
        'decrypted_fetch_address_matches': sum(p['source'] == 'FETCH' for p in pointers),
        'recognized_fetch_address_load_motifs': sum(p['instruction_pc'] is not None for p in pointers),
        'selector_immediate_opcode_motifs': len(selectors),
        'selector_near_indexed_jump_motifs': sum(l['jump'] is not None for l in leads),
        'selector_near_record_address_load_motifs': sum(l['pointer'] is not None for l in leads),
        'footer_field_semantics_decoded': False,
        'original_68000_interpreter_execution_verified': False,
        'animation_frame_duration_verified': False,
        'stage_actor_ownership_verified': False,
        'privacy': 'Metadata and addresses only; no private ROM, decrypted code, decoded artwork or instruction dumps.',
        'method': 'Aligned 32-bit exact pointer matches; Motorola 68000 immediate opcode motifs and nearby indexed JMP/JSR. No full instruction boundary proof or control/data-flow proof.',
    }
    (dest / OUTPUTS[4]).write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    (dest / OUTPUTS[5]).write_text(
        'MVC1 ORIGINAL 68000 FOOTER INTERPRETER LEADS - STATIC ONLY\n'
        'Original ROM record footers are hash-checked against their generating pose report.\n'
        '$0035..$0039 have NOT been demonstrated to be opcodes, frame clocks or branches.\n'
        'READ addresses are data pointers; FETCH motifs may contain extensions or embedded data.\n'
        'Observed 4-byte pointer blocks may be partial subsequences with unknown neighbors.\n'
        'A separate two-level index lists static MOVEA/READ-table/READ-footer links.\n'
        'Footer-shaped records missing from the sparse pose catalog remain visible.\n'
        'Pointer addresses overwhelmingly target the five-word block, not tile-pair starts.\n'
        'A nearby selector/JMP or record address load is a RESEARCH LEAD, not a decoded parser.\n'
        'To establish semantics, disassemble validated reachable instructions and observe actual\n'
        'source-ROM reads / dispatch PCs while MVC1 changes pose at native-frame resolution.\n'
        'No timing, actor category or stage identity is inferred from static proximity.\n'
        'No original code, ROM, artwork, or object observations are shared in this ZIP.\n',
        encoding='utf-8')
    bundle = dest / 'pose_footer_interpreter_leads.zip'
    tmp = bundle.with_suffix('.zip.tmp')
    with zipfile.ZipFile(tmp, 'w', zipfile.ZIP_DEFLATED) as z:
        for name in OUTPUTS:
            z.write(dest / name, arcname=name)
    tmp.replace(bundle)
    return manifest


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace-root', type=Path, required=True)
    p.add_argument('--pose-records', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    try:
        m = report(a.trace_root, a.pose_records, a.output)
        print('MVC_FOOTER_LEADS records={pose_records_verified_against_original_rom} pointers={exact_aligned_address_matches} selectors={selector_immediate_opcode_motifs} near_jumps={selector_near_indexed_jump_motifs}'.format(**m))
        print('MVC_FOOTER_LEADS_BUNDLE', a.output / 'pose_footer_interpreter_leads.zip')
        return 0
    except (OSError, ValueError, InterpreterError, zipfile.BadZipFile) as e:
        print('MVC_FOOTER_LEADS_ERROR', e, file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
