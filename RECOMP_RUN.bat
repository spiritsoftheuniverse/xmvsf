@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "BASH=C:\msys64\usr\bin\bash.exe"
if not exist "%BASH%" (echo ERROR: MSYS2 missing.& goto :fail)
if not exist "%PROJECT%\recomp\build\xmvsf\xmvsf.exe" (
 echo ERROR: Recomp executable not found. Run RECOMP_BUILD.bat first.
 goto :fail
)
pushd "%PROJECT%\recomp"
"%BASH%" -c "make GAME=xmvsf run"
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" goto :fail
exit /b 0
:fail
pause
exit /b 1
