# 87.167 full-cast replay: PASS for checkpoint completion and ROM action trace

Date: 2026-11-04 (America/New_York). This report is based on the user-supplied ZIP `87.167(3).zip` and its exact preserved original `87.167(2).zip`.

**Verdict: PASS — 29,827-frame host-aware replay and 150/150 ROM action trace equality.** This does *not* prove full framewise RAM/CPU determinism or the 68000 recognizer mapping for the whole cast.

## Chain of custody

- Original sealed recording `87.167(2).zip` SHA-256: `acfe1469894179c27d40eac7eaa460ab1fb4b0076406b35cf9c9a94b5ee8952a`.
- Playback evidence ZIP `87.167(3).zip` SHA-256: `f927dd7faa1fe02662f1b2a30a74f5d39f2a718345568bf73ee13782d612291e`.
- All **149 original non-directory entries** from `87.167(2).zip` are present and **byte-identical** in the new archive; ZIP CRC check passed.
- Playback diagnostics are under `replay_results/` and are separate from the immutable `replays/session_0001/` recording.

## Verified playback result

- `V2_MANIFEST_SEALED frames=29827 sessions=8 segments=68 reason=F9_STOP`
- `CHECKPOINT_LOADED`: **68 out of 68** (counted from replay-results status log).
- `GAME_BEGIN`: **8 sessions**.
- `SESSION_END next_session=`: **7 transitions**.
- `PLAY_END complete_frames=29827`: **present**.
- `REPLAY_DIVERGED`: **zero**.
- `rom_super_observed.tsv`: **150 out of 150** entries byte-for-byte identical, recording vs playback. SHA-256 of either: `3998b50f26290ada54739983424dc2dc5f024369ea3d6e92ea33556f0218bbc0`.
- `rom_super_catalog.tsv`, `rom_dispatch.tsv`, `dhc_native_handoff.tsv`: also **byte-for-byte identical**.

## Nonblocking diagnostic differences

- `super_chat.tsv`: playback omits two recording-time-only F10 probe ON/OFF messages; all other lines match.
- `tournament_rules.tsv`: one timestamp differs. Original active event frame **780**, playback **806**. The remaining tournament-rule flags are identical. Do not claim absolute framewise host-diagnostic equality or CPU/RAM determinism.
- Same-frame physical button transitions remain valid, and two known ROM catalog/export gaps (Sabretooth `0A/2E16/0C0E86`, Ryu `12/2E06/0D2E7C`) remain pending indexing repair.

## Implications for DHC work

The eight-session input and ROM-action evidence pipeline is reproducible enough to **proceed with the 68000 recognizer tracing**. We have **43/43 physical button-mask matches**, 150 ROM observations, and 22 physical-input-associated ROM script roots in this capture. No new script root should be classified as an executable super command until the input recognizer and native state transition are established. Akuma **Tenma Gou Zankuu stays explicitly excluded**. Magneto, M. Bison, Storm and Zangief have no observed 2E roots in this capture and still require separate verified recordings.

Use the existing root launchers `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat`; do not add version-numbered launchers or change Stable. Retain all original recordings, checkpoints, warm build objects, and history.

This is a verification record only; it does not implement, test or enable any new DHC move.
