@echo off
setlocal EnableExtensions
REM TEST 87.401: local native math only. No GitHub Actions or ROM required.
set "ROOT=%~dp0"
set "SOURCE=%ROOT%tests\test_boomer_motion_401.cpp"
set "OUTPUT=%ROOT%tests\test_boomer_motion_401.exe"
set "CXX=C:\msys64\ucrt64\bin\g++.exe"
if not exist "%CXX%" set "CXX=g++.exe"
echo Compiling Boomer Kuwanger pure motion assertions...
"%CXX%" -std=c++17 -Wall -Wextra -Werror -O2 "%SOURCE%" -o "%OUTPUT%"
if errorlevel 1 (
  echo TEST 87.401 MOTION BUILD FAILED
  pause
  exit /b 1
)
"%OUTPUT%"
if errorlevel 1 (
  echo TEST 87.401 MOTION FAILED
  pause
  exit /b 1
)
echo TEST 87.401 MOTION PASS
exit /b 0
