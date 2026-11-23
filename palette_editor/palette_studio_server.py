#!/usr/bin/env python3
"""XMVSF Palette Studio: local-only ROM palette browser and profile editor.

Only reads the original CPS-2 xvs.05a program ROM. Never patches a ROM,
replaces an original colour. Save All publishes a nine-slot selection catalog
beside the recomp's settings; the game-side X/Y reader is a separate feature.
Python standard library only; the browser provides the editing interface.
"""
from __future__ import annotations

import hashlib
import io
import json
import os
from pathlib import Path
import re
import secrets
import subprocess
import sys
import tempfile
import threading
import urllib.parse
import webbrowser
import zipfile
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parents[1]
HERE = Path(__file__).resolve().parent
PAGE = HERE / "palette_studio.html"
PROFILES = HERE / "profiles"
# Generated on explicit Save All; consumed by a future native X/Y selector.
GAME_CATALOG = ROOT / "recomp" / "work" / "xmvsf" / "palettes" / "selection_slots.json"
MAX_CUSTOM_SLOTS = 7
TOTAL_SELECT_SLOTS = 9
SPRITES = HERE / "sprites"
PORTRAITS = ROOT / "_art" / "menu" / "characterselect" / "portraits"
# Model viewer uses PlayerFolder()/exports. The shipped Windows RECOMP_RUN.bat
# gives --eeprom "work\xmvsf\eeprom.bin", but PlayerFolder() currently
# searches only for '/'. That can place exports at recomp/exports instead of
# recomp/work/xmvsf/exports. Read both, and accept an explicit override.
VIEWER_EXPORTS = (
    (Path(os.environ["XMVSF_MODEL_EXPORT_DIR"]),)
    if os.environ.get("XMVSF_MODEL_EXPORT_DIR") else
    (ROOT / "recomp" / "exports" / "palette_studio",
     ROOT / "recomp" / "exports",
     ROOT / "recomp" / "work" / "xmvsf" / "exports")
)

# These constants match recomp/games/xmvsf/src/xmvsf.h and game.cfg.
CHARACTERS = (
    "Wolverine", "Cyclops", "Storm", "Rogue", "Gambit",
    "Sabretooth", "Juggernaut", "Magneto", "Apocalypse",
    "Ryu", "Ken", "Chun-Li", "Dhalsim", "Zangief",
    "M. Bison", "Akuma", "Charlie", "Cammy", "Chun-Li (Alpha)"
)
PROGRAM_START = 0x100000
TABLE_ADDRESS = 0x1145A8
SET_SIZE = 0x60
LINES = 3
PENS = 16
ROM_SIZE = 0x80000
ORIGINAL_CRC = 0x7DB6025D
SOURCE_MAX = 64 * 1024 * 1024
SPRITE_MAX = 8 * 1024 * 1024
TOKEN = secrets.token_urlsafe(32)
COLOUR = re.compile(r"^#[0-9A-F]{6}$")
PROFILE_ID = re.compile(r"^[0-9a-f]{16}$")
DRAFT_KEY = re.compile(r"^draft-[0-9]{1,8}$")
SPRITE_NAME = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9_.-]{0,90}\.png$", re.I)
SAVE_LOCK = threading.RLock()
CAPTURE_LOCK = threading.RLock()
CAPTURE_JOB = {"running": False, "status": "idle", "message": "No export started",
               "requested": "", "exported": 0, "log": ""}
SOURCE = None
SOURCE_WARNING = ''


def slug(index: int) -> str:
    name = CHARACTERS[index]
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def character(index: object) -> int:
    if isinstance(index, bool):
        raise ValueError("Invalid character")
    try:
        number = int(index)
    except (TypeError, ValueError) as exc:
        raise ValueError("Invalid character") from exc
    if number < 0 or number >= len(CHARACTERS):
        raise ValueError("Unknown character")
    return number


def checked_profile_id(value: str) -> str:
    if not isinstance(value, str) or not PROFILE_ID.fullmatch(value):
        raise ValueError("Invalid profile identifier")
    return value


def quantized_hex(word: int) -> str:
    return "#" + "".join(f"{((word >> shift) & 15) * 17:02X}" for shift in (8, 4, 0))


def extract_program(content: bytes) -> bytes:
    """Accept the exact 512 KiB ROM or a ROM-set ZIP containing xvs.05a."""
    if len(content) > SOURCE_MAX:
        raise ValueError("ROM source exceeds maximum upload size")
    if content.startswith(b"PK\x03\x04"):
        with zipfile.ZipFile(io.BytesIO(content)) as archive:
            found = [entry for entry in archive.infolist()
                     if not entry.is_dir() and Path(entry.filename).name.lower() == "xvs.05a"]
            if len(found) != 1:
                raise ValueError("ZIP must contain exactly one xvs.05a file")
            if found[0].file_size != ROM_SIZE:
                raise ValueError("xvs.05a in ZIP has an unexpected size")
            content = archive.read(found[0])
    if len(content) != ROM_SIZE:
        raise ValueError("Expected xvs.05a (512 KiB) or its ROM-set ZIP")
    crc = zlib.crc32(content)
    if crc != ORIGINAL_CRC:
        raise ValueError(f"ROM CRC {crc:08X} differs from original 7DB6025D; refusing modified source")
    return content


