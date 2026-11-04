@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Dedicated replay channel. Existing BUILD_TEST/RUN_TEST names are unchanged.
REM Replay the original 87.161 F9 full-state + exact CPS2 inputs without
REM creating replacement ROM scripts, patching native input, or touching Stable.
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_163.exe"
if not exist "%EXE%" (
  echo ERROR: Need existing XMVSF_Test_87_163.exe. Run BUILD_TEST.bat.
  goto :fail
)
if not exist "%PROJECT%\FBNeo" (
  echo ERROR: FBNeo runtime game directory is missing.
  goto :fail
)
REM Allow explicit input filename (drag an _inputs.bin onto RUN_REPLAY.bat),
REM otherwise look for previously recorded F9 pairs in versioned TEST dirs.
REM The recorder's native _start.fs is always loaded alongside its input.
set "MOVIE="
if not "%~1"=="" (
  set "MOVIE=%~f1"
) else (
  for /d %%D in ("%PROJECT%\test\87.*") do (
    for %%F in ("%%~fD\replays\*_inputs.bin") do (
      if exist "%%~fF" set "MOVIE=%%~fF"
    )
  )
)
if not defined MOVIE (
  echo ERROR: No F9 input movie was found under test\87.*\replays.
  echo Record with F9 in RUN_TEST.bat, or drag an _inputs.bin onto RUN_REPLAY.bat.
  goto :fail
)
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
REM The 87.161 movie was recorded with Training host controls. Bypass
REM the application's original boot selector and start the CPS2 driver
REM immediately; otherwise no input movie frame is ever consumed.
set "XM8761_REPLAY_HOST_MODE=training"
set "XM8761_REPLAY_DIR=%PROJECT%\test\87.163\replays"
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
set "TRACE=%PROJECT%\test\87.163\replay_results"
if not exist "%TRACE%" mkdir "%TRACE%"
if not exist "%TRACE%" (
  echo ERROR: Cannot create isolated replay results directory.
  goto :fail
)
REM Append a new, dated session marker; never delete even prior replay logs.
set "XM8761_REPLAY_STATUS_LOG=%TRACE%\replay_status.log"
>> "%XM8761_REPLAY_STATUS_LOG%" echo ===== REPLAY START %DATE% %TIME% =====
>> "%XM8761_REPLAY_STATUS_LOG%" echo MOVIE="%MOVIE%"
>> "%XM8761_REPLAY_STATUS_LOG%" echo STATE="%STATE%"
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
echo XMVSF native F9 REPLAY - TEST 87.163; Stable untouched.
echo Movie input: "%MOVIE%"
echo Full-state anchor: "%STATE%"
echo Native replay results: "%TRACE%"
echo Replay status log: "%XM8761_REPLAY_STATUS_LOG%"
echo LEGACY INPUT-ONLY PLAYBACK: NO HOST MENU OR RESET JOURNAL IN F9 V1.
echo WARNING: Full 87.161 multi-character movie CANNOT be replayed faithfully.
echo The custom boot menu, Training menu changes and host savestate
echo reloads were never recorded. PLAY_END means input EOF only.
echo See dev\dhc\REPLAY_V2_HOST_TRANSITIONS.md for the real replay contract.
echo Starting ROM xmvsfu DIRECTLY in recorded Training mode - no menu.
echo Replay begins with the recorded save-state and first input frame.
echo No new DHC move mappings are enabled by this launcher.
echo LIMITATION: F9 records native CPS2 input ports and one initial state.
echo Training-menu character swaps / host savestate shortcuts are NOT in
echo this replay movie and may cause later playback to diverge.
echo Native ROM input/action correlations from the original capture are
echo preserved separately in dev\dhc\evidence\REPLAY_87_161_INPUT_NATIVE_ACTIONS.tsv
echo Progress appears in the game window title and replay_status.log.
echo When PLAY_END is shown, close the game to return to this launcher.
echo.
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%" xmvsfu -joy
set "RC=%ERRORLEVEL%"
popd
echo.
echo Original F9 recording and state anchor are unchanged.
echo Replay exit code: %RC%
if exist "%XM8761_REPLAY_STATUS_LOG%" (
  echo Native replay result:
  type "%XM8761_REPLAY_STATUS_LOG%"
) else (
  echo WARNING: No replay status log produced. Confirm ROM launched and first frame executed.
)
echo Output: "%TRACE%"
pause
exit /b %RC%
:fail
echo.
echo Replay refused. No Stable or historical executable was launched.
pause
exit /b 1
