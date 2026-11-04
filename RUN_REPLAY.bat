@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Dedicated replay channel. Existing BUILD_TEST/RUN_TEST names are unchanged.
REM Replay the original 87.161 F9 full-state + exact CPS2 inputs without
REM creating replacement ROM scripts, patching native input, or touching Stable.
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_162.exe"
if not exist "%EXE%" (
  echo ERROR: Need existing XMVSF_Test_87_162.exe. Run BUILD_TEST.bat.
  goto :fail
)
if not exist "%PROJECT%\FBNeo" (
  echo ERROR: FBNeo runtime game directory is missing.
  goto :fail
)
set "XM8761_REPLAY_DIR=%PROJECT%\test\87.161\replays"
set "MOVIE=%XM8761_REPLAY_DIR%\xm8761_f9_0001_inputs.bin"
if not "%~1"=="" set "MOVIE=%~f1"
if not exist "%MOVIE%" (
  echo ERROR: Native F9 input stream missing: "%MOVIE%"
  goto :fail
)
REM Original 87.61 recorder format names are a single input + state pair.
REM Substitution preserves a full path, including folders with spaces.
set "STATE=%MOVIE:_inputs.bin=_start.fs%"
if /I "%STATE%"=="%MOVIE%" (
  echo ERROR: F9 input filename must end in _inputs.bin
  goto :fail
)
if not exist "%STATE%" (
  echo ERROR: Companion full save-state missing: "%STATE%"
  goto :fail
)
set "XM8761_REPLAY_MODE=play"
set "XM8761_REPLAY_INPUT_FILE=%MOVIE%"
set "XM8761_REPLAY_STATE_FILE=%STATE%"
set "XM8761_DISABLE_RUNAHEAD=1"
REM Keep original 87.161 TEST gameplay toggles for deterministic playback.
set "XM87138_TAG_LAND_TEST=0"
set "XM87144_DHC_NATIVE_TEST=1"
set "XM87156_PROOF_NATIVE=0"
set "XM87157_EARLY_CANCEL=1"
set "XM8786_STORM_BUTTON_TEST=1"
set "XM8795_NATIVE_HIT_TEST=1"
set "XM8796_DIZZY_HANDOFF_TEST=1"
set "XM8799_GROUND_DEFENSE_TEST=1"
REM Fresh replay diagnostics must NEVER overwrite the submitted 87.161
REM original session logs or the saved movie pair.
set "TRACE=%PROJECT%\test\87.162\replay_results"
if not exist "%TRACE%" mkdir "%TRACE%"
if not exist "%TRACE%" (
  echo ERROR: Cannot create isolated replay results directory.
  goto :fail
)
set "XM87140_CHAT_DIR=%TRACE%"
set "XM87139_PROBE_DIR=%TRACE%"
set "XM87140_CHAT_LOG=%TRACE%\super_chat.tsv"
set "XM87142_ROM_TRACE=%TRACE%\rom_dispatch.tsv"
set "XM87144_DHC_LOG=%TRACE%\dhc_native_handoff.tsv"
set "XM87159_PROJECTILE_LOG=%TRACE%\p1_old_projectile_hitboxes.tsv"
set "XM87149_TOURNAMENT_AUDIT=%TRACE%\tournament_rules.tsv"
set "XM87160_DISCOVER=1"
set "XM87160_CATALOG_LOG=%TRACE%\rom_super_catalog.tsv"
set "XM87160_OBSERVED_LOG=%TRACE%\rom_super_observed.tsv"
REM Raw ROM analysis copies remain private and Git-ignored.
set "XM87160_PRIVATE_ROM_DIR=%PROJECT%\local_rom_dump"
if not exist "%XM87160_PRIVATE_ROM_DIR%" mkdir "%XM87160_PRIVATE_ROM_DIR%"
echo.
echo XMVSF native F9 REPLAY - existing TEST 87.161; Stable untouched.
echo Movie input: "%MOVIE%"
echo Full-state anchor: "%STATE%"
echo Native replay results: "%TRACE%"
echo No new DHC move mappings are enabled by this launcher.
echo Let the playback run, then close the game when PLAY_END is reported.
echo.
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%"
set "RC=%ERRORLEVEL%"
popd
echo.
echo Original F9 recording and state anchor are unchanged.
echo Replay exit code: %RC%
echo Output: "%TRACE%"
pause
exit /b %RC%
:fail
echo.
echo Replay refused. No Stable or historical executable was launched.
pause
exit /b 1
