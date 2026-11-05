@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM 87.175 read-only native state trace. Fails closed if logs cannot be saved.
set "OBSROOT=%PROJECT%\test\87.177\observer"
if not exist "%OBSROOT%" mkdir "%OBSROOT%"
if not exist "%OBSROOT%" (
  echo ERROR: Cannot create 87.175 native observer log directory.
  goto :fail
)
set "XM87175_DHC_OBSERVER=1"
set "XM87175_DHC_OBSERVER_LOG=%OBSROOT%\dhc_acceptance_test.tsv"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_177.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.177 TEST executable is missing.
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
set "XM8764_REPLAY_ROOT=%PROJECT%\test\87.177\replays"
if not exist "%XM8764_REPLAY_ROOT%" mkdir "%XM8764_REPLAY_ROOT%"
if not exist "%XM8764_REPLAY_ROOT%" (
  echo ERROR: Cannot create permanent app-level replay recording directory.
  goto :fail
)
REM Even an F9 directory or save failure gets a permanent, uploadable
REM diagnostic next to the new recordings (the game is not a console).
set "XM8764_REPLAY_STATUS_FILE=%XM8764_REPLAY_ROOT%\F9_status.log"
>>"%XM8764_REPLAY_STATUS_FILE%" echo ===== 87.177 TEST session %DATE% %TIME% =====
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
set "XM8761_REPLAY_DIR=%PROJECT%\test\87.177\replays"
if not exist "%XM8761_REPLAY_DIR%" mkdir "%XM8761_REPLAY_DIR%"
if not exist "%XM8761_REPLAY_DIR%" (
  echo ERROR: Cannot create native input replay directory.
  goto :fail
)
set "XM8761_REPLAY_MODE=off"
set "XM87139_PROBE_DIR=%PROJECT%\test\87.177"
set "XM87140_CHAT_DIR=%PROJECT%\test\87.177"
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
echo 87.175 READ-ONLY NATIVE OBSERVER: %XM87175_DHC_OBSERVER_LOG%
echo 87.177: QCF+LP+MP / LP+HP / MP+HP / 3P triggers Ken, 3P dash suppressed.
echo 87.177: Ken native QCF+2P and instant, no-charge arrival preserved.
echo XMVSF Stage 87.177 TEST - KEEP KEN SHORYUREPPA; CUT OUTGOING RYU PROJECTILE HITS
echo Executable: %EXE%
echo SAME BUILD, TWO DHC CHOICES:
echo   During Ryu Shinkuu Hadoken, QCF+ANY TWO PUNCHES or QCF+3P queues Ken SHORYUREPPA.
echo   During Ryu Shinkuu Hadoken, QCF+LK+MK queues Ken SHINRYUKEN 2E02/D6976.
echo Both ROM targets are verified and logged simultaneously in dhc_native_handoff.tsv.
echo Ryu's super native tag uses the proven 87.151 timer 28 window.
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
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%"
set "RC=%ERRORLEVEL%"
popd
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
  echo WARNING: Native 87.175 observer TSV was not written: "%XM87175_DHC_OBSERVER_LOG%"
)
echo Stage 87.177 TEST exit code: %RC%
echo Probe directory: %XM87139_PROBE_DIR%
pause
exit /b %RC%
:fail
echo.
echo TEST RUN STOPPED. No older executable was launched.
pause
exit /b 1
