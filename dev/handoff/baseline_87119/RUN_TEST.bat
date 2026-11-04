@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "PATH=C:\msys64\ucrt64\bin;%PROJECT%\runtime\stable;%PROJECT%\runtime\stage79;%PATH%"
set "SCRIPT=%PROJECT%\dev\stable\releases\87_119\run_87119.py"
if not exist "%SCRIPT%" (echo ERROR: Missing 87.119 runner. Apply handoff first.& goto :fail)
where py.exe >nul 2>nul
if not errorlevel 1 (py -3 "%SCRIPT%") else (python "%SCRIPT%")
if errorlevel 1 goto :fail
pause
exit /b 0
:fail
echo.
echo RUN STOPPED. Stable was not modified.
pause
exit /b 1
