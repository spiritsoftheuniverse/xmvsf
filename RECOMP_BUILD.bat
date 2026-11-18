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
where git.exe >nul 2>&1
if errorlevel 1 (echo ERROR: Git not found.& goto :fail)
git -C "%PROJECT%" submodule update --init --recursive
if errorlevel 1 (echo ERROR: Could not fetch pinned CPS-2 Recomp sources.& goto :fail)
if not exist "%PROJECT%\recomp\Makefile" (echo ERROR: Recomp source missing.& goto :fail)
if not exist "%PROJECT%\recomp\rom\xmvsf" (
 echo ERROR: Supply your own compatible XMVSF 961004 ROM set, including its key.
 echo Location: "%PROJECT%\recomp\rom\xmvsf"
 goto :fail
)
if not exist "%PROJECT%\test\build_logs" mkdir "%PROJECT%\test\build_logs"
pushd "%PROJECT%\recomp"
if errorlevel 1 goto :fail
echo Building the upstream recomp for XMVSF only. This does NOT alter Stable or 87.434.
"%BASH%" -c "set -o pipefail; make GAME=xmvsf NO_APP=1 2>&1 | tee ../test/build_logs/RECOMP_BUILD.log"
if errorlevel 1 (popd& goto :fail)
echo Discovering and generating the M68000 C++ translation...
"%BASH%" -c "set -o pipefail; make GAME=xmvsf explore 2>&1 | tee -a ../test/build_logs/RECOMP_BUILD.log"
if errorlevel 1 (popd& goto :fail)
"%BASH%" -c "set -o pipefail; make GAME=xmvsf NO_APP=1 2>&1 | tee -a ../test/build_logs/RECOMP_BUILD.log"
if errorlevel 1 (popd& goto :fail)
popd
echo.
echo Recomp build reported success. Start RECOMP_RUN.bat.
echo Build log: "%PROJECT%\test\build_logs\RECOMP_BUILD.log"
pause
exit /b 0
:fail
echo.
echo Recomp build did not complete. Check UCRT64 SDL3, CMake, Ninja, Python3, Zlib, and the ROM set.
pause
exit /b 1
