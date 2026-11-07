#!/usr/bin/env python3
"""Read-only cross-view MVC1 M68000 pose-pointer producer/consumer audit.

The 68000 FETCH stream is host-word swapped decrypted instructions, while
READ data is a separate host-word-swapped program image. Exact opcode motifs
are statically verified; they do not establish native execution, scene owner,
control semantics, frame duration or full table index bounds at runtime.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path
import sys
import zipfile

FILES = ('pose_lookup_instruction_evidence.tsv', 'pose_lookup_targets.tsv',
         'pose_block_reader_evidence.tsv', 'pose_lookup_manifest.json',
         'pose_lookup_readme.txt')
PRODUCERS = ((0x0790D8, 0x079108, 0x07910E, 0x0791A6, 0x07914A),
             (0x09C8A4, 0x09C8D4, 0x09C8DA, 0x09C978, 0x09C91C))
CONSUMERS = ((0x00208A, 0x00914000, 'FIRST_SECOND_THIRD'),
             (0x00216C, 0x00914000, 'FIRST_SKIP_SECOND_THIRD'),
             (0x0BC0D6, 0x0092E000, 'FIRST_SKIP_SECOND_THIRD'))

class AuditError(ValueError): pass

def u16(blob: bytes, addr: int) -> int:
    if not (0 <= addr <= len(blob)-2) or addr % 2:
        raise AuditError('out-of-bounds or unaligned 68000 word')
    return int.from_bytes(blob[addr:addr+2], 'little')

def u32(blob: bytes, addr: int) -> int:
    return (u16(blob, addr) << 16) | u16(blob, addr+2)

def check_words(blob: bytes, pc: int, sequence: tuple[int,...], label: str) -> None:
    if tuple(u16(blob, pc+2*i) for i in range(len(sequence))) != sequence:
        raise AuditError('unknown M68000 instruction stream at ' + label)

def decode(code: bytes, data: bytes):
    """Only exact, checked opcode signatures; not general disassembly."""
    producers=[]
    targets=[]
    for source_pc, select_pc, lookup_pc, primary, correction in PRODUCERS:
        # Source byte -> split into upper three and lower five bits.
        check_words(code, source_pc, (0x7000,0x1028,0x0063,0x0C00,0x00FF,0x6764,
                                      0x3200,0x0240,0x001F), f'{source_pc:06X}')
        check_words(code, select_pc,
           (0x342E,0x0052,0xD442,0x227C,primary>>16,primary&0xFFFF,
            0x2271,0x2000,0xEA49,0xD241,0xD241,0x2D71,0x1000,0x0040,
            0xD040,0x227B), f'{select_pc:06X}')
        if lookup_pc != select_pc+6:
            raise AuditError('producer PC layout unexpected')
        # PC-relative indexed second lookup uses the D2.W index again.
        ext=u16(code, select_pc+32)
        # In brief 68000 indexed-PC mode, PC points at extension word.
        base=select_pc+32 + (ext&255 if ext&255<128 else (ext&255)-256)
        if ((ext>>12)&7)!=2 or ext&0x8000 or base!=correction:
            raise AuditError('secondary PC-relative table identity changed')
        if u16(code, select_pc+34)!=0x3031 or u16(code, select_pc+36)!=0:
            raise AuditError('secondary word lookup changed')
        first=u32(data,primary)
        slots=(first-primary)//4
        if not (1<=slots<=128 and first>=primary+4 and
                u32(data,primary+4*(slots-1))<len(data)):
            raise AuditError('invalid first-level index topology')
        secondaries=[u32(data,primary+4*i) for i in range(slots)]
        secondary_unique=sorted(set(secondaries))
        if (len(secondary_unique)<2 or secondary_unique[0]!=first or
                any(addr%2 or addr+32>len(data) for addr in secondary_unique) or
                any(b-a!=32 for a,b in zip(secondary_unique,secondary_unique[1:]))):
            raise AuditError('invalid second-level index topology')
        correction_pointers=[u32(code, correction+4*i) for i in range(slots)]
        if any((a&1 or a+64>len(data)) for a in correction_pointers):
            raise AuditError('out-of-range correction lookup table')
        # At each first-level slot, D0's upper three bits select one of eight
        # long pointers; the lower five bits select one of 32 word entries.
        for slot, second in enumerate(secondaries):
            for upper in range(8):
                addr=second+4*upper
                target=u32(data,addr)
                if not (target%2==0 and target+10<=len(data)):
                    raise AuditError('second-level target out of original ROM')
                words=tuple(u16(data,target+2*i) for i in range(5))
                status=('FIVE_WORD_SIGNATURE' if words[0]==2 and
                        words[3] in range(0x35,0x3A) else 'OTHER_OR_UNKNOWN')
                targets.append((f'{lookup_pc:06X}',f'{primary:06X}',slot,
                   f'{primary+4*slot:06X}',f'{second:06X}',upper,
                   f'{addr:06X}',f'{target:06X}',
                   *(f'{w:04X}' for w in words),status))
        producers.append((f'{source_pc:06X}', f'{lookup_pc:06X}', f'{primary:06X}',
                 f'{select_pc:06X}', f'{correction:06X}',slots,len(secondary_unique),
                 'BYTE_$63_A0_EXCEPT_FF','FIRST_INDEX_2_TIMES_WORD_$52_A6',
                 'SECOND_INDEX_4_TIMES_BYTE_SHIFT_RIGHT_5',
                 'CONTROL_POINTER_WRITE_LONG_$40_A6',
                 'LOW_FIVE_BITS_WORD_LOOKUP_AND_ADD_$0C_A6',
                 'STATIC_INSTRUCTIONS_VALIDATED_RUNTIME_NOT_OBSERVED'))
    # Known direct consumers of the same A6+$40 field. Both FIRST cases
    # load word zero into D0 and add it to an absolute graphics RAM base.
    consumers=[]
    for pc, ram, shape in CONSUMERS:
        start=(0x286E,0x0040,0x7000,0x301C)
        check_words(code,pc,start,f'{pc:06X}')
        if pc==0x0BC0D6:
            expected=(0x227C,ram>>16,ram&0xFFFF,0xD3C0,0x49EC,0x0002)
        elif shape=='FIRST_SECOND_THIRD':
            expected=(0x43F9,ram>>16,ram&0xFFFF,0xD3C0,0x301C)
        else:
            expected=(0x43F9,ram>>16,ram&0xFFFF,0xD3C0,0x49EC,0x0002)
        check_words(code,pc+8,expected,f'{pc+8:06X}')
        consumers.append((f'{pc:06X}', 'MOVEA_L_$40_A6_INTO_A4',
               'MOVE_W_POSTINCREMENT_A4_TO_D0', f'{ram:08X}',
               'ADDA_L_D0_GRAPHICS_RAM_BASE',shape,
               'CONTROL_BLOCK_START_READ_STATIC_NOT_PROVEN_EXECUTED'))
    return producers, targets, consumers

def write_tsv(path:Path, fields, rows) -> None:
    with path.open('w', newline='',encoding='utf-8') as f:
        writer=csv.writer(f,delimiter='\t')
        writer.writerow(fields)
        writer.writerows(rows)

def report(trace_root:Path, pose_report:Path, output:Path) -> dict:
    trace_root=trace_root.resolve();pose_report=pose_report.resolve()
    output=output.resolve()
    if not pose_report.is_file(): raise AuditError('original pose manifest is missing')
    for src in (trace_root,pose_report,pose_report.parent):
        if output==src or output in src.parents or src in output.parents:
            raise AuditError('output overlaps source or private program')
    if (trace_root/'mvc_program_data_host_words.bin').is_file():
        run=trace_root
    else:
        runs=sorted((p for p in trace_root.glob('run_*') if p.is_dir()),
                  key=lambda p:int(p.name.rsplit('_',1)[-1]))
        run=next((p for p in reversed(runs) if
                    (p/'mvc_program_data_host_words.bin').is_file() and
                    (p/'mvc_opcodes_decrypted_host_words.bin').is_file()),None)
    if run is None:raise AuditError('no original MVC READ/FETCH image pair')
    code_path=run/'mvc_opcodes_decrypted_host_words.bin'
    data_path=run/'mvc_program_data_host_words.bin'
    if not code_path.is_file():raise AuditError('missing decrypted FETCH image')
    if (code_path.stat().st_size > 16*1024*1024 or
            data_path.stat().st_size > 16*1024*1024):
        raise AuditError('unexpected source image size')
    code=code_path.read_bytes();data=data_path.read_bytes()
    if len(data)%2 or len(code)%2:raise AuditError('bad Motorola source byte alignment')
    pose=json.loads(pose_report.read_text(encoding='utf-8'))
    if (pose.get('format')!='MVC1_68000_CANDIDATE_POSE_RECORDS_V1' or
           pose.get('private_rom_sha256_for_note_matching')!=hashlib.sha256(data).hexdigest()):
        raise AuditError('STALE original 68000 pose evidence or ROM hash mismatch')
    producers, targets, consumers=decode(code,data)
    output.mkdir(parents=True,exist_ok=True)
    write_tsv(output/FILES[0],('source_split_pc','lookup_pc','first_table',
       'first_index_setup_pc','pc_indexed_adjustment_table','first_level_slots',
       'unique_second_level_lists','input_source','first_level_index',
       'second_level_index','control_address_store','other_lookup',
       'evidence_status'),producers)
    write_tsv(output/FILES[1],('producer_pc','first_table','first_slot',
       'first_entry','secondary_list','upper_three_index','secondary_entry',
       'five_word_block_address', 'word0','word1','word2','word3','word4',
       'block_grammar'),targets)
    write_tsv(output/FILES[2],('static_reader_pc','pointer_source',
       'initial_read','graphics_ram_base','operation','subsequent_reads',
       'evidence_status'),consumers)
    sha=hashlib.sha256(data).hexdigest()
    unique=sorted({int(row[7],16) for row in targets})
    manifest={'format':'MVC1_M68000_POSE_POINTER_PRODUCER_CONSUMER_STATIC_V1',
       'private_original_read_sha256':sha,
       'decrypted_fetch_sha256':hashlib.sha256(code).hexdigest(),
       'producer_motifs_verified':len(producers),
       'consumer_motifs_verified':len(consumers),
       'indexed_leaf_occurrences':len(targets),
       'distinct_pointer_target_blocks':len(unique),
       'five_word_signature_leaf_occurrences':sum(row[-1]=='FIVE_WORD_SIGNATURE' for row in targets),
       'all_targets_are_executed':False,
       'control_block_start_is_read_by_consumer_code_static':True,
       'actual_producer_consumer_runtime_flow_verified':False,
       'stage_identity_verified':False,'frame_duration_verified':False,
       'fourth_fifth_word_semantics_verified':False,
       'method':'source-byte exact CPU opcode signatures, independent FETCH/READ views, exact ROM index entries',
       'privacy':'No program binary, decrypted instructions, graphics, raw OBJ logs in ZIP.'}
    (output/FILES[3]).write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (output/FILES[4]).write_text(
       'MVC1 Original Motorola 68000 object data-pointer lifecycle (STATIC)\n'
       'Decrypted FETCH reads byte 0x63(A0), splits high three and low five bits.\n'
       'High three bits index eight 32-bit pointers in the selected table.\n'
       'Word at 0x52(A6) determines first-level table byte offset (times 2).\n'
       'Selected long pointer is written to 0x40(A6).\n'
       'Other lower-five-bit word table contributes to 0x0C(A6); meaning not established.\n'
       'Three original reader opcode motifs load the stored long as A4 then\n'
       'read the FIRST WORD of the pointed control block. Thus the block\n'
       'is a pointer-targeted READ START, not exclusively a trailing footer.\n'
       'The fourth and fifth control words are preserved without interpretation.\n'
       'Executed reachability and frame timing remain UNVERIFIED.\n'
       'No private ROM/decrypted instructions or original gfx are exported.\n',
       encoding='utf-8')
    out=output/'pose_pointer_consumer_review.zip';tmp=output/'pose_pointer_consumer_review.zip.tmp'
    with zipfile.ZipFile(tmp,'w',zipfile.ZIP_DEFLATED) as z:
        for name in FILES:z.write(output/name,arcname=name)
    tmp.replace(out)
    return manifest

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--pose-manifest',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    try:
        m=report(args.trace_root,args.pose_manifest,args.output)
        print('MVC_POSE_POINTER_CONSUMERS producers={producer_motifs_verified} consumers={consumer_motifs_verified} targets={indexed_leaf_occurrences} unique_blocks={distinct_pointer_target_blocks}'.format(**m))
        print('MVC_POSE_POINTER_CONSUMERS_ZIP',args.output/'pose_pointer_consumer_review.zip')
        return 0
    except (OSError,ValueError,zipfile.BadZipFile) as exc:
        print('MVC_POSE_POINTER_CONSUMERS_ERROR',exc,file=sys.stderr)
        return 2

if __name__=='__main__':raise SystemExit(main())
