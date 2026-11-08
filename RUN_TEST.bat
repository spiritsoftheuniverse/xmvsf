@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
set "XMVSF_TEST_VERSION=87.213"
REM Offline artifact review uses Python only. Never reaches ROM checks or EXE.
if /I "%~1"=="review" goto :offline_clean_review
if /I "%~1"=="sprites" goto :offline_sprite_factory
if /I "%~1"=="m68k" goto :offline_m68k_sprite_analysis
if /I "%~1"=="poses" goto :offline_m68k_pose_analysis
if "%~1"=="1" goto :suite_test
if "%~1"=="2" goto :suite_asset
if "%~1"=="3" goto :suite_capture
if /I "%~1"=="art" if /I "%~2"=="open" goto :offline_mvc_art_launch
if /I "%~1"=="art" goto :offline_mvc_art_review
if /I "%~1"=="channels" goto :offline_all_sprite_channels
REM Stage 87.203 CAPTURE BRANCH: the input is MVC1 mvscur1, not XMVSF.
REM Explicit variants: RUN_TEST.bat mvsc / RUN_TEST.bat mvscu.
REM To run original XMVSF for DHC testing: RUN_TEST.bat xmvsfu.
set "SOURCE_ROM=%~1"
if not defined SOURCE_ROM goto :suite_menu
:game_run
REM Menu destinations preselect SOURCE_ROM; legacy named ROM arguments still work.
if not defined SOURCE_ROM set "SOURCE_ROM=mvscur1"
set "CLEAN_RIP_MODE=%~2"
REM The 68000 program dumper runs independently of full native stage
REM frame/tilemap telemetry. Default trace mode stays lightweight.
if /I "%CLEAN_RIP_MODE%"=="trace" set "XM87203_STAGE_CAPTURE_ENABLE=0"
if defined CLEAN_RIP_MODE if /I not "%CLEAN_RIP_MODE%"=="clean" if /I not "%CLEAN_RIP_MODE%"=="trace" (
  echo ERROR: Unknown second argument. Use clean or trace for MVC1 source capture.
  goto :fail
)
if /I "%SOURCE_ROM%"=="xmvsfu" if /I "%CLEAN_RIP_MODE%"=="clean" (
  echo ERROR: Native clean-rip mode only operates on Marvel vs. Capcom donor ROMs.
  goto :fail
)
if /I "%SOURCE_ROM%"=="xmvsfu" if /I "%CLEAN_RIP_MODE%"=="trace" (
  echo ERROR: Motorola 68000 source sprite tracing is scoped to MVC1 only.
  goto :fail
)
if /I not "%SOURCE_ROM:~0,4%"=="mvsc" if /I not "%SOURCE_ROM%"=="xmvsfu" (
  echo ERROR: Stage capture supports Marvel vs. Capcom mvsc* ROM sets.
  echo Optional XMVSF mode is xmvsfu; no arbitrary ROM fallback.
  goto :fail
)
REM F11 exclusively activates native level capture; no implicit huge capture on boot.
if not defined XM87203_STAGE_CAPTURE_ENABLE set "XM87203_STAGE_CAPTURE_ENABLE=0"
REM Legacy explicit clean mode still sets up both old render-pass and native data.
if /I "%CLEAN_RIP_MODE%"=="clean" set "XM87203_STAGE_CAPTURE_ENABLE=1"
set "XM87203_LEVEL_CAPTURE_ACTIVE=0"
set "XM87203_LEVEL_CAPTURE_GENERATION=0"
REM Old Stage44 donor capture remains opt-in; avoid duplicate huge outputs.
if not defined XM44_CAPTURE set "XM44_CAPTURE=0"
if "%XM87203_STAGE_CAPTURE_ENABLE%"=="1" (
  set "XM87203_STAGE_CAPTURE_DIR=%PROJECT%\test\87.203\stage_capture"
  if not exist "%PROJECT%\test\87.203\stage_capture" mkdir "%PROJECT%\test\87.203\stage_capture"
  if not exist "%PROJECT%\test\87.203\stage_capture" goto :fail
) else (
  set "XM87203_STAGE_CAPTURE_DIR="
)
echo Stage 87.213 TEST executable, source ROM: %SOURCE_ROM%
echo Stage capture: %XM87203_STAGE_CAPTURE_ENABLE%
echo Existing MSYS live console: bash dev/stage_capture/WATCH_CAPTURE.sh
echo F9 = replay, F10 = native probe, F11 = MVC1 level capture ON/OFF.
echo Shift+F11 = legacy FPS display; F11 starts a NEW capture session on each activation.
REM 87.175 read-only native state trace. Fails closed if logs cannot be saved.
set "OBSROOT=%PROJECT%\test\87.213\observer"
if not exist "%OBSROOT%" mkdir "%OBSROOT%"
if not exist "%OBSROOT%" (
  echo ERROR: Cannot create 87.175 native observer log directory.
  goto :fail
)
REM XM87.213 restore previously successful 87.121 Cyclops air-throw recovery.
REM Exact pre-throw airborne state after victim lands; no forced new jump.
REM Inherited XM87121_CYCLOPS_RELEASE=0 remains an explicit A/B opt-out.
if not defined XM87121_CYCLOPS_RELEASE set "XM87121_CYCLOPS_RELEASE=1"
if not defined XM87121_CYCLOPS_LOG_FILE set "XM87121_CYCLOPS_LOG_FILE=%OBSROOT%\cyclops_air_throw_release.tsv"
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
REM 'choice' returns ERRORLEVEL 1/2/3, which CMD echo redirection does
REM NOT clear on success. Do not mistake a successful log write for failure.
REM Explicit TYPE sets a fresh exit status and verifies append access.
type nul >>"%XM87181_DIAGNOSTIC_STATUS%" 2>nul
if errorlevel 1 (
  echo ERROR: Observer health log directory/file cannot be opened for append:
  echo   "%XM87181_DIAGNOSTIC_STATUS%"
  echo Check permissions, read-only attributes, and available disk space.
  goto :fail
)
>>"%XM87181_DIAGNOSTIC_STATUS%" echo RUN_TEST_START date=%DATE% time=%TIME%
if not defined XM87180_EXPERIMENTAL_DELAYED_PRIME set "XM87180_EXPERIMENTAL_DELAYED_PRIME=0"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_213.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.213 TEST executable is missing; no fallback to older builds.
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
set "XM8764_REPLAY_ROOT=%PROJECT%\test\87.213\replays"
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
set "XM8761_REPLAY_DIR=%PROJECT%\test\87.213\replays"
if not exist "%XM8761_REPLAY_DIR%" mkdir "%XM8761_REPLAY_DIR%"
if not exist "%XM8761_REPLAY_DIR%" (
  echo ERROR: Cannot create native input replay directory.
  goto :fail
)
set "XM8761_REPLAY_MODE=off"
set "XM87139_PROBE_DIR=%PROJECT%\test\87.213"
set "XM87140_CHAT_DIR=%PROJECT%\test\87.213"
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
echo Cyclops air-throw 87.121 recovery: %XM87121_CYCLOPS_RELEASE%  log: %XM87121_CYCLOPS_LOG_FILE%
echo 87.186 STARTUP HEALTH: %XM87181_DIAGNOSTIC_STATUS%
echo 87.175 NATIVE OBSERVER: %XM87175_DHC_OBSERVER_LOG%
echo Native actor rows require a running XMVSF combat session; menu exit writes health only.
echo 87.185: QCF+any 2P/3P recognizes DHC during real super; native tag still needs ROM acceptance.
echo 87.185: Ken native QCF+2P and instant, no-charge arrival preserved.
echo 87.185: UNSAFE delayed native 2P priming stays OFF in interactive TEST.
echo 87.185: immediate 3P/2P paths, Ken no-charge arrival, and F9 stay intact.
echo 87.185: delayed DHC input priming disabled by default, preserves original ROM state.
echo 87.184 native Shinkuu action lifetime: 2E08 startup/attack, 2E0A recovery, exact exit.
echo 87.213 ROM super start-to-real-exit DHC: %XM87191_TEST_OPEN_SUPER_LIFETIME%  (no phase/timer/air-mode availability veto)
echo Native 2E family: %XM87189_TEST_NATIVE_SUPER_FAMILY% full original attack: %XM87188_TEST_FULL_ATTACK% recovery: %XM87185_TEST_RECOVERY_REBOUND%
echo XMVSF Stage 87.213 TEST - KEEP KEN SHORYUREPPA; CUT OUTGOING RYU PROJECTILE HITS
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
  set "XM44_CAPTURE=0"
)
REM Opt-in Kawaks-style render-pass isolation: use the EXISTING XM44/CPS2
REM exporter and the ORIGINAL ROM pixel renderer, not inferred memory atlases.
REM F9/input replay and the separate 87.203 native telemetry stay unchanged.
if /I "%CLEAN_RIP_MODE%"=="clean" (
  set "XM44_CAPTURE=1"
  set "XM44_CAPTURE_DIR=%PROJECT%\test\87.203\clean_rips"
  if not exist "%PROJECT%\test\87.203\clean_rips" mkdir "%PROJECT%\test\87.203\clean_rips"
  if not exist "%PROJECT%\test\87.203\clean_rips" goto :fail
  REM Default is continuous auto-batching. Existing manual START.flag remains
  REM available if caller explicitly sets XM44_AUTO=0.
  if not defined XM44_AUTO set "XM44_AUTO=1"
  if not defined XM44_FRAMES set "XM44_FRAMES=60"
  if not defined XM44_STEP set "XM44_STEP=30"
  if not defined XM87203_BATCH_FRAMES set "XM87203_BATCH_FRAMES=12000"
  echo.
  echo CPS2 CLEAN RIP CONTINUOUS BATCHING: auto=%XM44_AUTO% burst_samples=%XM44_FRAMES% step=%XM44_STEP%
  echo Native renderer Scroll 1/2/3 + scenery composite, fighters/HUD excluded.
  echo Each burst writes a NEW shot_XXXX directory, no START.flag required in automatic mode.
  echo Native 87.203 telemetry also rolls into NEW session_XXXX every %XM87203_BATCH_FRAMES% frames.
  echo The same existing MSYS watcher reports BATCH_DONE and CLEAN_RIP_BURST_DONE.
  echo Stop by closing the game normally. Files stay intact; do not fill your disk unattended.
  echo Clean rip folder: "%PROJECT%\test\87.203\clean_rips"
  echo PNG converter: python dev\stage_capture\EXPORT_CLEAN_RIP.py --input "..." --output "..."
)
REM 87.203: opt-in Motorola 68000 writer-PC evidence. Never launch GitHub
REM Actions or rewrite Stable. The source ROM/decrypted code stays local.
if /I "%CLEAN_RIP_MODE%"=="trace" set "XM87203_M68K_TRACE=1"
if "%XM87203_M68K_TRACE%"=="1" if /I "%SOURCE_ROM:~0,4%"=="mvsc" (
  REM Always choose a NEW directory; never trust a stale inherited path.
  set "XM87203_M68K_DUMP_DIR="
  if not exist "%PROJECT%\test\87.203\m68k_trace" mkdir "%PROJECT%\test\87.203\m68k_trace"
  for /L %%N in (1,1,9999) do (
    if not defined XM87203_M68K_DUMP_DIR if not exist "%PROJECT%\test\87.203\m68k_trace\run_%%N" (
      mkdir "%PROJECT%\test\87.203\m68k_trace\run_%%N"
      if exist "%PROJECT%\test\87.203\m68k_trace\run_%%N" (
        set "XM87203_M68K_DUMP_DIR=%PROJECT%\test\87.203\m68k_trace\run_%%N"
        set "XM8716_SPRITE_WRITES_FILE=%PROJECT%\test\87.203\m68k_trace\run_%%N\sprite_writes.tsv"
      )
    )
  )
  if not defined XM87203_M68K_DUMP_DIR (
    echo ERROR: Could not reserve a unique M68K trace directory.
    goto :fail
  )
  echo M68K sprite writer PC capture configured under test\87.203\m68k_trace\run_XXXX
  echo Writer source dumps and traces remain LOCAL under ignored test\87.203.
)
pushd "%PROJECT%\FBNeo" || goto :fail
REM XMVSF has its own persistent boot/menu application. Passing xmvsfu as
REM a ROM argument bypasses that application and runs only generic FBNeo.
REM Stage 87.213: start the exact TEST executable without ROM argument.
REM Restore the pre-merge zero-argument native TEST launch for XMVSF.
REM MVC1 remains an explicit source-ROM invocation for capture option 3.
if /I "%SOURCE_ROM%"=="xmvsfu" (
  echo Launching native XMVSF TEST 87.213: Arcade / Training / Versus.
  "%EXE%"
) else (
  echo Launching original CPS2 donor ROM %SOURCE_ROM% with controller support.
  "%EXE%" %SOURCE_ROM% -joy
)
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
REM Source-only fast rip audit after game exits. Never changes the native
REM ROM exit code and never modifies original shot_XXXX files or Stable.
REM FULL (slow) audit is opt-in through REVIEW_CLEAN_RIPS.py --full.
if /I "%CLEAN_RIP_MODE%"=="clean" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    echo.
    echo Reviewing existing clean-rip evidence without requiring a working ROM.
    python.exe "%PROJECT%\dev\stage_capture\REVIEW_CLEAN_RIPS.py" --root "%PROJECT%\test\87.203\clean_rips" --output "%PROJECT%\test\87.203\clean_rips_review" --status-log "%PROJECT%\test\87.203\stage_capture\status.log"
    if errorlevel 1 echo NOTE: Review bundle includes missing/corrupt source diagnostics.
    echo Offline review ZIP: "%PROJECT%\test\87.203\clean_rips_review\clean_rips_review_bundle.zip"
  ) else (
    echo ERROR: Python unavailable for clean-rip review. Existing captures untouched.
  )
)
REM Generate the SMALL M68K writer-PC review automatically at MVC exit.
REM Never override the original ROM exit status or include private code bytes.
if "%XM87203_M68K_TRACE%"=="1" if /I "%SOURCE_ROM:~0,4%"=="mvsc" (
  where python.exe >nul 2>nul
  if not errorlevel 1 (
    python.exe "%PROJECT%\dev\stage_capture\ANALYZE_M68K_SPRITES.py" --root "%PROJECT%\test\87.203\m68k_trace" --output "%PROJECT%\test\87.203\m68k_report"
    if errorlevel 1 echo WARNING: M68K writer-PC report incomplete. Original trace retained.
    echo M68K review ZIP: "%PROJECT%\test\87.203\m68k_report\m68k_sprite_report.zip"
  ) else (
    echo WARNING: Python missing; original private M68K trace files remain available.
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

:offline_clean_review
echo.
echo STAGE 87.203: OFFLINE CLEAN RIP REVIEW ^(NO ROM LAUNCHED^)
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: python.exe not available on PATH.
  exit /b 1
)
python.exe "%PROJECT%\dev\stage_capture\REVIEW_CLEAN_RIPS.py" --root "%PROJECT%\test\87.203\clean_rips" --output "%PROJECT%\test\87.203\clean_rips_review" --status-log "%PROJECT%\test\87.203\stage_capture\status.log"
set "REVIEW_RC=%ERRORLEVEL%"
if exist "%PROJECT%\test\87.203\clean_rips_review\clean_rips_review_bundle.zip" (
  echo.
  echo Review ZIP: "%PROJECT%\test\87.203\clean_rips_review\clean_rips_review_bundle.zip"
) else (
  echo ERROR: Review bundle could not be created.
)
exit /b %REVIEW_RC%


:offline_sprite_factory
echo.
echo STAGE 87.203: OFFLINE CPS2 DIRECT-TILE SPRITE FACTORY (NO ROM LAUNCHED)
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Python unavailable. Original files remain unchanged.
  exit /b 1
)
if "%~2"=="" goto :offline_all_sprite_channels
if /I "%~2"=="channels" goto :offline_all_sprite_channels
if /I "%~2"=="scenery" goto :offline_sprite_all
if /I "%~2"=="all" goto :offline_sprite_all_legacy
echo Direct sprite tile ID: %~2
python.exe "%PROJECT%\dev\stage_capture\SPRITE_FACTORY.py" --clean-root "%PROJECT%\test\87.203\clean_rips" --capture "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\test\87.203\sprite_factory" --tile "%~2"
goto :offline_sprite_finished

