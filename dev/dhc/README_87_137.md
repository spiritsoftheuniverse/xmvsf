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
