# XMVSF Stage 87.173 — Real native command acceptance, not forced move selection

**Executed on the user's original CPS2 program-data ROM and decrypted 68000 opcode images.** This is an incremental, read-only reverse-engineering proof. No executable, source gameplay or stable build was changed. All raw proprietary binary images remain out of this ZIP and the repository.

## New verified results

- Rechecked **all 112** canonical input matcher call sites from Stage 87.170. Each is an original `JSR $013C30` immediately followed by a 68000 **`BEQ`** branch. The return condition matters. This is a *native recognizer result*, not an unconditional action-ID injection.
- Decoded exact success/rejection code for **four** specific character command paths: Ryu at **03298A**, Ken at **0346D0** and **03472E**, and Akuma at **0412CA**.
- On the tested success continuations, the actor's native command state **`$D6(A6)`** is updated and native mode **`$A0(A6)=2A`** is entered. **Those are not writes of action `2E`.** Thus command acceptance and later super launch are separate operations and must remain separate in DHC.
- Confirmed original **`014488`** button-group checker uses raw actor-relative `C0` (current), `C2` (previous), and `D0` (saved input word). It returns 1 precisely when `(((D0 ^ C2) | (C2 ^ C0)) & mask) == mask` (all operands 16-bit). Akuma's code calls this checker first with `mask=0070`, then with `mask=0700` on the second branch; these mask choices depend on earlier state conditions and **do not identify a named super**.
- The bounded forward inventory finds **102** direct D6 immediate signatures and **106** mode-2A immediate signatures among 112, but those are a *candidate substring scan*, not 102/106 fully proved executable paths. Special case routines, conditional register writes and indirect control flow require separate analysis. The 112 `JSR + BEQ` sites and the four selected paths are independently signature-checked.

## Exact checked pathways

| Fighter | Shared matcher call | Rejection destination | Successful state assignment | Mode assignment | Raw descriptor |
|---|---|---|---|---|---|
| Ryu | `03298A` | `0329DC` (return 0) | `D6=00` at `0329A4` | `A0=2A` at `0329B6` | `0344C6`: QCF-shaped `0004,0006,0002`, mask `0070` |
| Ken | `0346D0` | `03460A` (return 0) | `D6=0C` at `0346E6` | `A0=2A` at `0346F8` | `036590`: QCF-shaped `0004,0006,0002`, mask `0070` |
| Ken | `03472E` | `03460A` (return 0) | `D6=0E` at `034744` | `A0=2A` at `034756` | `0365A0`: QCF-shaped `0004,0006,0002`, mask `0700` |
| Akuma | `0412CA` | `041428` (return 0) | `D6=0C` at `041316` | `A0=2A` at `041328` | `0437E0`: `0002,0004,0006`, mask `0770` |

**Do not confuse `D6=0C` with action `2E0C`:** they are different actor fields. Akuma's `0412CA` descriptor has a 3-token input sequence; it has not been linked to Shun Goku Satsu's recorded five-button combination. Akuma's Tenma Gou Zankuu remains excluded.

The static control flow includes additional gates. Ryu checks native actor byte `+0340`; Ken conditionally calls `013F10` depending on actor byte `+0003`; Akuma performs additional button-group checks through `014488` and conditional actor state. A positive matcher result is **necessary but not sufficient** for the complete acceptance path.

## Implication for the DHC design

The native bridge is an **input submission plus acceptance observer**, not an arbitrary ROM PC call:

1. Capture a physical command during the outgoing fighter's super, keeping original button edges and directions.
2. Hold that command in a DHC transaction until the incoming actor is selected and the engine's native input updater can process its input. Do not copy a byte buffer into the incoming actor's `C0/C2/C6/CA` fields; those belong to the original engine.
3. Present the input through the existing CPS2 controller/port mapping and **permit the original actor-specific command handler to run** in a state where original eligibility checks may succeed.
4. Record whether the selected matcher returned success, whether `$D6`/`$A0` actually changed, and whether the engine *later* emitted an authentic `2E` action/script. Reject or unwind if any expected stage fails. Never directly set `action=2E` or run an animation script as the acceptance mechanism.
5. Preserve one-way DHC, original meter, replay determinism, no tag-in kick, outgoing-projectile offensive cutoff and normal failure recovery. None of these gameplay modifications is implemented in this read-only stage.

**Not yet proved:** proper timing to change incoming actor ownership and cancel tag-in, actor eligibility/meter state, exact Ryu/Ken handler execution at tag-in time, or the complete native path from `D6`/`A0=2A` to an eventual action `2E`. These require a live CPU/actor trace or more exhaustive static reachability analysis. A script ID is not a callable super handler. A read-only static audit cannot claim successful DHC gameplay.

## Reproduce (existing scripts only, no new batch launchers)

Run from the repository root after placing both original dumps in `local_rom_dump` and retaining the Stage 87.170 canonical descriptor JSON:

```bat
py -3 dev\dhc\tests\test_dhc_input_bridge_173.py
py -3 dev\dhc\tools\ANALYZE_68000_DHC_INPUT_BRIDGE.py --opcodes local_rom_dump\xmvsfu_decrypted_opcodes_host_swapped.bin --rom local_rom_dump\xmvsfu_program_data_host_swapped.bin --canonical-json test\87.167\decomp\68000_command_descriptor_audit.json --out test\87.167\decomp\68000_native_dhc_bridge_gates.json
```

Synthetic regression: 1,024 combinations of the original button-chord mask logic, exact byte-order conversion, short/long conditional branch destinations and a tampered signature refusal. The full analysis independently verifies **112/112** matcher/BEQ pairs, four selected native acceptance paths, the 014488 opcode signature and six button-check examples. JSON and CSV exports contain derived addresses and interpreted metadata, not ROM dump data.

**Engine version remains 87.167.** Stable, actual DHC gameplay, F9 recordings, build cache, existing BAT launchers and all original binaries were left untouched.