:offline_sprite_all
echo Selecting diverse non-HUD scene candidates; identities remain unverified.
python.exe "%PROJECT%\dev\stage_capture\SPRITE_FACTORY.py" --clean-root "%PROJECT%\test\87.203\clean_rips" --capture "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\test\87.203\sprite_factory" --focus scenery
goto :offline_sprite_finished

:offline_sprite_all_legacy
echo Selecting ALL source sprite objects, including fighters and HUD.
python.exe "%PROJECT%\dev\stage_capture\SPRITE_FACTORY.py" --clean-root "%PROJECT%\test\87.203\clean_rips" --capture "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\test\87.203\sprite_factory" --focus all

:offline_sprite_finished
set "SPRITE_RC=%ERRORLEVEL%"
if exist "%PROJECT%\test\87.203\sprite_factory\sprite_factory_review_bundle.zip" (
  echo.
  echo Sprite review ZIP: "%PROJECT%\test\87.203\sprite_factory\sprite_factory_review_bundle.zip"
) else (
  echo ERROR: Sprite Factory review ZIP was not created. Check original capture paths.
)
exit /b %SPRITE_RC%


:offline_all_sprite_channels
echo.
echo STAGE 87.203: OFFLINE ALL CPS2 SPRITE PRIORITIES 0-7 + SCROLL CHANNELS
echo No game, ROM, build, HUD/fighter filtering or guessed scenery labels.
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Python unavailable. Existing captures unchanged.
  exit /b 1
)
python.exe "%PROJECT%\dev\stage_capture\SPRITE_CHANNELS.py" --clean-root "%PROJECT%\test\87.203\clean_rips" --output "%PROJECT%\test\87.203\sprite_channels"
set "CHANNEL_RC=%ERRORLEVEL%"
if not "%CHANNEL_RC%"=="0" (
  echo ERROR: All-channel review failed. Any existing ZIP could be from a prior pass.
  exit /b %CHANNEL_RC%
)
if exist "%PROJECT%\test\87.203\sprite_channels\sprite_all_channels_review_bundle.zip" (
  echo All-channel review ZIP: "%PROJECT%\test\87.203\sprite_channels\sprite_all_channels_review_bundle.zip"
) else (
  echo ERROR: All-channel ZIP not produced.
  exit /b 2
)
exit /b 0