def decode_palettes(program: bytes, *, verify_crc: bool = True) -> tuple[list, list]:
    """Return original RGB swatches and exact 68000 program addresses.

    Source xvs.05a stores 68000 words LOW byte first. The recomp
    explicitly swaps each pair in machine/romset.cpp before Program()
    exposes the character palette pointer table. Decode raw ROM bytes
    with that exact per-word conversion, never treat raw as native big-endian.
    Each pointer leads to [P: 3x16 colours][K: 3x16 colours].
    The high nibble is native brightness/metadata, retained by the game.
    """
    if len(program) != ROM_SIZE:
        raise ValueError("Invalid program length")
    if verify_crc and zlib.crc32(program) != ORIGINAL_CRC:
        raise ValueError("The original xvs.05a CRC is required")
    table = TABLE_ADDRESS - PROGRAM_START
    colours, addresses = [], []
    for who in range(len(CHARACTERS)):
        # romset.cpp: for each word, set.program += [raw[i+1], raw[i]]
        # Two native big-endian words hold one four-byte 68000 pointer.
        pointer = program[table + who * 4:table + who * 4 + 4]
        base = int.from_bytes(bytes((pointer[1], pointer[0],
                                     pointer[3], pointer[2])), "big")
        if not PROGRAM_START <= base or base + SET_SIZE * 2 > PROGRAM_START + ROM_SIZE:
            raise ValueError(f"{CHARACTERS[who]} palette pointer 0x{base:06X} outside program ROM")
        sets, addrs = [], []
        for set_no in range(2):
            rows, row_addresses = [], []
            for line in range(LINES):
                row, addr_row = [], []
                for pen in range(PENS):
                    addr = base + set_no * SET_SIZE + line * PENS * 2 + pen * 2
                    off = addr - PROGRAM_START
                    word = int.from_bytes(program[off:off + 2], "little")
                    row.append(quantized_hex(word))
                    addr_row.append(f"0x{addr:06X}")
                rows.append(row)
                row_addresses.append(addr_row)
            sets.append(rows)
            addrs.append(row_addresses)
        colours.append(sets)
        addresses.append(addrs)
    return colours, addresses


def load_source(raw: bytes, label: str) -> dict:
    global SOURCE
    program = extract_program(raw)
    palettes, addresses = decode_palettes(program)
    item = {"label": label, "sha256": hashlib.sha256(program).hexdigest(),
            "crc32": f"{ORIGINAL_CRC:08X}", "originals": palettes, "addresses": addresses}
    with SAVE_LOCK:
        SOURCE = item
    return item


def auto_source() -> str:
    bases = (ROOT / "recomp" / "rom", ROOT / "rom")
    options = []
    for folder in bases:
        options += [
            folder / "xmvsf" / "xvs.05a",
            folder / "xmvsf" / "rom" / "xvs.05a",
            folder / "xmvsf" / "xmvsf.zip",
            folder / "xmvsf" / "rom" / "xmvsf.zip",
            folder / "xmvsf.zip",
            folder / "xvs.05a",
        ]
    errors = []
    for candidate in options:
        if candidate.is_file() and not candidate.is_symlink():
            try:
                load_source(candidate.read_bytes(), str(candidate.relative_to(ROOT)))
                return ""
            except (OSError, ValueError, zipfile.BadZipFile) as exc:
                errors.append(f"{candidate.name}: {exc}")
    if errors:
        return "; ".join(errors)
    return "Original xvs.05a not found automatically. Choose your xvs.05a or xmvsf.zip above."


def valid_colours(value: object) -> list:
    if not isinstance(value, list) or len(value) != 2:
        raise ValueError("Profile must have two sets (P and K)")
    result = []
    for palette_set in value:
        if not isinstance(palette_set, list) or len(palette_set) != LINES:
            raise ValueError("Each set requires three palette lines")
        rows = []
        for row in palette_set:
            if not isinstance(row, list) or len(row) != PENS:
                raise ValueError("Each line requires sixteen colors")
            converted = []
            for cell in row:
                if not isinstance(cell, str) or not COLOUR.fullmatch(cell):
                    raise ValueError("Colors must be uppercase #RRGGBB")
                if any(int(cell[i:i + 2], 16) % 17 for i in (1, 3, 5)):
                    raise ValueError("CPS-2 colors must use 4-bit RGB precision")
                converted.append(cell)
            rows.append(converted)
        result.append(rows)
    return result


def valid_swatch(value: object) -> str:
    """Explicit 24-bit character-select indicator, not a CPS-2 color pen."""
    if not isinstance(value, str) or not re.fullmatch(r"#[0-9a-fA-F]{6}", value):
        raise ValueError("Select-screen swatch must be #RRGGBB")
    return value.upper()


def suggested_swatch(rows: list) -> str:
    """Choose a visible starting swatch from the native body row.

    The original P/K samples are immutable; custom profiles may override
    this marker freely without altering any game palette pen.
    """
    best = "#888888"
    score = -1.0
    for colour in rows[0][1:15]:
        r, g, b = (int(colour[i:i + 2], 16) for i in (1, 3, 5))
        chroma = max(r, g, b) - min(r, g, b)
        brightness = (r + g + b) / 3
        weight = chroma * 2 + brightness * 0.12 - (100 if brightness < 35 else 0)
        if weight > score:
            score = weight
            best = colour
    return best.upper()


