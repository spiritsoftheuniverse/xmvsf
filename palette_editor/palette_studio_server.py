#!/usr/bin/env python3
"""XMVSF Palette Studio: local-only ROM palette browser and profile editor.

Only reads the original CPS-2 xvs.05a program ROM. Never patches a ROM,
replaces an original colour, or writes to the game's palettes folder.
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
SPRITES = HERE / "sprites"
PORTRAITS = ROOT / "_art" / "menu" / "characterselect" / "portraits"
# Model viewer uses PlayerFolder()/exports. The shipped Windows RECOMP_RUN.bat
# gives --eeprom "work\xmvsf\eeprom.bin", but PlayerFolder() currently
# searches only for '/'. That can place exports at recomp/exports instead of
# recomp/work/xmvsf/exports. Read both, and accept an explicit override.
VIEWER_EXPORTS = (
    (Path(os.environ["XMVSF_MODEL_EXPORT_DIR"]),)
    if os.environ.get("XMVSF_MODEL_EXPORT_DIR") else
    (ROOT / "recomp" / "exports",
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
SPRITE_NAME = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9_.-]{0,90}\.png$", re.I)
SAVE_LOCK = threading.RLock()
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
                                 "source_sha256": obj.get("source_sha256", "")})
        except (OSError, ValueError, TypeError, AttributeError):
            continue
    return sorted(profiles, key=lambda p: p["name"].lower())


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
    return obj


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
    identifier = obj.get("id")
    if identifier is not None:
        checked_profile_id(identifier)
        if not profile_path(who, identifier).is_file():
            raise ValueError("Selected profile no longer exists")
    else:
        identifier = secrets.token_hex(8)
    doc = {"version": 1, "id": identifier, "character": who,
           "name": name.strip(), "source_sha256": source["sha256"],
           "colours": colours}
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
    return {"id": identifier, "name": doc["name"], "character": who}


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
    pattern = re.compile(r"^" + re.escape(name) + r"(?: [2-9][0-9]*)?\.png$", re.I)
    found: dict[str, Path] = {}
    for directory in VIEWER_EXPORTS:
        if not directory.is_dir() or directory.is_symlink():
            continue
        for candidate in directory.iterdir():
            if (candidate.is_file() and not candidate.is_symlink()
                    and pattern.fullmatch(candidate.name)
                    and 8 < candidate.stat().st_size <= SPRITE_MAX):
                found.setdefault(candidate.name.lower(), candidate)
    # Exact character name is the canonical first standing export.
    return sorted(found.values(), key=lambda file: (
        file.name.lower() != (name + ".png").lower(), file.name.lower()))[:24]

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
        result.append({"name": "Standing" if file.stem.lower() == CHARACTERS[who].lower()
                       else "Standing " + file.stem[len(CHARACTERS[who]):].strip(),
                       "type": "viewer",
                       "url": f"/asset/viewer/{who}/{urllib.parse.quote(file.name)}"})
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
                })
            elif route == "/api/profile":
                who = character(query.get("character"))
                self.json_reply(read_profile(who, checked_profile_id(query.get("id", ""))))
            elif route == "/api/sprites":
                self.json_reply(sprite_list(character(query.get("character"))))
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
                SPRITE_MAX if route == "/api/sprite" else 200_000)
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
            elif route == "/api/profile/save":
                doc = json.loads(data)
                if not isinstance(doc, dict):
                    raise ValueError("Invalid profile")
                self.json_reply(save_profile(doc))
            elif route == "/api/sprite":
                who = character(query.get("character"))
                self.json_reply(save_sprite(who, self.headers.get("X-File-Name", "idle.png"), data))
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
