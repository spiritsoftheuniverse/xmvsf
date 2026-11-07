@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0..") do set "PROJECT=%%~fI"
REM Offline compiler for the MVC1 Asset Organizer, using current captured data.
REM No native FBNeo build, ROM execution, Stable write, or GitHub Actions.
REM The common root 'art build' route is the SINGLE authoritative pipeline:
REM 560 sampled art candidates, palette groups, aodata restore, source
REM families, native SCROLL1, and sampled animation/placement previews.
if not exist "%PROJECT%\RUN_TEST.bat" (
  echo ERROR: Missing root RUN_TEST.bat.
  exit /b 2
)
echo.
echo ASSET ORGANIZER: regenerate offline viewer and recovery indexes
echo Source: "%PROJECT%\test\87.203"
echo Output: "%PROJECT%\asset_organizer\index.html"
call "%PROJECT%\RUN_TEST.bat" art build
set "AO_RC=%ERRORLEVEL%"
if not "%AO_RC%"=="0" (
  echo ERROR: AO build failed with exit code %AO_RC%.
  echo Existing aodata.json and original capture inputs remain unchanged.
  exit /b %AO_RC%
)
if not exist "%PROJECT%\asset_organizer\index.html" (
  echo ERROR: AO build finished but index.html is missing.
  exit /b 2
)
echo ASSET ORGANIZER READY.
echo Use root RUN_TEST.bat option 2 to open the local editor.
exit /b 0