def require_source() -> dict:
    if SOURCE is None:
        raise ValueError("Load the original xvs.05a ROM before editing or saving")
    return SOURCE


def profile_path(who: int, profile_id: str) -> Path:
    return PROFILES / slug(who) / (checked_profile_id(profile_id) + ".json")


def profile_list(who: int) -> list:
    folder = PROFILES / slug(who)
    if not folder.is_dir() or folder.is_symlink():
        return []
    profiles = []
    for path in sorted(folder.glob("*.json")):
        if path.is_symlink() or not PROFILE_ID.fullmatch(path.stem):
            continue
        try:
            obj = json.loads(path.read_text(encoding="utf-8"))
            if obj.get("character") == who and isinstance(obj.get("name"), str):
                profiles.append({"id": path.stem, "name": obj["name"],
                                 "source_sha256": obj.get("source_sha256", ""),
                                 "swatch": obj.get("swatch")})
        except (OSError, ValueError, TypeError, AttributeError):
            continue
    # The browser's numbered strip must use the SAME saved nine-slot
    # ordering as the generated game catalog. Renaming cannot reshuffle.
    preferred = existing_slot_order().get(who, [])
    positions = {profile_id: i for i, profile_id in enumerate(preferred)}
    return sorted(profiles, key=lambda p: (positions.get(p["id"], len(preferred)),
                                           p["name"].lower(), p["id"]))


def read_profile(who: int, profile_id: str) -> dict:
    path = profile_path(who, profile_id)
    if not path.is_file() or path.is_symlink():
        raise ValueError("Profile not found")
    obj = json.loads(path.read_text(encoding="utf-8"))
    if obj.get("character") != who:
        raise ValueError("Profile character mismatch")
    source = require_source()
    if obj.get("source_sha256") != source["sha256"]:
        raise ValueError("Profile belongs to a different original ROM")
    obj["colours"] = valid_colours(obj.get("colours"))
    obj["swatch"] = valid_swatch(
        obj.get("swatch", suggested_swatch(obj["colours"][0])))
    return obj


def profile_previews(who: int) -> list[dict]:
    """Return only validated profiles for this character's sprite chooser.

    The immutable P/K ROM references are provided by /api/state; this
    endpoint contains just saved custom schemes. Invalid or stale profiles
    are never offered as deceptively functional thumbnails.
    """
    items = []
    for summary in profile_list(who):
        try:
            doc = read_profile(who, summary["id"])
            items.append({"id": doc["id"], "name": doc["name"],
                          "swatch": doc["swatch"], "colours": doc["colours"]})
        except (ValueError, OSError, TypeError, KeyError, json.JSONDecodeError):
            continue
    return items


def save_profile(obj: dict) -> dict:
    source = require_source()
    who = character(obj.get("character"))
    name = obj.get("name")
    if (not isinstance(name, str) or not 1 <= len(name.strip()) <= 60
            or any(ord(c) < 32 or c in "/\\" for c in name)):
        raise ValueError("Profile name must be 1–60 characters, without slash or controls")
    if obj.get("source_sha256") != source["sha256"]:
        raise ValueError("The original ROM has changed; reload the profile")
    colours = valid_colours(obj.get("colours"))
    swatch = valid_swatch(obj.get("swatch", suggested_swatch(colours[0])))
    identifier = obj.get("id")
    if identifier is not None:
        checked_profile_id(identifier)
        if not profile_path(who, identifier).is_file():
            raise ValueError("Selected profile no longer exists")
    else:
        identifier = secrets.token_hex(8)
    doc = {"version": 1, "id": identifier, "character": who,
           "name": name.strip(), "source_sha256": source["sha256"],
           "swatch": swatch, "colours": colours}
    target = profile_path(who, identifier)
    with SAVE_LOCK:
        target.parent.mkdir(parents=True, exist_ok=True)
        fd, tempname = tempfile.mkstemp(prefix=".palette-", suffix=".tmp", dir=target.parent)
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
                json.dump(doc, stream, indent=2)
                stream.write("\n")
            os.replace(tempname, target)
        finally:
            if os.path.exists(tempname):
                os.unlink(tempname)
    return {"id": identifier, "name": doc["name"],
            "swatch": swatch, "character": who}


def existing_slot_order() -> dict[int, list[str]]:
    """Preserve saved slot positions across profile renames and edits."""
    try:
        if GAME_CATALOG.is_symlink():
            return {}
        doc = json.loads(GAME_CATALOG.read_text(encoding="utf-8"))
        if doc.get("source_sha256") != require_source()["sha256"] or doc.get("version") != 1:
            return {}
        result = {}
        for entry in doc.get("characters", []):
            who = character(entry.get("character"))
            result[who] = [slot["profile_id"] for slot in entry.get("slots", [])[2:]
                           if isinstance(slot, dict) and
                           isinstance(slot.get("profile_id"), str) and
                           PROFILE_ID.fullmatch(slot["profile_id"])]
        return result
    except (OSError, ValueError, TypeError, KeyError, json.JSONDecodeError):
        return {}


