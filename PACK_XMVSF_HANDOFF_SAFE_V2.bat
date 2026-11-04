@echo off
setlocal EnableExtensions DisableDelayedExpansion
REM XMVSF safe transfer handoff. Original archive is NEVER changed.
REM Put anywhere and double-click. Uses Python 3, already installed for this project.
set "XMVSF_HANDOFF_SCRIPT=%~f0"
where py >nul 2>nul
if not errorlevel 1 (
  py -3 -c "import os,pathlib; p=pathlib.Path(os.environ['XMVSF_HANDOFF_SCRIPT']); s=p.read_text(encoding='utf-8-sig'); exec(compile(s.rsplit('REM BEGIN_EMBEDDED_PYTHON
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import re
import stat
import sys
import uuid
import zipfile

# Source layout established from the user-provided 50,547-file archive inventory.
DEFAULT_ROOT = r"F:\xmvsf"
RELEASE = "87_115"
BASE_EXE = "runtime/stable/XMVSF_Stable.exe"
HANDOFF_INSTALLER = b'@echo off\nsetlocal EnableExtensions DisableDelayedExpansion\nrem Run from extracted handoff dev\\handoff directory. Preview unless --apply supplied.\nset "XMVSF_APPLY_SCRIPT=%~f0"\nwhere py >nul 2>nul\nif not errorlevel 1 (\n py -3 -c "import os,pathlib; p=pathlib.Path(os.environ[\'XMVSF_APPLY_SCRIPT\']); exec(compile(p.read_text(encoding=\'utf-8-sig\').rsplit(\'REM BEGIN_EMBEDDED_PYTHON\',1)[1].lstrip(),str(p),\'exec\'))" %*\n if errorlevel 1 exit /b 2\n exit /b 0\n)\nwhere python >nul 2>nul\nif not errorlevel 1 (\n python -c "import os,pathlib; p=pathlib.Path(os.environ[\'XMVSF_APPLY_SCRIPT\']); exec(compile(p.read_text(encoding=\'utf-8-sig\').rsplit(\'REM BEGIN_EMBEDDED_PYTHON\',1)[1].lstrip(),str(p),\'exec\'))" %*\n if errorlevel 1 exit /b 2\n exit /b 0\n)\necho ERROR: Python 3 missing.\nexit /b 3\nREM BEGIN_EMBEDDED_PYTHON\nfrom __future__ import annotations\nimport argparse, hashlib, json, os, shutil, sys, uuid\nfrom datetime import datetime\nfrom pathlib import Path\n\nPROTECTED = {\'runtime/stable/xmvsf_stable.exe\': \'runtime/stable/XMVSF_Stable.exe\', \'dev/stable/stable_baseline.json\': \'dev/stable/STABLE_BASELINE.json\'}\nclass Stop(Exception): pass\n\ndef hashfile(p):\n    h=hashlib.sha256()\n    with p.open(\'rb\') as f:\n        for b in iter(lambda:f.read(1024*1024),b\'\'):h.update(b)\n    return h.hexdigest()\n\ndef safe_rel(name):\n    if \'\\\\\' in name or \':\' in name or name.startswith(\'/\'):\n        raise Stop(\'Unsafe manifest path: \'+name)\n    p=Path(name)\n    if not name or any(x in (\'..\',\'\') for x in name.split(\'/\')):raise Stop(\'Unsafe manifest path: \'+name)\n    return p\n\ndef main():\n    ap=argparse.ArgumentParser(description=\'XMVSF safe layout enforcer\')\n    ap.add_argument(\'--apply\',action=\'store_true\',help=\'Apply verified changes; otherwise preview only\')\n    ap.add_argument(\'--target\',default=os.environ.get(\'XMVSF_ROOT\',r\'F:\\xmvsf\'))\n    args=ap.parse_args()\n    script=Path(os.environ[\'XMVSF_APPLY_SCRIPT\']).resolve()\n    root=script.parent.parent.parent\n    if script.parent.name.lower()!=\'handoff\' or script.parent.parent.name.lower()!=\'dev\':raise Stop(\'Installer must be in dev/handoff under extracted handoff root\')\n    manifest_path=root/\'HANDOFF_MANIFEST_SHA256.json\'\n    if not manifest_path.is_file():raise Stop(\'Extract complete handoff first; missing SHA256 manifest\')\n    manifest=json.loads(manifest_path.read_text(encoding=\'utf-8-sig\'))\n    files=manifest.get(\'files\')\n    if not isinstance(files,dict) or not files:raise Stop(\'Invalid or empty handoff manifest\')\n    target=Path(args.target).resolve()\n    if target==root or target in root.parents or root in target.parents:raise Stop(\'Source and target must be separate directories\')\n    if target.name.lower() in (\'archive\',\'_archive\') or \'archive\' in [x.lower() for x in target.parts]:raise Stop(\'Refusing to update an archive directory\')\n    if not target.is_dir():raise Stop(\'Target does not exist: \'+str(target))\n    if not (target/\'dev\'/\'stable\').is_dir():raise Stop(\'Target lacks existing dev/stable; refusing to create an assumed project\')\n    lock=target/\'dev\'/\'stable\'/\'experiments\'/\'87_115\'/\'.build.lock\'\n    if lock.exists():raise Stop(\'Build lock exists; stop active build and inspect the lock before applying: \'+str(lock))\n    plan=[]; skip=[]\n    seen=set()\n    for name,meta in sorted(files.items()):\n        rel=safe_rel(name)\n        if name.casefold() in seen:raise Stop(\'Case-colliding manifest path: \'+name)\n        seen.add(name.casefold())\n        low=name.lower()\n        if low in PROTECTED:\n            skip.append(name)\n            continue\n        if low.endswith(\'.exe\') or low.endswith(\'.dll\') and low.startswith(\'dev/\'):\n            raise Stop(\'Unexpected executable or development DLL: \'+name)\n        src=root/rel; dst=target/rel\n        if src.is_symlink() or not src.is_file() or not isinstance(meta,dict):raise Stop(\'Missing or invalid handoff file: \'+name)\n        if src.stat().st_size!=meta.get(\'size\') or hashfile(src)!=meta.get(\'sha256\'):\n            raise Stop(\'Handoff SHA256 mismatch: \'+name)\n        if dst.is_symlink():raise Stop(\'Refusing to replace symlink: \'+str(dst))\n        if dst.exists() and not dst.is_file():raise Stop(\'Target is not a file: \'+str(dst))\n        if dst.is_file() and hashfile(dst)==meta[\'sha256\']:continue\n        plan.append((src,dst,rel,dst.is_file()))\n    # Verify same protected baseline before applying changes to an established install.\n    for name,original_name in sorted(PROTECTED.items()):\n        src=root/Path(original_name);dst=target/Path(original_name)\n        if not src.is_file() or not dst.is_file():raise Stop(\'Missing protected baseline: \'+name)\n        if hashfile(src)!=hashfile(dst):raise Stop(\'Protected baseline differs from package; refusing to merge: \'+name)\n    print(\'[SOURCE]\',root)\n    print(\'[TARGET]\',target)\n    print(\'[SHA256] All %d payload files verified; %d protected paths skipped\'%(len(files)-len(skip),len(skip)))\n    print(\'[PLAN]\',len(plan),\'files to create or update\')\n    for src,dst,rel,exists in plan[:50]:print(\' \', \'UPDATE\' if exists else \'CREATE\',rel.as_posix())\n    if len(plan)>50:print(\'  ... and\',len(plan)-50,\'more\')\n    if not args.apply:\n        print(\'PREVIEW ONLY. Nothing changed. Run with --apply to install verified files.\')\n        return\n    stamp=datetime.now().strftime(\'%Y%m%d_%H%M%S\')+\'_\'+uuid.uuid4().hex[:8]\n    backup=target/\'testdata\'/\'handoff_backups\'/stamp\n    if not plan:print(\'Already up to date.\');return\n    backup.mkdir(parents=True,exist_ok=False)\n    created=[];replaced=[]\n    try:\n        # Save originals BEFORE any replacement.\n        for src,dst,rel,exists in plan:\n            if exists:\n                b=backup/rel;b.parent.mkdir(parents=True,exist_ok=True)\n                shutil.copy2(dst,b)\n                if hashfile(b)!=hashfile(dst):raise Stop(\'Backup verification failed: \'+str(dst))\n        for src,dst,rel,exists in plan:\n            dst.parent.mkdir(parents=True,exist_ok=True)\n            tmp=dst.with_name(dst.name+\'.handoff_pending_\'+uuid.uuid4().hex)\n            try:\n                shutil.copy2(src,tmp)\n                if hashfile(tmp)!=hashfile(src):raise Stop(\'Staged SHA256 mismatch: \'+str(dst))\n                os.replace(tmp,dst)\n            finally:\n                if tmp.exists():tmp.unlink()\n            (replaced if exists else created).append((dst,rel))\n        (backup/\'INSTALL_AUDIT.json\').write_text(json.dumps({\'source\':str(root),\'target\':str(target),\'updated\':[x[1].as_posix() for x in replaced],\'created\':[x[1].as_posix() for x in created]},indent=2),encoding=\'utf-8\')\n    except BaseException:\n        print(\'INSTALL FAILED. Attempting rollback. Backups at:\',backup,file=sys.stderr)\n        for dst,rel in reversed(created):\n            if dst.is_file():dst.unlink()\n        for dst,rel in reversed(replaced):\n            b=backup/rel\n            if b.is_file():shutil.copy2(b,dst)\n        raise\n    print(\'SUCCESS: Applied\',len(plan),\'changes. Replaced originals backed up at:\',backup)\n    print(\'Stable baseline, original archive, and build lock unchanged. No build was started.\')\n\nif __name__==\'__main__\':\n    try:main()\n    except (Stop,OSError,ValueError,KeyError) as e:\n        print(\'HANDOFF STOPPED:\',e,file=sys.stderr);sys.exit(2)\n'

class PreflightError(Exception):
    pass

def must_file(p: Path, why: str) -> Path:
    if not p.is_file():
        raise PreflightError("MISSING " + why + ": " + str(p))
    return p

def digest(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()

def read_sha256_manifest(baseline: Path, tree: Path, stable_exe: Path):
    try:
        info = json.loads(baseline.read_text(encoding="utf-8-sig"))
        expected_exe = info["build_executable_sha256"]
        src_checks = info["source_sha256"]
    except (ValueError, KeyError, TypeError) as exc:
        raise PreflightError("Bad stable baseline manifest: " + str(exc))
    if info.get("version") not in ("87.86", "87.96") or info.get("compile_verified") is not True:
        raise PreflightError("Stable baseline is not the protected 87.86/87.96 required by 87.115")
    if not isinstance(src_checks, dict) or len(src_checks) < 55:
        raise PreflightError("Incomplete protected stable source hash manifest")
    print("[PRECHECK] Verifying protected stable executable SHA-256...")
    if digest(stable_exe).lower() != expected_exe.lower():
        raise PreflightError("Protected Stable.exe does not match STABLE_BASELINE.json; no package created")
    print("[PRECHECK] Verifying protected stable source SHA-256 (%s files)..." % len(src_checks))
    for relative, expected in src_checks.items():
        rel = Path(relative.replace("\\", "/"))
        if rel.is_absolute() or ".." in rel.parts:
            raise PreflightError("Unsafe baseline source path: " + relative)
        item = must_file(tree / "src" / rel, "protected source")
        if digest(item).lower() != expected.lower():
            raise PreflightError("Protected stable source checksum mismatch: " + relative)
    return info

def excluded(p: Path, rel: Path, group: str) -> bool:
    s = p.suffix.lower()
    parts = [x.lower() for x in rel.parts]
    if p.is_symlink():
        raise PreflightError("Refusing symbolic link in handoff: " + str(p))
    if "__pycache__" in parts or ".git" in parts:
        return True
    if group == "source":
        return s in (".pyc", ".log", ".tmp", ".bak")
    if group == "cache":
        return s in (".log", ".tmp", ".bak", ".dmp")
    if group == "releases":
        return s in (".pyc", ".log", ".tmp", ".bak", ".zip", ".7z", ".exe", ".dmp")
    if group == "assets":
        return s in (".tmp", ".bak", ".log", ".exe", ".zip")
    raise PreflightError("Unknown collector group: " + group)

def build_generated_launchers():
    # Self-contained verified release launchers.
    build = '@echo off\nsetlocal EnableExtensions DisableDelayedExpansion\nrem XMVSF minimal handoff -- guarded 87.115 build; run from project root.\nfor %%I in ("%~dp0.") do set "PROJECT=%%~fI"\ncd /d "%PROJECT%" || goto :failed\nset "TREE=%PROJECT%\\dev\\stable\\buildtree"\nset "RELEASE=%PROJECT%\\dev\\stable\\releases\\87_115"\nset "STABLE=%PROJECT%\\runtime\\stable"\nset "PATH=C:\\msys64\\ucrt64\\bin;C:\\msys64\\usr\\bin;%PATH%"\nset "MSYSTEM=UCRT64"\nset "XMVSF_PROJECT=%PROJECT%"\nif not defined XMVSF_BUILD_JOBS set "XMVSF_BUILD_JOBS=12"\necho XMVSF verified 87.115 handoff builder\nif not exist "%TREE%\\src\\burn\\drv\\capcom\\cps_obj.cpp" (\n echo ERROR: Active source tree is missing at "%TREE%".\n if exist "%PROJECT%\\archive\\dev\\stable\\buildtree" echo Archive found. Do not build inside the protected archive; extract the handoff into a separate working root first.\n goto :failed\n)\nif not exist "%TREE%\\makefile.sdl2" (echo ERROR: Missing makefile.sdl2 & goto :failed)\nif not exist "%TREE%\\makefile.burn_rules" (echo ERROR: Missing makefile.burn_rules & goto :failed)\nif not exist "%TREE%\\obj" (echo ERROR: Compiled object cache directory missing. Refusing a cold build. & goto :failed)\nif not exist "%PROJECT%\\dev\\stable\\STABLE_BASELINE.json" (echo ERROR: Missing protected stable manifest & goto :failed)\nif not exist "%STABLE%\\XMVSF_Stable.exe" (echo ERROR: Missing protected stable EXE needed for build verification & goto :failed)\nif not exist "%RELEASE%\\build_87115.py" (echo ERROR: Missing original 87.115 builder & goto :failed)\nif not exist "%PROJECT%\\dev\\stable\\releases\\87_109\\build_87109.py" (echo ERROR: Missing 87.109 dependency builder & goto :failed)\nif not exist "%PROJECT%\\dev\\stable\\releases\\87_114\\build_87114.py" (echo ERROR: Missing 87.114 dependency builder & goto :failed)\nwhere g++ >nul 2>&1 || (echo ERROR: MSYS2 UCRT64 g++ not found & goto :failed)\nwhere make >nul 2>&1 || (echo ERROR: MSYS2 make not found & goto :failed)\nset "OBJECTS=0"\nfor /r "%TREE%\\obj" %%F in (*.o) do set /a OBJECTS+=1 >nul\necho Existing compiled object files: %OBJECTS%\nif %OBJECTS% LSS 1000 (echo ERROR: Incomplete warm object cache. Refusing a surprise cold build. & goto :failed)\nif exist "%PROJECT%\\dev\\stable\\experiments\\87_115\\.build.lock" (echo ERROR: Build lock exists - verify no build is running, then rename stale lock manually. & goto :failed)\necho Building 87.115 using the original guarded release builder, without touching archive.\nwhere py >nul 2>&1\nif not errorlevel 1 (\n py -3 "%RELEASE%\\build_87115.py"\n) else (\n where python >nul 2>&1 || (echo ERROR: Python 3 not found & goto :failed)\n python "%RELEASE%\\build_87115.py"\n)\nif errorlevel 1 goto :failed\nif not exist "%STABLE%\\XMVSF_Test_87_115.exe" (echo ERROR: Builder returned success but no 87.115 executable was published & goto :failed)\nif not exist "%STABLE%\\XMVSF_Test_87_115.json" (echo ERROR: Missing verified publication manifest & goto :failed)\necho.\necho SUCCESS: "%STABLE%\\XMVSF_Test_87_115.exe"\necho Use RUN_TEST.bat to start this build.\npause\nexit /b 0\n:failed\necho.\necho BUILD STOPPED. Protected archive and stable executable were not modified by this launcher.\npause\nexit /b 1\n'
    run = '@echo off\nsetlocal EnableExtensions DisableDelayedExpansion\nrem XMVSF minimal handoff -- run the verified 87.115 test build.\nfor %%I in ("%~dp0.") do set "PROJECT=%%~fI"\nset "STABLE=%PROJECT%\\runtime\\stable"\nset "EXE=%STABLE%\\XMVSF_Test_87_115.exe"\nset "PATH=C:\\msys64\\ucrt64\\bin;%STABLE%;%PROJECT%\\runtime\\stage79;%PATH%"\nif not exist "%EXE%" (\n echo ERROR: XMVSF_Test_87_115.exe is not present.\n echo Minimal handoffs exclude test EXEs by design. Run BUILD_TEST.bat first.\n goto :failed\n)\nif not exist "%STABLE%\\XMVSF_Test_87_115.json" (echo ERROR: Missing 87.115 test publication manifest & goto :failed)\npowershell -NoProfile -Command "$m=Get-Content -LiteralPath ($env:STABLE + \'\\XMVSF_Test_87_115.json\') -Raw | ConvertFrom-Json; $h=(Get-FileHash -LiteralPath $env:EXE -Algorithm SHA256).Hash; if ($m.version -ne \'87.115\' -or $h -ne $m.test_executable_sha256) { Write-Error \'87.115 executable/manifest mismatch\'; exit 1 }"\nif errorlevel 1 (echo ERROR: The test executable does not match its publication manifest & goto :failed)\nset "GAMEROOT=%PROJECT%\\FBNeo"\nif not exist "%GAMEROOT%\\roms\\xmvsfu.zip" (\n if exist "F:\\dev\\FBNeo\\roms\\xmvsfu.zip" (\n  set "GAMEROOT=F:\\dev\\FBNeo"\n ) else (\n  echo ERROR: xmvsfu.zip missing from "%GAMEROOT%\\roms" and F:\\dev\\FBNeo\\roms.\n  goto :failed\n )\n)\nif not exist "%GAMEROOT%\\config\\games\\xmvsfu_training_match_87102.fs" (\n echo ERROR: Training match state missing from "%GAMEROOT%\\config\\games".\n goto :failed\n)\nfor /f "usebackq delims=" %%T in (`powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss_fff"`) do set "STAMP=%%T"\nif not defined STAMP (echo ERROR: Could not generate session timestamp & goto :failed)\nset "SESSION=%PROJECT%\\test\\87.115\\session_%STAMP%"\nif exist "%SESSION%" (echo ERROR: Session directory already exists & goto :failed)\nmkdir "%SESSION%" || goto :failed\nset "XM87102_KO_TRACE_DIR=%SESSION%"\nset "XM8797_DEBUG_LOG_FILE=%SESSION%\\xm8797_debug_perf.tsv"\nset "XM8798_INPUT_TRACE_FILE=%SESSION%\\xm8798_input_recognition.tsv"\nset "XM8798_PIPELINE_TRACE_FILE=%SESSION%\\xm8798_df_pipeline.tsv"\nset "XM8799_GROUND_DEFENSE_TEST=1"\nset "XM8799_GROUND_DEFENSE_LOG_FILE=%SESSION%\\xm8799_dizzy_ground_defense.tsv"\nset "XM8786_STORM_BUTTON_TEST=1"\nset "XM8786_STORM_BUTTON_LOG_FILE=%SESSION%\\xm8786_storm_button_guard.tsv"\nset "XM8795_NATIVE_HIT_TEST=1"\nset "XM8795_NATIVE_HIT_LOG_FILE=%SESSION%\\xm8795_native_dizzy_air_collision.tsv"\nset "XM8796_DIZZY_HANDOFF_TEST=1"\nset "XM8796_DIZZY_HANDOFF_LOG_FILE=%SESSION%\\xm8796_tournament_dizzy_handoff.tsv"\nset "XM8761_REPLAY_MODE=off"\nset "XM8761_DISABLE_RUNAHEAD=1"\nset "XM8635_QSOUND_MODE=normal"\nset "XM8632_STAGE5_OFF=1"\nset "XM8648_NATIVE05_OFF="\nset "XM8774_SELECTOR_CALIBRATE=1"\nset "XM8766_SELECT_HOLD=1"\nset "XM8659_FX_RULES_FILE=%STABLE%\\XMVSF_DoubleFeature_FX_Families.cfg"\nset "XM8665_SCENERY_RULES_FILE=%STABLE%\\XMVSF_DoubleFeature_Scenery_Families.cfg"\necho XMVSF 87.115: starting "%EXE%"\necho Session log: "%SESSION%\\xm_stable_session.log"\npushd "%GAMEROOT%" || goto :failed\n"%EXE%" 1>"%SESSION%\\xm_stable_session.log" 2>&1\nset "RC=%ERRORLEVEL%"\npopd\necho Native game exit code: %RC%\necho Session directory: "%SESSION%"\npause\nexit /b %RC%\n:failed\necho.\necho RUN STOPPED. No game executable or archive file was modified.\npause\nexit /b 2\n'
    return {"BUILD_TEST.bat": build.replace("\n", "\r\n").encode("ascii"),
            "RUN_TEST.bat": run.replace("\n", "\r\n").encode("ascii")}

class Collector:
    def __init__(self):
        self.files = {}  # ZIP-name -> (local source Path, bytes or None, provenance)
        self.count = {}

    def add(self, target: str, src: Path = None, data: bytes = None, provenance: str = "archive"):
        target = target.replace("\\", "/").lstrip("/")
        if not target or ".." in target.split("/"):
            raise PreflightError("Unsafe handoff path: " + target)
        if target in self.files:
            raise PreflightError("Duplicate handoff path: " + target)
        if src is not None:
            must_file(src, "handoff component")
            if src.is_symlink():
                raise PreflightError("Refusing symlink " + str(src))
        if target.lower().endswith(".exe") and target not in (
            BASE_EXE, "dev/stable/buildtree/obj/cpu/m68k/m68kmake.exe"):
            raise PreflightError("An unapproved EXE was selected: " + target)
        self.files[target] = (src, data, provenance)
        self.count[provenance] = self.count.get(provenance, 0) + 1

    def add_tree(self, root: Path, target: str, group: str, required: bool = True, provenance="archive"):
        if not root.is_dir():
            if required:
                raise PreflightError("MISSING %s directory: %s" % (group, root))
            return 0
        n = 0
        for src in sorted(root.rglob("*")):
            if not src.is_file():
                continue
            rel = src.relative_to(root)
            if excluded(src, rel, group):
                continue
            self.add(target.rstrip("/") + "/" + rel.as_posix(), src=src,
                     provenance=provenance)
            n += 1
        if required and n == 0:
            raise PreflightError("Empty required %s directory: %s" % (group, root))
        return n

    def from_first(self, target, sources, role, required=True):
        for src, provenance in sources:
            if src.is_file():
                self.add(target, src=src, provenance=provenance)
                return src
        if required:
            raise PreflightError("MISSING " + role + " (checked: " + ", ".join(str(p) for p, _ in sources) + ")")
        return None

def build_collection(root: Path, source: Path, strict_roms: bool):
    tree = source / "dev" / "stable" / "buildtree"
    stable = source / "runtime" / "stable"
    must_file(tree / "src" / "burn" / "drv" / "capcom" / "cps_obj.cpp", "CPS object source")
    must_file(tree / "makefile.sdl2", "SDL2 build makefile")
    must_file(tree / "makefile.burn_rules", "burn rules makefile")
    must_file(source / "dev" / "stable" / "releases" / RELEASE / "build_87115.py", "87.115 builder")
    must_file(source / "dev" / "stable" / "releases" / RELEASE / "test_87115.cpp", "87.115 test source")
    baseline = must_file(source / "dev" / "stable" / "STABLE_BASELINE.json", "protected stable baseline")
    stable_exe = must_file(stable / "XMVSF_Stable.exe", "protected Stable executable")
    read_sha256_manifest(baseline, tree, stable_exe)

    c = Collector()
    print("[COLLECT] Current stable FBNeo source + makefiles + one compiled object cache...")
    c.add_tree(tree / "src", "dev/stable/buildtree/src", "source")
    c.add_tree(tree / "obj", "dev/stable/buildtree/obj", "cache")
    for mf in sorted(tree.glob("makefile*")):
        if mf.is_file() and ".before_" not in mf.name and not mf.name.endswith(".stage20_before_native"):
            c.add("dev/stable/buildtree/" + mf.name, src=mf)
    for fn in ("BUILD_STABLE.sh", "VERIFY_STABLE.sh", "PROMOTE_STAGE87_STABLE.sh", "STABLE_BASELINE.json", "README_STAGE87.txt"):
        f = source / "dev" / "stable" / fn
        if f.is_file(): c.add("dev/stable/" + fn, src=f)
    print("[COLLECT] Stage 87 release dependency chain (including nested dependencies and tests)...")
    c.add_tree(source / "dev" / "stable" / "releases", "dev/stable/releases", "releases")

    print("[COLLECT] Stable executable (the protected baseline), game assets, configuration and DLLs...")
    c.add(BASE_EXE, src=stable_exe)
    c.add_tree(stable / "assets", "runtime/stable/assets", "assets")
    for f in sorted(stable.iterdir()):
        if f.is_file() and f.suffix.lower() in (".cfg", ".ini", ".dll"):
            c.add("runtime/stable/" + f.name, src=f)
    if (source / "runtime" / "config").is_dir():
        c.add_tree(source / "runtime" / "config", "runtime/config", "assets")
    stage79 = source / "runtime" / "stage79"
    for fn in ("SDL2.dll", "SDL2_image.dll", "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"):
        c.from_first("runtime/stage79/" + fn,
                     [(stage79 / fn, "archive"), (stable / fn, "archive"),
                      (Path(r"C:\msys64\ucrt64\bin") / fn, "MSYS2 UCRT64")],
                     "runtime DLL " + fn, required=(fn != "SDL2_image.dll"))

    # A playable project needs its actual ROMs. These were NOT in the uploaded archive inventory.
    # Never mislabel source-only as complete or generate dummy ROMs.
    game_roots = [source / "FBNeo", root / "FBNeo", Path(os.environ.get("XMVSF_GAME_ROOT", r"F:\dev\FBNeo"))]
    game_roots = [(p, "archive" if p == source / "FBNeo" else "live-game") for p in game_roots]
    rom_sources = [(g / "roms" / "xmvsfu.zip", label) for g, label in game_roots]
    required_rom = c.from_first("FBNeo/roms/xmvsfu.zip", rom_sources, "xmvsfu.zip gameplay ROM", strict_roms)
    for opt in ("mvscur1.zip", "xmvsf.zip", "mvsc.zip"):
        sources = [(g / "roms" / opt, label) for g, label in game_roots]
        c.from_first("FBNeo/roms/" + opt, sources, opt, required=False)

    for fn in ("fbneo.ini", "xmvsf_controller_map.ini"):
        c.from_first("FBNeo/" + fn,
                     [(g / fn, label) for g, label in game_roots] + [(stable / fn, "archive")],
                     fn, required=False)
    for fn in ("SDL2.dll", "SDL2_image.dll"):
        c.from_first("FBNeo/" + fn,
                     [(g / fn, label) for g, label in game_roots] + [(stable / fn, "archive")],
                     fn, required=False)
    for fn in ("xmvsfMenu.wav", "menu_movement1.wav", "menu_selection1.wav", "flyingscreen.wav"):
        c.from_first("FBNeo/assets/xmvsf/" + fn,
                     [(g / "assets" / "xmvsf" / fn, label) for g, label in game_roots] +
                     [(stable / "assets" / "xmvsf" / fn, "archive")],
                     "menu audio: " + fn, required=(fn != "flyingscreen.wav"))
    # Save states/config must be located under FBNeo, the runtime working directory.
    seen_cfg = set()
    all_cfgs = []
    for g, label in game_roots:
        if (g / "config" / "games").is_dir():
            all_cfgs.append((g / "config" / "games", label))
    all_cfgs.append((source / "runtime" / "config" / "games", "archive"))
    for config_root, label in all_cfgs:
        if not config_root.is_dir(): continue
        for f in sorted(config_root.rglob("*")):
            if not f.is_file() or f.is_symlink():continue
            if f.suffix.lower() not in (".fs", ".ini", ".nv"):
                continue
            rel = f.relative_to(config_root).as_posix()
            if rel.lower() not in seen_cfg:
                c.add("FBNeo/config/games/" + rel, src=f, provenance=label)
                seen_cfg.add(rel.lower())
    if strict_roms:
        for fn in ("xmvsfu_training_select.fs", "xmvsfu_training_match_87102.fs"):
            if "FBNeo/config/games/" + fn not in c.files:
                raise PreflightError("MISSING required training state " + fn +
                                     "; cannot claim complete runtime handoff")

    c.add("dev/handoff/APPLY_HANDOFF.bat", data=HANDOFF_INSTALLER, provenance="generated handoff installer")
    for target, data in build_generated_launchers().items():
        c.add(target, data=data, provenance="generated handoff launcher")
    # Do NOT include test JSON, test EXEs, or existing test launchers: the guarded
    # 87.115 builder requires all four publication artifacts to be absent or complete.
    note = ("XMVSF MINIMAL VERIFIED DEVELOPMENT HANDOFF\r\n"
            "Source archive: F:\\xmvsf\\archive\r\n"
            "Target: Stage 87.115 (the latest found in the supplied archive inventory).\r\n"
            "Build from source with BUILD_TEST.bat; run with RUN_TEST.bat.\r\n"
            "Contains protected Stable.exe ONLY because build_87115.py verifies its SHA-256.\r\n"
            "Contains all stable FBNeo source, warm obj and .d cache, release dependency\r\n"
            "chain, stable assets, DLLs, config and gameplay ROM(s) when available.\r\n"
            "Does NOT contain ANY XMVSF_Test_*.exe or test-publication artifacts.\r\n"
            "No original archive or stable executable has been edited, overwritten\r\n"
            "or deleted. Prior histories/captures/duplicate buildtrees remain in the\r\n"
            "original archive, not in this small handoff.\r\n"
            "Existing binaries newer than Stage 87.115 are NOT included.\r\n"
            "See HANDOFF_MANIFEST_SHA256.json for each included file and its hash.\r\n"
            + ("ROM dependency provided; runtime intended to be self-contained.\r\n"
               if required_rom else "WARNING: Source-only mode: gameplay ROM absent!\r\n"))
    c.add("HANDOFF_README.txt", data=note.encode("ascii"), provenance="generated handoff metadata")
    if not c.files: raise PreflightError("No files selected")
    return c

def create_zip(c: Collector, out: Path, dry_run: bool):
    print("[SELECTED] %s files total (before manifest)" % len(c.files))
    for grp, n in sorted(c.count.items()): print("   %s: %s" % (grp, n))
    print("[OUTPUT] " + str(out))
    if dry_run:
        print("DRY RUN ONLY. No ZIP created and no original file modified.")
        return
    if out.exists():
        raise PreflightError("Refusing to overwrite existing handoff: " + str(out))
    out.parent.mkdir(parents=True, exist_ok=True)
    pending = out.with_name(out.name + ".partial." + uuid.uuid4().hex)
    manifest = {"title": "XMVSF minimal source development handoff",
                "created_utc": datetime.now(timezone.utc).isoformat(),
                "target_release": "87.115",
                "protected_stable": BASE_EXE,
                "includes_test_executables": False,
                "rebuild_note": "87.115 builder requires protected Stable.exe; do not strip it",
                "files": {}}
    try:
        with zipfile.ZipFile(pending, "w", compression=zipfile.ZIP_DEFLATED,
                             compresslevel=5, allowZip64=True) as z:
            for i, (name, (src, data, provenance)) in enumerate(sorted(c.files.items()), 1):
                if name.lower().endswith(".exe") and name not in (
                    BASE_EXE, "dev/stable/buildtree/obj/cpu/m68k/m68kmake.exe"):
                    raise PreflightError("Unexpected executable: " + name)
                if src is not None:
                    before = src.stat()
                    h = hashlib.sha256()
                    zi = zipfile.ZipInfo.from_file(src, arcname=name)
                    zi.compress_type = zipfile.ZIP_DEFLATED
                    zi._compresslevel = 5
                    with src.open("rb") as inp, z.open(zi, "w", force_zip64=True) as dst:
                        while True:
                            chunk = inp.read(1024 * 1024)
                            if not chunk: break
                            dst.write(chunk)
                            h.update(chunk)
                    after = src.stat()
                    if before.st_size != after.st_size or before.st_mtime_ns != after.st_mtime_ns:
                        raise PreflightError("SOURCE CHANGED DURING PACKAGING: " + str(src))
                    size = before.st_size
                    checksum = h.hexdigest()
                else:
                    data = data if data is not None else b""
                    z.writestr(name, data, compress_type=zipfile.ZIP_DEFLATED)
                    size = len(data)
                    checksum = hashlib.sha256(data).hexdigest()
                manifest["files"][name] = {"size": size, "sha256": checksum,
                                           "origin": provenance}
                if i % 500 == 0:
                    print("[PACK] %s / %s files" % (i, len(c.files)), flush=True)
            z.writestr("HANDOFF_MANIFEST_SHA256.json",
                       json.dumps(manifest, indent=2, sort_keys=True).encode("utf-8") + b"\n")
        print("[VERIFY] Reading every ZIP entry back and comparing SHA-256...")
        with zipfile.ZipFile(pending, "r") as z:
            names = z.namelist()
            if len(names) != len(c.files) + 1 or len(names) != len(set(names)):
                raise PreflightError("ZIP file-count or duplicate-name failure")
            if set(names) != set(c.files) | {"HANDOFF_MANIFEST_SHA256.json"}:
                raise PreflightError("ZIP inventory mismatch")
            check = json.loads(z.read("HANDOFF_MANIFEST_SHA256.json"))
            if check != manifest:
                raise PreflightError("ZIP manifest differs from the source inventory")
            for j, (name, meta) in enumerate(manifest["files"].items(), 1):
                h = hashlib.sha256()
                sz = 0
                with z.open(name) as f:
                    for b in iter(lambda: f.read(1024 * 1024), b""):
                        h.update(b); sz += len(b)
                if sz != meta["size"] or h.hexdigest() != meta["sha256"]:
                    raise PreflightError("ZIP content corrupt or mismatched: " + name)
                if j % 500 == 0:
                    print("[VERIFY] %s / %s files" % (j, len(c.files)), flush=True)
        if out.exists():
            raise PreflightError("Output appeared during verification; refusing overwrite")
        pending.rename(out)
        print("\nSUCCESS. VERIFIED HANDOFF: " + str(out))
        print("Entries: " + str(len(c.files) + 1))
        print("ZIP bytes: " + str(out.stat().st_size))
        print("Original F:\\xmvsf\\archive and protected Stable.exe: UNTOUCHED")
    except BaseException:
        if pending.exists():
            print("[SAFE STOP] Partial ZIP retained for diagnosis: " + str(pending))
        raise

def main():
    parser = argparse.ArgumentParser(description="Safe, non-destructive XMVSF minimal handoff generator")
    parser.add_argument("--dry-run", action="store_true", help="Inspect selections without making a ZIP")
    parser.add_argument("--source-only", action="store_true", help="Allow no ROM / training state (NOT playable)")
    args = parser.parse_args()
    root = Path(os.environ.get("XMVSF_ROOT", DEFAULT_ROOT))
    source = Path(os.environ.get("XMVSF_ARCHIVE_DIR", str(root / "archive")))
    output_dir = Path(os.environ.get("XMVSF_HANDOFF_OUTPUT_DIR", str(root)))
    if not source.is_dir():
        raise PreflightError("Archive not found: " + str(source))
    if output_dir.resolve() == source.resolve() or source.resolve() in output_dir.resolve().parents:
        raise PreflightError("Output must be OUTSIDE the original archive")
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    output = output_dir / ("XMVSF_87_115_MINIMAL_HANDOFF_" + stamp + ".zip")
    print("[SOURCE] " + str(source))
    print("[MODE] " + ("SOURCE ONLY / ROM OPTIONAL" if args.source_only else "COMPLETE RUNTIME DEPENDENCIES REQUIRED"))
    c = build_collection(root, source, strict_roms=not args.source_only)
    create_zip(c, output, args.dry_run)

if __name__ == "__main__":
    try:
        main()
    except (PreflightError, OSError, ValueError, zipfile.BadZipFile) as exc:
        print("\nPACKAGING STOPPED SAFELY: " + str(exc), file=sys.stderr)
        print("No original XMVSF project files were modified or deleted.", file=sys.stderr)
        sys.exit(2)
