@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Freeze the target for this invocation: updates during a long build must not
REM make the wrapper look for a different version than the shell published.
set "XMVSF_TEST_VERSION=87.294"
set "XMVSF_TEST_TAG=%XMVSF_TEST_VERSION:.=_%"
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
