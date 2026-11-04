# XMVSF 87.160 — Autonomous native ROM super discovery (read-only)

## Proven baseline preserved

The supplied `87.159.zip` verified TWO native Ryu -> Ken DHC sequences.
No DHC gameplay, actor input, tag timing, Ken execution, or outgoing
projectile cutoff has been changed in this stage.

| attempt | Ryu 3200 tag | ROM point swap to Ken | kickless Ken 2A08 | Ken 2E00/D6722 |
|---|---:|---:|---:|---:|
| 1 | frame 1842 | 1856 | 1857 | **1865** |
| 2 | frame 2303 | 2318 | 2319 | **2327** |

The source's `XM87159Projectile` old-object hitbox cutoff is retained.
The stage 87.159 projectile log recorded 57 offensive clear operations
among the two DHC windows. This does not guarantee an absence of every
possible mid-frame hit; live ROM collision timing remains authoritative.

## Warning clean-up

Fix the actual misleading-indentation sources, rather than suppressing
all `-Wmisleading-indentation` diagnostics:
- `xmvsf_stage87_140_super_chat.h`: separate the one-line direction
  returns and the condition that skips duplicate motion-history samples.
- `test_dhc_super_chat_140.cpp`: remove the default lambda argument
  requiring a C++14 extension under the C++11 CI compiler.
- The existing narrow `-winwrite` console warning-line filter remains
  in `dev/dhc/BUILD_TEST.sh`. A failed compiler or make command still
  exits unsuccessfully because `pipefail` is enabled.

## Two independent layers of ROM truth

**Move identity**: original `CpsRom` host-word-swapped data pointers:
- fighter table `0x8A946 + fighter_id*2`
- action `2E` table offset
- per-character selector slots and bounded script records
- animation, script offset and script duration

This can enumerate candidate 2E scripts for EVERY fighter ID in the
original table. A script record is not, by itself, a hyper move or a
callable ROM handler. Unknown data is labeled `UNVERIFIED_ROM_2E_SCRIPT`
and NEVER automatically selected for DHC.

**Command identity**: the decrypted `CpsCode` 68000 executable code
plus actual controller transitions when the original ROM starts a move.
The native command-matching code must be disassembled / traced to verify
each motion/chord against the discovered action and selector. An input
observed near an action is a *candidate correlation*, not automatically
proof of the underlying command. Team-hyper / Variable Combination
entries must not be treated as single-fighter supers.

The existing verified catalog entries are:

| fighter | ROM action | script | command |
|---|---|---|---|
| Ryu `12` | `2E08` | `0D3070` | QCF + two punches |
| Ryu `12` | `2E00` | `0D2CFC` | QCB + two kicks |
| Ken `14` | `2E00` | `0D6722` | QCF + two punches |
| Ken `14` | `2E02` | `0D6976` | QCF + two kicks |

Ken's `2E06 / 0D6B3E` is a Variable Combination reference and is
explicitly marked `EXCLUDE_VARIABLE_COMBINATION`.

## What running TEST now does automatically

No additional launcher names. Pull the feature branch, use
`BUILD_TEST.bat` then `RUN_TEST.bat`. After loading xmvsfu:

1. Generate `test/87.160/rom_super_catalog.tsv` by traversing
   action-2E native script candidates in the original data ROM.
2. Generate `test/87.160/rom_super_observed.tsv` for actual live
   original-ROM 2E entries, including immediately preceding physical
   input chord and tentative directional motion. It marks TEST-injected
   commands separately from user input to avoid false inference.
3. Save the decrypted native opcode and ROM data byte arrays to
   `local_rom_dump/`, explicitly ignored by Git. These are *local
   private* ROM-derived files and are NOT committed or included in
   a normal diagnostic ZIP.
4. After the game closes, the existing `RUN_TEST.bat` invokes
   `dev/dhc/tools/DISASSEMBLE_SUPER_MOVES.py`. The resulting
   `test/87.160/native_super_reverse_engineering.json`
   combines ROM candidate scripts, live events and disassembly
   landmarks. If Python or Capstone are absent, native logs still work
   and the JSON describes the unavailable disassembly.
5. Decrypted instruction disassembly uses optional Python `capstone`.
   No dependency is installed automatically. Known dispatch PCs such as
   `01038E` and `013B8E` are starting landmarks, NOT assumed to
   be the complete command recognition system.

## Propagation rule

Do not generalize the live DHC implementation from Ryu/Ken by
guessing another fighter's script offsets. For each new pair, require:

1. Native point/reserve and 3200-like outgoing tag behavior recorded.
2. Fighter-specific incoming tag and nonattacking cancel verified.
3. Reserve super command identified from actual 68000 handler,
   independently matched against observed inputs and native 2E script.
4. Outgoing projectile collision ownership isolated as in 87.159.
5. Repeatable real-game tests before promoting the mapping to the
   gameplay DHC registry.

Only step 1 of *move discovery* is automated by 87.160. Native input
decompilation is now enabled by private opcode capture, but its decoded
character-specific command tables still require analysis. The 87.160
code **never executes newly enumerated supers**.

CI: compile existing DHC tests and the new C++11 isolated ROM inventory
test; smoke-test the offline Python ROM parser against synthetic ROM
tables. Native Windows gameplay must still be tested in the local
TEST executable. Stable is untouched.
