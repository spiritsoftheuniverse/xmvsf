#!/usr/bin/env python3
"""Manual, local-only OST Studio regression checks. Not part of native BUILD_TEST."""
import hashlib
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import wave

sys.path.insert(0, str(Path(__file__).resolve().parent))
import audio_studio_server as studio


def make_wav(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as writer:
        writer.setnchannels(1)
        writer.setsampwidth(2)
        writer.setframerate(22050)
        samples = [round(6000 * math.sin(2 * math.pi * 440 * n / 22050))
                   for n in range(4410)]
        writer.writeframes(struct.pack("<{}h".format(len(samples)), *samples))


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def first_peak(path: Path) -> int:
    with wave.open(str(path), "rb") as reader:
        raw = reader.readframes(reader.getnframes())
    values = struct.unpack("<{}h".format(len(raw) // 2), raw)
    return max(abs(x) for x in values)


class StudioTest(unittest.TestCase):
    def setUp(self) -> None:
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.old = (studio.AUDIO, studio.STATE, studio.BACKUPS, studio.LEVELS)
        studio.AUDIO = Path(self.folder.name) / "audio"
        studio.STATE = studio.AUDIO / ".ost_studio"
        studio.BACKUPS = studio.STATE / "originals"
        studio.LEVELS = studio.STATE / "levels.json"
        self.addCleanup(self.reset_paths)
        make_wav(studio.AUDIO / "music" / "theme.wav")
        make_wav(studio.AUDIO / "impact.wav")

    def reset_paths(self) -> None:
        studio.AUDIO, studio.STATE, studio.BACKUPS, studio.LEVELS = self.old

    def test_recursive_tabs_and_private_backup_exclusion(self):
        library = studio.media_files()
        self.assertEqual([(r["group"], r["name"]) for r in library],
                         [("Effects", "impact.wav"), ("Music", "theme.wav")])
        self.assertFalse(any(".ost_studio" in x["path"] for x in library))

    def test_path_traversal_rejected(self):
        for name in ("../outside.wav", "/outside.wav", ".ost_studio/levels.json",
                     "music\\theme.wav", "missing.wav"):
            with self.subTest(name=name):
                with self.assertRaises((ValueError, OSError)):
                    studio.resolve_audio(name)

    @unittest.skipUnless(studio.FFMPEG and studio.FFPROBE, "FFmpeg/FFprobe not installed")
    def test_gain_is_always_from_first_original(self):
        audio = studio.AUDIO / "music" / "theme.wav"
        pristine_hash = digest(audio)
        original_peak = first_peak(audio)
        result = studio.save_audio("music/theme.wav", 50)
        backup = studio.AUDIO / result["backup"]
        self.assertTrue(backup.is_file())
        self.assertEqual(digest(backup), pristine_hash)
        self.assertAlmostEqual(first_peak(audio), original_peak * 0.5, delta=3)
        studio.save_audio("music/theme.wav", 150)
        self.assertAlmostEqual(first_peak(audio), original_peak * 1.5, delta=4)
        self.assertEqual(digest(backup), pristine_hash)
        studio.save_audio("music/theme.wav", 100)
        self.assertEqual(digest(audio), pristine_hash)
        self.assertEqual(studio.load_levels()["music/theme.wav"], 100)

    @unittest.skipUnless(studio.FFMPEG and studio.FFPROBE, "FFmpeg/FFprobe not installed")
    def test_failed_export_keeps_current_file_and_original(self):
        path = studio.AUDIO / "impact.wav"
        source_hash = digest(path)
        old_codec = studio.codec_args
        studio.codec_args = lambda *_: ["-c:a", "not_a_codec"]
        try:
            with self.assertRaises(RuntimeError):
                studio.save_audio("impact.wav", 150)
        finally:
            studio.codec_args = old_codec
        self.assertEqual(digest(path), source_hash)
        self.assertEqual(digest(studio.BACKUPS / "impact.wav"), source_hash)

    @unittest.skipUnless(studio.FFMPEG and studio.FFPROBE, "FFmpeg/FFprobe not installed")
    def test_peak_scan_original_after_save(self):
        first = studio.peak_scan("music/theme.wav")["peak_dbfs"]
        studio.save_audio("music/theme.wav", 180)
        original = studio.peak_scan("music/theme.wav")["peak_dbfs"]
        self.assertAlmostEqual(original, first, delta=0.1)


if __name__ == "__main__":
    unittest.main()