def saved_original_swatches() -> dict[int, dict[int, str]]:
    """Read only the two display-color overrides from the published catalog.

    Original palette pens stay ROM-backed and immutable. An invalid or
    unrelated source catalog cannot contaminate this editor's defaults.
    """
    try:
        if GAME_CATALOG.is_symlink():
            return {}
        document = json.loads(GAME_CATALOG.read_text(encoding="utf-8"))
        if (document.get("version") != 1 or
                document.get("source_sha256") != require_source()["sha256"]):
            return {}
        result = {}
        for item in document.get("characters", []):
            who = character(item["character"])
            slots = item["slots"]
            if not isinstance(slots, list) or len(slots) < 2:
                continue
            for bank in (0, 1):
                slot = slots[bank]
                if (isinstance(slot, dict) and
                        slot.get("kind") == "original" and
                        slot.get("bank") == bank):
                    color = valid_swatch(slot.get("swatch"))
                    result.setdefault(who, {})[bank] = color
        return result
    except (OSError, ValueError, TypeError, KeyError, IndexError, json.JSONDecodeError):
        return {}


def build_selection_catalog(preferred_order: dict[int, list[str]] | None = None,
                            original_swatches: dict[int, dict[int, str]] | None = None) -> dict:
    """Build all 19 rosters, nine slots each, from verified ROM and profiles.

    This is a data contract for the next native X/Y selection feature,
    not an assertion that today's game already reads the catalog.
    """
    source = require_source()
    preferred = preferred_order if preferred_order is not None else existing_slot_order()
    markers = original_swatches if original_swatches is not None else saved_original_swatches()
    roster = []
    for who, name in enumerate(CHARACTERS):
        originals = source["originals"][who]
        slots = []
        for bank, label in ((0, "Original P"), (1, "Original K")):
            slots.append({"slot": bank, "kind": "original", "bank": bank,
                          "name": label, "profile_id": None,
                          "swatch": markers.get(who, {}).get(bank, suggested_swatch(originals[bank])),
                          "colours": originals[bank]})
        stored = {}
        for summary in profile_list(who):
            try:
                doc = read_profile(who, summary["id"])
                stored[doc["id"]] = doc
            except (OSError, ValueError, TypeError, KeyError, json.JSONDecodeError):
                continue
        order = [pid for pid in preferred.get(who, []) if pid in stored]
        order.extend(pid for pid in stored if pid not in order)
        if len(order) > MAX_CUSTOM_SLOTS:
            raise ValueError(f"{name} has {len(order)} custom palettes; maximum is {MAX_CUSTOM_SLOTS} for 9 total slots")
        for pid in order:
            doc = stored[pid]
            slots.append({"slot": len(slots), "kind": "custom", "bank": 0,
                          "name": doc["name"], "profile_id": pid,
                          "swatch": doc["swatch"], "colours": doc["colours"][0]})
        slots.extend([None] * (TOTAL_SELECT_SLOTS - len(slots)))
        roster.append({"character": who, "name": name, "rom_id": f"0x{who * 2:02X}",
                       "slots": slots})
    return {"version": 1, "format": "xmvsf-palette-slots",
            "source_sha256": source["sha256"], "slots_per_character": TOTAL_SELECT_SLOTS,
            "characters": roster}


def write_catalog(catalog: dict) -> None:
    """Replace one game-data catalog only after validating all included data."""
    folder = GAME_CATALOG.parent
    if GAME_CATALOG.is_symlink() or folder.is_symlink():
        raise ValueError("Palette catalog destination must not be a symlink")
    folder.mkdir(parents=True, exist_ok=True)
    fd, tempname = tempfile.mkstemp(prefix=".selection-slots-", suffix=".tmp", dir=folder)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(catalog, stream, indent=2)
            stream.write("\n")
        os.replace(tempname, GAME_CATALOG)
    finally:
        if os.path.exists(tempname):
            os.unlink(tempname)


