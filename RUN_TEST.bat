@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
if not exist "%PROJECT%\dev\stable\releases\87_132\run_87132.py" (echo ERROR: Missing 87.132 script. Apply handoff first.& goto :fail)
echo XMVSF 87.132: F9 record/stop, Shift+F9 quick save
where py.exe >nul 2>nul
if not errorlevel 1 (
 py -3 "%PROJECT%\dev\stable\releases\87_132\run_87132.py" 
) else (
 python "%PROJECT%\dev\stable\releases\87_132\run_87132.py" 
)
if errorlevel 1 goto :fail
echo.
echo Completed successfully.
pause
exit /b 0
:fail
echo.
echo OPERATION STOPPED. See error above.
pause
exit /b 1
