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
