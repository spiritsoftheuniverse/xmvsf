# TEST 87.421 — F7 M68K PC and ROM-state overlay in F6 PNG

Press **F10 at character selection**, so the existing native instruction
probe records FF5000 status-writing instructions. Press **F7** to show
the bottom-left overlay. When the versus box walls are at the desired
position, press **F6**. The PNG includes the visible wall placement and
M68K metadata in one native-resolution 384x224 image.

The overlay shows: native frame, current FF5000 phase, FF5104 WORD match
step, FF5113 stage, WRPC (executed instruction PPC of most recent FF5000
change), NEXT (following PC), and EDGE (status before/after, frame).
It is the exact existing F10 PPC of the status-writing instruction,
NOT a guessed code address based on a screenshot. If F10 is off, the
overlay explicitly says so rather than fabricating a PC.

F7 toggles the overlay off. F10 stops the normal probe recording.
No changes to approved 87.417 HUD rendering/timing, audio, stage
artwork, gameplay, or key 8/9. 87.420 runtime rollback remains intact.