def save_all_profiles(payload: dict) -> dict:
    """One Save All action persists changes across characters plus game catalog.

    Validate every changed profile BEFORE any write. An existing profile keeps
    its ID; new drafts receive real IDs and fixed slots in the manifest.
    Existing custom-slot ordering is preserved, even when names change.
    """
    source = require_source()
    if not isinstance(payload, dict) or payload.get("source_sha256") != source["sha256"]:
        raise ValueError("ROM source changed; reload before Save All")
    updates = payload.get("profiles")
    original_updates = payload.get("original_swatches", [])
    if not isinstance(original_updates, list) or len(original_updates) > len(CHARACTERS) * 2:
        raise ValueError("Save All original swatches must be a list of at most 38 entries")
    checked_originals = []
    seen_originals = set()
    for entry in original_updates:
        if not isinstance(entry, dict):
            raise ValueError("Invalid original swatch entry")
        who = character(entry.get("character"))
        bank = entry.get("bank")
        if type(bank) is not int or bank not in (0, 1):
            raise ValueError("Original swatch bank must be 0 or 1")
        if (who, bank) in seen_originals:
            raise ValueError("Duplicate original swatch override")
        seen_originals.add((who, bank))
        checked_originals.append((who, bank, valid_swatch(entry.get("swatch"))))
    if not isinstance(updates, list) or len(updates) > len(CHARACTERS) * MAX_CUSTOM_SLOTS:
        raise ValueError("Save All requires a list of at most seven edited profiles per character")
    prepared = []
    seen = set()
    new_count = {who: 0 for who in range(len(CHARACTERS))}
    for item in updates:
        if not isinstance(item, dict):
            raise ValueError("Invalid palette in Save All")
        who = character(item.get("character"))
        key = item.get("client_key")
        if not isinstance(key, str) or not (
                DRAFT_KEY.fullmatch(key) or PROFILE_ID.fullmatch(key)):
            raise ValueError("Invalid staged palette key")
        if (who, key) in seen:
            raise ValueError("Duplicate staged palette")
        seen.add((who, key))
        identifier = item.get("id")
        if identifier is None:
            if not DRAFT_KEY.fullmatch(key):
                raise ValueError("New profile requires a draft key")
            new_count[who] += 1
        else:
            checked_profile_id(identifier)
            if key != identifier or not profile_path(who, identifier).is_file():
                raise ValueError("Saved palette ID mismatch")
        name = item.get("name")
        if not isinstance(name, str) or not 1 <= len(name.strip()) <= 60 or any(
                ord(c) < 32 or c in "/\\" for c in name):
            raise ValueError("Invalid palette name (1–60 characters)")
        colours = valid_colours(item.get("colours"))
        swatch = valid_swatch(item.get("swatch"))
        prepared.append((who, key, {"id": identifier, "character": who,
                      "name": name.strip(), "source_sha256": source["sha256"],
                      "swatch": swatch, "colours": colours}))

    for who in range(len(CHARACTERS)):
        count = sum(1 for summary in profile_list(who) if
                    summary.get("source_sha256") == source["sha256"])
        if count + new_count[who] > MAX_CUSTOM_SLOTS:
            raise ValueError(f"{CHARACTERS[who]} allows seven custom palettes alongside Original P/K")

    with SAVE_LOCK:
        prior = existing_slot_order()
        # A first-ever Save All should retain the order already displayed
        # in the browser; drafts append in user creation order, NOT by name.
        for who in range(len(CHARACTERS)):
            if who not in prior:
                prior[who] = [p["id"] for p in profile_list(who)
                              if p.get("source_sha256") == source["sha256"]]
        ids = {}
        for who, key, document in prepared:
            result = save_profile(document)
            ids[key] = result["id"]
            if result["id"] not in prior[who]:
                prior[who].append(result["id"])
        originals = saved_original_swatches()
        for who, bank, marker in checked_originals:
            originals.setdefault(who, {})[bank] = marker
        catalog = build_selection_catalog(prior, originals)
        write_catalog(catalog)
    return {"saved": len(prepared), "original_swatches_saved": len(checked_originals), "ids": ids,
            "catalog": str(GAME_CATALOG),
            "characters": len(catalog["characters"]),
            "slots_per_character": TOTAL_SELECT_SLOTS}


def export_text(who: int, doc: dict) -> str:
    source = require_source()
    colours = valid_colours(doc["colours"])
    originals = source["originals"][who]
    text = [f"# {CHARACTERS[who]} — {doc['name']}",
            "# XMVSF Palette Studio; copy to the recomp game's palettes folder.",
            "# 4-bit RGB (#RRGGBB); - means keep the original whole line."]
    for set_no, name in enumerate(("P", "K")):
        text.extend(("", f"[{name}]"))
        for row in range(LINES):
            if colours[set_no][row] == originals[set_no][row]:
                text.append("-")
            else:
                text.append(" ".join(colours[set_no][row]))
    return "\n".join(text) + "\n"


def portrait_path(who: int) -> Path | None:
    rom_id = who * 2
    files = sorted(PORTRAITS.glob(f"*_0x{rom_id:02X}_*.png"))
    return files[0] if files else None


def model_viewer_exports(who: int) -> list[Path]:
    """Read native standing exports, never animation frames or other fighters.

    Windows's shipped RECOMP_RUN.bat can place output under recomp/exports
    because Port::PlayerFolder searches '/' while its EEPROM uses '\\'.
    Check both known real locations, preferring the current Windows path
    when the same filename exists in both.
    """
    name = CHARACTERS[who]
    pattern = re.compile(
        r"^" + re.escape(name) + r"(?: (?:Row [23](?: [0-9]{2})?|[2-9][0-9]*))?\.png$", re.I)
    found: dict[str, Path] = {}
    for directory in VIEWER_EXPORTS:
        if not directory.is_dir() or directory.is_symlink():
            continue
        for candidate in directory.iterdir():
            if (candidate.is_file() and not candidate.is_symlink()
                    and pattern.fullmatch(candidate.name)
                    and 8 < candidate.stat().st_size <= SPRITE_MAX):
                found.setdefault(candidate.name.lower(), candidate)
    # Keep every generated numbered sample in deterministic row/index
    # order; never allow another character's files into this gallery.
    def priority(file: Path) -> tuple[int, int, str]:
        suffix = file.stem[len(name):].strip().lower()
        if not suffix:
            return (0, 0, file.name.lower())
        sample = re.fullmatch(r"row ([23])(?: ([0-9]{2}))?", suffix)
        if sample:
            return (int(sample.group(1)) - 1,
                    int(sample.group(2) or 1), file.name.lower())
        return (4, 0, file.name.lower())
    return sorted(found.values(), key=priority)[:80]