:offline_m68k_sprite_analysis
echo.
echo STAGE 87.203: OFFLINE MVC MOTOROLA 68000 SPRITE-WRITER ANALYSIS
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Python unavailable. Existing traces preserved.
  exit /b 1
)
python.exe "%PROJECT%\dev\stage_capture\ANALYZE_M68K_SPRITES.py" --root "%PROJECT%\test\87.203\m68k_trace" --output "%PROJECT%\test\87.203\m68k_report"
set "M68K_RC=%ERRORLEVEL%"
if not "%M68K_RC%"=="0" (
  echo WARNING: 68000 sprite-writer analysis failed. Existing local source files preserved.
  exit /b %M68K_RC%
)
if exist "%PROJECT%\test\87.203\m68k_report\m68k_sprite_report.zip" echo M68K analysis ZIP: "%PROJECT%\test\87.203\m68k_report\m68k_sprite_report.zip"
REM Follow upstream from the final OAM copier to the original MVC staging
REM dispatcher and indexed tile descriptor writers. Offline; never launches ROM.
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_STAGING.py" --root "%PROJECT%\test\87.203\m68k_trace" --output "%PROJECT%\test\87.203\m68k_report"
set "STAGING_RC=%ERRORLEVEL%"
if not "%STAGING_RC%"=="0" (
  echo ERROR: M68K staging map failed or source snapshot mismatched.
  exit /b %STAGING_RC%
)
if exist "%PROJECT%\test\87.203\m68k_report\m68k_staging_report.zip" (
  echo Staging analysis ZIP: "%PROJECT%\test\87.203\m68k_report\m68k_staging_report.zip"
) else (
  echo ERROR: Staging map ZIP was not generated.
  exit /b 2
)
REM Stage87.203: correlate the original 68000 builder literals with ALL
REM observed OBJ priorities and the original decoded MVC1 GFX. No ROM run.
REM The raw stage observations come from the prior offline Sprite Factory.
if not exist "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" goto :offline_m68k_owner_no_observations
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_OBJECT_OWNERS.py" --observations "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" --trace-root "%PROJECT%\test\87.203\m68k_trace" --staging-emitters "%PROJECT%\test\87.203\m68k_report\staging_descriptor_emitters.tsv" --capture-root "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\test\87.203\m68k_owner"
if errorlevel 1 (
  echo ERROR: M68K owner-evidence correlation failed. Original traces untouched.
  exit /b 2
)
if not exist "%PROJECT%\test\87.203\m68k_owner\m68k_owner_evidence_review.zip" (
  echo ERROR: M68K ownership ZIP not generated.
  exit /b 2
)
echo Owner-evidence ZIP: "%PROJECT%\test\87.203\m68k_owner\m68k_owner_evidence_review.zip"
REM Stage87.203: index ORIGINAL MVC1 68000 tile+attribute assembly pairs,
REM match their parts to real CPS2 OBJ samples and camera scroll. Reuse the
REM previous full-rate native capture, no ROM re-execution and no new builds.
REM Private original program/decrypted bytes never leave local test/87.203.
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_ROM_ASSEMBLIES.py" --observations "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" --trace-root "%PROJECT%\test\87.203\m68k_trace" --owner-evidence "%PROJECT%\test\87.203\m68k_owner\object_family_evidence.tsv" --capture-root "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\test\87.203\m68k_rom_assemblies"
set "ROM_FAMILY_RC=%ERRORLEVEL%"
if not "%ROM_FAMILY_RC%"=="0" (
  echo ERROR: Offline 68000 ROM assembly correlation failed. Existing source preserved.
  exit /b %ROM_FAMILY_RC%
)
if not exist "%PROJECT%\test\87.203\m68k_rom_assemblies\rom_assemblies_review_bundle.zip" (
  echo ERROR: 68000 assembly review ZIP not generated.
  exit /b 2
)
echo ROM assembly ZIP: "%PROJECT%\test\87.203\m68k_rom_assemblies\rom_assemblies_review_bundle.zip"
REM Continue with original 68000 control-footer record boundaries, sampled
REM pose transitions and separately hash-guarded human visual annotations.
REM No ROM restart: reuse only existing private READ data and sprite logs.
call "%~f0" poses
exit /b %ERRORLEVEL%

