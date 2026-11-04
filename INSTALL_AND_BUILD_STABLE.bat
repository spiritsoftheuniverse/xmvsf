@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "INCOMING=%%~fI"
set "PROJECT=F:\xmvsf"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
echo XMVSF 87.132.4 - Consolidated clean Stable build
where py.exe >nul 2>nul
if not errorlevel 1 (
 py -3 "%INCOMING%\INSTALL_87132_4.py" "%PROJECT%"
) else (
 python "%INCOMING%\INSTALL_87132_4.py" "%PROJECT%"
)
if errorlevel 1 goto :fail
echo.
echo Compiling and promoting Stable from the cleaned source...
if exist "%PROJECT%\BUILD_STABLE.bat" (call "%PROJECT%\BUILD_STABLE.bat") else (goto :fail)
if errorlevel 1 goto :fail
exit /b 0
:fail
echo.
echo FAILED: see the preceding error; existing Stable remains backed up/protected.
pause
exit /b 1
