# XMVSF DHC — native implementation track (Stage 87.137)

**CURRENT TEST: Stage 87.142** — native ROM-table resolution of real Ryu/Ken super scripts and actual tag/partner allocation evidence; facing proxy fixed. See [ROM_SCOUT_87_142.md](ROM_SCOUT_87_142.md). The old 87.138–87.141 notes below are historical. DHC gameplay not yet connected.

**CURRENT EXPERIMENTAL TEST: Stage 87.141.** Ryu/Ken-specific super-input chat and F10 native capture repair. [Read the 87.141 handoff](TEST_87_141_RYU_KEN_F10.md). Older 87.138/87.139/87.140 procedures below are historical and should not be used for current diagnostics. No playable DHC is enabled.

**CURRENT TEST IS 87.139 — F10 READ-ONLY DIAGNOSTICS.**
[Use the Stage 87.139 diagnostic instructions](TEST_87_139_DIAGNOSTICS.md).
Prior 87.138 tag-landing instructions below are historical and must **not**
be enabled during ordinary tag or super testing. That experimental hook
changed point-fighter X/facing on a *normal tag* and is removed from the
current native frame and painter paths. No playable DHC is connected yet.


Status: **DHC decision layer introduced; gameplay transition not connected yet.** This branch does not change the working build or Stable.

## Requested gameplay
- While a point fighter's native super is active, read an input corresponding to **one of the reserve fighter's actual supers**, not the point fighter's command list.
- On a valid request: pay **one additional stock**, cancel the outgoing super, stage a ROM-compatible tag-out/tag-in beside the opponent, on the outgoing fighter's side, and darken the stage **without any incoming super portrait**. The reserve fighter stays visible as they land beside the opponent and animate the selected super startup.
- Only **one DHC** per uninterrupted super sequence. No return DHC or ping-pong until the incoming super actually ends.
- Reject invalid commands, insufficient meter, non-combat phase, unavailable reserve, inconsistent fighter identity, and unknown super animation/command state. Never consume meter on rejection.
- Keep existing tag, Triple-Start, Training selection, KO/dizzy, Tournament, and Stable behavior unchanged.

## Verified source anchors
- Active combat is verified with FF5000 == 08 and FFA014 == 00FF4000.
- Fighter records: FF4000 P1 point, FF4400 P2 point, FF4800 P1 reserve, FF4C00 P2 reserve. Native fighter ID at record +0x52 (87.135 roster snapshot).
- Existing stock byte reading: FF4214 P1 / FF4614 P2 (87.118 meter observer). **Reading is proven; writing/spending requires validating the ROM's own stock-debit path.**
- Current global super tracker `xmvsf_stage87_59_global_super_audit.h` explicitly says its action transitions are **unverified candidates**, not a canonical super begin/end signal.
- 87.120 Cyclops native-state transition demonstrates that merely changing a character ID is insufficient: action, script pointers, animation, movement, and partner ownership state must agree.
- Stage 87.63's portrait-exit observer is renderer evidence for normal supers, **not** a DHC release signal. DHC suppresses the incoming super portrait, so timing must depend on verified tag landing and native first-active readiness, not a portrait exit or superjump (+AA).

## Safe implementation stages
1. **Pure arbitration** (this branch): per-player DHC state, reserve-super command whitelist, one-use lock, no meter debit until commit, reset only after verified incoming-super exit. Unit-testable without ROM writes.
2. **ROM-backed input tables:** capture native command parsing and accepted super IDs for every fighter, including alternate supers. Record specific character ID, input signature, resulting action/script, and stock change. Never fabricate command tables from motions alone.
3. **Native tag transition:** compare normal tag-in (including Sabretooth entrance) with native super-start snapshots; reproduce required point/reserve slot and control ownership transactions without overwriting HP, dizzy, opponent state, or scripts.
4. **DHC-only visual / freeze integration:** dim the native stage, **bypass the native incoming portrait-intro animation and its timer** (not only its sprites), filter any residual DHC portrait objects, and keep incoming landing/super startup visible. Freeze world/opponent/outgoing attacks until first-active release; never suppress normal super portraits.
5. **Real-frame-only integration:** honor runahead, mode and phase gates. Input capture on edges; commit exactly once at safe post-CPU boundary. Keep arbitrary debug logging opt-in.
6. **Test matrix:** all eligible point/reserve combinations, each actual reserve super, both sides, 0/1/2+ meter, already-tagging, grounded/air, hits vs misses, DF/Classic/Tournament/Training/Versus, subsequent fresh supers, and Triple-Start reset. Reject unverified super forms instead of silently invoking the wrong move.

