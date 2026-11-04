@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "SCRIPT=%~dp0APPLY_87_132.py"
where py.exe >nul 2>nul
if not errorlevel 1 (py -3 "%SCRIPT%" %*) else (python "%SCRIPT%" %*)
exit /b %errorlevel%
