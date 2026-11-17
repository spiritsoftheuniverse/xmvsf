@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Freeze the target for this invocation: updates during a long build must not
REM make the wrapper look for a different version than the shell published.
set "XMVSF_TEST_VERSION=87.415"
set "XMVSF_TEST_TAG=%XMVSF_TEST_VERSION:.=_%"
REM Explicit Joe-only refresh: BUILD_TEST.bat joe. No other PSD PNG is touched.
REM Normal BUILD_TEST.bat keeps the original complete SHA-inventory validation.
set "XMVSF_GUILE_ACTOR_ONLY="
if /I "%~1"=="joe" (
  set "XMVSF_GUILE_ACTOR_ONLY=Joe"
  echo Joe-only Guile PSD refresh requested; build still uses native warm cache.
)
REM Build logs are written BEFORE checking prerequisites. This path is
REM stable across point versions and never touches or deletes build objects.
set "LOGDIR=%PROJECT%\test\build_logs"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"
if not exist "%LOGDIR%" goto :no_log
set "BUILDLOG=%LOGDIR%\BUILD_TEST.log"
>>"%BUILDLOG%" echo.
>>"%BUILDLOG%" echo ========== XMVSF BUILD_TEST.bat %XMVSF_TEST_VERSION% START %DATE% %TIME% ==========
>>"%BUILDLOG%" echo Project: %PROJECT%
echo XMVSF %XMVSF_TEST_VERSION% incremental TEST build
echo Source: "%PROJECT%\dev\stable\buildtree"
echo Permanent build transcript: "%BUILDLOG%"
echo All build output is displayed AND appended to the transcript.
echo Stable, prior TEST binaries, and warm object cache are preserved.
if not exist "%PROJECT%\dev\dhc\BUILD_TEST.sh" (
  echo ERROR: Current native TEST builder missing. Update repository source to the latest checkpoint.
  >>"%BUILDLOG%" echo ERROR: Missing dev\dhc\BUILD_TEST.sh
  goto :fail
)
REM TEST 87.401: fail before compilation if the authored GIF pack is absent.
REM The source repo contains these as binary Git blobs; DO NOT regenerate them.
for %%G in (rotor.gif triblaster.gif triblaster_shot.gif triblaster_bullet.gif) do (
  if not exist "%PROJECT%\_art\boomerkuwanger\%%G" (
    echo ERROR: Missing Boomer sprite: _art\boomerkuwanger\%%G
    >>"%BUILDLOG%" echo ERROR: Missing Boomer sprite _art\boomerkuwanger\%%G
    goto :fail
  )
)
echo Boomer GIF asset preflight: all four sprites present.
REM 87.403: compile the exact stage GIF header against the installed
REM SDL2_image API BEFORE the expensive full native Make. This catches
REM misspelled IMG_Animation types in seconds without changing warm cache.
set "BOOMER_PREFLIGHT=%LOGDIR%\BOOMER_GIF_HEADER_403.log"
if not exist "C:\msys64\ucrt64\include\SDL2\SDL_image.h" (
  echo ERROR: UCRT64 SDL2_image headers are missing.
  >>"%BUILDLOG%" echo ERROR: UCRT64 SDL2_image headers missing.
  goto :fail
)
echo Boomer SDL2_image GIF type smoke check...
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -fsyntax-only -I"C:\msys64\ucrt64\include\SDL2" "%PROJECT%\dev\boomer\tests\test_boomer_gif_header_403.cpp" >"%BOOMER_PREFLIGHT%" 2>&1
set "BOOMER_RC=%ERRORLEVEL%"
type "%BOOMER_PREFLIGHT%"
type "%BOOMER_PREFLIGHT%" >>"%BUILDLOG%"
if not "%BOOMER_RC%"=="0" (
  echo ERROR: Boomer GIF header failed compiler smoke check. Aborting before full native build.
  >>"%BUILDLOG%" echo ERROR: Boomer GIF header failed compiler smoke check.
  goto :fail
)
echo Boomer GIF native header smoke PASS.
REM 87.404: run geometry/layer tests before the expensive native Make.
REM Tests use pure standard C++; never touch Stable or cached FBNeo objects.
set "BOOMER_MOTION_LOG=%LOGDIR%\BOOMER_MOTION_405.log"
set "BOOMER_MOTION_EXE=%LOGDIR%\BOOMER_MOTION_405.exe"
echo Boomer world-anchor/camera/layer filter smoke test...
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -Wall -Wextra -Werror -O2 "%PROJECT%\dev\boomer\tests\test_boomer_motion_401.cpp" -o "%BOOMER_MOTION_EXE%" >"%BOOMER_MOTION_LOG%" 2>&1
if errorlevel 1 (
  type "%BOOMER_MOTION_LOG%"
  type "%BOOMER_MOTION_LOG%" >>"%BUILDLOG%"
  echo ERROR: Boomer geometry/layer C++ test did not compile.
  goto :fail
)
"%BOOMER_MOTION_EXE%" >>"%BOOMER_MOTION_LOG%" 2>&1
if errorlevel 1 (
  type "%BOOMER_MOTION_LOG%"
  type "%BOOMER_MOTION_LOG%" >>"%BUILDLOG%"
  echo ERROR: Boomer geometry/layer C++ assertions failed.
  goto :fail
)
type "%BOOMER_MOTION_LOG%"
type "%BOOMER_MOTION_LOG%" >>"%BUILDLOG%"
REM 87.408: cheap compile-only metadata/catalog check; no image reads
REM or extra native build/link work.
set "STAGE_CATALOG_PREFLIGHT=%LOGDIR%\STAGE_CATALOG_408.log"
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -fsyntax-only "%PROJECT%\dev\boomer\tests\test_stage_catalog_408.cpp" >"%STAGE_CATALOG_PREFLIGHT%" 2>&1
if errorlevel 1 (
  type "%STAGE_CATALOG_PREFLIGHT%"
  type "%STAGE_CATALOG_PREFLIGHT%" >>"%BUILDLOG%"
  echo ERROR: Native stage catalog failed C++ preflight.
  goto :fail
)
echo Stage screenshot catalog compile PASS.
REM 87.414 also tests native OBJ source and repeated-match key-9 rearm.
set "BOOMER_CUTOUT_LOG=%LOGDIR%\BOOMER_CUTOUT_412.log"
set "BOOMER_CUTOUT_EXE=%LOGDIR%\BOOMER_CUTOUT_412.exe"
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -Wall -Wextra -Werror -O2 "%PROJECT%\dev\boomer\tests\test_boomer_cutout_412.cpp" -o "%BOOMER_CUTOUT_EXE%" >"%BOOMER_CUTOUT_LOG%" 2>&1
if errorlevel 1 (
  type "%BOOMER_CUTOUT_LOG%"
  type "%BOOMER_CUTOUT_LOG%" >>"%BUILDLOG%"
  echo ERROR: Boomer native HUD/source cutout regression did not compile.
  goto :fail
)
"%BOOMER_CUTOUT_EXE%" >>"%BOOMER_CUTOUT_LOG%" 2>&1
if errorlevel 1 (
  type "%BOOMER_CUTOUT_LOG%"
  type "%BOOMER_CUTOUT_LOG%" >>"%BUILDLOG%"
  echo ERROR: Boomer native HP and SDL stun cutout assertions failed.
  goto :fail
)
type "%BOOMER_CUTOUT_LOG%"
type "%BOOMER_CUTOUT_LOG%" >>"%BUILDLOG%"
REM 87.415: verify portrait frames, point/time atlas and strict forest veto.
REM Previous builds ran only the Boomer policy test and missed 02/04 regression.
set "SHOCK_HUD_LOG=%LOGDIR%\HUD_SHOCK_413.log"
set "SHOCK_HUD_EXE=%LOGDIR%\HUD_SHOCK_413.exe"
"C:\msys64\ucrt64\bin\g++.exe" -std=c++11 -O2 -Wall -Wextra "%PROJECT%\dev\hud\tests\test_hud_shock_392.cpp" -o "%SHOCK_HUD_EXE%" >"%SHOCK_HUD_LOG%" 2>&1
if errorlevel 1 (
  type "%SHOCK_HUD_LOG%"
  type "%SHOCK_HUD_LOG%" >>"%BUILDLOG%"
  echo ERROR: Full top-HUD shock regression C++ test did not compile.
  goto :fail
)
"%SHOCK_HUD_EXE%" >>"%SHOCK_HUD_LOG%" 2>&1
if errorlevel 1 (
  type "%SHOCK_HUD_LOG%"
  type "%SHOCK_HUD_LOG%" >>"%BUILDLOG%"
  echo ERROR: Full top-HUD shock entry regression failed.
  goto :fail
)
type "%SHOCK_HUD_LOG%"
type "%SHOCK_HUD_LOG%" >>"%BUILDLOG%"
REM TEST 87.409: prevent grounded/ordinary-jump normals from inheriting
REM Tournament flying-screen reaction writes. Fast ROM-free C++ test only;
REM preserve warm cache and do not extend the full native test suite.
set "FS_SCOPE_LOG=%LOGDIR%\FS_SUPERJUMP_SCOPE_409.log"
set "FS_SCOPE_EXE=%LOGDIR%\FS_SUPERJUMP_SCOPE_409.exe"
"C:\msys64\ucrt64\bin\g++.exe" -std=c++17 -Wall -Wextra -Werror -O2 -I"%PROJECT%\dev\stable\buildtree\src\burn\drv\capcom" "%PROJECT%\dev\dhc\tests\test_tournament_fs_superjump_scope_409.cpp" -o "%FS_SCOPE_EXE%" >"%FS_SCOPE_LOG%" 2>&1
if errorlevel 1 (
  type "%FS_SCOPE_LOG%"
  type "%FS_SCOPE_LOG%" >>"%BUILDLOG%"
  echo ERROR: Tournament superjump FS scope regression did not compile.
  goto :fail
)
"%FS_SCOPE_EXE%" >>"%FS_SCOPE_LOG%" 2>&1
if errorlevel 1 (
  type "%FS_SCOPE_LOG%"
  type "%FS_SCOPE_LOG%" >>"%BUILDLOG%"
  echo ERROR: Ground/normal-jump FS leak regression failed.
  goto :fail
)
type "%FS_SCOPE_LOG%"
type "%FS_SCOPE_LOG%" >>"%BUILDLOG%"
where bash.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: MSYS2 bash.exe not found on PATH.
  >>"%BUILDLOG%" echo ERROR: MSYS2 bash.exe not found on PATH
  goto :fail
)
where tee.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: MSYS2 tee.exe not found on PATH. Cannot write live build log.
  >>"%BUILDLOG%" echo ERROR: MSYS2 tee.exe not found on PATH
  goto :fail
)
pushd "%PROJECT%\dev\dhc"
if errorlevel 1 (
  echo ERROR: Cannot enter dev\dhc build directory.
  >>"%BUILDLOG%" echo ERROR: Cannot enter dev\dhc build directory
  goto :fail
)
REM Native shell syntax check is fast and catches a damaged build launcher
REM before touching any native objects or TEST output.
bash.exe -n BUILD_TEST.sh
if errorlevel 1 (
  echo ERROR: BUILD_TEST.sh has a shell syntax error; no build started.
  >>"%BUILDLOG%" echo ERROR: BUILD_TEST.sh shell syntax invalid
  popd
  goto :fail
)
REM Validate the compact-link helper too; its fallback must never hide a syntax error.
if exist FAST_MENU_LINK_INPUTS_235.sh (
  bash.exe -n FAST_MENU_LINK_INPUTS_235.sh
  if errorlevel 1 (
    echo ERROR: FAST_MENU_LINK_INPUTS_235.sh has a shell syntax error; no build started.
    >>"%BUILDLOG%" echo ERROR: FAST_MENU_LINK_INPUTS_235.sh shell syntax invalid
    popd
    goto :fail
  )
)
bash.exe BUILD_TEST.sh
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" goto :fail
REM The shell must have actually compiled/linked and published the current
REM native TEST executable. Never fall back to Stable or a stale numbered TEST.
if not exist "%PROJECT%\runtime\stable\XMVSF_Test_%XMVSF_TEST_TAG%.exe" (
  echo ERROR: Native compiler returned success but TEST %XMVSF_TEST_VERSION% executable is missing.
  >>"%BUILDLOG%" echo ERROR: TEST %XMVSF_TEST_VERSION% executable missing after successful make
  goto :fail
)
for %%I in ("%PROJECT%\runtime\stable\XMVSF_Test_%XMVSF_TEST_TAG%.exe") do if %%~zI LEQ 0 (
  echo ERROR: TEST %XMVSF_TEST_VERSION% output is empty.
  >>"%BUILDLOG%" echo ERROR: TEST %XMVSF_TEST_VERSION% executable has zero bytes.
  goto :fail
)
echo.
echo TEST %XMVSF_TEST_VERSION% BUILD READY. Run RUN_TEST.bat option 1.
echo Full transcript: "%BUILDLOG%"
>>"%BUILDLOG%" echo BATCH RESULT: SUCCESS %DATE% %TIME%
pause
exit /b 0
:fail
echo.
echo TEST BUILD FAILED. Existing Stable and previous TEST binaries preserved.
echo Diagnostic transcript: "%BUILDLOG%"
>>"%BUILDLOG%" echo BATCH RESULT: FAILURE %DATE% %TIME%
pause
exit /b 1
:no_log
echo ERROR: Cannot create "%LOGDIR%" for the build transcript.
echo The build was NOT started. Verify write permissions.
pause
exit /b 2
