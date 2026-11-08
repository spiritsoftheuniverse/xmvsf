# Cyclops air-throw recovery — CPS2 event scope and release timing

Confirmed native Windows behavior as of TEST 87.216: Cyclops's air-throw
recovery became **consistent** once we used a real per-throw state
transaction and a 36-committed-frame delay. The ROM's fighter animation
pose field is **not** a reliable clock. This page preserves that engine
finding for later combat changes.

## The engine quirk

This is not a single animation-frame release pulse. The catch/grab
transition `0x200C -> 0x200E` identifies **grab ended**, but the ROM
continues running Cyclops's native throw controller afterward. The
`+0x42` animation/sprite field may remain the same for multiple game
updates; counting changes to that field missed releases. Run-ahead
callbacks can also be speculative. The host-side recovery tracker is
not itself in a CPS2 savestate, so feeding it speculative frames risks
consuming a one-shot grab-end edge that the committed game did not keep.

The working design is:

1. Capture Cyclops's **actual preceding airborne controller**
   (native actions `0x0200/0x0202`) while airborne and retain its
   velocity, script, gravity, control and step fields.
2. Create a **fresh per-throw transaction** on first `0x200C`.
   Clear waiting, pending follow, landing latch, animation counters,
   and previous-throw timers; give every attempt a distinct `throw_id`.
3. Start timing only at real `0x200C -> 0x200E` **grab end**.
   Advance exactly once per **committed, non-run-ahead emulation update**.
   Do not depend on animation pose changes or temporary victim actions.
4. While it remains Cyclops's native throw recovery, keep the same
   transaction through related throw-action substates and mode `0x32`.
   Cancel safely if he already exits throw control or naturally lands.
5. At the deadline, restore the saved **pre-throw airborne state**,
   not a synthesized jump or a one-frame external animation.
   Do not write opponent RAM, world X/Y, HP, meter or fighter identity.

The TEST 87.213 observer recorded Cyclops reaching natural landing
around **51–52 committed frames after grab end**, canceling the former
100-frame release. Lowering the deadline to **36 frames (0.6s)** made
release consistent in user gameplay tests.

## TEST 87.219 — Native combo-air grabs and variable stage world Y

**Attached evidence:** `87.218.zip/observer/cyclops_air_throw_release.tsv`
contains nine throw captures. Four had `AIRBORNE_SAVED`, entered
`GRAB_ENDED`, and released at exactly **24 native recovery updates**.
Five captures were `NO_AIRBORNE_BASELINE` at world Y positions
**810, 681, 865, 823, and 805**, despite native airborne flag
`AA=1/2` and grab controller `32`. This invalidates using world
`Y<472` as a universal airborne/grounded condition.

The included DHC post-CPU trace independently shows the last actions
before those misses. Some were neutral airborne `0200/0202` at world
Y above 472, and two were native combo air substates **`020C`**
(mode `1C1A`) and **`0214`** (mode `1C28`). For the `0214`
attempt, a valid `0202` neutral controller occurred 39 frames
earlier; the previous 24-frame source-age limit discarded it.
For the `020C` attempt, the last recorded neutral action was
473 frames earlier, too stale to restore.

The fix is restricted to air-throw eligibility and snapshot selection:

- Trust native airborne actions `0200/0202`, the native `1C`
  airborne controller and `AA` rising/descending flags, **not**
  an absolute stage-Y threshold.
- Retain a known neutral-air snapshot for up to **96 committed
  frames**, allowing 39-frame aerial-combo sequences to reconnect.
- When no recent neutral snapshot exists, accept a recent **native**
  `0200–021E` aerial substate (including `020C/0214`) with
  both mode `1C` and `AA=1/2`. Restore that recorded controller,
  never invent an animation/script/jump.
- Start at `200C -> 200E` regardless of world-Y. Cancel waiting
  only when the native throw family **and** mode32 are both gone;
  world Y does not cancel a live throw.
- Log `AIRBORNE_SAVED`, `COMBO_NEUTRAL_RETAINED` or
  `COMBO_AIR_SAVED` for each capture, making the three paths
  independently diagnosable.

**Input interpretation:** the X cancel observed after a successful
release is **intentional human input**. It is not a delayed recovery
failure and is not intercepted or suppressed by this work.

The proven **24-frame release** and one-time zero at horizontal
velocity `+0x14` remain unchanged, as do run-ahead protection,
per-throw rearming, animation fields, vertical physics, other
characters, and the six-frame native SF2 popup. The fallback combo
substates require native gameplay confirmation before merging.

## TEST 87.218 — Stop inherited horizontal momentum on release

The preceding airborne state contains two separately observed 32-bit
fields: `velocityX` at Cyclops fighter RAM `+0x14`, and `motionX`
at `+0x18`. Because the earlier release restored the entire snapshot,
Cyclops also inherited his pre-grab forward X velocity and kept
moving forward when the throw released.

Change ONLY the release restoration: write **zero at +0x14**
instead of the captured `velocityX`. Retain the saved `+0x18`
movement-control field and `+0x1C/+0x20` vertical velocity/gravity,
world position, animation/controller fields, timing, event identity,
and opponent data unchanged. This is a **one-shot write at release**,
not permanent movement suppression during the grab or afterward.
A natural exit without a forced release must not have its velocity changed.

The native deadline remains **24 committed frames (0.4 seconds)**.
The new ROM-free TEST 87.218 regression explicitly checks the
horizontal velocity before release, at the deadline, after a second
independent grab with fresh momentum, and after a natural action exit.
Actual Windows gameplay remains the final verification.

## TEST 87.217 experiment — earlier release

The deadline is now **24 committed native frames (0.4s at 60Hz)** by
request. *Only the delay changes*. All successful scope/rearm, real-frame
guard and saved-air restoration logic remains unchanged. This is not yet
native-gameplay-verified. A regression checks exactly 24 native ticks,
no early release, no double counting, and consecutive throws.

Logs:
`test/87.217/observer/cyclops_air_throw_release.tsv` contains
`throw_id`, `THROW_CAPTURE`, `GRAB_ENDED`, `RECOVERY_TICK`,
`RELEASED`, `RELEASE_CANCEL` and `SCOPE_END`. If early timing causes
a regression, use that evidence before changing event scope.

This note is intended to prevent future work from reintroducing the
animation-pose counter, a 100-frame wait beyond natural landing, or
speculative-frame consumption. No GitHub Actions; test on the existing
Windows native TEST executable.
