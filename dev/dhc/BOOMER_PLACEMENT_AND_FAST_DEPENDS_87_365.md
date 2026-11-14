# XMVSF 87.365 — Boomer stage placement debugger and cached native dependency graph

## Runtime placement controls

On **Boomer Kuwanger Stage only** (custom ID 7), use held arrow keys in live gameplay:

- **Up / Down**: move original `a.png` vertically one authored pixel per real game frame. Up decreases the saved Y offset; Down increases it. The original CPS2 floor, camera, actors, hitboxes, collisions and HUD never move.
- **Left / Right**: move the two independently rendered halves of `b.png` in toward the shaft center / out toward the edges, one authored pixel per side per real game frame.
- No arrow resets the position on release. The final offsets are the *new placement target* and are saved once when the arrows are released (and again at renderer teardown if necessary). They reload on future runs, using the same `SDL_GetPrefPath("XMVSF","Stages")` convention as Akuma Stage's moon offset.
- Saved text file: `boomer_placement.txt` in the SDL preferences Stages folder. Format: `<a_y_offset> <b_wall_spread>` (signed integer pixels). Values are clamped to Y ±256 and wall spread ±384. Reset by deleting the file or setting `0 0`.
- `XM87.365 BOOMER_TARGET_LOADED` and `BOOMER_TARGET_SAVED` messages report exact values and file location.

**Runtime B split:** Use two `SDL_Rect` source rectangles cropped from the left and right halves of the original *single* `b.png` GPU texture; no new image processing, re-export, duplicate textures or runtime PNG writes. The half nearest the center is independently translated to its new X coordinate, with repeats continuing only outward; both halves tile vertically and continue descending at 30px/s. This does not fill in the adjustable central gap. Layer C retains its slower downward tiling and horizontal parallax. Layer A stays frontmost of C/B and is vertically offset on screen only.

## Native build speed

The existing Makefile and its GCC-generated `.d` and `.o` files stay authoritative and are never deleted. A new, short helper `dev/dhc/FAST_NATIVE_DEPENDS_365.sh` is sourced from `BUILD_TEST.sh`. After a **successful** native compile/link, TEST publication and verification, the helper records the complete native dependency path inventory to the machine-local `test/build_cache/native_dep_graph_365.paths`.

Later builds, including modified existing engine `.cpp` and header files, may skip FBNeo's expensive full MAKELEVEL=1 dependency scan only when:
- compiler/settings fingerprint matches the last successful build;
- original `fbneo.exe`, generated `driverlist.h`, a successful manifest, and 500+ complete GCC `.d` files exist;
- the current `.d` inventory matches the saved inventory and has no empty files;
- no game-driver, Makefile, generator, or added/deleted C/C++ translation units are detected between the prior successful revision and the current committed and working sources;
- no active one-time native cache repair is pending.

Under those conditions, it executes the **full MAKELEVEL=2 target** with DEPEND=1, the original GCC `.d` files included and remade by Make when their source or headers have changed. Object rebuilding and linking still happen normally; this is not a timestamp-touch shortcut or false no-op. Any guard failure uses the old full two-pass path (including existing menu optimization). `XMVSF_FAST_NATIVE_DEPENDS=0` disables this new optimization.

**First build:** Full Make still runs because the new dependency inventory does not exist. Once that build succeeds, subsequent eligible builds may skip pass 1. The helper adds no new mandatory regression gate, does not invoke GitHub Actions, and does not touch Stable. A failed build cannot seal the cache.

## Testing limitations

These are source changes committed to the Boomer branch. Local Windows/SDL keyboard input, source-PNG proportions, wall alignment and precise build speed remain to be verified using the existing `BUILD_TEST.bat` and `RUN_TEST.bat`. The new source version is 87.365.
