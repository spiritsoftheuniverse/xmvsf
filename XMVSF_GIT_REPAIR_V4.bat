@echo off
setlocal EnableExtensions DisableDelayedExpansion
REM XMVSF Git Repair V4. Put this file in F:\xmvsf and run it by name.
REM Uses the current directory INSIDE the repository rather than git -C.
REM Does not delete, reset, force, checkout, clean, rebuild or alter history.
REM Modes: fallback (default), repair (alias), status, pull.

set "REPO=F:\xmvsf"
REM Fixed, known checkout path. Never substitute the BAT folder or a trailing slash.
set "MODE=%~1"
if not defined MODE set "MODE=fallback"
set "GIT="
set "VALID=0"
for %%M in (fallback repair status pull) do if /I "%MODE%"=="%%M" set "VALID=1"

echo.
echo ================================================================
echo   XMVSF GIT REPAIR V4 - WORKING DIRECTORY CHECK
echo ================================================================
echo Script: %~nx0
echo Repo: "%REPO%"
echo Mode: %MODE%
echo.

if "%VALID%"=="0" (
    echo ERROR: Use fallback, repair, status, or pull.
    pause
    exit /b 2
)
if not exist "%REPO%\.git\HEAD" (
    echo ERROR: No .git\HEAD found at "%REPO%".
    echo Confirm the local checkout really is at F:\xmvsf.
    pause
    exit /b 2
)

REM Search for Git for Windows; never assume PATH has the same Git as Desktop.
if exist "%ProgramFiles%\Git\cmd\git.exe" set "GIT=%ProgramFiles%\Git\cmd\git.exe"
if not defined GIT if exist "%LocalAppData%\Programs\Git\cmd\git.exe" set "GIT=%LocalAppData%\Programs\Git\cmd\git.exe"
if not defined GIT for /f "delims=" %%G in ('where.exe git.exe 2^>nul') do if not defined GIT set "GIT=%%G"
if not defined GIT for /d %%D in ("%LocalAppData%\GitHubDesktop\app-*") do if exist "%%~fD\resources\app\git\cmd\git.exe" set "GIT=%%~fD\resources\app\git\cmd\git.exe"
if not defined GIT (
    echo ERROR: Could not locate git.exe.
    echo GitHub Desktop may have installed Git in a different location.
    pause
    exit /b 2
)

echo Git executable: "%GIT%"
"%GIT%" --version
if errorlevel 1 (
    echo ERROR: git.exe exists but could not run.
    pause
    exit /b 2
)

REM CRITICAL: avoid the original -C "F:\xmvsf\" argument entirely.
REM PUSH changes both drive and folder even when launched elsewhere.
pushd "%REPO%"
if errorlevel 1 (
    echo ERROR: Windows could not enter "%REPO%".
    echo Check that the F: drive is present and the folder is accessible.
    pause
    exit /b 2
)
echo Working directory: "%CD%"
echo Checking the actual repository with Git, errors shown below:
"%GIT%" rev-parse --show-toplevel
if errorlevel 1 (
    echo.
    echo ERROR: Git cannot open this checkout. Read the message above.
    echo It may be a Git safe.directory, permissions, or environment issue.
    echo No Git settings were changed.
    popd
    pause
    exit /b 2
)

if exist ".git\index.lock" (
    echo.
    echo ERROR: .git\index.lock is present.
    echo Close GitHub Desktop and any active Git process first.
    echo This script will not delete the lock.
    popd
    pause
    exit /b 2
)

REM Status-only mode intentionally changes no configuration.
if /I "%MODE%"=="status" (
    echo.
    echo Tracked status - no untracked directory walk:
    "%GIT%" status --short --branch -uno
    if errorlevel 1 (
        echo ERROR: Git status failed.
        popd
        pause
        exit /b 1
    )
    echo.
    echo Current local performance settings:
    "%GIT%" config --local --get core.fsmonitor
    "%GIT%" config --local --get core.untrackedCache
    echo.
    echo No configuration was changed.
    popd
    pause
    exit /b 0
)

REM Back up ONLY the Git config once, never the index or working files.
if exist ".git\config" if not exist ".git\xmvsf_git_before_v4.config" (
    copy /B ".git\config" ".git\xmvsf_git_before_v4.config" >nul
    if errorlevel 1 echo WARNING: Could not back up local Git config.
)

REM Fallback to reliable ordinary tracked-file scanning. No forced rescan.
echo.
echo Disabling potentially stale FSMonitor and untracked cache...
"%GIT%" config --local core.fsmonitor false
if errorlevel 1 (
    echo ERROR: Could not change core.fsmonitor. Git error is above.
    popd
    pause
    exit /b 1
)
"%GIT%" config --local core.untrackedCache false
if errorlevel 1 (
    echo ERROR: Could not change core.untrackedCache. Git error is above.
    popd
    pause
    exit /b 1
)
"%GIT%" config --local core.fscache true
if errorlevel 1 (
    echo ERROR: Could not enable core.fscache.
    popd
    pause
    exit /b 1
)
"%GIT%" config --local core.preloadIndex true
if errorlevel 1 (
    echo ERROR: Could not enable core.preloadIndex.
    popd
    pause
    exit /b 1
)
echo Applied reliable fallback settings to this repository only.

REM This avoids the costly update-index --refresh + full untracked scan.
echo.
echo Checking tracked status:
"%GIT%" status --short --branch -uno
if errorlevel 1 (
    echo ERROR: Status failed. Git error is above.
    popd
    pause
    exit /b 1
)

if /I "%MODE%"=="pull" (
    echo.
    echo Pulling current checked-out branch - fast-forward only.
    echo This does not switch branches or reset local files.
    "%GIT%" pull --ff-only --no-tags
    if errorlevel 1 (
        echo ERROR: Pull failed safely. Git error is above.
        echo No forced merge, reset, or cleanup was performed.
        popd
        pause
        exit /b 1
    )
    echo.
    echo Pull finished. Current tracked status:
    "%GIT%" status --short --branch -uno
)

echo.
echo ================================================================
echo  DONE - V4 verified Git from inside the checkout.
echo ================================================================
echo If GitHub Desktop is still slow after reopening it, the problem is
echo likely separate from the old BAT path/check failure.
echo.
popd
pause
exit /b 0
