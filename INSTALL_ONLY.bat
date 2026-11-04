@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "INCOMING=%%~fI"
where py.exe >nul 2>nul
if not errorlevel 1 (py -3 "%INCOMING%\INSTALL_87132_4.py" "F:\xmvsf") else (python "%INCOMING%\INSTALL_87132_4.py" "F:\xmvsf")
if errorlevel 1 (echo INSTALL FAILED& pause& exit /b 1)
echo Source installed; build with F:\xmvsf\BUILD_TEST.bat or BUILD_STABLE.bat.
pause
