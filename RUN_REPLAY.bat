@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Independent replay capture; OFF by default. Existing replay playback intact.
if not defined XM87203_STAGE_CAPTURE_ENABLE set "XM87203_STAGE_CAPTURE_ENABLE=0"
if "%XM87203_STAGE_CAPTURE_ENABLE%"=="1" (
  set "XM87203_STAGE_CAPTURE_DIR=%PROJECT%\test\87.203\stage_capture_replay"
  if not exist "%PROJECT%\test\87.203\stage_capture_replay" mkdir "%PROJECT%\test\87.203\stage_capture_replay"
  if not exist "%PROJECT%\test\87.203\stage_capture_replay" goto :fail
) else (
  set "XM87203_STAGE_CAPTURE_DIR="
)
REM 87.186 traces early native Shinkuu action without changing F9 inputs.
set "OBSROOT=%PROJECT%\test\87.230\observer"
if not exist "%OBSROOT%" mkdir "%OBSROOT%"
if not exist "%OBSROOT%" goto :fail
REM Maintain exact Cyclops 87.121 air-throw recovery during F9 playback.
REM Inherited =0 remains A/B opt-out, including older ROM-only comparisons.
if not defined XM87121_CYCLOPS_RELEASE set "XM87121_CYCLOPS_RELEASE=1"
if not defined XM87121_CYCLOPS_LOG_FILE set "XM87121_CYCLOPS_LOG_FILE=%OBSROOT%\cyclops_air_throw_replay.tsv"
REM Dash startup progress from native F9 playback, never modifies input recording.
if not defined XM87220_DASH_LOG_FILE set "XM87220_DASH_LOG_FILE=%OBSROOT%\dash_startup_replay.tsv"
set "XM87223_STORM_FLIGHT_LOG_FILE=%OBSROOT%\storm_hp_native_fs_replay.tsv"
set "XM87223_M68K_CANDIDATES_FILE=%OBSROOT%\storm_fs_68000_opcode_candidates_replay.tsv"
set "XM87127_STORM_VOICE_MUTE=1"
set "XM87127_STORM_VOICE_LOG_FILE=%OBSROOT%\storm_voice_all_modes_replay.tsv"
set "XM8786_STORM_BUTTON_LOG_FILE=%OBSROOT%\storm_like_flight_events_replay.tsv"
set "XM87224_CHARLIE_M68K_TRACE=1"
set "XM87224_CHARLIE_M68K_TRACE_LOG=%OBSROOT%\charlie_native_68000_pc_writes_replay.tsv"
set "XM87175_DHC_OBSERVER=1"
set "XM87175_DHC_OBSERVER_LOG=%OBSROOT%\dhc_acceptance_replay.tsv"
REM 87.180: native input CPU-stage diagnostic independent of F9 inputs.
set "XM87180_NATIVE_SUBFRAME_LOG=%OBSROOT%\native_subframe_replay.tsv"
REM 87.186 read-only startup trace from original 87.185 F9 movie.
set "XM87186_OPENING_OBSERVER=1"
set "XM87186_OPENING_OBSERVER_LOG=%OBSROOT%\native_opening_replay.tsv"
REM 87.182 only: instruction-completion PC trace for armed Ryu-Ken handoff.
set "XM87182_NATIVE_PC_TRACE=1"
REM Native consumed-punch safety remains ON during 87.185 baseline replay.
REM 87.184 replay uses 87.183 SAFE handoff, not unsafe forensic bypass.
set "XM87183_REPLAY_UNSAFE_CONSUMED_EDGE=0"
REM Baseline replay retains old 2E08 timing while TEST can probe recovery.
REM 87.186 replay reproduces the stable 87.185 early attack and recovery handoff flags.
REM The original 87.184 native PC trace already proves the old 2E0A->2E08 restart.
set "XM87184_EXPERIMENTAL_RECOVERY_TAG=1"
set "XM87185_TEST_EARLY_ACTIVE_TAG=1"
REM 87.203 full original ATTACK window (both 2A02 and 2A28).
set "XM87188_TEST_FULL_ATTACK=1"
REM Replay captured 87.189 air-super inputs through 87.203 continuous native lifecycle.
set "XM87189_TEST_NATIVE_SUPER_FAMILY=1"
REM 87.203 replay native full super lifetime without separate stage gates.
set "XM87191_TEST_OPEN_SUPER_LIFETIME=1"
REM 87.203 original incoming fighter command ownership, no named super list.
set "XM87192_TEST_RESERVE_OWNERSHIP=1"
REM 87.203: native outgoing punch edge while reserve owns original KK.
set "XM87193_TEST_DETERMINISTIC_EXIT=1"
REM 87.203: native descent and opponent-relative passive landing first.
set "XM87194_TEST_NATIVE_PASSIVE_JUMP=1"
REM 87.203: direct original 0212 gravity entry after native swap.
set "XM87196_TEST_IMMEDIATE_NATIVE_FALL=1"
REM 87.203: native opponent-only cinematic hold until verified Ken 2E.
set "XM87197_TEST_OPPONENT_FREEZE=1"
REM 87.203: original ROM-proven Ken input and attacking animation cues.
set "XM87198_TEST_KEN_HCF_PRETAG_REJECTION=1"
set "XM87198_TEST_NATIVE_OFFENSE_RELEASE=1"
REM 87.203: menu GIF is loaded at runtime from local _art, no binary redistribution.
set "XM87185_TEST_RECOVERY_REBOUND=1"
REM Read-only historical 87.185 F9 replay must NOT inject
REM a native tag in the previously untested first-four attack frames.
set "XM87186_TEST_FIRST_FOUR_ATTACK=0"
REM 87.203: retain the ROM-proven air selector rebound in replay.
set "XM87202_TEST_AIR_FOLLOWUP_REBOUND=1"
set "XM87182_NATIVE_PC_TRACE_LOG=%OBSROOT%\native_pc_trace_replay.tsv"
REM 87.181 launch marker survives app-menu exits and failed CPS2 boot.
set "XM87181_DIAGNOSTIC_STATUS=%OBSROOT%\observer_health_replay.log"
>>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_REPLAY_START date=%DATE% time=%TIME%
if errorlevel 1 (
  echo ERROR: Cannot append native replay health log.
  goto :fail
)
REM 87.182 replay matches stable 87.181 native behavior, no guessed prime.
REM Replay PC tracing only; prior 87.179 speculative prime disabled.
set "XM87180_EXPERIMENTAL_DELAYED_PRIME=0"
REM Dedicated replay channel. Existing BUILD_TEST/RUN_TEST names are unchanged.
REM Replay the original 87.161 F9 full-state + exact CPS2 inputs without
REM creating replacement ROM scripts, patching native input, or touching Stable.
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_230.exe"
if not exist "%EXE%" (
  echo ERROR: Need XMVSF_Test_87_230.exe. Run BUILD_TEST.bat (no old TEST fallback).
  goto :fail
)
if not exist "%PROJECT%\FBNeo" (
  echo ERROR: FBNeo runtime game directory is missing.
  goto :fail
)
REM 87.167: prefer a COMPLETE host-aware session journal with native
REM checkpoints over legacy input-only F9 movies. The latest pointer is
REM written by the actual v2 F9 recorder, never guessed from timestamps.
set "V2_SESSION="
if not "%~1"=="" (
  if exist "%~f1\manifest.tsv" set "V2_SESSION=%~f1"
  if /I "%~x1"==".tsv" if exist "%~f1" set "V2_SESSION=%~dp1"
)
REM Current v2 recordings live alongside the current version's replay
REM files so they are included in ordinary versioned test ZIPs.
if not defined V2_SESSION if exist "%PROJECT%\test\87.230\replays\latest.txt" (
  for /f "usebackq delims=" %%S in ("%PROJECT%\test\87.230\replays\latest.txt") do set "V2_SESSION=%%S"
)
REM Keep replays available even AFTER a version bump: find the latest
REM completed session in older numbered TEST directories, newest first.
REM This never deletes or migrates the original recording/checkpoints.
if not defined V2_SESSION (
  for /f "delims=" %%D in ('dir /b /ad /o:-n "%PROJECT%\test\87.*" 2^>nul') do (
    if not defined V2_SESSION if exist "%PROJECT%\test\%%D\replays\latest.txt" (
      for /f "usebackq delims=" %%S in ("%PROJECT%\test\%%D\replays\latest.txt") do set "V2_SESSION=%%S"
    )
  )
)
REM Preserve older v2 recordings made with the original 87.164 layout.
if not defined V2_SESSION if exist "%PROJECT%\test\replay_sessions\latest.txt" (
  for /f "usebackq delims=" %%S in ("%PROJECT%\test\replay_sessions\latest.txt") do set "V2_SESSION=%%S"
)
if defined V2_SESSION if exist "%V2_SESSION%\manifest.tsv" goto :run_v2

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
set "XM8761_REPLAY_DIR=%PROJECT%\test\87.230\replays"
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
set "TRACE=%PROJECT%\test\87.230\replay_results"
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
set "XM87192_RESERVE_SUPER_LOG=%TRACE%\reserve_owned_supers.tsv"
set "XM87159_PROJECTILE_LOG=%TRACE%\p1_old_projectile_hitboxes.tsv"
set "XM87149_TOURNAMENT_AUDIT=%TRACE%\tournament_rules.tsv"
set "XM87160_DISCOVER=1"
set "XM87160_CATALOG_LOG=%TRACE%\rom_super_catalog.tsv"
set "XM87160_OBSERVED_LOG=%TRACE%\rom_super_observed.tsv"
REM Raw ROM analysis copies remain private and Git-ignored.
set "XM87160_PRIVATE_ROM_DIR=%PROJECT%\local_rom_dump"
if not exist "%XM87160_PRIVATE_ROM_DIR%" mkdir "%XM87160_PRIVATE_ROM_DIR%"
echo.
echo XMVSF native F9 REPLAY - TEST 87.230; Stable untouched.
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
REM Summarize read-only native acceptance trace without changing replay status.
if exist "%XM87175_DHC_OBSERVER_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_DHC_NATIVE_OBSERVER_175.py" --input "%XM87175_DHC_OBSERVER_LOG%" --out "%OBSROOT%\dhc_acceptance_replay_summary.json"
    if errorlevel 1 echo WARNING: Observer report failed, original TSV preserved.
  )
)
REM Read-only subframe analysis never overrides original replay status.
if exist "%XM87180_NATIVE_SUBFRAME_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_DHC_SUBFRAME_180.py" --input "%XM87180_NATIVE_SUBFRAME_LOG%" --out "%OBSROOT%\native_subframe_replay_summary.json"
    if errorlevel 1 echo WARNING: 87.180 subframe summary failed.
  )
)
>>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_REPLAY_EXIT code=%RC%
REM 87.186: summarize after EXE returns, never before the replay begins.
if exist "%XM87186_OPENING_OBSERVER_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_NATIVE_OPENING_TRACE_186.py" --input "%XM87186_OPENING_OBSERVER_LOG%" --out "%OBSROOT%\native_opening_replay_summary.json"
    if errorlevel 1 echo WARNING: opening summary failed; raw TSV retained.
  )
)

