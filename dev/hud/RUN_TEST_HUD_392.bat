@echo off
setlocal
cd /d "%~dp0"
set "COMPILER=C:\msys64\ucrt64\bin\g++.exe"
if not exist "%COMPILER%" set "COMPILER=g++"
set "OUTPUT=%TEMP%\xmvsf_hud_shock_398_test.exe"
echo [HUD 87.398] Compiling standalone C++11 test...
"%COMPILER%" -std=c++11 -O2 -Wall -Wextra -pedantic "%~dp0tests\test_hud_shock_392.cpp" -o "%OUTPUT%"
if errorlevel 1 goto failed
echo [HUD 87.398] Running...
"%OUTPUT%"
if errorlevel 1 goto failed
echo [HUD 87.398] PASS
exit /b 0
:failed
echo [HUD 87.398] FAILED
pause
exit /b 1
