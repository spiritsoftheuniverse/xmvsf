@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_142.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.142 TEST executable is missing.
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
set "XM87139_PROBE_DIR=%PROJECT%\test\87.142"
set "XM87140_CHAT_DIR=%PROJECT%\test\87.142"
if not exist "%XM87139_PROBE_DIR%" mkdir "%XM87139_PROBE_DIR%"
if not exist "%XM87139_PROBE_DIR%" (
  echo ERROR: Cannot create diagnostic output directory.
  goto :fail
)
if not exist "%XM87140_CHAT_DIR%" mkdir "%XM87140_CHAT_DIR%"
if not exist "%XM87140_CHAT_DIR%" (
  echo ERROR: Cannot create chat log directory.
  goto :fail
)
set "XM87140_CHAT_LOG=%XM87140_CHAT_DIR%\super_chat.tsv"
set "XM87142_ROM_TRACE=%XM87140_CHAT_DIR%\rom_dispatch.tsv"
echo XMVSF Stage 87.142 TEST - ROM-resolved super scripts and native point/reserve tag
echo Executable: %EXE%
echo Super-candidate inputs print to Warcraft-style chat and fade after 3 sec.
echo Shift+G opens/closes the full chat history.
echo F10 native capture ON/OFF is acknowledged in chat and written to disk.
echo F9 replay; Shift+F10 quick-load.
echo Super chat transcript: %XM87140_CHAT_LOG%
echo ROM script/role events: %XM87142_ROM_TRACE%
echo F10 probe files: %XM87139_PROBE_DIR%
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%"
set "RC=%ERRORLEVEL%"
popd
echo.
echo Stage 87.142 TEST exit code: %RC%
echo Probe directory: %XM87139_PROBE_DIR%
pause
exit /b %RC%
:fail
echo.
echo TEST RUN STOPPED. No older executable was launched.
pause
exit /b 1
