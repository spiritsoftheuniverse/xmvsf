@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0..") do set "XMVSF_PROJECT=%%~fI"
cd /d "%XMVSF_PROJECT%"
echo ====================================================
echo   XMVSF OST NORMALIZATION STUDIO
echo ====================================================
echo.
echo Project: "%XMVSF_PROJECT%"
echo Audio:   "%XMVSF_PROJECT%\audio"
echo Backups: "%XMVSF_PROJECT%\audio\.ost_studio\originals"
echo.
echo Local browser UI. Press Ctrl+C to close the studio.
echo This console remains open while the studio is running.
echo.
where py.exe >nul 2>nul
if not errorlevel 1 goto :python_launcher
where python.exe >nul 2>nul
if not errorlevel 1 goto :python_direct
echo ERROR: Python 3 not found.
pause
exit /b 2

:python_launcher
py -3 "%~dp0audio_studio_server.py"
set "RC=%ERRORLEVEL%"
goto :done

:python_direct
python.exe "%~dp0audio_studio_server.py"
set "RC=%ERRORLEVEL%"
goto :done

:done
if not "%RC%"=="0" (
  echo.
  echo Studio stopped with error code %RC%.
  pause
)
exit /b %RC%
