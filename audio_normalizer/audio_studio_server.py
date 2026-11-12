#!/usr/bin/env python3
"""Local-only XMVSF OST Studio. Standard-library UI/server; FFmpeg for saving."""
from __future__ import annotations

import json
import mimetypes
import os
from pathlib import Path, PurePosixPath
import re
import secrets
import shutil
import subprocess
import sys
import tempfile
import threading
import urllib.parse
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parents[1]
AUDIO = ROOT / "audio"
STATE = AUDIO / ".ost_studio"
BACKUPS = STATE / "originals"
LEVELS = STATE / "levels.json"
PAGE = Path(__file__).with_name("audio_studio.html")
EXTENSIONS = {".wav", ".ogg", ".flac", ".mp3", ".opus", ".m4a", ".aif", ".aiff"}
SAVE_LOCK = threading.Lock()
TOKEN = secrets.token_urlsafe(30)


def tool_path(name: str) -> str | None:
    env = os.environ.get("XMVSF_FFMPEG", "").strip().strip('"')
    candidates = []
    if env:
        specified = Path(env)
        candidates.append(specified / (name + ".exe") if specified.is_dir() else
                          specified.with_name(name + specified.suffix))
    candidates.extend((ROOT / "tools" / (name + ".exe"),
                       Path("C:/msys64/ucrt64/bin") / (name + ".exe"),
                       Path("C:/ffmpeg/bin") / (name + ".exe")))
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate)
    return shutil.which(name)


FFMPEG = tool_path("ffmpeg")
FFPROBE = tool_path("ffprobe")


def media_files() -> list[dict]:
    found = []
    if not AUDIO.is_dir():
        return found
    for path in AUDIO.rglob("*"):
        if not path.is_file() or path.is_symlink() or path.suffix.lower() not in EXTENSIONS:
            continue
        rel_path = path.relative_to(AUDIO)
        if any(part.startswith(".") for part in rel_path.parts):
            continue
        group = "Effects" if len(rel_path.parts) == 1 else rel_path.parts[0]
        found.append({
            "path": rel_path.as_posix(),
            "name": path.name,
            "group": "Music" if group.lower() == "music" else group,
            "bytes": path.stat().st_size,
            "backup": (BACKUPS / rel_path).is_file(),
        })
    return sorted(found, key=lambda item: (item["group"].lower(), item["name"].lower(), item["path"]))


def resolve_audio(name: str) -> tuple[Path, Path]:
    if not isinstance(name, str) or not name or "\\" in name or "\x00" in name:
        raise ValueError("Invalid audio filename")
    rel = PurePosixPath(name)
    if rel.is_absolute() or any(p in ("..", ".", ".ost_studio") or p.startswith(".") for p in rel.parts):
        raise ValueError("Path is outside the audio library")
    path = (AUDIO / Path(*rel.parts)).resolve(strict=True)
    if not path.is_file() or path.is_symlink() or path.suffix.lower() not in EXTENSIONS:
        raise ValueError("Unsupported audio file")
    if not path.is_relative_to(AUDIO.resolve()):
        raise ValueError("Path is outside the audio library")
    # Verify that no parent is an alias or symlink, even if it resolved inside audio.
    current = AUDIO
    for part in rel.parts:
        current = current / part
        if current.is_symlink():
            raise ValueError("Symlinked audio paths are not editable")
    return path, Path(*rel.parts)


def load_levels() -> dict:
    try:
        obj = json.loads(LEVELS.read_text(encoding="utf-8"))
        return obj if isinstance(obj, dict) else {}
    except FileNotFoundError:
        return {}
    except (ValueError, OSError) as err:
        print("WARNING: existing volume settings unreadable:", err, file=sys.stderr)
        return {}


def atomic_settings(levels: dict) -> None:
    STATE.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=".levels-", suffix=".json", dir=STATE)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as fp:
            json.dump(levels, fp, indent=2, sort_keys=True)
            fp.write("\n")
        os.replace(name, LEVELS)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def run_command(argv: list[str], timeout: int = 900) -> subprocess.CompletedProcess:
    creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    return subprocess.run(argv, stdin=subprocess.DEVNULL, capture_output=True,
                          text=True, errors="replace", timeout=timeout,
                          creationflags=creationflags, check=False)


def probe(path: Path) -> dict:
    if not FFPROBE:
        raise RuntimeError("FFprobe not found. Install FFmpeg/FFprobe or set XMVSF_FFMPEG.")
    args = [FFPROBE, "-v", "error", "-select_streams", "a:0",
            "-show_entries", "stream=codec_name,sample_rate,channels:format=duration",
            "-of", "json", str(path)]
    result = run_command(args)
    if result.returncode:
        raise RuntimeError("FFprobe failed: " + result.stderr[-900:])
    data = json.loads(result.stdout)
    streams = data.get("streams", [])
    if not streams:
        raise RuntimeError("No decodable audio stream found")
    return {"codec": streams[0].get("codec_name", ""),
            "duration": float(data.get("format", {}).get("duration") or 0),
            "rate": streams[0].get("sample_rate"),
            "channels": streams[0].get("channels")}


