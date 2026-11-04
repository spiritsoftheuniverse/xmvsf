@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "SCRIPT=%~dp0dev\handoff\pack_current.py"
if not exist "%SCRIPT%" (echo ERROR: Missing handoff packer.& goto :fail)
where py.exe >nul 2>nul
if not errorlevel 1 (py -3 "%SCRIPT%" %*) else (python "%SCRIPT%" %*)
if errorlevel 1 goto :fail
echo.
echo Handoff completed. Working source and Stable were not modified.
pause
exit /b 0
:fail
echo HANDOFF FAILED. Working source and Stable were not modified.
pause
exit /b 1