## Ownership contract
`DHCArbiter` makes no native memory writes. The eventual host must supply:
- `superActive`: authoritative native super period.
- `incomingCommand`: parsed command mapped to **reserve** character's super list, with unique move ID.
- `stockAvailable`: additional stock is available under current Training/meter rules.
- `nativeCommit()`: atomic swap/cinematic/super dispatch and **single** stock debit; only after all validations pass.
- `incomingSuperEnded`: verified event, not elapsed time or portrait-only disappearance.

Do not merge as a playable DHC until these host contracts are supplied and validated.

## Non-negotiable DHC timing — zero visible startup

The request is not "start reserve's super when darkness ends." Its startup is
**completely advanced while the STAGE IS STILL DARKENED**. The match and the
departing super remain frozen throughout. On unfreeze the next native game
frame is **already the incoming super's FIRST active frame**.

Three independent progress domains:
- World/opponent/outgoing actions, collision and super projectiles: HOLD.
- Incoming fighter native tag entrance and super startup animation: ADVANCE
  **visibly in the dimmed scene**, first land beside the opponent on the outgoing fighter's side,
  then start the actual selected super. Do not advance any other move.
- DHC-only cinematic renderer: darken the stage; bypass the native portrait
  intro script AND its wait timer; NEVER draw the incoming super's portrait
  or splash, but KEEP THE INCOMING FIGHTER DRAWN at every
  step of entry and startup. No opaque full-frame black overlay. The normal
  super portrait pipeline stays untouched for non-DHC supers.

### Lookahead contract (no move-specific fixed startup timer)

The repository already has FBNeo `StateRunAheadSave/StateRunAheadLoad` in
`src/burner/sdl/run.cpp`, and the Stage 23 hitbox gather path knows native
fighter and projectile offensive rectangles. Together they suggest a precise
solution without guessing 10/20/40-frame startup lengths:

1. At a *safe post-CPU* boundary during DHC blackout, save the complete
   emulated state **in memory**, with external side effects suppressed.
2. Speculatively run **one next released native frame** with the requested
   incoming super. Observe whether this frame enters that move's native
   first-active state. For strikes this may be a verified offensive hitbox;
   projectiles, grabs, counters, and install supers need their **own** verified
   native activation predicate. Do not mistake any hitbox for the incoming move.
3. Restore the full emulated state and verify restoration. This preview MUST
   leave no committed gameplay tick, input edge, meter debit, QSound playback,
   logging side effect, autosave, or fighter state change.
4. If preview says NOT READY, keep the world frozen and the stage darkened,
   advance the incoming fighter's actual native super startup **visibly** one
   tick, then try again. If READY,
   stop hiding startup; unfreeze before the next actual native frame.
5. The first actual visible native frame MUST be the move's first active frame.
   A missed activation or failed restore triggers a safe rollback, never a
   guessed startup adjustment or a blind release.

Because the native fighter can have asynchronous effects and object ownership
data, neither a full ordinary `Cps2Frame` under frozen video nor merely
swapping `+0x52` character IDs qualifies as selective incoming advancement.
The actual host must intercept the native **gameplay update ownership** or
restore all non-incoming state at each hidden tick, prove collision consistency,
and keep the DHC-only stage dimming and visible incoming fighter intact.
The renderer must identify the native incoming portrait objects precisely,
not hide all priority-7 HUD art. This is the remaining difficult part.

