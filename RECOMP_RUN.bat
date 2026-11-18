@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "EXE=%PROJECT%\recomp\build\xmvsf\xmvsf.exe"
if not exist "%EXE%" (
  echo ERROR: Recomp executable not found. Run RECOMP_BUILD.bat first.
  goto :fail
)
if not exist "%PROJECT%\recomp\rom\xmvsf" (
  echo ERROR: Compatible XMVSF ROM set missing at recomp\rom\xmvsf.
  goto :fail
)
if not exist "%PROJECT%\recomp\work\xmvsf" mkdir "%PROJECT%\recomp\work\xmvsf"
pushd "%PROJECT%\recomp"
if errorlevel 1 goto :fail
echo Starting pinned XMVSF recomp. No rebuild or translation step.
"build\xmvsf\xmvsf.exe" --rom "rom\xmvsf" --eeprom "work\xmvsf\eeprom.bin" --settings "work\xmvsf\settings.ini" --record-entries "work\xmvsf\entries.txt" %*
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" goto :fail
exit /b 0
:fail
pause
exit /b 1