def viewer_asset(who: int, filename: str) -> Path:
    if not filename or Path(filename).name != filename:
        raise ValueError("Invalid model viewer sprite")
    for candidate in model_viewer_exports(who):
        if candidate.name == filename:
            return candidate
    raise ValueError("Model viewer picture not found for this character")


def sprite_list(who: int) -> list:
    result = []
    for file in model_viewer_exports(who):
        suffix = file.stem[len(CHARACTERS[who]):].strip().lower()
        sample = re.fullmatch(r"row ([23])(?: ([0-9]{2}))?", suffix)
        line = int(sample.group(1)) - 1 if sample else 0
        number = int(sample.group(2) or 1) if sample else 0
        label = ("Body / Character" if not suffix else
                 f"Palette row {line + 1} · {number:02d}" if sample else
                 "Standing " + suffix)
        result.append({"name": label, "line": line, "sample": number, "type": "viewer",
                       "url": f"/asset/viewer/{who}/{urllib.parse.quote(file.name)}?rev={file.stat().st_mtime_ns}"})
    folder = SPRITES / slug(who)
    if folder.is_dir() and not folder.is_symlink():
        for file in sorted(folder.glob("*.png"))[:64]:
            if file.is_file() and not file.is_symlink() and SPRITE_NAME.fullmatch(file.name):
                result.append({"name": file.stem, "type": "sprite",
                               "url": f"/asset/sprite/{who}/{urllib.parse.quote(file.name)}"})
    portrait = portrait_path(who)
    if portrait:
        result.append({"name": "Portrait reference", "type": "portrait",
                       "url": f"/asset/portrait/{who}"})
    return result


def save_sprite(who: int, filename: str, data: bytes) -> dict:
    if not data.startswith(b"\x89PNG\r\n\x1a\n") or len(data) > SPRITE_MAX:
        raise ValueError("Upload a PNG image smaller than 8 MiB")
    if len(data) < 24 or int.from_bytes(data[16:20], "big") > 2048 or int.from_bytes(data[20:24], "big") > 2048:
        raise ValueError("Sprite dimensions must be at most 2048x2048")
    basename = Path(filename.replace("\\", "/")).name
    stem = re.sub(r"[^a-zA-Z0-9_-]+", "-", Path(basename).stem).strip("-")[:48] or "idle"
    final = stem + "-" + secrets.token_hex(3) + ".png"
    folder = SPRITES / slug(who)
    with SAVE_LOCK:
        folder.mkdir(parents=True, exist_ok=True)
        (folder / final).write_bytes(data)
    return {"name": final[:-4], "type": "sprite",
            "url": f"/asset/sprite/{who}/{urllib.parse.quote(final)}"}


def native_runtime_environment(*, windows: bool | None = None,
                               ucrt_root: Path | None = None,
                               base_env: dict[str, str] | None = None) -> dict[str, str]:
    """Match RECOMP_RUN.bat's DLL search path for the isolated native exporter.

    Launching Palette Studio from Explorer/VS Code does not inherit the
    MSYS2 UCRT64 PATH installed by RECOMP_RUN.bat. The headless exe uses
    the same MinGW runtime, even though the editor itself needs only Python.
    Do not copy system DLLs into this repository or change the parent's PATH.
    """
    env = dict(os.environ if base_env is None else base_env)
    if windows is None:
        windows = os.name == "nt"
    if not windows:
        return env
    # RECOMP_RUN.bat uses C:\\msys64\\ucrt64\\bin. Allow a nondefault
    # installation without importing a different MinGW/MSYS toolchain.
    if ucrt_root is None:
        ucrt_root = Path(env.get("XMVSF_UCRT64_ROOT") or
                         env.get("UCRT_PREFIX") or "C:/msys64/ucrt64")
    bin_dir = ucrt_root / "bin"
    libgcc = bin_dir / "libgcc_s_seh-1.dll"
    if not libgcc.is_file():
        raise OSError(
            f"Native sprite generator missing MSYS2 UCRT64 runtime: {libgcc}. "
            "Install MSYS2 UCRT64 GCC runtime (mingw-w64-ucrt-x86_64-gcc-libs) "
            "or set XMVSF_UCRT64_ROOT to your UCRT64 directory. "
            "The normal RECOMP_RUN.bat requires this same runtime.")
    usr_dir = ucrt_root.parent / "usr" / "bin"
    paths = [str(bin_dir)]
    if usr_dir.is_dir():
        paths.append(str(usr_dir))
    if env.get("PATH"):
        paths.append(env["PATH"])
    env["PATH"] = ";".join(paths)  # Windows path separator, also in unit tests
    env["MSYSTEM"] = "UCRT64"
    return env


