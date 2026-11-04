@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
set "EXE=%PROJECT%\runtime\stable\XMVSF_Test_87_138.exe"
if not exist "%EXE%" (
  echo ERROR: Stage 87.138 TEST executable is missing.
  echo Run BUILD_TEST.bat first. Will NOT fall back to 87.132.4 or Stable.
  goto :fail
)
if not exist "%PROJECT%\FBNeo" (
  echo ERROR: FBNeo game data directory is missing.
  goto :fail
)
set "XM87138_TAG_LAND_TEST=1"
if not defined XM87138_TAG_LAND_LOG set "XM87138_TAG_LAND_LOG=%TEMP%\xm87138_tag_landing.tsv"
set "XM8761_DISABLE_RUNAHEAD=1"
echo XMVSF Stage 87.138 TEST - native tag-in beside opponent (NOT full DHC)
echo Executable: %EXE%
echo Stage 05 tag landing enabled. Log: %XM87138_TAG_LAND_LOG%
pushd "%PROJECT%\FBNeo" || goto :fail
"%EXE%"
set "RC=%ERRORLEVEL%"
popd
echo.
echo Stage 87.138 TEST exit code: %RC%
echo Tag landing log: %XM87138_TAG_LAND_LOG%
pause
exit /b %RC%
:fail
echo.
echo TEST RUN STOPPED. No older executable was launched.
pause
exit /b 1
