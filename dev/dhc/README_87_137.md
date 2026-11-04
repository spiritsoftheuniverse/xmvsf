# XMVSF DHC — native implementation track (Stage 87.137)

Status: **DHC decision layer introduced; gameplay transition not connected yet.** This branch does not change the working build or Stable.

## Requested gameplay
- While a point fighter's native super is active, read an input corresponding to **one of the reserve fighter's actual supers**, not the point fighter's command list.
- On a valid request: pay **one additional stock**, cancel the outgoing super, stage a ROM-compatible tag-out/tag-in at the outgoing fighter's current world position, and use the native super portrait/darkening flow to pause action while the reserve fighter starts the requested super.
- Only **one DHC** per uninterrupted super sequence. No return DHC or ping-pong until the incoming super actually ends.
- Reject invalid commands, insufficient meter, non-combat phase, unavailable reserve, inconsistent fighter identity, and unknown super animation/command state. Never consume meter on rejection.
- Keep existing tag, Triple-Start, Training selection, KO/dizzy, Tournament, and Stable behavior unchanged.

## Verified source anchors
- Active combat is verified with FF5000 == 08 and FFA014 == 00FF4000.
- Fighter records: FF4000 P1 point, FF4400 P2 point, FF4800 P1 reserve, FF4C00 P2 reserve. Native fighter ID at record +0x52 (87.135 roster snapshot).
- Existing stock byte reading: FF4214 P1 / FF4614 P2 (87.118 meter observer). **Reading is proven; writing/spending requires validating the ROM's own stock-debit path.**
- Current global super tracker `xmvsf_stage87_59_global_super_audit.h` explicitly says its action transitions are **unverified candidates**, not a canonical super begin/end signal.
- 87.120 Cyclops native-state transition demonstrates that merely changing a character ID is insufficient: action, script pointers, animation, movement, and partner ownership state must agree.
- Existing user-requested cinematic cues are the super portrait exit and ROM action end. These must be verified for each supported super and must not trigger from super jumps (+AA).

## Safe implementation stages
1. **Pure arbitration** (this branch): per-player DHC state, reserve-super command whitelist, one-use lock, no meter debit until commit, reset only after verified incoming-super exit. Unit-testable without ROM writes.
2. **ROM-backed input tables:** capture native command parsing and accepted super IDs for every fighter, including alternate supers. Record specific character ID, input signature, resulting action/script, and stock change. Never fabricate command tables from motions alone.
3. **Native tag transition:** compare normal tag-in (including Sabretooth entrance) with native super-start snapshots; reproduce required point/reserve slot and control ownership transactions without overwriting HP, dizzy, opponent state, or scripts.
4. **Cinematic / freeze integration:** reuse verified portrait/overlay/freeze mechanism; ensure outgoing super hitboxes end cleanly before incoming attack starts, and don't freeze ROM permanently.
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
**completely advanced while the BLACKOUT IS STILL DRAWN**. The match and the
departing super remain frozen throughout. On unfreeze the next native game
frame is **already the incoming super's FIRST active frame**.

Three independent progress domains:
- World/opponent/outgoing actions, collision and super projectiles: HOLD.
- Incoming fighter native tag entrance and super startup animation: ADVANCE
  while black; do not advance a different move.
- Cinematic renderer: present native super portrait/blackening independent of
  simulated world-time; keep it until the incoming super is ready.

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
4. If preview says NOT READY, keep blackout/freeze and advance the incoming
   fighter's actual native startup one hidden tick, then try again. If READY,
   stop hiding startup; unfreeze before the next actual native frame.
5. The first actual visible native frame MUST be the move's first active frame.
   A missed activation or failed restore triggers a safe rollback, never a
   guessed startup adjustment or a blind release.

Because the native fighter can have asynchronous effects and object ownership
data, neither a full ordinary `Cps2Frame` under frozen video nor merely
swapping `+0x52` character IDs qualifies as selective incoming advancement.
The actual host must intercept the native **gameplay update ownership** or
restore all non-incoming state at each hidden tick, prove collision consistency,
and keep the black cinematic visible. This is the remaining difficult part.

### New isolated source files

- `xmvsf_stage87_137_dhc_timing.h` — explicit blackout/startup/release FSM;
  cannot release without hidden startup, black presentation and readiness.
- `xmvsf_stage87_137_dhc_lookahead.h` — speculative one-frame readiness
  probe with unconditional rollback and restoration check.
- `tests/test_dhc_timing.cpp` and `tests/test_dhc_lookahead.cpp` — deterministic
  edge/failure tests. Created but not yet run against a Windows native build.
- `xmvsf_stage87_137_dhc_arbiter.h` redirects to the new coordinator rather
  than allowing a premature unlock on incoming action exit.

**Not yet connected to `Cps2Frame` or `RunFrame`; not playable.** The
repository currently lacks validated native move-specific super command tables,
first-active predicates and independently-steppable incoming-fighter update.
These cannot safely be derived from the 87.55 / 87.59 candidate super probes.
Do not merge into main/Stable until native proof and gameplay tests exist.
