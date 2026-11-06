@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Stage 87.203 CAPTURE BRANCH: the input is MVC1 mvscur1, not XMVSF.
REM Explicit variants: RUN_TEST.bat mvsc / RUN_TEST.bat mvscu.
REM To run original XMVSF for DHC testing: RUN_TEST.bat xmvsfu.
set "SOURCE_ROM=%~1"
if not defined SOURCE_ROM set "SOURCE_ROM=mvscur1"
if /I not "%SOURCE_ROM:~0,4%"=="mvsc" if /I not "%SOURCE_ROM%"=="xmvsfu" (
  echo ERROR: Stage capture supports Marvel vs. Capcom mvsc* ROM sets.
  echo Optional XMVSF mode is xmvsfu; no arbitrary ROM fallback.
  goto :fail
)
if not defined XM87203_STAGE_CAPTURE_ENABLE (
  if /I "%SOURCE_ROM:~0,4%"=="mvsc" (
    set "XM87203_STAGE_CAPTURE_ENABLE=1"
  ) else (
    set "XM87203_STAGE_CAPTURE_ENABLE=0"
  )
)
REM Old Stage44 donor capture remains opt-in; avoid duplicate huge outputs.
if not defined XM44_CAPTURE set "XM44_CAPTURE=0"
if "%XM87203_STAGE_CAPTURE_ENABLE%"=="1" (
  set "XM87203_STAGE_CAPTURE_DIR=%PROJECT%\test\87.203\stage_capture"
  if not exist "%PROJECT%\test\87.203\stage_capture" mkdir "%PROJECT%\test\87.203\stage_capture"
  if not exist "%PROJECT%\test\87.203\stage_capture" goto :fail
) else (
  set "XM87203_STAGE_CAPTURE_DIR="
)
echo Stage 87.203 source ROM: %SOURCE_ROM%
echo Stage capture: %XM87203_STAGE_CAPTURE_ENABLE%
echo Existing MSYS live console: bash dev/stage_capture/WATCH_CAPTURE.sh
echo F9 records the MARVEL VS CAPCOM source session, not XMVSF.
REM 87.175 read-only native state trace. Fails closed if logs cannot be saved.
set "OBSROOT=%PROJECT%\test\87.203\observer"
if not exist "%OBSROOT%" mkdir "%OBSROOT%"
if not exist "%OBSROOT%" (
  echo ERROR: Cannot create 87.175 native observer log directory.
  goto :fail
)
set "XM87175_DHC_OBSERVER=1"
set "XM87175_DHC_OBSERVER_LOG=%OBSROOT%\dhc_acceptance_test.tsv"
REM 87.185: subframe 68000 trace; delayed priming is unsafe by default.
set "XM87180_NATIVE_SUBFRAME_LOG=%OBSROOT%\native_subframe_test.tsv"
REM 87.186 opening gate: original ROM action entry/attack, read-only.
set "XM87186_OPENING_OBSERVER=1"
set "XM87186_OPENING_OBSERVER_LOG=%OBSROOT%\native_opening_test.tsv"
REM 87.185: exact 68000 executed-PC trace is opt-in during interactive gameplay.
REM 87.185: trace only during active native Ryu-Ken handoff.
set "XM87182_NATIVE_PC_TRACE=1"
REM Safety gate ON for interactive play; no consumed ROM punch edge tags.
set "XM87183_REPLAY_UNSAFE_CONSUMED_EDGE=0"
REM 87.186 TEST-only nonterminal native 2E0A recovery handoff experiment.
if not defined XM87184_EXPERIMENTAL_RECOVERY_TAG set "XM87184_EXPERIMENTAL_RECOVERY_TAG=1"
REM 87.186 TEST-only: arm controller 8 while the verified active action edge is fresh.
if not defined XM87185_TEST_EARLY_ACTIVE_TAG set "XM87185_TEST_EARLY_ACTIVE_TAG=1"
REM 87.203: clean current-frame DHC throughout original 2A02/2A28 ATTACK.
if not defined XM87188_TEST_FULL_ATTACK set "XM87188_TEST_FULL_ATTACK=1"
REM 87.203: original 2E script-loop and subaction continuity; Ryu->Ken only.
REM Set 0 in parent CMD to compare against 87.189 while keeping logs.
if not defined XM87189_TEST_NATIVE_SUPER_FAMILY set "XM87189_TEST_NATIVE_SUPER_FAMILY=1"
REM 87.203: actual native super start-to-end is the whole DHC window.
REM Set 0 before launching to compare the historical 87.190 gates.
if not defined XM87191_TEST_OPEN_SUPER_LIFETIME set "XM87191_TEST_OPEN_SUPER_LIFETIME=1"
REM 87.203: incoming fighter's real ROM parser owns a generic motion/chord.
REM Unset to compare legacy named-super selection and fixed Ken 2E scripts.
if not defined XM87192_TEST_RESERVE_OWNERSHIP set "XM87192_TEST_RESERVE_OWNERSHIP=1"
REM 87.203: outgoing 2K/3K -> native LP+MP before CPS input read.
REM Incoming reserve retains ORIGINAL KK for Shinryuken. Set 0 for old behavior.
if not defined XM87193_TEST_DETERMINISTIC_EXIT set "XM87193_TEST_DETERMINISTIC_EXIT=1"
REM 87.203: incoming native airborne motion remains active until real ground contact.
REM The existing opponent-relative target is reached DURING its descent, not teleported.
REM Set 0 to compare the proven 87.193 instant Ken arrival and Shinryuken.
if not defined XM87194_TEST_NATIVE_PASSIVE_JUMP set "XM87194_TEST_NATIVE_PASSIVE_JUMP=1"
REM 87.203: skip the ROM-confirmed 67-frame Ken 2A08 tag-in lag,
REM start original 0212 falling action immediately (no floor warp).
REM Set 0 to compare the prior 87.195 full tag-in animation.
if not defined XM87196_TEST_IMMEDIATE_NATIVE_FALL set "XM87196_TEST_IMMEDIATE_NATIVE_FALL=1"
REM 87.203: keep opposing native fighter frozen from outgoing 3200
REM through Ken landing/input until actual incoming 2E ROM acceptance.
REM Ken/gravity/CPU stay active. 0 for original 87.196 A/B.
if not defined XM87197_TEST_OPPONENT_FREEZE set "XM87197_TEST_OPPONENT_FREEZE=1"
REM 87.203: original Ken HCF+2P/2K was 9/9 native 0006 (NOT 2E).
REM Refuse that gesture before tag, not after Ken lands doing nothing.
REM Set 0 for unrestricted 87.197 comparison.
if not defined XM87198_TEST_KEN_HCF_PRETAG_REJECTION set "XM87198_TEST_KEN_HCF_PRETAG_REJECTION=1"
REM 87.203: keep opponent frozen through original Ken 2E STARTUP,
REM release on first ROM-owned OFFENSIVE hitbox (47-53f later).
REM Set 0 to compare premature 87.197 2E-entry release.
if not defined XM87198_TEST_NATIVE_OFFENSE_RELEASE set "XM87198_TEST_NATIVE_OFFENSE_RELEASE=1"
REM 87.203: animated local _art/marvel-vs-capcom1-super-effect2.gif under menu
REM at 30 percent brightness, missing GIF does not block gameplay.
REM 87.186 TEST-only: after exact 2E0A->2E08 native rebound, use original ROM tag script bridge.
if not defined XM87185_TEST_RECOVERY_REBOUND set "XM87185_TEST_RECOVERY_REBOUND=1"
REM 87.186 TEST-only: first four real ATTACK frames (+4E=40..3D).
REM To isolate stable 87.185 behavior, set this to 0 in parent CMD.
if not defined XM87186_TEST_FIRST_FOUR_ATTACK set "XM87186_TEST_FIRST_FOUR_ATTACK=1"
REM 87.203: only native PC010392 air 2E0E->2E0C rebound; 0 for A/B.
if not defined XM87202_TEST_AIR_FOLLOWUP_REBOUND set "XM87202_TEST_AIR_FOLLOWUP_REBOUND=1"
set "XM87182_NATIVE_PC_TRACE_LOG=%OBSROOT%\native_pc_trace_test.tsv"
REM 87.181 append-only launch health. This marker is created even if
REM the menu closes without entering a CPS2 driver.
set "XM87181_DIAGNOSTIC_STATUS=%OBSROOT%\observer_health_test.log"
>>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_TEST_START date=%DATE% time=%TIME%
if errorlevel 1 (
  echo ERROR: Cannot append observer health file: "%XM87181_DIAGNOSTIC_STATUS%"
  goto :fail
)
if not defined XM87180_EXPERIMENTAL_DELAYED_PRIME set "XM87180_EXPERIMENTAL_DELAYED_PRIME=0"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_203.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.203 TEST executable is missing.
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
REM F9 v2 captures the actual XMVSF application: boot choice, repeated
REM game sessions, Training UI state, native reset checkpoints and inputs.
REM The original 87.61 recorder remains available when this root is absent.
REM V2 F9 sessions now live INSIDE the same current TEST replay folder
REM users already package and inspect, not hidden under test\replay_sessions.
REM Every F9 recording gets manifest.tsv + .fs + .bin in its own subfolder.
set "XM8764_REPLAY_ROOT=%PROJECT%\test\87.203\replays"
if not exist "%XM8764_REPLAY_ROOT%" mkdir "%XM8764_REPLAY_ROOT%"
if not exist "%XM8764_REPLAY_ROOT%" (
  echo ERROR: Cannot create permanent app-level replay recording directory.
  goto :fail
)
REM Even an F9 directory or save failure gets a permanent, uploadable
REM diagnostic next to the new recordings (the game is not a console).
set "XM8764_REPLAY_STATUS_FILE=%XM8764_REPLAY_ROOT%\F9_status.log"
>>"%XM8764_REPLAY_STATUS_FILE%" echo ===== 87.186 TEST session %DATE% %TIME% =====
echo.
echo F9 recording root: "%XM8764_REPLAY_ROOT%"
echo F9 persistent status: "%XM8764_REPLAY_STATUS_FILE%"
echo F9 creates session_XXXX\manifest.tsv and input/state files.
echo The red RECORDING counter is visible in main menu AND gameplay.
echo.
REM Restore original native state-anchored F9 recorder (XM87.61/XM87.132).
REM F9 starts/stops each independent full-state + input movie.
REM The recorder requires an existing replay directory, not an invented
REM input format or a new launcher.
set "XM8761_REPLAY_DIR=%PROJECT%\test\87.203\replays"
if not exist "%XM8761_REPLAY_DIR%" mkdir "%XM8761_REPLAY_DIR%"
if not exist "%XM8761_REPLAY_DIR%" (
  echo ERROR: Cannot create native input replay directory.
  goto :fail
)
set "XM8761_REPLAY_MODE=off"
set "XM87139_PROBE_DIR=%PROJECT%\test\87.203"
set "XM87140_CHAT_DIR=%PROJECT%\test\87.203"
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
REM Experimental TEST only: P1 Ryu->Ken native transaction; all P1/P2
REM directions and six buttons now logged as candidates, but unmapped moves
REM NEVER dispatch guessed ROM actions or consume meter.
set "XM87144_DHC_NATIVE_TEST=1"
REM 87.175: PROOF mode bypasses the forced 87.155 Ryu controller8 RAM
REM mutation. It waits for natural Ryu super exit, issues authentic
REM HP+HK native tag and Ken QCF+2P/2K through CPS2 input mapper.
REM This is deliberately sequential; proves move entry BEFORE true DHC.
set "XM87156_PROOF_NATIVE=0"
REM 87.175 returns to 87.151 Ryu to Ken transition at super timer 28.
REM Ken arrival and his first attack cancellation are from that trace.
REM The destination is a single Ken hyper move, not a team hyper move.
set "XM87157_EARLY_CANCEL=1"
set "XM87144_DHC_LOG=%XM87140_CHAT_DIR%\dhc_native_handoff.tsv"
set "XM87192_RESERVE_SUPER_LOG=%OBSROOT%\reserve_owned_supers.tsv"
REM DHC-only: outgoing Ryu projectiles remain visible but cannot keep
REM offensive collision after the native Ryu-to-Ken tag. Track identities
REM through the verified P1 native projectile object list.
set "XM87159_PROJECTILE_LOG=%XM87140_CHAT_DIR%\p1_old_projectile_hitboxes.tsv"
REM 87.175: catalog every candidate native 2E script from original CPS2
REM ROM tables; log *observed* action/physical-input correlation separately.
REM Neither file enables an unverified super for gameplay DHC.
set "XM87160_DISCOVER=1"
set "XM87160_CATALOG_LOG=%XM87140_CHAT_DIR%\rom_super_catalog.tsv"
set "XM87160_OBSERVED_LOG=%XM87140_CHAT_DIR%\rom_super_observed.tsv"
REM Keep raw decrypted opcode/data snapshots out of Git and handoff ZIPs.
REM These private local files allow 68000 command-handler disassembly.
set "XM87160_PRIVATE_ROM_DIR=%PROJECT%\local_rom_dump"
if not exist "%XM87160_PRIVATE_ROM_DIR%" mkdir "%XM87160_PRIVATE_ROM_DIR%"
if not exist "%XM87160_PRIVATE_ROM_DIR%" (
  echo ERROR: Cannot create private local ROM analysis directory.
  goto :fail
)
REM Enable the latest Tournament-only rule implementations in TEST.
REM Native handlers ALSO require the live menu selection tournament=1.
REM Classic remains ROM-original; do NOT enable retired 80/83/84/85 flight writers.
set "XM8786_STORM_BUTTON_TEST=1"
set "XM8795_NATIVE_HIT_TEST=1"
set "XM8796_DIZZY_HANDOFF_TEST=1"
set "XM8799_GROUND_DEFENSE_TEST=1"
set "XM87149_TOURNAMENT_AUDIT=%XM87140_CHAT_DIR%\tournament_rules.tsv"
REM Marvel vs. Capcom capture should not print XMVSF DHC instructions.
if /I "%SOURCE_ROM:~0,4%"=="mvsc" goto :mvc_source_banner
echo 87.186 STARTUP HEALTH: %XM87181_DIAGNOSTIC_STATUS%
echo 87.175 NATIVE OBSERVER: %XM87175_DHC_OBSERVER_LOG%
echo Native actor rows require a running XMVSF combat session; menu exit writes health only.
echo 87.185: QCF+any 2P/3P recognizes DHC during real super; native tag still needs ROM acceptance.
echo 87.185: Ken native QCF+2P and instant, no-charge arrival preserved.
echo 87.185: UNSAFE delayed native 2P priming stays OFF in interactive TEST.
echo 87.185: immediate 3P/2P paths, Ken no-charge arrival, and F9 stay intact.
echo 87.185: delayed DHC input priming disabled by default, preserves original ROM state.
echo 87.184 native Shinkuu action lifetime: 2E08 startup/attack, 2E0A recovery, exact exit.
echo 87.203 ROM super start-to-real-exit DHC: %XM87191_TEST_OPEN_SUPER_LIFETIME%  (no phase/timer/air-mode availability veto)
echo Native 2E family: %XM87189_TEST_NATIVE_SUPER_FAMILY% full original attack: %XM87188_TEST_FULL_ATTACK% recovery: %XM87185_TEST_RECOVERY_REBOUND%
echo XMVSF Stage 87.203 TEST - KEEP KEN SHORYUREPPA; CUT OUTGOING RYU PROJECTILE HITS
echo Executable: %EXE%
echo SAME BUILD, TWO DHC CHOICES:
echo   During Ryu Shinkuu Hadoken, QCF+ANY TWO PUNCHES or QCF+3P queues Ken SHORYUREPPA.
echo   During Ryu Shinkuu Hadoken, QCF+LK+MK queues Ken SHINRYUKEN 2E02/D6976.
echo Both ROM targets are verified and logged simultaneously in dhc_native_handoff.tsv.
echo Standard native tag retains proven timer 28; the 87.186 TEST separately tries earlier active attacks.
echo Immediately on real Ken arrival, cancel his 2A0C kick into nonattack 2A08.
echo At Ken's first confirmed 2A08 frame, enter verified Ken ROM idle and submit a short QCF+2P / QCF+2K.
echo Ken's recorded Hadoken transition is the baseline. The requested replacement is Ken's super.
echo Requires real Ken 2E00 or 2E02 ROM acceptance; it is not yet a fully validated DHC.
echo This DHC route does not use the game's Variable Combination.
echo Invalid Ryu 2C04 native dispatch aborts without copying stale controller state.
echo Training menu options persist in FBNeo\config\games\xmvsfu_training_options.cfg
echo FRAME DATA: ATTACK DURATION ends on native action exit - directions, jumps, and height do not extend it.
echo Verified modes: normal 28xx, special 2Cxx, and multi-step super 2Exx.
echo Saved: hitboxes, display, meter, dizzy, damage mode, two dummy HPs, player select, frame data.
echo Super-candidate inputs print to Warcraft-style chat and fade after 3 sec.
echo Shift+G opens/closes the full chat history.
echo F10 native capture ON/OFF is acknowledged in chat and written to disk.
echo F9 now toggles HOST-AWARE v2 recording, including in the main menu.
echo V2 sessions: %XM8764_REPLAY_ROOT%\session_XXXX\manifest.tsv
echo Multiple menu/game resets and native state reloads are checkpointed.
echo Shift+F9 quick-save is unchanged. Original v1 movie files remain readable.
echo New v2 F9 recordings: %XM8764_REPLAY_ROOT%\session_XXXX
echo New v2 recording status: %XM8764_REPLAY_STATUS_FILE%
echo Previous v1 .bin/.fs pairs remain readable but use another format.
echo Shift+F10 quick-load remains unchanged.
echo Super chat transcript: %XM87140_CHAT_LOG%
echo ROM script/role events: %XM87142_ROM_TRACE%
echo Prepared queue events and selections: %XM87140_CHAT_LOG%
echo DHC native step-by-step transaction: %XM87144_DHC_LOG%
echo In EARLY mode, Ken's FIRST incoming kick is suppressed as in 87.150; no second attack.
echo After fast Ken 2A08 cancel, TEST submits one QCF+2P or QCF+2K through native CPS-2 ports.
echo Native 2E00 Shoryureppa / 2E02 Shinryuken are independently verified; no host script injection.
echo All physical button inputs remain intact outside the bounded Ken native input window.
echo Ryu tag-out and super-charge streaks retained.
echo Failed input dispatch, normal super recovery and native point swap release the DHC lock.
echo EXPERIMENT: Ryu-to-Ken 87.151 handoff + independent outgoing projectile offensive cutoff.
echo Original Ryu super projectile stays visible, but its P1 attack hitboxes are disabled after the confirmed DHC tag.
echo Ken's newly spawned projectiles and P2 projectile hitboxes remain active. No Variable Combination injected.
echo Native projectile identity log: %XM87159_PROJECTILE_LOG%
echo ROM native 2E script catalog: %XM87160_CATALOG_LOG%
echo Live ROM-to-input candidates: %XM87160_OBSERVED_LOG%
echo Private decrypted code/data analysis copies: %XM87160_PRIVATE_ROM_DIR%
echo New native move scripts are NOT enabled for DHC until proven in ROM and live input logs.
echo Existing 87.159 Ryu to Ken cancellation, timings, and projectile collision stay unchanged.
echo Warning lines containing -winwrite are filtered by the TEST builder; other diagnostics remain.
echo Stable untouched.
echo Active Tournament rules audit: %XM87149_TOURNAMENT_AUDIT%
echo Tournament rules are applied ONLY when selected in the Options menu.
echo F10 probe files: %XM87139_PROBE_DIR%
:mvc_source_banner
if /I "%SOURCE_ROM:~0,4%"=="mvsc" (
  echo Marvel vs. Capcom 1 native donor: %SOURCE_ROM%
  echo Stage capture: "%XM87203_STAGE_CAPTURE_DIR%"
  echo Live MSYS console: bash dev/stage_capture/WATCH_CAPTURE.sh
  echo F9 records actual MVC inputs and savestates.
  echo No XMVSF DHC diagnostic analysis is needed for this ROM.
)
REM MVC1 is the donor, not the DHC host. Disable all XMVSF-only test
REM mutations/lighting before starting original MVC1 execution.
if /I "%SOURCE_ROM:~0,4%"=="mvsc" (
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
  set "XM44_CAPTURE=0"
)
pushd "%PROJECT%\FBNeo" || goto :fail
echo Launching original CPS2 donor ROM %SOURCE_ROM% with controller support.
"%EXE%" %SOURCE_ROM% -joy
set "RC=%ERRORLEVEL%"
popd
>>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_TEST_EXIT code=%RC%
if /I "%SOURCE_ROM:~0,4%"=="mvsc" (
  if not "%RC%"=="0" (
    echo ERROR: Marvel vs. Capcom donor ROM failed to load or execute.
    echo FBNeo requires the EXACT program files specified by its driver.
    echo mvsc uses mvce.03a; mvscur1 uses mvcu.03 and shared parent data.
    echo Check the configured FBNeo ROM search paths and ROM archive contents.
    echo No stage-capture success will be reported.
    if "%XM87203_STAGE_CAPTURE_ENABLE%"=="1" (
      >>"%XM87203_STAGE_CAPTURE_DIR%\status.log" echo RUN_TEST_EXIT	ERROR	donor_ROM_load_or_execution_failed;_no_CPS2_frames_certified
    )
  )
  goto :mvc_capture_verify
)
:mvc_capture_verify
REM Read-only 87.203 capture verification is nonfatal to game exit code.
REM The builder must attest to camera/animation coverage before LEVEL_DONE.
if "%XM87203_STAGE_CAPTURE_ENABLE%"=="1" (
  if exist "%XM87203_STAGE_CAPTURE_DIR%\latest.txt" (
    where python.exe >nul 2>nul
    if not errorlevel 1 (
      python.exe "%PROJECT%\dev\stage_capture\VERIFY_CAPTURE_STATUS.py" --root "%XM87203_STAGE_CAPTURE_DIR%"
      if errorlevel 1 >>"%XM87203_STAGE_CAPTURE_DIR%\status.log" echo RUN_TEST_EXIT	INCOMPLETE	stage_capture_not_yet_certified
    )
  ) else (
    >>"%XM87203_STAGE_CAPTURE_DIR%\status.log" echo RUN_TEST_EXIT	INCOMPLETE	stage_builder_manifest_not_yet_available
  )
)
REM Once MVC capture verification has run, skip XMVSF-only DHC reports.
if /I "%SOURCE_ROM:~0,4%"=="mvsc" goto :mvc_donor_post
REM 87.175: after native play, analyze only PRIVATE locally captured ROM
REM bytes. ROM binaries are outside Git; the emitted JSON is evidence,
REM never an instruction to activate unverified DHC moves.
if exist "%XM87160_PRIVATE_ROM_DIR%\xmvsfu_program_data_host_swapped.bin" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\DISASSEMBLE_SUPER_MOVES.py" --rom "%XM87160_PRIVATE_ROM_DIR%\xmvsfu_program_data_host_swapped.bin" --opcodes "%XM87160_PRIVATE_ROM_DIR%\xmvsfu_decrypted_opcodes_host_swapped.bin" --observations "%XM87160_OBSERVED_LOG%" --out "%XM87140_CHAT_DIR%\native_super_reverse_engineering.json"
    if errorlevel 1 echo WARNING: ROM analysis report failed; original TEST and game exit result preserved.
  ) else (
    echo INFO: Python not on PATH. Native ROM TSV catalogs were still written.
  )
)
echo.
REM After closing the game, derive compact observer results WITHOUT changing the run return code.
if exist "%XM87175_DHC_OBSERVER_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_DHC_NATIVE_OBSERVER_175.py" --input "%XM87175_DHC_OBSERVER_LOG%" --out "%OBSROOT%\dhc_acceptance_test_summary.json"
    if errorlevel 1 echo WARNING: Observer summary failed; original TSV and gameplay result preserved.
  ) else (
    echo INFO: Python missing. Observer TSV remains available for upload.
  )
) else (
  echo WARNING: Native actor TSV missing. See the 87.181 health report for the exact reason.
)
REM 87.180 native subframe diagnostics: read-only and preserves EXE result.
if exist "%XM87180_NATIVE_SUBFRAME_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_DHC_SUBFRAME_180.py" --input "%XM87180_NATIVE_SUBFRAME_LOG%" --out "%OBSROOT%\native_subframe_test_summary.json"
    if errorlevel 1 echo WARNING: 87.180 subframe summary failed, raw TSV retained.
  )
)
REM 87.185: report why old/new observer files are absent or empty.
REM Diagnostic report cannot alter %RC% or overwrite previous traces.
where python.exe >nul 2>nul
if not errorlevel 1 (
  python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_OBSERVER_HEALTH_181.py" --health "%XM87181_DIAGNOSTIC_STATUS%" --native "%XM87175_DHC_OBSERVER_LOG%" --subframe "%XM87180_NATIVE_SUBFRAME_LOG%" --out "%OBSROOT%\observer_health_summary.json"
  if errorlevel 1 echo WARNING: Health summary unavailable; raw observer_health_test.log retained.
) else (
  echo WARNING: Python unavailable. Observer health raw log: "%XM87181_DIAGNOSTIC_STATUS%"
)
REM Exact native executed-PC analysis (only when explicitly enabled).
if exist "%XM87182_NATIVE_PC_TRACE_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_NATIVE_PC_ARBITRATION_182.py" --input "%XM87182_NATIVE_PC_TRACE_LOG%" --out "%OBSROOT%\native_pc_trace_test_summary.json"
    if errorlevel 1 echo WARNING: Native PC summary failed, source TSV preserved.
  )
)
REM 87.186 original action entry/beginning DHC trace summary.
if exist "%XM87186_OPENING_OBSERVER_LOG%" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\dhc\tools\ANALYZE_NATIVE_OPENING_TRACE_186.py" --input "%XM87186_OPENING_OBSERVER_LOG%" --out "%OBSROOT%\native_opening_test_summary.json"
    if errorlevel 1 echo WARNING: opening trace analyzer failed; raw TSV retained.
  )
)
:mvc_donor_post
echo Stage 87.203 TEST exit code: %RC%
echo Probe directory: %XM87139_PROBE_DIR%
pause
exit /b %RC%
:fail
echo.
echo TEST RUN STOPPED. No older executable was launched.
pause
exit /b 1
