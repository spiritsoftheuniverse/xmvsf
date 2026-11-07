@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
REM Build logs are written BEFORE checking prerequisites. This path is
REM stable across point versions and never touches or deletes build objects.
set "LOGDIR=%PROJECT%\test\build_logs"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"
if not exist "%LOGDIR%" goto :no_log
set "BUILDLOG=%LOGDIR%\BUILD_TEST.log"
>>"%BUILDLOG%" echo.
>>"%BUILDLOG%" echo ========== XMVSF BUILD_TEST.bat 87.212 START %DATE% %TIME% ==========
>>"%BUILDLOG%" echo Project: %PROJECT%
echo XMVSF 87.212 incremental TEST build
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
bash.exe BUILD_TEST.sh
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" goto :fail
REM The shell must have actually compiled/linked and published the current
REM native TEST executable. Never fall back to Stable or a stale numbered TEST.
if not exist "%PROJECT%\runtime\stable\XMVSF_Test_87_212.exe" (
  echo ERROR: Native compiler returned success but TEST 87.212 executable is missing.
  >>"%BUILDLOG%" echo ERROR: TEST 87.212 executable missing after successful make
  goto :fail
)
for %%I in ("%PROJECT%\runtime\stable\XMVSF_Test_87_212.exe") do if %%~zI LEQ 0 (
  echo ERROR: TEST 87.212 output is empty.
  >>"%BUILDLOG%" echo ERROR: TEST 87.212 executable has zero bytes.
  goto :fail
)
echo.
echo TEST 87.212 BUILD READY. Run RUN_TEST.bat option 1.
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