def native_capture_worker(requested: str) -> None:
    """Run the compiled game's private headless Model viewer, never a shell.

    The button is explicit; no process is launched by selecting a fighter or
    loading the page. ROM and settings paths are fixed to the local project.
    """
    args = [
        str(ROOT / "recomp" / "build" / "xmvsf" / "xmvsf.exe"),
        "--rom", "rom/xmvsf",
        "--eeprom", "work/xmvsf/palette_studio_eeprom.bin",
        "--settings", "work/xmvsf/palette_studio_settings.ini",
        "--headless", "--mute", "--palette-sprites", requested,
    ]
    try:
        # Resolve DLLs BEFORE touching existing examples. On Windows a
        # missing libgcc otherwise creates an OS modal error with no log.
        child_env = native_runtime_environment()
        (ROOT / "recomp" / "work" / "xmvsf").mkdir(parents=True, exist_ok=True)
        # Only dedicated generated files of this fighter may be replaced.
        # Clear previous numbered gallery frames so missing native poses
        # cannot remain as stale "successful" samples on regeneration.
        generated = ROOT / "recomp" / "exports" / "palette_studio"
        if generated.is_symlink():
            raise OSError("Generated sample directory is a symlink")
        who = character(requested)
        names = {CHARACTERS[who] + suffix for suffix in
                 (".png", " Row 2.png", " Row 3.png",
                  " Effect A.png", " Effect B.png")}
        for line in (2, 3):
            names.update(f"{CHARACTERS[who]} Row {line} {n:02d}.png"
                         for n in range(2, 33))
        for name in names:
            example = generated / name
            if example.is_symlink():
                raise OSError("Generated sample file is a symlink")
            if example.is_file():
                example.unlink()
        result = subprocess.run(
            args, cwd=ROOT / "recomp", capture_output=True,
            text=True, encoding="utf-8", errors="replace", timeout=300,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
            env=child_env, check=False)
        transcript = (result.stdout + "\n" + result.stderr).strip()
        exported = sum(1 for line in transcript.splitlines()
                       if line.startswith("Palette Studio ") and line.endswith(": EXPORTED"))
        if result.returncode or exported < 3:
            # A genuinely missing effect sample remains missing, never
            # silently falls back to a duplicate standing sprite.
            status = "failed"
            message = f"Generated {exported}/3 palette-row samples; missing rows are not substituted"
        else:
            status = "ready"
            message = "Generated Body plus multiple native Row 2 and Row 3 samples; use the gallery arrows to cycle them"
        with CAPTURE_LOCK:
            CAPTURE_JOB.update(running=False, status=status, message=message,
                               exported=exported, log=transcript[-5000:])
    except (OSError, subprocess.TimeoutExpired) as exc:
        with CAPTURE_LOCK:
            CAPTURE_JOB.update(running=False, status="failed",
                               message=str(exc), exported=0, log=str(exc))


def start_native_capture(requested: str) -> dict:
    who = character(requested)
    exe = ROOT / "recomp" / "build" / "xmvsf" / "xmvsf.exe"
    if not exe.is_file():
        raise ValueError("Recomp executable missing. Run RECOMP_BUILD.bat once before generating genuine combat sprites.")
    if not (ROOT / "recomp" / "rom" / "xmvsf").is_dir():
        raise ValueError("Expected original ROM set at recomp/rom/xmvsf")
    with CAPTURE_LOCK:
        if CAPTURE_JOB["running"]:
            return dict(CAPTURE_JOB)
        CAPTURE_JOB.update(running=True, status="running",
                           message="Discovering distinct native Row 2/3 frames for " + CHARACTERS[who],
                           requested=requested, exported=0, log="")
    threading.Thread(target=native_capture_worker, args=(requested,), daemon=True).start()
    return dict(CAPTURE_JOB)


class StudioServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


