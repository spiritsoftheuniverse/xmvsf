# XMVSF 87.157 — recover early Ryu/Ken cancellation, keep real Ken supers

Input observations from user uploads:
- `87.150(1).zip`: Ryu Shinkuu 2E08/2A28 late timer +4E=8 at frame
  2302. Native tag acceptance and host bridge 3200 at 2303, real Ken
  ownership swap at 2317. At 2318 native Ken 2A0C incoming kick was
  replaced with ROM 2A08 nonattacking tag exit (D6=12, D567E script).
  At 2320 a premature guessed D6=0A/A0=2A28 native-super handoff ran
  `2C32` dash at 2321 — NOT Shoryureppa.
- `87.156.zip`: native-input proof successfully performs all three:
  Ken Shinryuken 2E02/D6976 starts at frame 5087 (after normal tag
  Ken swap at 5008); another Shinryuken at 6985 (swap 6906);
  Shoryureppa 2E00/D6722 starts at 7841 (swap 7762).
  However, normal Ken 2A0C→2A0E attack sequence remained live for
  60 frames: Ken first true neutral 0000/A0=002A, D6=10,
  ROM script D39FA at frame 5068. The proof waited 79 frames between
  Ken swap and real Shinryuken: excellent ROM correctness, terrible
  cancel timing.

## Stage 87.157 experiment

RUN_TEST.bat now sets `XM87157_EARLY_CANCEL=1` and turns OFF the
sequential `XM87156_PROOF_NATIVE` mode. It preserves the exact standard
BUILD_TEST.bat and RUN_TEST.bat filenames and incremental Make dependency
cache (no forced `-W` recompiles).

1. User requests reserve Ken's super during Ryu 2E08 with exactly
   QCF+two punches or QCF+two kicks. Both 2E script addresses are resolved.
2. Ryu's first super remains active until the captured successful 87.150
   controller handoff window (+4E from 1 to 8). The previous bridge into
   ROM tag 3200 is preserved; tag ownership swap must be ROM confirmed.
3. At Ken's **FIRST** native 2A0C incoming kick frame, restore the proven
   87.150 2A08 NONATTACKING script (D567E, D6=12), position and face
   the opponent. This is the early tag-kick cancellation (previously
   disappeared entirely from 87.156's proof).
4. At the next ROM-confirmed 2A08 frame, and only if D6 and script are
   still exactly the expected Ken exit handler, resolve Ken's real
   neutral action 0000 ROM script **D39FA**; initialize it via the
   existing bounded ROM table+script initializer; restore ROM-observed
   A0=002A, D6=10, no input latch or inherited dash velocity.
   This avoids the previously invented 2C00/2C32 animation.
5. ONE frame later, inject a compact native QCF (2 frames down, 2
   down-forward, 1 forward, 2 forward+PP/KK, then release), preserving
   user-facing physical inputs outside the bounded window.
6. Confirm a genuine Ken 2E00/D6722 or 2E02/D6976 with ROM +A0=2A02,
   reject any 2C dash, wrong super or timeout and release the lock.
   No speculative meter debit, infinite action override, or fake super.

**The slower proof implementation remains in the source**, unchanged.
To compare it using the standard TEST launcher, change the two lines
inside `RUN_TEST.bat` to `XM87156_PROOF_NATIVE=1` and
`XM87157_EARLY_CANCEL=0` before launching (the current launcher
explicitly sets them to 0 and 1 respectively). The proof uses the
87.156 confirmed HP+HK tag after Ryu finishes, then Ken's entire
native entry and recovery.

Expected early-cancel latency if the ROM accepts shortened QCF:
Ken ownership swap → native 2A0C→2A08 one frame later →
Ken ROM neutral two frames later → PP/KK approx 8 frames after
ownership swap → ROM 2E00/2E02 shortly afterward.

No claim of complete DHC is made until a *new live* 87.157 user capture
demonstrates the actual Ken super inside the desired early timing window.
The synthetic CI test verifies strict ROM table guards, no attempted
2C action, both PP/KK input masks and wrong-state no-RAM-mutation, but
is not a native Windows playthrough.
