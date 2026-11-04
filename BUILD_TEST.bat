@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
if not exist "%PROJECT%\dev\dhc\BUILD_TEST.sh" (
  echo ERROR: Stage 87.147 TEST builder not found. Switch to feature/dhc-mvc2-stage87 and pull.
  goto :fail
)
where bash.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: MSYS2 bash.exe not found at C:\msys64\usr\bin
  goto :fail
)
echo XMVSF Stage 87.147 TEST builder - CURRENT warm-cache source
echo Source: %PROJECT%\dev\stable\buildtree
echo Stable and the old 87.132.4 executable will not be overwritten.
pushd "%PROJECT%\dev\dhc" || goto :fail
bash.exe BUILD_TEST.sh
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" goto :fail
echo.
echo Stage 87.147 TEST build published. Run RUN_TEST.bat.
pause
exit /b 0
:fail
echo.
echo TEST BUILD FAILED. Existing Stable and previous TEST binaries preserved.
pause
exit /b 1
