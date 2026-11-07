# XMVSF local TEST / Asset Organizer / Level Capture suite

This is the consolidated **native Windows** entry point. GitHub is source control
only: builds, playback, recording, extraction, and verification occur on the
local Windows machine. Never use GitHub Actions for this project.

## Main launcher: RUN_TEST.bat

Run `RUN_TEST.bat` with no arguments. It displays:

| Option | Operation | Source and output |
| --- | --- | --- |
| **1 - Test game** | Launch the current native XMVSF TEST executable (xmvsfu), not Stable. | `runtime/stable/XMVSF_Test_87_203.exe` |
| **2 - Asset Organizer** | Call `asset_organizer/BUILD_AO.bat`, then start the localhost editor. | `asset_organizer/index.html` and `asset_organizer/aodata.json` |
| **3 - Capture Level** | Show a category list; MVC1 (`mvscur1`) is the first and only donor. | `test/87.203/stage_capture/session_XXXX` |

The direct shortcuts `RUN_TEST.bat 1`, `RUN_TEST.bat 2`,
and `RUN_TEST.bat 3` are also supported. Advanced existing commands
remain available: `RUN_TEST.bat art`, `art build`, `art open`,
`sprites`, `channels`, `m68k`, `poses`, `review` and direct ROM
arguments. The main launcher never silently substitutes Stable if TEST is absent.

## Native build: BUILD_TEST.bat

`BUILD_TEST.bat` invokes the existing `dev/dhc/BUILD_TEST.sh` with MSYS2
UCRT64, performs the native C++ tests, then calls incremental GNU Make from
`dev/stable/buildtree`. Changed C/C++ files recompile with dependency-aware
cache reuse; the linked executable is atomically published to **TEST only**.
The permanent build transcript is `test/build_logs/BUILD_TEST.log`.
A missing final TEST executable is an error, not a successful build.
The Stable executable and previous captures are never overwritten.

## Asset Organizer builder

`asset_organizer/BUILD_AO.bat` recompiles the standalone offline HTML/metadata
art workbench from existing original captures. It **does not** compile or start
FBNeo, start the localhost server, or change the saved `aodata.json`.
To build and **open** the editor, use launcher option 2. The local server
uses `127.0.0.1:18720`, saves reviewed metadata atomically to
`asset_organizer/aodata.json`, and regenerates derived previews.

The full workbench tracks Wanted, types, color assignments and raw ROM offsets.
Source-family reconstruction is seed-based: unobserved adjacent records are
suggested but never silently approved, and rejected records remain rejected.
Separate SCROLL1 extraction supplies original near/foreground scenery such as
Monster Rock Bar tables and chairs; SCROLL2 alone is incomplete.

The following generated files stay local / Git-ignored, as they may contain
original copyrighted game art: `index.html`, `aodata.json`,
`animations.html`, `source_families/`, and `foreground_recovered/`.

## F9 / F10 / F11 hotkey contract

| Hotkey | Behavior |
| --- | --- |
| F9 | Existing host-aware state/input recording on/off (Shift+F9 quick-save). |
| F10 | Existing native diagnostic probe on/off (Shift+F10 quick-load). |
| **F11** | MVC1-only original CPS2 level-capture on/off while using option 3. |
| Shift+F11 | Previous FPS display toggle (moved off unmodified F11). |

F11 is handled at the native SDL keyboard-event boundary, before menus can
consume the event. There is **no automatic native stage capture on launch**:
the MVC1 donor enters with level capture **off**, and the first F11 press
arms the read-only hardware recorder. The next press stops and seals the
session. Every subsequent ON creates a fresh `session_XXXX`, without
overwriting old frames. The recorder writes original CPS2 register, map,
palette, OAM and decoded graphics evidence. The stage identity and precise
OBJ-to-scroll native priority remain independently unverified.

The separate legacy `clean` capture mode still permits opt-in original
render-pass recording, but its output is independent of the new F11 gate.
Capture snapshots are not automatically interpreted as complete game stages.

## Local verification

```bat
python dev\stage_capture\test_capture_suite_contract.py
python asset_organizer\test_asset_organizer.py
python asset_organizer\test_source_and_foreground.py
BUILD_TEST.bat
RUN_TEST.bat
```

The Python contract checks verify code presence/dispatch ordering only. Real
F9/F10/F11 behavior, ROM loading, compiler/linker results, native performance,
and exact stage coverage must be verified using the local Windows runner and
its logs. None of these checks use GitHub Actions.

## Branch checkpoint

The completed original MVC1 capture / Asset Organizer suite was consolidated
from `feature/stage-87-203-native-layer-recorder` into `main`.
Subsequent level asset extraction belongs on a **new feature branch**.
Keep the original branch as a history checkpoint; do not delete source or
rewrite Git history when beginning the next extraction phase.
