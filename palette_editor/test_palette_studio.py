#!/usr/bin/env python3
"""Local regression tests for the ROM-backed XMVSF Palette Studio.

Synthetic program bytes only. Does not need the user's copyrighted ROM.
Run: py -3 palette_editor\test_palette_studio.py
"""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from io import BytesIO
from unittest import mock

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("xmvsf_palette_server", HERE / "palette_studio_server.py")
assert spec and spec.loader
studio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(studio)


def synthetic_program() -> bytes:
    """19 safe palette pointers + deterministic 0xFRGB colour words."""
    mem = bytearray(studio.ROM_SIZE)
    offset = studio.TABLE_ADDRESS - studio.PROGRAM_START
    for who in range(len(studio.CHARACTERS)):
        base = studio.PROGRAM_START + 0x20000 + who * 0xC0
        # Raw xvs.05a is word-swapped relative to recomp Program().
        pointer = base.to_bytes(4, "big")
        mem[offset + who * 4:offset + who * 4 + 4] = bytes(
            (pointer[1], pointer[0], pointer[3], pointer[2]))
        for set_no in range(2):
            for line in range(3):
                for pen in range(16):
                    word = 0xF000 | (((who + pen) & 15) << 8) | (((set_no + pen) & 15) << 4) | ((line + pen) & 15)
                    at = base - studio.PROGRAM_START + set_no * studio.SET_SIZE + line * 32 + pen * 2
                    mem[at:at + 2] = word.to_bytes(2, "little")
    return bytes(mem)