:offline_m68k_owner_no_observations
echo NOTICE: M68K writer and staging reports completed. Full owner correlation needs
echo the previous offline Sprite Factory observations. To produce them, run:
echo RUN_TEST.bat sprites all
echo and then rerun RUN_TEST.bat m68k. No game or new capture is needed.
exit /b 0

:offline_m68k_pose_analysis
echo.
echo STAGE 87.203: OFFLINE MVC ORIGINAL 68000 POSE RECORD BOUNDARIES
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Python unavailable. Original local sources are preserved.
  exit /b 1
)
if not exist "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" (
  echo ERROR: Missing existing sprite observations. Run RUN_TEST.bat sprites all first.
  exit /b 2
)
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_POSE_BOUNDARIES.py" --observations "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" --trace-root "%PROJECT%\test\87.203\m68k_trace" --owner-evidence "%PROJECT%\test\87.203\m68k_owner\object_family_evidence.tsv" --capture-root "%PROJECT%\test\87.203\stage_capture" --visual-notes "%PROJECT%\dev\stage_capture\ROM_POSE_VISUAL_NOTES.tsv" --output "%PROJECT%\test\87.203\m68k_pose_boundaries"
set "POSE_RC=%ERRORLEVEL%"
if not "%POSE_RC%"=="0" (
  echo ERROR: 68000 pose structural analysis failed. Original local sources unchanged.
  exit /b %POSE_RC%
)
if not exist "%PROJECT%\test\87.203\m68k_pose_boundaries\rom_pose_boundaries_review.zip" (
  echo ERROR: Pose boundary review ZIP not generated.
  exit /b 2
)
echo Candidate pose boundary ZIP: "%PROJECT%\test\87.203\m68k_pose_boundaries\rom_pose_boundaries_review.zip"
REM Cross-reference exact original pose-record addresses with decrypted
REM 68000 selector/dispatch opcode MOTIFS. Static only, no animation timing.
REM Recheck the private ROM hash against the pose manifest before any join.
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_POSE_INTERPRETER.py" --trace-root "%PROJECT%\test\87.203\m68k_trace" --pose-records "%PROJECT%\test\87.203\m68k_pose_boundaries\rom_pose_records.tsv" --output "%PROJECT%\test\87.203\m68k_pose_interpreter"
set "INTERPRETER_RC=%ERRORLEVEL%"
if not "%INTERPRETER_RC%"=="0" (
  echo ERROR: Original M68K footer interpreter lead analysis failed. Sources untouched.
  exit /b %INTERPRETER_RC%
)
if not exist "%PROJECT%\test\87.203\m68k_pose_interpreter\pose_footer_interpreter_leads.zip" (
  echo ERROR: M68K footer interpreter leads ZIP missing.
  exit /b 2
)
echo Footer interpreter leads ZIP: "%PROJECT%\test\87.203\m68k_pose_interpreter\pose_footer_interpreter_leads.zip"
REM Source-verified split-byte lookup plus actual A6+40 pointer consumers.
REM This remains OFFLINE: no native 68000 execution or real timing claim.
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_POSE_CONSUMERS.py" --trace-root "%PROJECT%\test\87.203\m68k_trace" --pose-manifest "%PROJECT%\test\87.203\m68k_pose_boundaries\rom_pose_manifest.json" --output "%PROJECT%\test\87.203\m68k_pose_consumers"
set "CONSUMER_RC=%ERRORLEVEL%"
if not "%CONSUMER_RC%"=="0" (
  echo ERROR: Original M68K pose-pointer consumer analysis failed. Sources preserved.
  exit /b %CONSUMER_RC%
)
if not exist "%PROJECT%\test\87.203\m68k_pose_consumers\pose_pointer_consumer_review.zip" (
  echo ERROR: M68K consumer report ZIP missing.
  exit /b 2
)
echo Pose pointer consumer ZIP: "%PROJECT%\test\87.203\m68k_pose_consumers\pose_pointer_consumer_review.zip"
REM Complete ORIGINAL MVC ROM five-word record grammar survey, including
REM structures never seen by sampled sprite tiles. No runtime or timing claim.
python.exe "%PROJECT%\dev\stage_capture\TRACE_MVC_RECORD_LENGTHS.py" --trace-root "%PROJECT%\test\87.203\m68k_trace" --pose-manifest "%PROJECT%\test\87.203\m68k_pose_boundaries\rom_pose_manifest.json" --output "%PROJECT%\test\87.203\m68k_record_lengths"
set "RECORD_RC=%ERRORLEVEL%"
if not "%RECORD_RC%"=="0" (
  echo ERROR: MVC program structural record-length survey failed. Originals preserved.
  exit /b %RECORD_RC%
)
if not exist "%PROJECT%\test\87.203\m68k_record_lengths\rom_structural_records_review.zip" (
  echo ERROR: MVC record-length review ZIP missing.
  exit /b 2
)
echo Original ROM record-length ZIP: "%PROJECT%\test\87.203\m68k_record_lengths\rom_structural_records_review.zip"
exit /b 0

