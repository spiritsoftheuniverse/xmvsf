@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "SCRIPT=%PROJECT%\dev\stable\releases\87_132\build_87132.py"
if not exist "%SCRIPT%" (echo ERROR: Install the consolidated handoff first.& goto :fail)
echo XMVSF 87.132.4 TEST: clean compile, no automatic captures, warm cache preserved
where py.exe >nul 2>nul
if not errorlevel 1 (py -3 "%SCRIPT%" reset-test) else (python "%SCRIPT%" reset-test)
if errorlevel 1 goto :fail
echo.
echo Build successful.
pause
exit /b 0
:fail
echo.
echo BUILD STOPPED. Original Stable and object cache preserved on compiler failure.
pause
exit /b 1
