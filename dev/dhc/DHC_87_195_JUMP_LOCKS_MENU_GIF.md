# XMVSF 87.195 — DHC native-entry reliability and Marvel vs. Capcom animated menu GIF

## Ground truth: supplied 87.193(2).zip

Native `dhc_native_handoff.tsv` has **13 DHC attempts**, with 9 original ROM point swaps and 7 confirmed incoming ROM 2E supers. Four failed before native outgoing action 3200, and two swapped but ended in actual non-super actions. The experimental reserve parser is NOT allowed to invent a 2E action when the original ROM chooses a normal/special. The confirmed incoming supers include both Ken Shoryureppa (2E00/D6722) and Shinryuken (2E02/D6976).

At frame **5702** Ryu's 2E02 super owns the point. Host arms native controller8 with 259=08 and captured259=00. On frame **5703**, original 68000 selects a genuine *wrong* `2C26` action, consumes 256=00, but leaves 259=08. The old 87.178 stale-latch predicate refuses this case because the original action changed. The resulting host-owned 259=08 was NOT released. 87.195 adds a narrow cleanup just for a consumed, failed DHC controller8 with same point/reserve and original captured259=00. It never changes the actual ROM action, script, mode, partner or meter. The other three wrong-action examples (`2E0C`) are not falsely counted as tag success.

Entry interpolation was also corrected. 87.194 followed native descending Y but spread the horizontal travel across only `startY..floor`. If the genuine ROM movement first ascends to a jump apex, that makes the horizontal movement unnaturally rush at the end. 87.195 records the lowest native Y/apex and distributes X target guidance across the WHOLE observed descent from apex to floor. No guessed duration, fabricated Y velocity or instant ground snap is added. Native 2A08 continued descent still awaits an actual 87.194+ Windows F9 recording; do not treat this as confirmed if the ROM does not move.

## Animated background from local `_art`

Exact user-supplied filename: `_art/marvel-vs-capcom1-super-effect2.gif`. The file is not in the Git-tracked repository tree; no replacement graphic is generated or committed. The loader looks in the current working directory's `_art`, its nearby parent directories, and the actual executable base path. For the normal Windows build at `<repo>/runtime/stable/XMVSF_Test_87_195.exe`, it locates `<repo>/_art/marvel-vs-capcom1-super-effect2.gif` automatically, regardless of the current working directory.

SDL2_image >= 2.6.0 loads the original animated GIF's frames and per-frame delays, loops them continuously beneath the boot menu, all menu pages, options, controls, Versus panels and the F9 badge. The backdrop covers the 960×540 logical canvas with aspect-ratio-preserving center crop. `SDL_SetTextureColorMod(texture,77,77,77)` applies 77/255 (approximately **30% RGB brightness**) while maintaining original transparency; interactive text/buttons are drawn afterward at full brightness. The image and texture are released safely when leaving the boot menu or destroying the SDL renderer, with graceful no-art fallback on missing file. Older SDL2_image can show a static first frame rather than introducing a new dependency or breaking the game. The filename remains the artist's original GIF.

## Build and verification

Only the existing root `BUILD_TEST.bat`, `RUN_TEST.bat` and `RUN_REPLAY.bat` launcher names are used, selecting `XMVSF_Test_87_195.exe`, logs `test/87.195`. No warm-object clean, no older TEST overwrite, and no Stable promotion.

C++11 regressions: `test_dhc_native_entry_arc_195.cpp` proves apex-based descent guidance, mirrored target, and that a stall or a one-frame floor teleport cannot count as a landing. `test_dhc_host_latch_cleanup_195.cpp` proves exact frame5703 latch cleanup and forbids touching real 3200/other ownership/ordinary actions. CI additionally checks the actual new menu header inclusion, exact requested GIF filename, 30% color modulation and source/build routing.

**What needs genuine Windows confirmation:** the source GIF must actually exist locally in `_art` to see its animated frames; this repository branch cannot ship or verify a non-Git-tracked user asset. The next native recording must come from 87.195, not 87.193, to confirm that Ken's original 2A08 airborne controller continues descending through touchdown and that the next real super still executes cleanly. If the original ROM stalls, the native action/velocity trace must determine the true Sabretooth-style passive entry controller before more changes.
