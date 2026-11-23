@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0..") do set "XMVSF_PROJECT=%%~fI"
cd /d "%XMVSF_PROJECT%"
REM Match RECOMP_RUN.bat: the native sprite generator needs UCRT64 GCC DLLs.
REM Only this local launcher inherits the path; the host system is unchanged.
if not defined XMVSF_UCRT64_ROOT set "XMVSF_UCRT64_ROOT=C:\msys64\ucrt64"
set "MSYSTEM=UCRT64"
set "PATH=%XMVSF_UCRT64_ROOT%\bin;%XMVSF_UCRT64_ROOT%\..\usr\bin;%PATH%"
echo ====================================================
echo   XMVSF II PALETTE STUDIO
echo ====================================================
echo.
echo Project: "%XMVSF_PROJECT%"
echo Palette profiles: "%XMVSF_PROJECT%\palette_editor\profiles"
echo Imported sprites: "%XMVSF_PROJECT%\palette_editor\sprites"
echo.
echo A local browser window will open. Press Ctrl+C to quit.
echo The editor never modifies original ROM files.
echo.
where py.exe >nul 2>nul
if not errorlevel 1 goto :py
where python.exe >nul 2>nul
if not errorlevel 1 goto :python
echo ERROR: Python 3 was not found.
pause
exit /b 2

:py
py -3 "%~dp0palette_studio_server.py"
set "RC=%ERRORLEVEL%"
goto :done
:python
python.exe "%~dp0palette_studio_server.py"
set "RC=%ERRORLEVEL%"
:done
if not "%RC%"=="0" (
 echo.
 echo Palette Studio stopped with error code %RC%.
 pause
)
exit /b %RC%
