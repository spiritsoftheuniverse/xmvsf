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
        self.assertIn("read-only source PNG", (HERE / "README.md").read_text(encoding="utf-8"))
        self.assertIn('id="portraitPreview"', html)
        self.assertIn('class="preview-pair"', html)
        self.assertIn('id="preview"', html)
        self.assertIn("kind==='sprite'||kind==='viewer'", html)
        self.assertIn("paintSprite(canvas,img,'portrait')", html)
        self.assertIn("if(kind==='portrait'&&original()&&target)", html)
        self.assertIn("function drawProfileStrip()", html)

    def test_missing_rom_does_not_disable_new_and_explains_missing_sprite(self):
        html = (HERE / "palette_studio.html").read_text(encoding="utf-8")
        server = (HERE / "palette_studio_server.py").read_text(encoding="utf-8")
        # A failed auto-load must not strand the user with an inert button.
        self.assertIn("$('newProfile').disabled=false;", html)
        self.assertNotIn("newProfile').disabled=!original()", html)
        self.assertIn("state.newAfterLoad=true;", html)
        self.assertIn("if(state.newAfterLoad){state.newAfterLoad=false;makeProfile();}", html)
        self.assertIn("$('romInput').click();", html)
        self.assertIn("source_warning", html)
        self.assertIn('"source_warning": SOURCE_WARNING', server)
        # No actual Model viewer export -> visible, actionable explanation.
        self.assertIn("NO NATIVE EXAMPLE", html)
        self.assertIn("Generate 3 examples", html)
        self.assertIn("NO NATIVE EXAMPLE", html)

    def test_custom_palette_name_and_unified_bank_controls(self):
        html = (HERE / "palette_studio.html").read_text(encoding="utf-8")
        self.assertIn('for="selectedName">Custom palette name', html)
        self.assertIn('id="selectedName" type="text"', html)
        self.assertIn("state.name=$('selectedName').value.trim();", html)
        self.assertIn("state.colors[0][state.line][state.pen]=hex.toUpperCase();", html)
        self.assertIn("state.colors[1][state.line][state.pen]=hex.toUpperCase();", html)
        self.assertIn("state.colors=[copy(base),copy(base)];", html)
        self.assertNotIn('id="setP"', html)
        self.assertNotIn('id="setK"', html)
        self.assertNotIn("$('setP')", html)
        self.assertNotIn("$('setK')", html)

    def test_two_originals_appear_as_read_only_dropdown_options(self):
        html = (HERE / "palette_studio.html").read_text(encoding="utf-8")
        self.assertIn("['original','Original P · read only']", html)
        self.assertIn("['original-k','Original K · read only']", html)
        self.assertIn("state.set=mode==='original-k'?1:0", html)
        self.assertIn("state.mode==='original-k'?'Original K · read only'", html)
        self.assertIn("state.colors=[copy(base),copy(base)];state.set=0;", html)
        self.assertIn('id="profileStrip"', html)
        self.assertIn("function drawProfileStrip()", html)
        self.assertIn("fetchProfilePreviews()", html)
        self.assertIn("button.onclick=()=>openProfile(entry.mode==='saved'?entry.id:entry.mode)", html)
        self.assertIn("paintSprite(canvas,img,image.type,{colours:entry.colours,bank:entry.bank}", html)
        self.assertNotIn('id="spriteBase"', html)
        self.assertNotIn('id="spriteStrip"', html)
        self.assertNotIn("spriteBase').onchange", html)
        self.assertNotIn('id="generateAllSprites"', html)
        self.assertIn("drawExampleTypes()", html)

    def test_native_sprite_generator_calls_headless_recomp_only_on_command(self):
        server = (HERE / "palette_studio_server.py").read_text(encoding="utf-8")
        model_header = (HERE.parent / "recomp" / "model" / "model.h").read_text(encoding="utf-8")
        model_cpp = (HERE.parent / "recomp" / "model" / "model.cpp").read_text(encoding="utf-8")
        main = (HERE.parent / "recomp" / "port" / "main.cpp").read_text(encoding="utf-8")
        html = (HERE / "palette_studio.html").read_text(encoding="utf-8")
        self.assertIn('bool ExportStandingPng(const std::string &folder);', model_header)
        self.assertIn('bool ExportStandingPng(const std::string &folder) {', model_cpp)
        self.assertIn('Cut({frame}, g_modelWidth, Video::kHeight', model_cpp)
        self.assertIn('image_files::WritePng(target, pictures[0], 1)', model_cpp)
        self.assertIn("model::ExportStandingPng(out)", main)
        self.assertIn("model::Scan(*p.machine)", main)
        self.assertIn("model::AnimationForLine(1)", main)
        self.assertIn("model::AnimationForLine(2)", main)
        self.assertIn("model::AnimationShown() == choice", main)
        self.assertIn("model::ExportPaletteExamplePng(*p.machine, out, line)", main)
        self.assertIn("model::ShowDetachedEffectsForSamples(true)", main)
        self.assertIn("model::ShowDetachedEffectsForSamples(false)", main)
        self.assertIn("g_d.nearestOnly && !g_sampleDetachedEffects", model_cpp)
        self.assertIn("bool ExportPaletteExamplePng(Machine &machine", model_header)
        self.assertIn("CountLines(machine, counts);", model_cpp)
        self.assertIn('line == 1 ? " Effect A.png" : " Effect B.png"', model_cpp)
        self.assertIn('--palette-sprites N', main)
        self.assertIn('std::strcmp(c.name, "model_start")', main)
        self.assertIn('model::CharacterNumber() == character', main)
        self.assertIn('creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)', server)
        self.assertIn('route == "/api/capture"', server)
        self.assertIn('route == "/api/capture-status"', server)
        self.assertIn("ROOT / \"recomp\" / \"exports\" / \"palette_studio\"", server)
        self.assertIn('id="generateExamples"', html)
        self.assertNotIn('id="generateSprite"', html)
        self.assertNotIn('id="generateAllSprites"', html)
        self.assertIn('function startCapture(who)', html)
        self.assertIn("if(kind==='portrait'&&original()&&target)", html)
        self.assertIn("drawProfileStrip();drawExampleTypes();drawPreview();drawPortrait();", html)

    def test_native_capture_missing_effect_is_not_reported_as_ready(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as folder:
            old_root, old_job = studio.ROOT, studio.CAPTURE_JOB.copy()
            studio.ROOT = Path(folder)
            try:
                fake = SimpleNamespace(returncode=1, stdout="",
                    stderr="Palette Studio Body / Character: EXPORTED\\n"
                           "Palette Studio Effect A: EXPORTED\\n"
                           "Palette Studio Effect B: NO NATIVE ANIMATION\\n")
                with mock.patch.object(studio.subprocess, "run", return_value=fake):
                    studio.native_capture_worker("0")
                self.assertEqual(studio.CAPTURE_JOB["status"], "failed")
                self.assertEqual(studio.CAPTURE_JOB["exported"], 2)
                self.assertIn("2/3", studio.CAPTURE_JOB["message"])
            finally:
                studio.ROOT = old_root
                studio.CAPTURE_JOB.clear()
                studio.CAPTURE_JOB.update(old_job)

    def test_explicit_regeneration_clears_only_three_generated_examples(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as folder:
            old_root, old_job = studio.ROOT, studio.CAPTURE_JOB.copy()
            studio.ROOT = Path(folder)
            examples = Path(folder) / "recomp" / "exports" / "palette_studio"
            examples.mkdir(parents=True)
            for name in ("Wolverine.png", "Wolverine Effect A.png",
                         "Wolverine Effect B.png", "Ken.png"):
                (examples / name).write_bytes(b"old sprite")
            try:
                fake = SimpleNamespace(returncode=1, stdout="",
                    stderr="Palette Studio Body / Character: NOT DRAWN\\n")
                with mock.patch.object(studio.subprocess, "run", return_value=fake):
                    studio.native_capture_worker("0")
                for name in ("Wolverine.png", "Wolverine Effect A.png",
                             "Wolverine Effect B.png"):
                    self.assertFalse((examples / name).exists())
                self.assertTrue((examples / "Ken.png").exists())
            finally:
                studio.ROOT = old_root
                studio.CAPTURE_JOB.clear()
                studio.CAPTURE_JOB.update(old_job)

    def test_native_capture_worker_uses_fixed_recomp_and_isolated_settings(self):
        # Simulate success without running Windows or requiring copyrighted
        # game ROMs. The process must receive a fixed array, never a shell.
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as folder:
            old_root, old_status = studio.ROOT, studio.CAPTURE_JOB.copy()
            studio.ROOT = Path(folder)
            try:
                fake = SimpleNamespace(returncode=0,
                    stdout="Palette Studio Body / Character: EXPORTED\\n"
                           "Palette Studio Effect A: EXPORTED\\n"
                           "Palette Studio Effect B: EXPORTED\\n", stderr="")
                with mock.patch.object(studio.subprocess, "run", return_value=fake) as called:
                    studio.native_capture_worker("0")
                cmd = called.call_args.args[0]
                self.assertEqual(Path(cmd[0]).name, "xmvsf.exe")
                self.assertEqual(cmd[-2:], ["--palette-sprites", "0"])
                self.assertIn("palette_studio_eeprom.bin", " ".join(cmd))
                self.assertIn("palette_studio_settings.ini", " ".join(cmd))
                self.assertEqual(called.call_args.kwargs["cwd"], Path(folder) / "recomp")
                self.assertNotIn("shell", called.call_args.kwargs)
                self.assertFalse(studio.CAPTURE_JOB["running"])
                self.assertEqual(studio.CAPTURE_JOB["status"], "ready")
                self.assertEqual(studio.CAPTURE_JOB["exported"], 3)
            finally:
                studio.ROOT = old_root
                studio.CAPTURE_JOB.clear()
                studio.CAPTURE_JOB.update(old_status)

    def test_native_sprite_capture_does_not_execute_arbitrary_command(self):
        # A browser request can ONLY choose a character index; paths and
        # executable are pinned in the server, never supplied by the client.
        with self.assertRaises(ValueError):
            studio.start_native_capture("../something")
        with self.assertRaises(ValueError):
            studio.start_native_capture("19")
        with self.assertRaises(ValueError):
            studio.start_native_capture("1;cmd")

    def test_model_viewer_standing_export_discovery(self):
        # The recomp's Model viewer uses bare Character.png for standing
        # and Character N.png for a repeated standing export. Never infer
        # animation names or a different character by prefix.
        with tempfile.TemporaryDirectory() as folder:
            old = studio.VIEWER_EXPORTS
            studio.VIEWER_EXPORTS = (Path(folder),)
            try:
                for name in ("Ryu.png", "Ryu Effect A.png", "Ryu Effect B.png",
                             "Ryu 2.png", "Ryu A12.png",
                             "Ryu 1.png", "Ryu-special.png", "Ken.png",
                             "Chun-Li.png", "Chun-Li (Alpha).png"):
                    (Path(folder) / name).write_bytes(b"synthetic-data")
                matches = studio.model_viewer_exports(9)
                self.assertEqual([f.name for f in matches], ["Ryu.png", "Ryu Effect A.png", "Ryu Effect B.png", "Ryu 2.png"])
                examples = [v for v in studio.sprite_list(9) if v["type"] == "viewer"]
                self.assertEqual([(v["name"], v["line"]) for v in examples[:3]],
                                 [("Body / Character", 0), ("Effect A", 1), ("Effect B", 2)])
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
                self.assertEqual(sprites[0]["name"], "Body / Character")
                self.assertEqual(sprites[0]["line"], 0)
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

    def test_profile_thumbnail_data_is_saved_and_source_validated(self):
        doc = self.make_doc()
        doc["name"] = "Blue Scheme"
        doc["colours"][0][0][1] = "#112233"
        saved = studio.save_profile(doc)
        items = studio.profile_previews(9)
        self.assertEqual(len(items), 1)
        self.assertEqual(items[0]["id"], saved["id"])
        self.assertEqual(items[0]["name"], "Blue Scheme")
        self.assertEqual(items[0]["colours"][0][0][1], "#112233")
        self.assertEqual(studio.profile_previews(10), [])
        studio.SOURCE["sha256"] = "other-ROM"
        self.assertEqual(studio.profile_previews(9), [])

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
