@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_156.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.156 TEST executable is missing.
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
set "XM87139_PROBE_DIR=%PROJECT%\test\87.156"
set "XM87140_CHAT_DIR=%PROJECT%\test\87.156"
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
REM 87.156: PROOF mode bypasses the forced 87.155 Ryu controller8 RAM
REM mutation. It waits for natural Ryu super exit, issues authentic
REM HP+HK native tag and Ken QCF+2P/2K through CPS2 input mapper.
REM This is deliberately sequential; proves move entry BEFORE true DHC.
set "XM87156_PROOF_NATIVE=1"
set "XM87144_DHC_LOG=%XM87140_CHAT_DIR%\dhc_native_handoff.tsv"
REM Enable the latest Tournament-only rule implementations in TEST.
REM Native handlers ALSO require the live menu selection tournament=1.
REM Classic remains ROM-original; do NOT enable retired 80/83/84/85 flight writers.
set "XM8786_STORM_BUTTON_TEST=1"
set "XM8795_NATIVE_HIT_TEST=1"
set "XM8796_DIZZY_HANDOFF_TEST=1"
set "XM8799_GROUND_DEFENSE_TEST=1"
set "XM87149_TOURNAMENT_AUDIT=%XM87140_CHAT_DIR%\tournament_rules.tsv"
echo XMVSF Stage 87.156 TEST - Ryu-to-Ken native super entry PROOF, NOT full DHC yet
echo Executable: %EXE%
echo SAME BUILD, TWO DHC CHOICES:
echo   During Ryu Shinkuu Hadoken, QCF+LP+MP queues Ken SHORYUREPPA 2E00/D6722.
echo   During Ryu Shinkuu Hadoken, QCF+LK+MK queues Ken SHINRYUKEN 2E02/D6976.
echo Both ROM targets are verified and logged simultaneously in dhc_native_handoff.tsv.
echo After Ryu finishes, TEST uses native HP+HK tag, waits for Ken to finish arrival, then his own QCF+2P/2K.
echo This is a sequential proof of real ROM command acceptance, not a completed DHC.
echo Native variable combination QCF+HP+HK is NEVER submitted by the test.
echo Invalid Ryu 2C04 native dispatch aborts without copying stale controller state.
echo Training menu options persist in FBNeo\config\games\xmvsfu_training_options.cfg
echo FRAME DATA: ATTACK DURATION ends on native action exit - directions, jumps, and height do not extend it.
echo Verified modes: normal 28xx, special 2Cxx, and multi-step super 2Exx.
echo Saved: hitboxes, display, meter, dizzy, damage mode, two dummy HPs, player select, frame data.
echo Super-candidate inputs print to Warcraft-style chat and fade after 3 sec.
echo Shift+G opens/closes the full chat history.
echo F10 native capture ON/OFF is acknowledged in chat and written to disk.
echo F9 replay; Shift+F10 quick-load.
echo Super chat transcript: %XM87140_CHAT_LOG%
echo ROM script/role events: %XM87142_ROM_TRACE%
echo Prepared queue events and selections: %XM87140_CHAT_LOG%
echo DHC native step-by-step transaction: %XM87144_DHC_LOG%
echo In PROOF mode, Ken's normal native entry and exit are untouched (no kick suppression hacks).
echo After Ken is ready, TEST submits one QCF+2P or QCF+2K through native CPS-2 ports.
echo Native 2E00 Shoryureppa / 2E02 Shinryuken are independently verified; no host script injection.
echo All physical button inputs remain intact outside the 17-frame DHC command window.
echo Ryu tag-out and super-charge streaks retained.
echo Failed input dispatch, normal super recovery and native point swap release the DHC lock.
echo EXPERIMENT: P1 Ryu-to-Ken only. Uses a genuine ROM tag input after natural Ryu super end. Stable untouched.
echo Active Tournament rules audit: %XM87149_TOURNAMENT_AUDIT%
echo Tournament rules are applied ONLY when selected in the Options menu.
echo F10 probe files: %XM87139_PROBE_DIR%
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%"
set "RC=%ERRORLEVEL%"
popd
echo.
echo Stage 87.156 TEST exit code: %RC%
echo Probe directory: %XM87139_PROBE_DIR%
pause
exit /b %RC%
:fail
echo.
echo TEST RUN STOPPED. No older executable was launched.
pause
exit /b 1
