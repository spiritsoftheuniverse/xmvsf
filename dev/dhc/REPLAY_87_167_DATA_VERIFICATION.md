# TEST 87.167 — replay capture validation (2026-11-04)

## Current verdict

**OFFLINE DATA PASS. WINDOWS REPLAY COMPLETION + ROM ACTION TRACE PASS (87.167_2.zip). FULL CPU/RAM DETERMINISM NOT YET PROVEN.**

## Recorded replay completion — independently inspected 87.167_2.zip

- New evidence ZIP SHA-256: `28cd91161e4cc98c317c2c409e77f34328bcb6c7a31200ab8af1b98620ebe333`.
- ZIP integrity: CRC checks pass; embedded `87.167.zip` SHA-256 equals the original `01cda0b08c8427b0e91ad09a3237f29e2d9c57cdd6a510be259ad7954893ad14`. All 43 common original archive entries are byte-identical.
- `replay_results/replay_v2_status.log` shows sealed manifest `6395/2/14`, all fourteen `CHECKPOINT_LOADED` entries, transition `SESSION_END next_session=2`, and `PLAY_END complete_frames=6395`; zero `REPLAY_DIVERGED` entries.
- `replay_results/rom_super_observed.tsv` has 39 events and is **byte-identical** to the original capture `rom_super_observed.tsv` (SHA-256 `f971af18cd48f797b367ba9b1018e97043b5fe1c99ec74af3ecf3f7d56319f8e`). This checks fighter IDs, ROM frame/movie frame, selector, scripts, animation, last inputs, and classifications.
- `rom_super_catalog.tsv`, `rom_dispatch.tsv`, and `dhc_native_handoff.tsv` also match their replay-results counterparts byte-for-byte.
- `tournament_rules.tsv` differs **only** in initial active-frame stamp (capture `780`, replay `851`); all ten enabled/installed/status fields on that row match. This is a diagnostic timing difference, *not* yet established to be a behavior divergence.
- Observed fighters: `1E` (20 events), `0E` (12), `18` (5), `0C` (2); 18 distinct native script roots. 13 `PHYSICAL_CANDIDATE` events, 26 `NONE`.
- **Gate:** recording/replay and ROM-action correlations are ready for expanding cast collection; command-recognizer disassembly and framewise deterministic RAM hashes remain separate future gates. Do not mislabel unverified script roots as verified move inputs.



Audited original user-supplied `87.167.zip`, SHA-256:
`01cda0b08c8427b0e91ad09a3237f29e2d9c57cdd6a510be259ad7954893ad14`.

The archive is a sealed `XM8764V2` version-1 capture: FINAL records **6395 frames, 2 sessions, 14 segments, F9_STOP**, with 16 ordered host events. Fourteen distinct full FBNeo `.fs` checkpoint files and 14 matching native `XM8761R1` inputs exist. All input payloads are correctly sized, have a single matching 29-slot layout and sequential frame indices. ZIP CRC passed. The state files have the correct FBNeo header; native state *loading* has not been exercised here.

The 39 ROM action-`2E` observations in `rom_super_observed.tsv` all match `rom_super_catalog.tsv` and the corresponding entries of `native_super_reverse_engineering.json` for **fighter IDs 0C, 0E, 18, 1E**. This yields **18 distinct script roots**. Of these observations, **13** are marked `PHYSICAL_CANDIDATE`: all 13 correspond exactly to the captured attack-button bytes at the previous movie frame, using the native CPS2 fighting input layout in `d_cps2.cpp`. The other **26** report no recent physical attack and **must not be treated as independent user commands**.

ROM frame to monotonic movie frame offsets: session 1 **-851** and session 2 **+3547**, proving that the recorded log can be joined to the two-session input timeline despite the host reset. The ROM catalog has 261 candidates while the JSON inventory has 238; 23 catalog-only entries belong to fighter ID `26`, and are not ready for move mapping.

**No move input is certified solely by a ROM action-2E/script match.** Decode the actual 68000 command recognition and use button edge/direction timing before adding a moveset entry.

## Repeatable verification

On branch `feature/dhc-mvc2-stage87` with the captured TEST folder in `test\87.167`:

```bat
python dev\dhc\tools\VERIFY_REPLAY_V2.py test\87.167\replays\session_0001
python dev\dhc\tools\VERIFY_DHC_ROM_ACTIONS.py test\87.167
```

The new second script is **read-only**: verifies the archive's ROM-event/catalog/input-byte relationship and optionally compares a fresh replay action trace.

After compiling the existing 87.167 test executable using the permanent `BUILD_TEST.bat`, run permanent `RUN_REPLAY.bat`. The existing replay launcher now automatically runs the ROM-action verification gate at playback completion and returns a nonzero result if the replay or gate fails. The GitHub feature branch itself had only the 87.166 TEST binary at audit time; **no executable runtime test was performed in this validation**.

Then check the resulting action trace with:

```bat
python dev\dhc\tools\VERIFY_DHC_ROM_ACTIONS.py test\87.167 --replay-results test\87.167\replay_results
```

**Acceptance before whole-cast recording:** the second verifier command now requires a latest-run marker, successful loads of all 14 checkpoints, runtime status `PLAY_END complete_frames=6395` without `REPLAY_DIVERGED`, and the captured vs replayed 39 ROM events identical (fighter, ROM frame, movie frame, selector, script, animation, physical input history). Replay-action equality is still a weaker guarantee than per-frame RAM/state hashes.

**Preservation fix:** the replay host's status writer was changed to avoid appending to captured session logs during playback. The permanent `RUN_REPLAY.bat` now writes to `test\87.167\replay_results\replay_v2_status.log`, with a per-run marker. Rebuild the 87.167 TEST executable to receive the fix. Keep the original ZIP immutable regardless. No gameplay/DHC command mappings were changed in this audit.