where python.exe >nul 2>nul
if not errorlevel 1 (
  python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_OBSERVER_HEALTH_181.py" --health "%XM87181_DIAGNOSTIC_STATUS%" --native "%XM87175_DHC_OBSERVER_LOG%" --subframe "%XM87180_NATIVE_SUBFRAME_LOG%" --out "%OBSROOT%\observer_health_replay_summary.json"
  REM Native 87.182 PPC trace is independent of the historical ROM verifier.
  if exist "%XM87182_NATIVE_PC_TRACE_LOG%" (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_NATIVE_PC_ARBITRATION_182.py" --input "%XM87182_NATIVE_PC_TRACE_LOG%" --out "%OBSROOT%\native_pc_trace_replay_summary.json"
    if errorlevel 1 echo WARNING: Native PPC summary failed, source TSV retained.
  )
)
echo Output: "%TRACE%"
pause
exit /b %RC%
:fail
if defined XM87181_DIAGNOSTIC_STATUS >>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_REPLAY_EXIT code=1 preflight_failure
echo.
echo Replay refused. No Stable or historical executable was launched.
pause
exit /b 1

:run_v2
REM Read-only preflight. A sealed latest.txt recording needs FINAL;
REM an explicitly dropped session folder/manifest may be an older
REM complete-frame UNSEALED PREFIX (not mislabeled as a successful stop).
REM Preserve both the original input bytes and full native checkpoints.
if not exist "%PROJECT%\dev\dhc\tools\VERIFY_REPLAY_V2.py" (
  echo ERROR: Missing read-only verifier dev\dhc\tools\VERIFY_REPLAY_V2.py.
  goto :fail
)
where python.exe >nul 2>nul
if not errorlevel 1 (
  echo Validating selected replay session without modifying files:
  if "%~1"=="" (
    python.exe "%PROJECT%\dev\dhc\tools\VERIFY_REPLAY_V2.py" "%V2_SESSION%"
  ) else (
    echo EXPLICIT legacy selection: allowing an unsealed complete-frame prefix.
    python.exe "%PROJECT%\dev\dhc\tools\VERIFY_REPLAY_V2.py" "%V2_SESSION%" --allow-unsealed
  )
  if errorlevel 1 (
    echo ERROR: Replay data contract failed. No game or Stable executable launched.
    goto :fail
  )
) else (
  echo WARNING: Python unavailable. Native V2 manifest and payload validation remains active.
)
REM Stage87.203: the original Wily donor F9 recording is MVC1, not XMVSF.
REM Read the REAL ROM identity from the saved F9 native input header.
REM Fail closed if the detector is missing or the session mixes drivers.
if not exist "%PROJECT%\dev\stage_capture\DETECT_REPLAY_DRIVER.py" (
  echo ERROR: Missing v2 donor ROM detector.
  goto :fail
)
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Python required to verify v2 replay source ROM.
  goto :fail
)
set "V2_DRIVER="
for /f "usebackq delims=" %%D in (`python.exe "%PROJECT%\dev\stage_capture\DETECT_REPLAY_DRIVER.py" "%V2_SESSION%"`) do set "V2_DRIVER=%%D"
if not defined V2_DRIVER (
  echo ERROR: No validated ROM driver found in F9 session input headers.
  goto :fail
)
echo F9 source ROM verified: %V2_DRIVER%
if /I "%V2_DRIVER:~0,4%"=="mvsc" (
  REM Replaying original MVC1 must not enable XMVSF DHC/flight mutations.
  set "XM87121_CYCLOPS_RELEASE=0"
  set "XM87144_DHC_NATIVE_TEST=0"
  set "XM87156_PROOF_NATIVE=0"
  set "XM87157_EARLY_CANCEL=0"
  set "XM87184_EXPERIMENTAL_RECOVERY_TAG=0"
  set "XM87185_TEST_EARLY_ACTIVE_TAG=0"
  set "XM87189_TEST_NATIVE_SUPER_FAMILY=0"
  set "XM87191_TEST_OPEN_SUPER_LIFETIME=0"
  set "XM87192_TEST_RESERVE_OWNERSHIP=0"
  set "XM87193_TEST_DETERMINISTIC_EXIT=0"
  set "XM87194_TEST_NATIVE_PASSIVE_JUMP=0"
  set "XM87196_TEST_IMMEDIATE_NATIVE_FALL=0"
  set "XM87197_TEST_OPPONENT_FREEZE=0"
  set "XM87198_TEST_NATIVE_OFFENSE_RELEASE=0"
  set "XM87202_TEST_AIR_FOLLOWUP_REBOUND=0"
  set "XM8786_STORM_BUTTON_TEST=0"
  set "XM8795_NATIVE_HIT_TEST=0"
  set "XM8796_DIZZY_HANDOFF_TEST=0"
  set "XM8799_GROUND_DEFENSE_TEST=0"
  set "XM87160_DISCOVER=0"
  set "XM8649_WILY_RENDER=0"
)
REM XMVSF multi-menu replay still boots its original application shell.
REM MVC1 directly boots its recorded ROM without invoking XMVSF's menu.
set "XM8764_REPLAY_SESSION=%V2_SESSION%"
set "XM8764_FAST_FORWARD=32"
set "XM8761_REPLAY_MODE=off"
set "XM8761_DISABLE_RUNAHEAD=1"
set "XM87138_TAG_LAND_TEST=0"
set "XM87144_DHC_NATIVE_TEST=1"
set "XM87156_PROOF_NATIVE=0"
set "XM87157_EARLY_CANCEL=1"
set "XM8786_STORM_BUTTON_TEST=1"
set "XM8795_NATIVE_HIT_TEST=1"
set "XM8796_DIZZY_HANDOFF_TEST=1"
set "XM8799_GROUND_DEFENSE_TEST=1"
set "TRACE=%PROJECT%\test\87.230\replay_results"
if not exist "%TRACE%" mkdir "%TRACE%"
if not exist "%TRACE%" goto :fail
REM Keep all replay-time host diagnostics outside the original F9 session.
set "XM8764_REPLAY_STATUS_FILE=%TRACE%\replay_v2_status.log"
>>"%XM8764_REPLAY_STATUS_FILE%" echo ===== 87.180 HOST REPLAY START %DATE% %TIME% =====
set "XM87140_CHAT_DIR=%TRACE%"
set "XM87139_PROBE_DIR=%TRACE%"
set "XM87140_CHAT_LOG=%TRACE%\super_chat.tsv"
set "XM87142_ROM_TRACE=%TRACE%\rom_dispatch.tsv"
set "XM87144_DHC_LOG=%TRACE%\dhc_native_handoff.tsv"
set "XM87192_RESERVE_SUPER_LOG=%TRACE%\reserve_owned_supers.tsv"
set "XM87159_PROJECTILE_LOG=%TRACE%\p1_old_projectile_hitboxes.tsv"
set "XM87149_TOURNAMENT_AUDIT=%TRACE%\tournament_rules.tsv"
set "XM87160_DISCOVER=1"
set "XM87160_CATALOG_LOG=%TRACE%\rom_super_catalog.tsv"
set "XM87160_OBSERVED_LOG=%TRACE%\rom_super_observed.tsv"
echo.
echo XMVSF HOST-AWARE F9 REPLAY V2 - CURRENT TEST 87.230
echo Manifest: "%V2_SESSION%\manifest.tsv"
echo Application menu, game resets, native savestate reloads and inputs
echo will follow the recorded session/checkpoint timeline.
echo HIGH SPEED: %XM8764_FAST_FORWARD% consecutive CPS2 frames per display update.
echo To change speed, edit XM8764_FAST_FORWARD in this BAT. 0 = normal speed.
echo Status: "%XM8764_REPLAY_STATUS_FILE%"
echo Native diagnostic results: "%TRACE%"
echo Stable executable and all recorded checkpoints are never overwritten.
echo.
pushd "%PROJECT%\FBNeo" || goto :fail
if /I "%V2_DRIVER:~0,4%"=="mvsc" (
  "%EXE%" %V2_DRIVER% -joy
) else (
  "%EXE%" -joy
)
set "RC=%ERRORLEVEL%"
popd
echo.
echo HOST-AWARE REPLAY EXIT CODE: %RC%
if exist "%XM8764_REPLAY_STATUS_FILE%" (
  echo Replay checkpoint/playback transcript (original session log preserved):
  type "%XM8764_REPLAY_STATUS_FILE%"
) else (
  echo ERROR: no host replay status was written. Check executable and manifest.
)
REM Verify ROM events and complete checkpoint progression after v2 playback.
REM Use the original record only for read-only input/reference comparison.
if "%RC%"=="0" (
  if /I "%V2_DRIVER:~0,4%"=="mvsc" (
    echo MVC1 donor replay: native F9 manifest/input headers validated.
    echo DHC-specific XMVSF ROM action verifier is not applicable to MVC.
    echo Check the fresh replay_v2_status.log for PLAY_END; no fabricated stage completion.
  ) else (
    where python.exe >nul 2>nul
    if errorlevel 1 (
      echo ERROR: Python missing - XMVSF replay not fully verified.
      set "RC=3"
    ) else (
      echo Verifying XMVSF DHC checkpoint/ROM actions...
      python.exe "%PROJECT%\dev\dhc\tools\VERIFY_DHC_ROM_ACTIONS.py" "%V2_SESSION%" --replay-results "%TRACE%"
      if errorlevel 1 (
        echo ERROR: ROM action replay verification FAILED. Original capture preserved.
        set "RC=3"
      ) else (
        echo PASS: XMVSF checkpoint and ROM action trace match.
      )
    )
  )
) else (
  echo ERROR: Replay executable failed; action verification not claimed.
)
REM 87.180 subframe log is useful even when the intentional DHC
REM gameplay change causes historical ROM-action replay divergence.
if exist "%XM87180_NATIVE_SUBFRAME_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_DHC_SUBFRAME_180.py" --input "%XM87180_NATIVE_SUBFRAME_LOG%" --out "%OBSROOT%\native_subframe_replay_summary.json"
    if errorlevel 1 echo WARNING: 87.180 subframe summary failed.
  )
)
>>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_REPLAY_EXIT code=%RC%
REM 87.186: summarize after EXE returns, never before the replay begins.
if exist "%XM87186_OPENING_OBSERVER_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_NATIVE_OPENING_TRACE_186.py" --input "%XM87186_OPENING_OBSERVER_LOG%" --out "%OBSROOT%\native_opening_replay_summary.json"
    if errorlevel 1 echo WARNING: opening summary failed; raw TSV retained.
  )
)

where python.exe >nul 2>nul
if not errorlevel 1 (
  python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_OBSERVER_HEALTH_181.py" --health "%XM87181_DIAGNOSTIC_STATUS%" --native "%XM87175_DHC_OBSERVER_LOG%" --subframe "%XM87180_NATIVE_SUBFRAME_LOG%" --out "%OBSROOT%\observer_health_replay_summary.json"
  REM Native 87.182 PPC trace is independent of the historical ROM verifier.
  if exist "%XM87182_NATIVE_PC_TRACE_LOG%" (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_NATIVE_PC_ARBITRATION_182.py" --input "%XM87182_NATIVE_PC_TRACE_LOG%" --out "%OBSROOT%\native_pc_trace_replay_summary.json"
    if errorlevel 1 echo WARNING: Native PPC summary failed, source TSV retained.
  )
)
REM Post-replay observer is independent of ROM action verification and exit status.
if exist "%XM87175_DHC_OBSERVER_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_DHC_NATIVE_OBSERVER_175.py" --input "%XM87175_DHC_OBSERVER_LOG%" --out "%OBSROOT%\dhc_acceptance_replay_summary.json"
    if errorlevel 1 echo WARNING: Observer summary failed, replay result preserved.
  )
)
pause
exit /b %RC%