### New isolated source files

- `xmvsf_stage87_137_dhc_timing.h` — landing-gated, dimmed-stage/startup/release FSM;
  cannot release without confirmed landing, native portrait-intro bypass,
  real super startup and readiness. It requests DHC-only portrait filtering
  and intro bypass; the native host must carry these out.
- `xmvsf_stage87_137_dhc_lookahead.h` — speculative one-frame readiness
  probe with unconditional rollback and restoration check.
- `tests/test_dhc_timing.cpp` and `tests/test_dhc_lookahead.cpp` — deterministic
  edge/failure tests. Created but not yet run against a Windows native build.
- `xmvsf_stage87_137_dhc_arbiter.h` redirects to the new coordinator rather
  than allowing a premature unlock on incoming action exit.

**Not yet connected to `Cps2Frame` or `RunFrame`. Both the native portrait-intro bypass and portrait rendering suppression are policy gates, not implemented native hooks; not playable.** The
repository currently lacks validated native move-specific super command tables,
first-active predicates and independently-steppable incoming-fighter update.
These cannot safely be derived from the 87.55 / 87.59 candidate super probes.
Do not merge into main/Stable until native proof and gameplay tests exist.

### DHC visual sequence (revision: portrait-free)

1. Detect a valid **reserve fighter super command** while the outgoing
   character's original super remains active, verify one additional stock and
   commit only once.
2. Freeze the existing world, opponent and outgoing attack state; dim the
   stage. Apply the DHC-only incoming portrait filter **before** the reserve
   starts a native super. The host must also bypass the DHC incoming super's
   **native portrait intro script and wait timer**, rather than allowing a
   hidden native cinematic to play. Ordinary super cinematics remain unchanged.
3. Animate the outgoing tag departure if required, bring in the reserve
   character and land them beside the opponent, on the departing fighter's
   side, with native ground/facing and slot ownership.
4. Keep drawing the incoming fighter on top of the darkened stage. Only after
   landing, begin the **actual selected super's native startup animation**.
   Continue that animation while the rest of the match is held; no giant
   portrait, no additional cinematic flash/overlay.
5. Use a reversible one-frame preview and **move-specific native activation**
   evidence to know when the next real frame is the incoming super's first
   active attack tick. Remove dimming/freeze immediately before that frame.
6. Let the selected super resolve naturally. Never permit another DHC in the
   same super sequence, and consume exactly one additional stock for the DHC.

**Do not unfreeze just because the actor landed** or because a native super's
regular portrait timer expired. A landing is necessary but not sufficient:
the incoming super must still finish its startup within the DHC freeze.

## 87.137 native capture integration — TEST branch

The experimental branch now modifies the **live native CPS2 test source**
`dev/stable/buildtree/src/burn/drv/capcom/cps_run.cpp` by adding one
post-CPU, real-frame-only call into
`xmvsf_stage87_137_dhc_native_capture.h`. The recorder is dormant when
its TSV environment variable is absent; it never writes game RAM or ROM,
consumes inputs, pauses, changes sprite composition, or spends meter.

For a **branch-only TEST build** after safely switching/pulling source
changes (do not overwrite uncommitted working files), run from a Windows
Command Prompt:

```bat
cd /d F:\xmvsf
set "XM87137_DHC_CAPTURE_TSV=%TEMP%\xm87137_dhc.tsv"
set "XM87137_DHC_CAPTURE_RAM=%TEMP%\xm87137_dhc.ram"
BUILD_TEST.bat
RUN_TEST.bat
```

In a TEST session perform native tag entries and the normal point-character
supers for the fighters being targeted. Run both move choices when a
fighter has multiple supers, and allow each super to reach first active
frame and finish. Capture P1 and P2 too if possible.

The TSV records every real combat frame for each native point and reserve
slot: accepted fighter ID, position, mode, animation, action, script
pointers, existing hitbox IDs/gates, live CPS2 buttons, and meter.
The companion binary captures native 0xFF4000–0xFF53FF RAM and
the real input bits for deeper control-flow matching (no ROM distributed).
Both are **opt-in** and capped at 18,000 combat frames, about 5 minutes at
60fps. They do not establish universal super IDs by themselves.