REM 87.203: root asset organizer. Original sprite graphics remain strictly local.
REM The generated index.html embeds MVC1 original pixels and is ignored by Git.
:offline_mvc_art_review
echo.
echo STAGE 87.203: OFFLINE MVC1 ASSET ORGANIZER
where python.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Python unavailable. Original captures untouched.
  exit /b 1
)
if not exist "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" (
  echo ERROR: Missing original sprite observations.
  echo Run RUN_TEST.bat sprites all first, then RUN_TEST.bat art.
  exit /b 2
)
if not exist "%PROJECT%\asset_organizer\ASSET_ORGANIZER_TEMPLATE.html" (
  echo ERROR: Root asset organizer template is missing.
  exit /b 2
)
if not exist "%PROJECT%\asset_organizer" mkdir "%PROJECT%\asset_organizer"
python.exe "%PROJECT%\dev\stage_capture\BUILD_MVC_ART_REVIEW.py" --trace-root "%PROJECT%\test\87.203\m68k_trace" --observations "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" --capture "%PROJECT%\test\87.203\stage_capture" --template "%PROJECT%\asset_organizer\ASSET_ORGANIZER_TEMPLATE.html" --output "%PROJECT%\asset_organizer\index.html"
set "ART_RC=%ERRORLEVEL%"
if not "%ART_RC%"=="0" (
  echo ERROR: MVC1 original-art organizer export failed. Source captures untouched.
  exit /b %ART_RC%
)
if not exist "%PROJECT%\asset_organizer\index.html" (
  echo ERROR: Asset Organizer HTML missing.
  exit /b 2
)
echo Generated: "%PROJECT%\asset_organizer\index.html"
REM Only review metadata is written to aodata.json; no private ROM or graphics.
REM Preserve any existing aodata.json: rebuilding only reads it, never clears it.
if exist "%PROJECT%\asset_organizer\aodata.json" (
  python.exe "%PROJECT%\asset_organizer\BUILD_WANTED_SEQUENCES.py" --trace-root "%PROJECT%\test\87.203\m68k_trace" --review "%PROJECT%\asset_organizer\aodata.json" --output "%PROJECT%\asset_organizer\wanted_sequences.json"
  if errorlevel 1 (
    echo ERROR: Wanted source-neighbor analysis failed. aodata.json remains unchanged.
    exit /b 2
  )
) else (
  echo NOTICE: No asset_organizer\aodata.json yet. All 560 assets begin unreviewed.
  echo Load the last full organizer JSON or copy it into asset_organizer\aodata.json.
)
REM Assemble selected original MVC1 sampled poses and SCREEN-OBSERVED
REM placements over the captured SCROLL2 diagnostic background. No gameplay
REM stage changes, no private ROM writes. aodata.json remains the source of truth.
if exist "%PROJECT%\asset_organizer\aodata.json" (
  if not exist "%PROJECT%\asset_organizer\BUILD_CAPTURED_ANIMATIONS.py" (
    echo ERROR: Captured animation builder is missing.
    exit /b 2
  )
  python.exe "%PROJECT%\asset_organizer\BUILD_CAPTURED_ANIMATIONS.py" --review "%PROJECT%\asset_organizer\aodata.json" --adjacency "%PROJECT%\asset_organizer\wanted_sequences.json" --gallery "%PROJECT%\asset_organizer\index.html" --observations "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" --capture "%PROJECT%\test\87.203\stage_capture" --trace-root "%PROJECT%\test\87.203\m68k_trace" --template "%PROJECT%\asset_organizer\CAPTURED_ANIMATIONS_TEMPLATE.html" --output "%PROJECT%\asset_organizer\animations.html"
  if errorlevel 1 (
    echo ERROR: Original MVC captured animations could not be generated.
    echo The existing aodata.json remains unchanged.
    exit /b 2
  )
  echo Original captured animation preview: "%PROJECT%\asset_organizer\animations.html"
  echo Original captured pose/camera metadata: "%PROJECT%\asset_organizer\captured_animations.json"
)
REM Recover EVERY native tile referenced by full original ROM source-record
REM regions containing at least one Wanted seed. Never promote user-rejected
REM frames; all newly discovered records remain source candidates.
REM Restore original SCROLL1 near/foreground maps (including Monster Rock Bar
REM tables and props) and recover all recorded 8px map-tile variants.
REM Read-only original program/capture inputs. Generated assets stay ignored.
if exist "%PROJECT%\asset_organizer\aodata.json" (
  python.exe "%PROJECT%\asset_organizer\BUILD_SOURCE_FAMILIES.py" --review "%PROJECT%\asset_organizer\aodata.json" --trace-root "%PROJECT%\test\87.203\m68k_trace" --observations "%PROJECT%\test\87.203\sprite_factory\sprite_observations.tsv.gz" --capture "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\asset_organizer\source_families"
  if errorlevel 1 (
    echo ERROR: Original ROM source-family closure failed. aodata.json preserved.
    exit /b 2
  )
  python.exe "%PROJECT%\asset_organizer\BUILD_FOREGROUND_MAPS.py" --review "%PROJECT%\asset_organizer\aodata.json" --capture "%PROJECT%\test\87.203\stage_capture" --output "%PROJECT%\asset_organizer\foreground_recovered"
  if errorlevel 1 (
    echo ERROR: Captured original SCROLL1 foreground recovery failed. Inputs untouched.
    exit /b 2
  )
  echo Source-family recovered art: "%PROJECT%\asset_organizer\source_families\family_recovery.html"
  echo Original SCROLL1 foreground: "%PROJECT%\asset_organizer\foreground_recovered\foreground.html"
)
if /I "%~2"=="build" (
  echo.
  echo ASSET ORGANIZER BUILD READY. No editor server launched.
  echo Open through RUN_TEST.bat option 2 or RUN_TEST.bat art.
  exit /b 0
)
:offline_mvc_art_launch
if not exist "%PROJECT%\asset_organizer\index.html" (
  echo ERROR: Asset Organizer not built. Run asset_organizer\BUILD_AO.bat.
  exit /b 2
)
if not exist "%PROJECT%\asset_organizer\SERVE_ASSET_ORGANIZER.py" (
  echo ERROR: Asset Organizer localhost server is missing.
  exit /b 2
)
echo Opening local-only Asset Organizer: http://127.0.0.1:18720/
echo Save writes exactly to "%PROJECT%\asset_organizer\aodata.json"
echo Mark all as assigns ONE exclusive type; Wanted decisions remain unchanged.
echo The localhost server stays alive in its own console until you close it.
start "MVC1 Asset Organizer" /D "%PROJECT%" python.exe "%PROJECT%\asset_organizer\SERVE_ASSET_ORGANIZER.py" --root "%PROJECT%\asset_organizer" --trace-root "%PROJECT%\test\87.203\m68k_trace" --port 18720 --open
exit /b 0

