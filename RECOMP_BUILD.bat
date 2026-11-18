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
