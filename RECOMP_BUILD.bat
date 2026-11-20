@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "BASH=C:\msys64\usr\bin\bash.exe"
if not exist "%BASH%" (
  echo ERROR: MSYS2 UCRT64 is required at C:\msys64.
  goto :fail
)
REM Recomp requires SDL3, not the SDL2 used by the old FBNeo build.
REM Fail immediately with the exact MSYS2 UCRT64 package instead of an opaque CMake error.
set "UCRT_PREFIX=C:\msys64\ucrt64"
set "SDL3_CMAKE=%UCRT_PREFIX%\lib\cmake\SDL3\SDL3Config.cmake"
if not exist "%SDL3_CMAKE%" (
  echo ERROR: SDL3 UCRT64 development package not found.
  echo Required CMake file: "%SDL3_CMAKE%"
  echo Open MSYS2 UCRT64 and install:
  echo   pacman -S --needed mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-zlib
  goto :fail
)
REM Prefer the correct UCRT64 package discovery, never a stray Windows or MINGW64 SDL3.
set "SDL3_DIR=%UCRT_PREFIX%\lib\cmake\SDL3"
set "CMAKE_PREFIX_PATH=%UCRT_PREFIX%"

if not exist "%PROJECT%\recomp\Makefile" (
  echo ERROR: Recomp source missing.
  goto :fail
)
if not exist "%PROJECT%\recomp\rom\xmvsf" (
  echo ERROR: Supply your own compatible XMVSF 961004 ROM set with key.
  echo Location: "%PROJECT%\recomp\rom\xmvsf"
  goto :fail
)
if not exist "%PROJECT%\test\build_logs" mkdir "%PROJECT%\test\build_logs"
if not exist "%PROJECT%\test\build_logs" goto :fail
set "LOG=%PROJECT%\test\build_logs\RECOMP_BUILD.log"
pushd "%PROJECT%\recomp"
if errorlevel 1 goto :fail

REM Check all Python files and prevent stale TEST version pins BEFORE installers.
"%BASH%" -c "python3 ../recomp_port/tests/test_version_guard.py"
if errorlevel 1 (
  echo ERROR: XMVSF Python source and version policy preflight failed.
  popd
  goto :fail
)

REM Apply our original XMVSF menu as an SDL3-native overlay within the pinned recomp.
REM The installer is idempotent: Ninja only rebuilds files whose contents changed.
"%BASH%" -c "python3 ../recomp_port/apply_menu.py"
if errorlevel 1 (
  echo ERROR: XMVSF menu source integration failed. Upstream may have changed.
  popd
  goto :fail
)

REM Reuse our old SDL2 rendering policy through SDL3's accelerated 2D renderer.
REM Bypasses recomp's custom GPU sampler/intermediate smoother for XMVSF only.
"%BASH%" -c "python3 ../recomp_port/apply_classic_renderer.py"
if errorlevel 1 (
  echo ERROR: XMVSF classic renderer patch failed. Existing build is untouched.
  popd
  goto :fail
)

REM 87.441: compose original menu assets as persistent GPU textures, not a
REM full 960x540 CPU image and upload each frame (the original SDL2 model).
"%BASH%" -c "python3 ../recomp_port/apply_textured_menu.py"
if errorlevel 1 (
  echo ERROR: XMVSF texture menu integration failed. Existing build unchanged.
  popd
  goto :fail
)

REM 87.458: verify F6 native snapshots for home, Options, ingame UI, gameplay.
REM Source-only preflight; do not regenerate 68k translation or clear caches.
"%BASH%" -c "python3 ../recomp_port/tests/test_native_capture.py"
if errorlevel 1 (
  echo ERROR: XMVSF F6 native capture integrity failed. No build performed.
  popd
  goto :fail
)

REM Fail early if the three source installers disagree or an SDL integration hook
REM is missing. Source-only test; zero M68k translation or build-cache impact.
"%BASH%" -c "python3 ../recomp_port/tests/test_textured_menu.py"
if errorlevel 1 (
  echo ERROR: XMVSF SDL texture menu regression failed; not compiling.
  popd
  goto :fail
)

REM 87.464: compile curl-noise edge smoke; validate 20% entrance dither on all film layers.
"%BASH%" -c "python3 ../recomp_port/tests/test_menu_film.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.464 menu film integrity failed. No build performed.
  popd
  goto :fail
)

REM 87.454: exercise the original cold-boot clock and verify that each
REM frame/palette/audio hook was installed before touching the Ninja cache.
"%BASH%" -c "python3 ../recomp_port/tests/test_logo_intro.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.454 logo intro integrity preflight FAILED. No build performed.
  popd
  goto :fail
)

REM 87.442: native Game/Graphics/Controller tabs, real SDL3 remapping,
REM persisted INI bindings, and the current recomp Game version V2.
"%BASH%" -c "python3 ../recomp_port/apply_options.py"
if errorlevel 1 (
  echo ERROR: XMVSF native Options integration failed. No build performed.
  popd
  goto :fail
)

REM 87.451: compile and run the real C++ capture release-gate tests
REM BEFORE the controller installer or the warm incremental Ninja build.
"%BASH%" -c "python3 ../recomp_port/tests/test_capture_lifecycle.py"
if errorlevel 1 (
  echo ERROR: XMVSF capture activator/deactivator regression failed.
  popd
  goto :fail
)

REM 87.450: execute actual controller installer twice IN MEMORY before
REM any file writes or Ninja compilation. Catches regex/anchor bugs early.
"%BASH%" -c "python3 ../recomp_port/tests/test_pad_owners_preflight.py"
if errorlevel 1 (
  echo ERROR: XMVSF controller preflight FAILED. No files modified by controller installer.
  popd
  goto :fail
)

