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
  echo Fetching pinned upstream source submodule...
  where git.exe >nul 2>nul
  if errorlevel 1 (echo ERROR: Git not found.& goto :fail)
  git -C "%PROJECT%" submodule update --init --recursive
  if errorlevel 1 (echo ERROR: Recomp checkout failed.& goto :fail)
)
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