class Handler(BaseHTTPRequestHandler):
    server_version = "XMVSF-Palette-Studio/1"

    def json_reply(self, data: object, status: int = 200) -> None:
        raw = json.dumps(data, separators=(",", ":")).encode("utf-8")
        self.send_bytes(raw, "application/json; charset=utf-8", status)

    def send_bytes(self, raw: bytes, mime: str, status: int = 200,
                   filename: str | None = None) -> None:
        self.send_response(status)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(raw)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Content-Security-Policy", "default-src 'self' 'unsafe-inline' data: blob:")
        if filename:
            self.send_header("Content-Disposition", 'attachment; filename="' + filename.replace('"', "") + '"')
        self.end_headers()
        self.wfile.write(raw)

    def error_reply(self, error: Exception, status: int = 400) -> None:
        self.json_reply({"error": str(error)}, status)

    def query(self) -> tuple[str, dict]:
        u = urllib.parse.urlsplit(self.path)
        return u.path, dict(urllib.parse.parse_qsl(u.query, keep_blank_values=True))

    def do_GET(self) -> None:
        route, query = self.query()
        try:
            if route == "/":
                html = PAGE.read_text(encoding="utf-8").replace("__PALETTE_TOKEN__", TOKEN)
                self.send_bytes(html.encode("utf-8"), "text/html; charset=utf-8")
            elif route == "/api/state":
                source = SOURCE
                self.json_reply({
                    "source": ({"label": source["label"], "sha256": source["sha256"],
                                "crc32": source["crc32"]} if source else None),
                    "characters": [{"id": i, "name": name, "rom_id": f"0x{i * 2:02X}",
                                    "profiles": profile_list(i)}
                                   for i, name in enumerate(CHARACTERS)],
                    "originals": source["originals"] if source else None,
                    "addresses": source["addresses"] if source else None,
                    "source_warning": SOURCE_WARNING if not source else "",
                    "original_swatches": saved_original_swatches() if source else {},
                })
            elif route == "/api/profile":
                who = character(query.get("character"))
                self.json_reply(read_profile(who, checked_profile_id(query.get("id", ""))))
            elif route == "/api/sprites":
                self.json_reply(sprite_list(character(query.get("character"))))
            elif route == "/api/profile-previews":
                self.json_reply(profile_previews(character(query.get("character"))))
            elif route == "/api/capture-status":
                with CAPTURE_LOCK:
                    self.json_reply(dict(CAPTURE_JOB))
            elif route == "/api/export":
                who = character(query.get("character"))
                doc = read_profile(who, checked_profile_id(query.get("id", "")))
                safe = re.sub(r"[^a-zA-Z0-9._ -]", "", CHARACTERS[who])
                self.send_bytes(export_text(who, doc).encode("utf-8"),
                                "text/plain; charset=utf-8", filename=safe + ".txt")
            elif route.startswith("/asset/portrait/"):
                who = character(route.removeprefix("/asset/portrait/"))
                path = portrait_path(who)
                if not path or not path.is_file():
                    raise ValueError("Portrait artwork not available")
                self.send_bytes(path.read_bytes(), "image/png")
            elif route.startswith("/asset/viewer/"):
                parts = route.split("/")
                if len(parts) != 5:
                    raise ValueError("Invalid Model viewer request")
                who = character(parts[3])
                filename = urllib.parse.unquote(parts[4])
                path = viewer_asset(who, filename)
                self.send_bytes(path.read_bytes(), "image/png")
            elif route.startswith("/asset/sprite/"):
                parts = route.split("/")
                if len(parts) != 5:
                    raise ValueError("Invalid sprite request")
                who = character(parts[3])
                filename = urllib.parse.unquote(parts[4])
                if not SPRITE_NAME.fullmatch(filename):
                    raise ValueError("Invalid sprite filename")
                path = SPRITES / slug(who) / filename
                if path.is_symlink() or not path.is_file():
                    raise ValueError("Sprite not found")
                self.send_bytes(path.read_bytes(), "image/png")
            else:
                self.error_reply(ValueError("Not found"), 404)
        except (ValueError, OSError, json.JSONDecodeError) as exc:
            self.error_reply(exc)

    def do_POST(self) -> None:
        route, query = self.query()
        origin = self.headers.get("Origin")
        expected_origin = f"http://127.0.0.1:{self.server.server_port}"
        if origin not in (None, expected_origin) or self.headers.get("X-XMVSF-Token") != TOKEN:
            self.error_reply(ValueError("Request authorization failed"), 403)
            return
        try:
            length = int(self.headers.get("Content-Length", "-1"))
            cap = SOURCE_MAX if route == "/api/source" else (
                SPRITE_MAX if route == "/api/sprite" else
                2_000_000 if route == "/api/save-all" else 200_000)
            if length < 0 or length > cap:
                raise ValueError("Unexpected request length")
            data = self.rfile.read(length)
            if len(data) != length:
                raise ValueError("Incomplete request")
            if route == "/api/source":
                name = self.headers.get("X-Source-Name", "Selected ROM")[:120]
                item = load_source(data, name)
                self.json_reply({"source": {"label": item["label"], "sha256": item["sha256"],
                                             "crc32": item["crc32"]}})
            elif route == "/api/save-all":
                self.json_reply(save_all_profiles(json.loads(data)))
            elif route == "/api/profile/save":
                doc = json.loads(data)
                if not isinstance(doc, dict):
                    raise ValueError("Invalid profile")
                self.json_reply(save_profile(doc))
            elif route == "/api/sprite":
                who = character(query.get("character"))
                self.json_reply(save_sprite(who, self.headers.get("X-File-Name", "idle.png"), data))
            elif route == "/api/capture":
                requested = query.get("character", "")
                self.json_reply(start_native_capture(requested))
            else:
                self.error_reply(ValueError("Not found"), 404)
        except (ValueError, OSError, zipfile.BadZipFile, json.JSONDecodeError, OverflowError) as exc:
            self.error_reply(exc)

    def log_message(self, fmt: str, *args) -> None:
        if not self.path.startswith("/api/state"):
            print("[Palette Studio]", fmt % args)


def main() -> int:
    global SOURCE_WARNING
    if not PAGE.is_file():
        print("Missing palette_studio.html", file=sys.stderr)
        return 2
    warning = auto_source()
    SOURCE_WARNING = warning
    if SOURCE:
        print("ROM palette source:", SOURCE["label"], "CRC32", SOURCE["crc32"])
    else:
        print("ROM source:", warning)
    with StudioServer(("127.0.0.1", 0), Handler) as server:
        address = f"http://127.0.0.1:{server.server_port}/"
        print("XMVSF Palette Studio:", address)
        print("Profiles are separate JSON files; the original ROM is NEVER modified.")
        print("Press Ctrl+C to close.", flush=True)
        try:
            webbrowser.open(address)
            server.serve_forever(poll_interval=0.25)
        except KeyboardInterrupt:
            print("\nPalette Studio closed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