`dev/dhc/tests/test_dhc_native_capture.cpp` is a portable C++11
unit test of the recorder's byte layout, fighter slots, action edges,
phase gating, duplicate-frame suppression and binary record length.
This test compiles without SDL, Windows or game ROMs.

**Still missing for playable DHC:** a demonstrated reserve super input
dispatch, native tag-to-super action swap, one-stock ROM debit, DHC-only
native portrait-intro bypass + object suppression, isolated incoming-only
animation stepping, and verified per-move first-active release. Do not
mistake a successful capture for a working DHC. No changes to Stable.

## 87.137 reserve command queue — source connected

The live SDL mapped-input path now stores a 48-frame ring for P1 and P2
containing forwarded left/right/up/down, six attack buttons and new-press edges.
The existing game input delivery is unchanged. It exports the data through
`xm87137ReadMappedInput()` for the eventual CPS2 native DHC dispatcher.

`dev/dhc/xmvsf_stage87_137_dhc_command_queue.h` implements a **pure**,
facing-relative motion matcher and a per-side, once-per-super command latch.
It supports quarter-circle, dragon-punch, half-circle and double quarter-circle
patterns, plus exact/two-punch/two-kick/three-button requirements. The registry
starts EMPTY. Only entries explicitly marked native-verified may be registered,
and conflicting matches fail closed. **No super moves are guessed or injected**.
This parser is not yet called by a gameplay dispatcher.

Added isolated C++11 tests for input history, motion matching and native
capture. They compile independently of the Windows build; Windows native TEST
and in-game behavior remain unverified.

Before actual DHC can be switched on, use the source-backed trace to map every
reserve fighter's ROM-accepted super entry, associated script, charge/debit and
first-active signal. The DHC-only portrait animation/timer bypass and
incoming-fighter selective advancement are still not implemented.

## Stage 87.138 root TEST launcher repair

The **experimental branch** now has corrected standard root launchers:
`BUILD_TEST.bat` invokes `dev/dhc/BUILD_TEST.sh`, which compiles the
**current** local `dev/stable/buildtree` using its warm object cache.
After successful linking and Stage 87.138 native marker verification, it
publishes **only** `runtime/stable/XMVSF_Test_87_138.exe`.
`RUN_TEST.bat` launches **only that binary** from the existing
`FBNeo` game-data working directory, never 87.132.4 or Stable, and
sets the optional Stage 05 tag-landing experiment flag for this TEST.

`BUILD_STABLE.bat`, `RUN_STABLE.bat`, `XMVSF_Stable.exe` and all
existing older TEST builds remain untouched. Neither the recursively
nested excluded `dev/stable/releases` directory nor the historical
87.132 Python builders are required for the new TEST launchers.

To receive the fix in GitHub Desktop:
1. Preserve any uncommitted local edits.
2. Select the `feature/dhc-mvc2-stage87` branch (not `main`).
3. **Pull origin again** after this launcher fix.
4. Run the original root `BUILD_TEST.bat` and then `RUN_TEST.bat`.

The newly compiled 87.138 binary remains an experimental native tag
**landing** test, not an implemented super-cancel DHC. GitHub CI checks
C++11 test components and bash syntax; full MSYS2 native compilation
and in-game behavior must be tested locally.

## 87.137 placement correction — prioritize the playable DHC

**The incoming partner JUMPS NEXT TO THE OPPONENT**, not next to or on top
of the outgoing point character. Aim the existing native tag-in jump at an
opponent-relative landing X on the outgoing fighter's side; keep the normal
jump arc and landing. After landing, animate the selected incoming super
against the darkened stage, no portrait/portrait timer, then unfreeze on the
first active attack frame. The opponent's position stays unchanged.