REM Single root launch suite. No arguments opens this menu; legacy named
REM ROM and advanced capture/analysis commands remain backward compatible.
:suite_menu
echo.
echo ============================================================
echo   XMVSF - TEST 87.213 / ASSET / CAPTURE SUITE
echo ============================================================
echo   [1] Test game - XMVSF native TEST 87.213
echo   [2] Asset Organizer - build and open local editor
echo   [3] Capture Level - select original donor game
echo.
choice /C 123 /N /M "Select 1, 2 or 3: "
if errorlevel 3 goto :suite_capture
if errorlevel 2 goto :suite_asset
if errorlevel 1 goto :suite_test
exit /b 1

:suite_test
set "SOURCE_ROM=xmvsfu"
set "XM87203_STAGE_CAPTURE_ENABLE=0"
goto :game_run

:suite_asset
if not exist "%PROJECT%\asset_organizer\BUILD_AO.bat" (
  echo ERROR: Missing asset_organizer\BUILD_AO.bat.
  exit /b 2
)
call "%PROJECT%\asset_organizer\BUILD_AO.bat"
if errorlevel 1 exit /b 2
goto :offline_mvc_art_launch

:suite_capture
echo.
echo -------- LEVEL CAPTURE CATEGORY --------
echo   [1] Marvel vs. Capcom 1 (MVC1 / mvscur1)
echo   [Q] Back to main menu
choice /C 1Q /N /M "Select donor game: "
if errorlevel 2 goto :suite_menu
if errorlevel 1 goto :suite_capture_mvc1
exit /b 1

:suite_capture_mvc1
REM Original MVC1 level telemetry stays dormant until first F11 press.
set "SOURCE_ROM=mvscur1"
set "XM87203_STAGE_CAPTURE_ENABLE=1"
goto :game_run
