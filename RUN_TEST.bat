@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_139.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.139 TEST executable is missing.
  echo Run BUILD_TEST.bat first. Will NOT fall back to 87.132.4 or Stable.
  goto :fail
)
if not exist "%PROJECT%\FBNeo" (
  echo ERROR: FBNeo game data directory is missing.
  goto :fail
)
REM 87.139 isolation: disable the old 87.138 normal-tag mutation.
set "XM87138_TAG_LAND_TEST=0"
set "XM8761_DISABLE_RUNAHEAD=1"
set "XM87139_PROBE_DIR=%PROJECT%\test\87.139"
if not exist "%XM87139_PROBE_DIR%" mkdir "%XM87139_PROBE_DIR%"
if not exist "%XM87139_PROBE_DIR%" (
  echo ERROR: Cannot create diagnostic output directory.
  goto :fail
)
echo XMVSF Stage 87.139 TEST - UNMODIFIED native tags, F10 input/actor/OAM recorder
echo Executable: %EXE%
echo F10 starts/stops probing. F9 remains replay recording; Shift+F10 quick-load.
echo Diagnostic files: %XM87139_PROBE_DIR%
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%"
set "RC=%ERRORLEVEL%"
popd
echo.
echo Stage 87.139 TEST exit code: %RC%
echo Probe directory: %XM87139_PROBE_DIR%
pause
exit /b %RC%
:fail
echo.
echo TEST RUN STOPPED. No older executable was launched.
pause
exit /b 1
