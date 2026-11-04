@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "STABLE=%~dp0"
for %%I in ("%STABLE%..\..") do set "PROJECT=%%~fI"
set "EXE=%STABLE%XMVSF_Test_87_115.exe"
if not exist "%EXE%" (echo ERROR: Missing Stage87.115 native executable & exit /b 2)
for /f "usebackq delims=" %%T in (`powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss_fff"`) do set "STAMP=%%T"
if not defined STAMP (echo ERROR: Could not generate session timestamp & exit /b 2)
set "SESSION=%PROJECT%\test\87.115\session_%STAMP%"
if exist "%SESSION%" (echo ERROR: Session already exists & exit /b 2)
mkdir "%SESSION%" || exit /b 2
set "XM87102_KO_TRACE_DIR=%SESSION%"
set "XM8797_DEBUG_LOG_FILE=%SESSION%\xm8797_debug_perf.tsv"
set "XM8798_INPUT_TRACE_FILE=%SESSION%\xm8798_input_recognition.tsv"
set "XM8798_PIPELINE_TRACE_FILE=%SESSION%\xm8798_df_pipeline.tsv"
set "XM8799_GROUND_DEFENSE_TEST=1"
set "XM8799_GROUND_DEFENSE_LOG_FILE=%SESSION%\xm8799_dizzy_ground_defense.tsv"
set "XM8786_STORM_BUTTON_TEST=1"
set "XM8786_STORM_BUTTON_LOG_FILE=%SESSION%\xm8786_storm_button_guard.tsv"
set "XM8795_NATIVE_HIT_TEST=1"
set "XM8795_NATIVE_HIT_LOG_FILE=%SESSION%\xm8795_native_dizzy_air_collision.tsv"
set "XM8796_DIZZY_HANDOFF_TEST=1"
set "XM8796_DIZZY_HANDOFF_LOG_FILE=%SESSION%\xm8796_tournament_dizzy_handoff.tsv"
set "XM8761_REPLAY_MODE=off"
set "XM8761_DISABLE_RUNAHEAD=1"
set "XM8635_QSOUND_MODE=normal"
set "XM8632_STAGE5_OFF=1"
set "XM8648_NATIVE05_OFF="
set "XM8774_SELECTOR_CALIBRATE=1"
set "XM8766_SELECT_HOLD=1"
set "XM8659_FX_RULES_FILE=%STABLE%XMVSF_DoubleFeature_FX_Families.cfg"
set "XM8665_SCENERY_RULES_FILE=%STABLE%XMVSF_DoubleFeature_Scenery_Families.cfg"
set "GAMEROOT=%PROJECT%\FBNeo"
if not exist "%GAMEROOT%\roms\xmvsfu.zip" (
 if exist "F:\dev\FBNeo\roms\xmvsfu.zip" (set "GAMEROOT=F:\dev\FBNeo") else (echo ERROR: Missing XMVSFU ROM & exit /b 3)
)
echo XM87.115: supplemental flying-screen WAV DISABLED
echo XM87.115 Training damage: Regenerate / KO Enabled.
echo Training Dummy HP 1: Numpad Minus / Plus; 2: Numpad Divide / Multiply.
echo KO Enabled: KO landing returns to the saved match start.
echo Training match snapshot: %GAMEROOT%\config\games\xmvsfu_training_match_87102.fs
echo TRAINING RESTART: Start x3 center ^| Right+Start x3 right ^| Left+Start x3 left corner
echo KO investigation logs: %SESSION%
pushd "%GAMEROOT%" || exit /b 3
"%EXE%" 1>"%SESSION%\xm_stable_session.log" 2>&1
set "RC=%ERRORLEVEL%"
popd
echo Native game exit code: %RC%
echo Session: %SESSION%
pause
exit /b %RC%
