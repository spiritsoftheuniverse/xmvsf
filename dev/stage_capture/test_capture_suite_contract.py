"""Regression guard for the Windows TEST / AO / MVC1 F11 capture suite.

Read-only static checks (no FBNeo/ROM execution or GitHub Actions). Native
runtime and the Windows BAT interpreter must still be checked locally.
"""
from __future__ import annotations

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def source(relative: str) -> str:
    return (ROOT / relative).read_text(encoding='utf-8')


class SuiteContractTests(unittest.TestCase):
    def test_option_driven_launcher_keeps_legacy_commands(self):
        batch = source('RUN_TEST.bat')
        self.assertIn('choice /C 123', batch)
        self.assertIn('choice /C 1Q', batch)
        for option, label in [('1', ':suite_test'), ('2', ':suite_asset'),
                              ('3', ':suite_capture')]:
            self.assertIn(f'if "%~1"=="{option}" goto {label}', batch)
        for old in ('"art" goto :offline_mvc_art_review',
                    '"m68k" goto :offline_m68k_sprite_analysis',
                    '"poses" goto :offline_m68k_pose_analysis',
                    '"sprites" goto :offline_sprite_factory'):
            self.assertIn(old, batch)
        self.assertIn('set "SOURCE_ROM=xmvsfu"', batch)
        self.assertIn('set "SOURCE_ROM=mvscur1"', batch)

    def test_option_one_cannot_inherit_choice_errorlevel_for_observer_log(self):
        batch = source('RUN_TEST.bat')
        self.assertIn('choice /C 123', batch)
        self.assertIn('type nul >>"%XM87181_DIAGNOSTIC_STATUS%" 2>nul', batch)
        start = batch.index('set "XM87181_DIAGNOSTIC_STATUS=')
        canary = batch.index('type nul >>"%XM87181_DIAGNOSTIC_STATUS%" 2>nul', start)
        error = batch.index('if errorlevel 1 (', canary)
        marker = batch.index('echo RUN_TEST_START date=', error)
        self.assertLess(start, canary)
        self.assertLess(canary, error)
        self.assertLess(error, marker)
        self.assertNotIn('echo RUN_TEST_START date=', batch[canary:error])

    def test_xmvsf_test_game_enters_native_boot_shell_not_direct_rom(self):
        launcher = source('RUN_TEST.bat')
        host = source('dev/stable/buildtree/src/burner/sdl/main.cpp')
        self.assertIn('set "SOURCE_ROM=xmvsfu"', launcher)
        self.assertIn('if /I "%SOURCE_ROM%"=="xmvsfu" (', launcher)
        start = launcher.index('REM Restore the pre-merge zero-argument native TEST launch for XMVSF.')
        end = launcher.index('set "RC=%ERRORLEVEL%"', start)
        launch_block = launcher[start:end]
        self.assertIn('"%EXE%"\\n) else ('.replace('\\n','\n'), launch_block)
        self.assertIn('"%EXE%" %SOURCE_ROM% -joy', launch_block)
        self.assertLess(launch_block.index('"%EXE%"\\n'.replace('\\n','\n')),
                        launch_block.index('"%EXE%" %SOURCE_ROM% -joy'))
        self.assertIn('romname == NULL && !usemenu && !dat', host)
        self.assertIn('xmvsfBootSelector()', host)
        self.assertIn('romname = "xmvsfu";', host)

    def test_ao_build_is_independent_and_compiles_only_offline_art(self):
        builder = source('asset_organizer/BUILD_AO.bat')
        root = source('RUN_TEST.bat')
        self.assertIn('RUN_TEST.bat" art build', builder)
        self.assertIn('if /I "%~2"=="build" (', root)
        self.assertIn(':offline_mvc_art_launch', root)
        self.assertIn('BUILD_AO.bat', root)
        self.assertIn('BUILD_MVC_ART_REVIEW.py', root)
        self.assertIn('BUILD_SOURCE_FAMILIES.py', root)
        self.assertIn('BUILD_FOREGROUND_MAPS.py', root)
        self.assertNotIn('BUILD_TEST.bat', builder)
        self.assertNotIn('make clean', builder)

    def test_native_builder_compiles_and_checks_publication(self):
        batch = source('BUILD_TEST.bat')
        shell = source('dev/dhc/BUILD_TEST.sh')
        self.assertIn('bash.exe BUILD_TEST.sh', batch)
        self.assertIn('XMVSF_Test_87_203.exe', batch)
        self.assertIn('MAKELEVEL=1 make -f makefile.sdl2', shell)
        self.assertIn('cp -p "$BUILT" "$PENDING"', shell)
        self.assertIn('mv -f "$PENDING" "$DEST"', shell)
        self.assertNotIn('make clean', shell)

    def test_hotkeys_early_and_separate(self):
        run = source('dev/stable/buildtree/src/burner/sdl/run.cpp')
        f9 = run.index('if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F9)')
        f10 = run.index('if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F10)')
        f11 = run.index('if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F11)')
        replay = run.index('if(XM8764::isPlayback() &&', f11)
        self.assertLess(f9, f10)
        self.assertLess(f10, f11)
        self.assertLess(f11, replay)
        self.assertIn('XM8764::toggleF9()', run)
        self.assertIn('XM87139Probe::state().toggle(bDrvOkay!=0)', run)
        self.assertIn('XM87203_LEVEL_CAPTURE_ACTIVE', run)
        self.assertIn('KMOD_SHIFT', run[f11:replay])
        self.assertIn('bAppShowFPS=!bAppShowFPS', run[f11:replay])

    def test_capture_starts_disarmed_and_seals_at_f11_stop(self):
        launcher = source('RUN_TEST.bat')
        emulator = source('dev/stable/buildtree/src/burn/drv/capcom/cps_run.cpp')
        self.assertIn('set "XM87203_LEVEL_CAPTURE_ACTIVE=0"', launcher)
        self.assertIn('set "XM87203_STAGE_CAPTURE_ENABLE=1"', launcher)
        self.assertIn('captureRequested && !bBurnRunAheadFrame', emulator)
        self.assertIn('F11_CAPTURE_STOP', emulator)
        self.assertIn('XM87203Stage::state().resetBatch()', emulator)
        self.assertIn('XM87203Stage::state().observe(f)', emulator)
        self.assertIn('XM87203Stage::isMarvelVsCapcom', emulator)


if __name__ == '__main__':
    unittest.main()