class DecoderTests(unittest.TestCase):
    def setUp(self) -> None:
        self.program = synthetic_program()
        self.originals, self.addresses = studio.decode_palettes(self.program, verify_crc=False)

    def test_exact_character_roster(self):
        self.assertEqual(len(self.originals), 19)
        self.assertEqual(studio.CHARACTERS[8], "Apocalypse")
        self.assertEqual(studio.CHARACTERS[18], "Chun-Li (Alpha)")
        self.assertEqual(studio.ORIGINAL_CRC, 0x7DB6025D)

    def test_exact_68000_addresses_and_shapes(self):
        base = studio.PROGRAM_START + 0x20000
        self.assertEqual(self.addresses[0][0][0][0], f"0x{base:06X}")
        self.assertEqual(self.addresses[0][1][2][15], f"0x{base + 0x60 + 2 * 32 + 15 * 2:06X}")
        self.assertEqual(self.addresses[18][0][0][0], f"0x{base + 18 * 0xC0:06X}")
        self.assertEqual([len(s) for s in self.originals[0]], [3, 3])
        self.assertEqual(len(self.originals[0][1][2]), 16)

    def test_rom_file_endianness_matches_recomp_loader(self):
        # The xvs.05a file stores low byte before high byte, and the
        # original recomp explicitly swaps these pairs when loading.
        source = (HERE.parent / "recomp" / "machine" / "romset.cpp").read_text(encoding="utf-8")
        self.assertIn("set.program.push_back(data[program[n].highFirst ? i : i + 1]);", source)
        self.assertIn('int.from_bytes(program[off:off + 2], "little")',
                      (HERE / "palette_studio_server.py").read_text(encoding="utf-8"))
        table = studio.TABLE_ADDRESS - studio.PROGRAM_START
        # 0x120000 is native big-endian, but on disk appears 12 00 00 00.
        self.assertEqual(self.program[table:table + 4], bytes.fromhex("12000000"))
        self.assertEqual(self.originals[0][0][0][1], "#111111")
        # The former decoder interpreted raw 12 00 00 00 as address 0x12000000.
        self.assertNotEqual(int.from_bytes(self.program[table:table + 4], "big"), 0x120000)

    def test_colour_translation_is_4bit(self):
        # For character=0, set=P, line=Body, pen=1 -> 0xF111.
        self.assertEqual(self.originals[0][0][0][1], "#111111")
        self.assertEqual(self.originals[0][1][2][15], "#FF0011")
        self.assertEqual(studio.quantized_hex(0xFABC), "#AABBCC")

    def test_wrong_crc_and_bad_pointer_are_refused(self):
        with self.assertRaises(ValueError):
            studio.decode_palettes(self.program)
        with self.assertRaises(ValueError):
            studio.extract_program(self.program)
        corrupted = bytearray(self.program)
        at = studio.TABLE_ADDRESS - studio.PROGRAM_START
        corrupted[at:at + 4] = (0x500000).to_bytes(4, "big")
        with self.assertRaises(ValueError):
            studio.decode_palettes(bytes(corrupted), verify_crc=False)

    def test_zip_rom_rejects_wrong_crc_without_writing(self):
        stream = BytesIO()
        with zipfile.ZipFile(stream, "w") as archive:
            archive.writestr("xmvsf/xvs.05a", self.program)
        with self.assertRaises(ValueError):
            studio.extract_program(stream.getvalue())

    def test_original_colors_are_immutable(self):
        reference = copy.deepcopy(self.originals)
        working = copy.deepcopy(self.originals[9])
        working[0][0][1] = "#AABBCC"
        self.assertEqual(self.originals, reference)
        self.assertNotEqual(working, reference[9])

    def test_original_png_sources_are_reference_only(self):
        # Portraits are always alongside the source-native combat frame,
        # and remain unchanged by palette edits.
        html = (HERE / "palette_studio.html").read_text(encoding="utf-8")
        self.assertIn("Portrait reference", (HERE / "palette_studio_server.py").read_text(encoding="utf-8"))
        self.assertIn("portrait is **never**", (HERE / "README.md").read_text(encoding="utf-8"))
        self.assertIn('id="portraitPreview"', html)
        self.assertIn('class="preview-pair"', html)
        self.assertIn('id="preview"', html)
        self.assertIn("kind==='sprite'||kind==='viewer'", html)
        self.assertIn("paintSprite(canvas,img,'portrait')", html)
        self.assertIn("sprite.type==='portrait'", html)
        self.assertIn("refreshSprites", html)

    def test_model_viewer_standing_export_discovery(self):
        # The recomp's Model viewer uses bare Character.png for standing
        # and Character N.png for a repeated standing export. Never infer
        # animation names or a different character by prefix.
        with tempfile.TemporaryDirectory() as folder:
            old = studio.VIEWER_EXPORTS
            studio.VIEWER_EXPORTS = (Path(folder),)
            try:
                for name in ("Ryu.png", "Ryu 2.png", "Ryu A12.png",
                             "Ryu 1.png", "Ryu-special.png", "Ken.png",
                             "Chun-Li.png", "Chun-Li (Alpha).png"):
                    (Path(folder) / name).write_bytes(b"synthetic-data")
                matches = studio.model_viewer_exports(9)
                self.assertEqual([f.name for f in matches], ["Ryu.png", "Ryu 2.png"])
                self.assertEqual([f.name for f in studio.model_viewer_exports(10)], ["Ken.png"])
                self.assertEqual([f.name for f in studio.model_viewer_exports(11)], ["Chun-Li.png"])
                self.assertEqual([f.name for f in studio.model_viewer_exports(18)], ["Chun-Li (Alpha).png"])
                self.assertEqual(studio.viewer_asset(9, "Ryu.png").name, "Ryu.png")
                with self.assertRaises(ValueError):
                    studio.viewer_asset(9, "Ken.png")
                with self.assertRaises(ValueError):
                    studio.viewer_asset(9, "../Ryu.png")
                with self.assertRaises(ValueError):
                    studio.viewer_asset(9, "Ryu A12.png")
            finally:
                studio.VIEWER_EXPORTS = old

    def test_model_viewer_png_is_discovered_without_upload(self):
        with tempfile.TemporaryDirectory() as folder:
            old = studio.VIEWER_EXPORTS
            old_sprites = studio.SPRITES
            old_portraits = studio.PORTRAITS
            studio.VIEWER_EXPORTS = (Path(folder),)
            studio.SPRITES = Path(folder) / "sprite-storage"
            studio.PORTRAITS = Path(folder) / "portraits"
            try:
                (Path(folder) / "Wolverine.png").write_bytes(b"synthetic-data")
                sprites = studio.sprite_list(0)
                self.assertEqual(len(sprites), 1)
                self.assertEqual(sprites[0]["type"], "viewer")
                self.assertIn("Wolverine.png", sprites[0]["url"])
                self.assertEqual(sprites[0]["name"], "Standing")
                self.assertEqual(sprites[0]["url"].count("asset/viewer/"), 1)
            finally:
                studio.VIEWER_EXPORTS = old
                studio.SPRITES = old_sprites
                studio.PORTRAITS = old_portraits


class ProfileTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.orig_profile_dir = studio.PROFILES
        self.orig_source = studio.SOURCE
        studio.PROFILES = Path(self.tmp.name)
        originals, addresses = studio.decode_palettes(synthetic_program(), verify_crc=False)
        studio.SOURCE = {"label": "synthetic for testing", "sha256": "test-source-sha",
                         "crc32": "DUMMY", "originals": originals, "addresses": addresses}
        self.addCleanup(self.restore)

    def restore(self) -> None:
        studio.PROFILES = self.orig_profile_dir
        studio.SOURCE = self.orig_source

    def make_doc(self) -> dict:
        return {"id": None, "name": "Color Test", "character": 9,
                "source_sha256": "test-source-sha",
                "colours": copy.deepcopy(studio.SOURCE["originals"][9])}

    def test_create_save_open_and_export(self):
        doc = self.make_doc()
        doc["colours"][0][0][1] = "#AABBCC"
        saved = studio.save_profile(doc)
        self.assertRegex(saved["id"], r"^[0-9a-f]{16}$")
        self.assertTrue(studio.profile_path(9, saved["id"]).is_file())
        self.assertEqual(len(studio.profile_list(9)), 1)
        loaded = studio.read_profile(9, saved["id"])
        self.assertEqual(loaded["colours"][0][0][1], "#AABBCC")
        self.assertEqual(studio.SOURCE["originals"][9][0][0][1], "#AA1111")
        exported = studio.export_text(9, loaded)
        self.assertIn("[P]", exported)
        self.assertIn("[K]", exported)
        self.assertIn("#AABBCC", exported)
        self.assertIn("\n-\n", exported)
        self.assertTrue(exported.endswith("\n"))

    def test_repeated_save_uses_same_profile_file(self):
        obj = self.make_doc()
        saved = studio.save_profile(obj)
        obj["id"] = saved["id"]
        obj["colours"][1][2][5] = "#000000"
        studio.save_profile(obj)
        self.assertEqual(len(studio.profile_list(9)), 1)
        self.assertEqual(studio.read_profile(9, saved["id"])["colours"][1][2][5], "#000000")

    def test_profile_original_sha_is_enforced(self):
        obj = self.make_doc()
        obj["source_sha256"] = "another-rom"
        with self.assertRaises(ValueError):
            studio.save_profile(obj)
        obj["source_sha256"] = "test-source-sha"
        saved = studio.save_profile(obj)
        studio.SOURCE["sha256"] = "new-source"
        with self.assertRaises(ValueError):
            studio.read_profile(9, saved["id"])

    def test_rejects_unsafe_profile_input(self):
        doc = self.make_doc()
        for name in ("../bad", "x/y", " ", "n" * 61):
            doc["name"] = name
            with self.assertRaises(ValueError):
                studio.save_profile(doc)
        doc["name"] = "Good"
        doc["colours"][0][0][0] = "#F01234"
        with self.assertRaises(ValueError):
            studio.save_profile(doc)
        doc["colours"][0][0][0] = "#AA1122"
        doc["colours"][1][2] = []
        with self.assertRaises(ValueError):
            studio.save_profile(doc)

    def test_profile_id_cannot_escape_folder(self):
        for unsafe in ("../main", "..", "/tmp/foo", "x" * 20):
            with self.assertRaises(ValueError):
                studio.profile_path(9, unsafe)
        with self.assertRaises(ValueError):
            studio.read_profile(18, "0123456789abcdef")

    def test_user_content_never_overwrites_rom(self):
        source_path = Path(self.tmp.name) / "xvs.05a"
        original = synthetic_program()
        source_path.write_bytes(original)
        studio.save_profile(self.make_doc())
        self.assertEqual(source_path.read_bytes(), original)


if __name__ == "__main__":
    unittest.main(verbosity=2)
