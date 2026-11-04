@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0.") do set "PROJECT=%%~fI"
set "MSYSTEM=UCRT64"
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "XMVSF_PROJECT=%PROJECT%"
if not defined XMVSF_BUILD_JOBS set "XMVSF_BUILD_JOBS=12"
set "SCRIPT=%PROJECT%\dev\stable\releases\87_119\build_87119.py"
if not exist "%SCRIPT%" (echo ERROR: Missing 87.119 builder. Apply handoff first.& goto :fail)
where py.exe >nul 2>nul
if not errorlevel 1 (py -3 "%SCRIPT%") else (python "%SCRIPT%")
if errorlevel 1 goto :fail
echo.
echo Build successful. Use RUN_TEST.bat to launch 87.119.
pause
exit /b 0
:fail
echo.
echo BUILD STOPPED. Stable, archive, and previous test binaries were not overwritten by this launcher.
pause
exit /b 1