The new pure `xmvsf_stage87_137_dhc_landing.h` computes the world-space jump
target and facing from point/opponent X, a temporary separation supplied by
the native host, and caller-verified stage boundaries. It has **no gameplay
hook yet** and no hardcoded guessed pushbox dimensions.

For the first working implementation, focus on this one complete sequence:
**input during super → one stock → opponent-relative tag jump → land →
visible super startup in darkened scene → first active frame release**.
Additional refinements can follow a functioning proof of concept.


## Stage 87.138 — first opt-in native landing smoke test

The branch now contains a **real native post-CPU tag placement experiment**.
It listens for an actual ROM point/reserve fighter-ID swap (not a synthetic
tag). During an airborne incoming-fighter frame on Stage 05 only, it
targets the incoming point's world X **48 native units beside the opponent**
on the outgoing fighter's side, sets facing toward the opponent, preserves
the fractional X, and leaves opponent/world Y and the rest of the fighter
record untouched. When the incoming native actor is grounded or the scene
exits combat, the renderer stops the special darkening.

In Wily Stage 05, the SDL painter draws the scene then applies a translucent
black rectangle **before** adding the Wily actor/HUD and genuine CPS2 fighter
sprite layers. The fighters therefore stay visible against the darkened
background for the real tag-in jump. Ordinary modes are unchanged.

**Only for an experimental TEST, not Stable:**

1. In GitHub Desktop switch to `feature/dhc-mvc2-stage87` after preserving
   any outstanding local work. Pull the branch.
2. In the same Windows Command Prompt:

```bat
cd /d F:\xmvsf
set "XM87138_TAG_LAND_TEST=1"
set "XM87138_TAG_LAND_LOG=%TEMP%\xm87138_tag_landing.tsv"
BUILD_TEST.bat
RUN_TEST.bat
```

3. Enter Stage 05 Training or Versus. Perform a **normal native tag** while
   both fighters are on-screen. Confirm that the incoming native tag jump is
   repositioned next to the opponent, and that the stage briefly darkens
   while the actual incoming actor remains visible.
4. Check the optional TSV: `POSITIONED_IN_NATIVE_TAG_JUMP` confirms the
   experiment actually moved the incoming point actor, while
   `TAG_SWAP_AT_OR_AFTER_LANDING` or `TAG_SWAPPED_BUT_INCOMING_INACTIVE`
   means the ROM changed roles outside this narrow airborne write window.

This is a concrete, opt-in **tag landing and visual test, NOT DHC**. It does
not interrupt a super, trigger the incoming super, spend meter, bypass the
native super portrait, or freeze the opponent/world. Those are the next
native combat steps. The original root launcher **names** are preserved;
the new experimental versions build and run the current Stage 87.138 TEST
directly, without historical release scripts. The project was not built on
the user's Windows machine here; the isolated C++11 tests run on GitHub Actions.

The revised TEST runner enables the experimental Stage 05 landing flag in
its own local environment only. Closing the Command Prompt ends the session
without changing Stable.

## 87.138 TEST builder fix — missing cmp / stale make pass

An MSYS2 TEST build printed `make: Nothing to be done for 'all'.`, then
failed at `cmp: command not found`. Fixed in the experimental branch:

- `BUILD_TEST.sh` now executes `MAKELEVEL=1 make -f makefile.sdl2`
  to follow this FBNeo tree's two-pass compile/link pipeline (as its
  existing Stable builder already does).
- Only `cps_run.cpp`, `inp_sdl2.cpp`, and `vid_sdl2.cpp` are treated
  as newer using GNU Make's `-W` option. This forces the DHC-modified
  translation units to compile and link without editing source timestamps
  or clearing the warm object cache.
- The optional `cmp` byte-comparison command was removed. A successful
  `cp -p` and nonempty pending binary are verified with Bash built-ins
  before the dedicated 87.138 TEST executable is published.

Pull `feature/dhc-mvc2-stage87` again, then run the **same** root
`BUILD_TEST.bat` and `RUN_TEST.bat`. No new launcher names, automatic
source replacement, history restore, or Stable build is involved.
