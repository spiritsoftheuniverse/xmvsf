# XMVSF TEST 87.253 — HP=1 red markers, generic QCD and visual hitstop

## Reported gameplay
The 87.252 trace shows damage markers progressing until the training dummy reaches HP=1 at host 1325. With native Training minimum-health protection, subsequent impacts cannot satisfy `health<previousHealth`; the old recorder will stop showing red ticks even when actual hits continue. Cyclops QCD+PP aimed super is reported as not reading, while other projectile supers have improved.

## Generalized contact evidence
1. Preserve HP loss as the primary confirmation.
2. Sample the game's observed combo value for each fighter, and accept a rising combo counter as a hit only while the owner has an offensive box, previously linked projectile or running accepted native super. The opposite side's own concurrent offense disqualifies ambiguous counter ownership.
3. When no HP or combo increment occurs, recognize a defender `+85` hit-stop **reload** (not continuation or initial cinematic freeze), gated by a sampled attack box or an owned live projectile. This can mark actual hits at minimum HP.
4. Preserve one red marker per *host observation tick* on the owning card: several genuine contacts can share the same native owner frame during frozen callbacks. De-duplicate overlapping evidence on a single host tick.
5. For diagnostics, separate cumulative `hp_proofs`, `combo_proofs`, `stop_proofs`, and `unassigned_proofs` in the opt-in `xmvsf_frame_clock_253.tsv`.

## Cyclops QCD+PP and other supers
- Support 6-3-2 / 4-1-2 as an actual generic QCD motion, labeled `QD`, independent of character ID.
- Native action/linked projectile ownership remains unchanged from 87.252. This motion fix corrects the tag and input classifier; it does **not by itself** prove the unrecorded super's full trace has been resolved.
- C++ tests include generic QCD recognition, HP-clamped combo-proof red markers, native stop-reload backup, and a boxless cinematic that must not produce false red ticks.

## User-requested hit-stop visualization
- Native `playerFrame[]` and S/A/duration calculations stay pause-aware and therefore do **not** include hit-stop in the measured counts.
- Independently maintain an always-advancing, per-player host-observation clock (`wallFrame[]`). Preserve a bounded 4,096-tick ring of `{visual tick, native tick, stopped}`.
- Frame-data charts render using host observation ticks so stopped frames occupy visible horizontal space instead of vanishing. Draw each held frame in **dark neutral gray (43,48,57)** in both the hitbox and action-duration lanes, darker than normal idle gray. For cards with unknown action timing, draw the second lane in neutral gray and darken its holds as well.
- Preserve native timing for labels and measurements; map the IN, ON, OFF, SP, END and red-contact markers onto the host axis. Store `contactWallFrames` so repeated contacts during the same frozen native frame remain visibly distinct.
- The native gameplay and controller pipeline is untouched. The new visual clock is read-only and cannot advance fighter physics or stop counters.

## Verification boundaries
Repository-only code and structural validation; Windows-native `BUILD_TEST.bat`, unit test compilation, and actual Ryu/Ken/Cyclops gameplay still require local execution. Source-control only; no GitHub Actions, Stable overwrite, or simulated gameplay claims.
