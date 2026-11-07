@echo off
setlocal EnableExtensions DisableDelayedExpansion
REM Compatibility launcher only. Actual TEST build/run entry points remain
REM BUILD_TEST.bat and RUN_TEST.bat. Never pull another branch on launch.
echo XMVSF NATIVE TEST 87.204: forwarding to RUN_TEST.bat
if not exist "%~dp0RUN_TEST.bat" (
  echo ERROR: RUN_TEST.bat is missing from the project root.
  exit /b 2
)
call "%~dp0RUN_TEST.bat" %*
exit /b %ERRORLEVEL%