REM 87.450: Stable P1/P2 ownership, old SDL2 capture, edge-only menus.
"%BASH%" -c "python3 ../recomp_port/apply_pad_owners.py"
if errorlevel 1 (
  echo ERROR: XMVSF controller ownership patch failed. No build performed.
  popd
  goto :fail
)
"%BASH%" -c "python3 ../recomp_port/tests/test_pad_owners.py"
if errorlevel 1 (
  echo ERROR: XMVSF ownership/press-edge regression failed.
  popd
  goto :fail
)

REM Check the old menu renderer and new input hooks before warm Ninja build.
"%BASH%" -c "python3 ../recomp_port/tests/test_options.py"
if errorlevel 1 (
  echo ERROR: XMVSF Options regression failed. No build performed.
  popd
  goto :fail
)

REM 87.466: install the stage picker after film, Options and controller
REM patches; preserve the existing film, F6 and input paths.
"%BASH%" -c "python3 ../recomp_port/apply_stage_picker.py"
if errorlevel 1 (
  echo ERROR: XMVSF Training stage picker integration failed. No build performed.
  popd
  goto :fail
)
"%BASH%" -c "python3 ../recomp_port/tests/test_stage_picker.py"
if errorlevel 1 (
  echo ERROR: XMVSF Training stage picker regression failed. No build performed.
  popd
  goto :fail
)

REM 87.467: validate Training boot skips the disclaimer until character select.
REM Source-only, independent of the ROM or regenerated M68K translation.
"%BASH%" -c "python3 ../recomp_port/tests/test_training_entry.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.467 Training entry regression failed. No build performed.
  popd
  goto :fail
)

REM 87.468: validate full-screen noise reveal and existing characterselect Ogg.
"%BASH%" -c "python3 ../recomp_port/tests/test_select_reveal.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.468 selection reveal regression failed. No build performed.
  popd
  goto :fail
)

REM 87.469: confirm stage-selected Fight cue finishes BEFORE character select.
"%BASH%" -c "python3 ../recomp_port/tests/test_fight_voice_entry.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.469 Fight voice timing preflight failed. No build performed.
  popd
  goto :fail
)

REM 87.479: accepted-count input handoff without hiding native selection video/audio.
"%BASH%" -c "python3 ../recomp_port/tests/test_p2_early_takeover.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.479 FBNeo handoff and native AV regression failed. No build performed.
  popd
  goto :fail
)

REM 87.479: restore ROM-native selector OAM frames and flicker.
"%BASH%" -c "python3 ../recomp_port/tests/test_p2_selector_gap.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.479 native selector rendering / instant handoff regression failed.
  popd
  goto :fail
)

REM 87.473: syntax-check F10 native M68K capture without failing on upstream unused parameters.
REM Diagnostic only; no local ROMs, save states or generated M68K modified.
"%BASH%" -c "python3 ../recomp_port/tests/test_f10_m68k_capture.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.475 F10 M68K capture regression failed. No build performed.
  popd
  goto :fail
)

REM 87.474: ensure F10 shows REC/FRAMES while recording and SAVED/FAILED on stop.
"%BASH%" -c "python3 ../recomp_port/tests/test_f10_status.py"
if errorlevel 1 (
  echo ERROR: XMVSF 87.475 F10 on-screen status regression failed. No build performed.
  popd
  goto :fail
)

REM Default is FAST once configured. It only recompiles changed C++ objects.
REM refresh explicitly regenerates M68000 translation and reconfigures CMake.
if /I "%~1"=="refresh" goto :refresh
if exist "build\xmvsf\build.ninja" if exist "work\xmvsf\gen\native_table.cpp" goto :fast

:refresh
echo RECOMP REFRESH: regenerate translation and build (first build may be lengthy).
echo To force this later: RECOMP_BUILD.bat refresh
"%BASH%" -c "set -o pipefail; make GAME=xmvsf NO_APP=1 2>&1 | tee -a ../test/build_logs/RECOMP_BUILD.log"
if errorlevel 1 (popd& goto :fail)
REM Upstream documents that a first checkout may need a second pass.
if not exist "work\xmvsf\gen\native_table.cpp" (
  echo Initial interpreter bootstrap finished; running translation pass...
  "%BASH%" -c "set -o pipefail; make GAME=xmvsf NO_APP=1 2>&1 | tee -a ../test/build_logs/RECOMP_BUILD.log"
  if errorlevel 1 (popd& goto :fail)
)
goto :check

:fast
echo RECOMP FAST: Ninja compiles only changed files and relinks.
echo Use RECOMP_BUILD.bat refresh after changing game.cfg, translation inputs, or build configuration.
"%BASH%" -c "set -o pipefail; ninja -C build/xmvsf xmvsf 2>&1 | tee -a ../test/build_logs/RECOMP_BUILD.log"
if errorlevel 1 (popd& goto :fail)

:check
if not exist "build\xmvsf\xmvsf.exe" (
  echo ERROR: Successful command but expected recomp executable is missing.
  popd
  goto :fail
)
popd
echo.
echo Recomp build READY. Run RECOMP_RUN.bat.
echo Build log: "%LOG%"
pause
exit /b 0

:fail
echo.
echo Recomp build FAILED. Existing FBNeo Stable and TEST builds are untouched.
echo Inspect the build transcript and required tools: SDL3, CMake, Ninja, Zlib, Python 3.
pause
exit /b 1