def codec_args(path: Path, original_codec: str) -> list[str]:
    ext = path.suffix.lower()
    if ext == ".wav":
        options = {"pcm_u8", "pcm_s16le", "pcm_s24le", "pcm_s32le",
                   "pcm_f32le", "pcm_f64le"}
        return ["-c:a", original_codec if original_codec in options else "pcm_s24le"]
    if ext in (".aif", ".aiff"):
        options = {"pcm_s16be", "pcm_s24be", "pcm_s32be"}
        return ["-c:a", original_codec if original_codec in options else "pcm_s24be"]
    if ext == ".flac":
        return ["-c:a", "flac"]
    if ext == ".opus" or original_codec == "opus":
        return ["-c:a", "libopus", "-b:a", "192k"]
    if ext == ".ogg":
        return ["-c:a", "libvorbis", "-q:a", "8"]
    if ext == ".mp3":
        return ["-c:a", "libmp3lame", "-q:a", "2"]
    if ext == ".m4a":
        return ["-c:a", "aac", "-b:a", "256k"]
    raise ValueError("Unsupported audio container")


def original_backup(path: Path, rel: Path) -> Path:
    backup = BACKUPS / rel
    if backup.is_file():
        return backup
    backup.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=".original-", dir=backup.parent)
    os.close(fd)
    try:
        shutil.copy2(path, temp_name)
        # The lock protects the studio's own writes. Never replace an existing backup.
        if not backup.exists():
            os.replace(temp_name, backup)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)
    if not backup.is_file() or not backup.stat().st_size:
        raise RuntimeError("Original backup was not created")
    return backup


def save_audio(relative: str, gain: float) -> dict:
    if isinstance(gain, bool) or not isinstance(gain, (float, int)) or not 0 <= gain <= 300:
        raise ValueError("Volume must be between 0% and 300%")
    if not FFMPEG or not FFPROBE:
        raise RuntimeError("FFmpeg + FFprobe required to save. Install FFmpeg and restart the studio.")
    with SAVE_LOCK:
        path, rel = resolve_audio(relative)
        # All edits use the first pristine backup. Re-saving NEVER compounds lossy encoding.
        backup = original_backup(path, rel)
        source_meta = probe(backup)
        fd, temp_name = tempfile.mkstemp(prefix=".xmvsf-audio-", suffix=path.suffix, dir=path.parent)
        os.close(fd)
        try:
            if abs(gain - 100.0) < 0.00001:
                shutil.copy2(backup, temp_name)  # byte-perfect restoration of original
            else:
                args = [FFMPEG, "-hide_banner", "-nostdin", "-loglevel", "error",
                        "-y", "-i", str(backup), "-map", "0:a:0", "-map_metadata", "0",
                        "-vn", "-af", "volume={:.6f}".format(gain / 100.0)]
                args += codec_args(path, source_meta["codec"])
                args += [temp_name]
                result = run_command(args)
                if result.returncode:
                    raise RuntimeError("FFmpeg export failed: " + result.stderr[-1500:])
            if not os.path.getsize(temp_name):
                raise RuntimeError("Export produced an empty file")
            output_meta = probe(Path(temp_name))
            source_duration = source_meta["duration"]
            if source_duration and abs(output_meta["duration"] - source_duration) > max(0.4, source_duration * 0.005):
                raise RuntimeError("Export duration differs from original; original left unchanged")
            os.replace(temp_name, path)  # never replace original until fully verified
            settings = load_levels()
            settings[rel.as_posix()] = gain
            try:
                atomic_settings(settings)
            except OSError as err:
                # The audio save succeeded; report the settings write separately.
                print("WARNING: gain preference could not be written:", err, file=sys.stderr)
            return {"path": rel.as_posix(), "gain": gain,
                    "backup": backup.relative_to(AUDIO).as_posix(),
                    "bytes": path.stat().st_size}
        finally:
            if os.path.exists(temp_name):
                os.unlink(temp_name)


def peak_scan(relative: str) -> dict:
    if not FFMPEG:
        raise RuntimeError("FFmpeg required for peak analysis")
    path, rel = resolve_audio(relative)
    # Analyze the pristine source whenever available, so gain suggestions
    # aren't distorted by previous edits.
    pristine = BACKUPS / rel
    source = pristine if pristine.is_file() else path
    result = run_command([FFMPEG, "-hide_banner", "-nostdin", "-i", str(source),
                          "-af", "volumedetect", "-vn", "-f", "null", "-"], timeout=900)
    if result.returncode:
        raise RuntimeError("Peak analysis failed: " + result.stderr[-900:])
    match = re.search(r"max_volume:\s*(-?(?:\d+(?:\.\d+)?|inf))\s*dB", result.stderr)
    if not match:
        raise RuntimeError("No peak result from FFmpeg")
    peak = float(match.group(1))
    return {"peak_dbfs": peak, "max_unclipped_gain_percent":
            round(100 * (10 ** (-peak / 20)), 1) if peak != float("-inf") else None}


class StudioServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


class Handler(BaseHTTPRequestHandler):
    server_version = "XMVSFOSTStudio/1.0"

    def log_message(self, fmt, *args):
        print("[OST] " + fmt % args)

    def answer(self, status: int, obj: dict) -> None:
        content = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(content)

    def authorized(self, query: dict | None = None) -> bool:
        given = self.headers.get("X-Studio-Token", "")
        if query is not None:
            given = query.get("token", [""])[0]
        return secrets.compare_digest(given, TOKEN)

    def do_GET(self) -> None:
        parsed = urllib.parse.urlsplit(self.path)
        if parsed.path == "/":
            html = PAGE.read_text(encoding="utf-8").replace("__TOKEN__", TOKEN)
            raw = html.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(raw)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Security-Policy", "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; media-src 'self'; img-src 'self' data:; connect-src 'self'")
            self.end_headers()
            self.wfile.write(raw)
            return
        if parsed.path == "/api/files":
            if not self.authorized():
                return self.answer(403, {"error": "Not authorized"})
            return self.answer(200, {"files": media_files(), "levels": load_levels(),
                                      "ffmpeg": bool(FFMPEG and FFPROBE)})
        if parsed.path.startswith("/media/"):
            if not self.authorized(urllib.parse.parse_qs(parsed.query)):
                return self.answer(403, {"error": "Not authorized"})
            try:
                path, _ = resolve_audio(urllib.parse.unquote(parsed.path[7:]))
                length = path.stat().st_size
                if not length:
                    raise ValueError("Empty audio file")
                start, end = 0, length - 1
                requested = self.headers.get("Range", "")
                if requested:
                    match = re.fullmatch(r"bytes=(\d*)-(\d*)", requested)
                    if not match:
                        return self.answer(416, {"error": "Invalid media byte range"})
                    if match.group(1):
                        start = int(match.group(1))
                        if match.group(2):
                            end = min(int(match.group(2)), end)
                    elif match.group(2):
                        start = max(0, length - int(match.group(2)))
                    if start > end or start >= length:
                        return self.answer(416, {"error": "Range outside media"})
                mime = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
                self.send_response(206 if requested else 200)
                self.send_header("Content-Type", mime)
                self.send_header("Accept-Ranges", "bytes")
                self.send_header("Cache-Control", "no-store")
                self.send_header("Content-Length", str(end - start + 1))
                if requested:
                    self.send_header("Content-Range", "bytes {}-{}/{}".format(start, end, length))
                self.end_headers()
                with path.open("rb") as fp:
                    fp.seek(start)
                    remaining = end - start + 1
                    while remaining > 0:
                        chunk = fp.read(min(65536, remaining))
                        if not chunk:
                            break
                        self.wfile.write(chunk)
                        remaining -= len(chunk)
            except (OSError, ValueError, BrokenPipeError, ConnectionResetError) as err:
                print("[OST] media request:", err)
            return
        self.answer(404, {"error": "Unknown route"})

    def do_POST(self) -> None:
        path = urllib.parse.urlsplit(self.path).path
        if path not in ("/api/save", "/api/peak"):
            return self.answer(404, {"error": "Unknown route"})
        origin = self.headers.get("Origin", "")
        local_origin = "http://127.0.0.1:{}".format(self.server.server_port)
        if origin != local_origin or not self.authorized():
            return self.answer(403, {"error": "Invalid origin or session"})
        try:
            length = int(self.headers.get("Content-Length", "-1"))
            if length < 0 or length > 8192:
                raise ValueError("Invalid request length")
            data = json.loads(self.rfile.read(length))
            if not isinstance(data, dict) or not isinstance(data.get("path"), str):
                raise ValueError("Expected audio path")
            if path == "/api/save":
                result = save_audio(data["path"], data.get("gain"))
            else:
                result = peak_scan(data["path"])
            self.answer(200, result)
        except (ValueError, RuntimeError, OSError, subprocess.TimeoutExpired,
                json.JSONDecodeError) as err:
            self.answer(400, {"error": str(err)})
        except Exception as err:
            print("Unexpected audio operation failure:", repr(err), file=sys.stderr)
            self.answer(500, {"error": "Unexpected error; original audio was not intentionally replaced"})


def main() -> int:
    if not PAGE.is_file():
        print("ERROR: Missing audio_studio.html next to server script", file=sys.stderr)
        return 2
    AUDIO.mkdir(parents=True, exist_ok=True)
    print("XMVSF OST STUDIO 1.0")
    print("Library:", AUDIO)
    print("FFmpeg:", FFMPEG or "MISSING (preview works; saving disabled)")
    print("FFprobe:", FFPROBE or "MISSING (saving disabled)")
    print("Backups:", BACKUPS)
    with StudioServer(("127.0.0.1", 0), Handler) as server:
        url = "http://127.0.0.1:{}/".format(server.server_port)
        print("Studio:", url, "(Ctrl+C closes)")
        webbrowser.open(url)
        try:
            server.serve_forever(poll_interval=0.25)
        except KeyboardInterrupt:
            pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
