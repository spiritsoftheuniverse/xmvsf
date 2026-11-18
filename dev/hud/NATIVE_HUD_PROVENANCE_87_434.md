# TEST 87.434 — Native M68K/object-RAM provenance capture

This is the first data-gathering checkpoint to replace the failing HUD
tile filter. No shock/timing/framebuffer changes were made.

**Run** after pulling `feature/native-stage-capture-catalog-87-408`:

1. `BUILD_TEST.bat` — require **TEST BUILD READY** for 87.434.
2. `RUN_TEST.bat xmvsfu provenance` — choose Version 4.
3. Play the damaged-wall stage through the pre-fight and shock frames.
4. Close the game to flush trace files.
5. Upload both:
   - `test/87.434/observer/hud_provenance/m68k_oam_writes.tsv`
   - `test/87.434/observer/hud_provenance/oam_drawn.tsv`
   Include an F6 screenshot if practical.

**Trace mechanics:** Existing 0x708000..0x70FFFF object memory WRITE
interceptors now log X, Y, tile and attr writes during native 02/04/06/08.
The draw provenance recorder, previously live-combat-only, now records
all objects during these phases and adds original raw bank/slot columns.
Join actual raw bank+slot across the buffered object-frame delay, then
compare executed ROM routines and source registers across original
stages to distinguish HUD producers from wall producers. The reported
`PC` is obtained in the memory-write callback; review against `PPC`
instruction traces before treating it as a definitive instruction
address. Different graphics can share a generic copy routine.

The existing `XM8717::Recent` four-frame source-age rule remains;
raw slot+bank is kept even when that source tag expires. No source
identity is yet trusted for masking. No writes to ROM/RAM, no new
classification or tile heuristics. F10 is not necessary for the
dedicated writer log, but F6 can correlate visible timing.

**Limitations:** This first pass caps legacy writer logs at 1.5 million
rows and draw rows at 2 million. Both TSVs overwrite if you launch
the provenance mode again; save them elsewhere between runs. Nothing
has been compiled or visually validated on the local Windows machine.
