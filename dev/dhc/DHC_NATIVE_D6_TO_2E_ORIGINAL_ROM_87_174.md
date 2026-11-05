# XMVSF Stage 87.174 — Native D6 → phase zero → action 2E

**Status: five original-ROM static source paths verified; no gameplay code changed.**

This pass uses the user's private original 1 MB decrypted CPS2 opcode image and 3.5 MB program-data image, plus the derived Stage 87.170 canonical and Stage 87.171 alternate-matcher catalogs. The analyzer **does not ship or commit any proprietary ROM/opcode bytes**. It reads the private images and outputs derived addresses, selectors, and SHA-256 verification hashes.

## New result

The accepted input matcher does not jump directly to animation script `2E`. Instead, character-specific code sets command state `$D6(A6)` and mode `$A0(A6)=2A`; later, a native two-level dispatch indexes `$D6` to a character-specific handler and `$08(A6)` to that handler's phase. Its phase-zero body selects native action `2E` and D1 selector, then jumps to the original generic dispatcher at `01038E`. The ROM dispatcher's original character/action tables choose the corresponding animation script.

| Fighter | Input matcher call | Accepted D6 | D6 dispatch table | D6 target | Phase-zero target | Original action/selector setter | Original ROM script |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Ryu | `0329EE` → `013C30` | `0A` | `0337C8` | `033D92` | `033DA6` | `033E0C`: `2E08` or conditional `2E0C` | `0D3070` / `0D31A8` |
| Ryu | `032A4E` → `013C30` | `0C` | `0337C8` | `033F0E` | `033F28` | `033F80`: `2E00` | `0D2CFC` |
| Ken | `0346D0` → `013C30` | `0C` | `035364` | `0359F6` | `035A10` | `035A4E`: `2E00` | `0D6722` |
| Ken | `03472E` → `013C30` | `0E` | `035364` | `035C42` | `035C5A` | `035CB0`: `2E02` | `0D6976` |
| Akuma | **`04158A`** → alternate `014392` | **`10`** | `042318` | `042C0E` | `042C20` | `042C54`: `2E0C` | `0EB986` |

The Akuma call site is **04158A**, not 04158C (which is the address of the first word of the JSR's absolute target operand). The 87.171 alternate descriptor lives at `043830` and carries state `D6=10`. This is the correct *static source chain* for the observed Shun Goku Satsu script, not the 87.173 canonical Akuma `D6=0C` example. Akuma Tenma Gou Zankuu remains excluded from all DHC enabling.

### Native two-level indexed control flow

First-stage dispatch uses original opcode signature `7000 102E 00D6 303B 0006 4EFB 0002`: zero D0, load actor `D6` into D0, fetch a word offset from a PC-relative table indexed by D0, then jump through that offset. D6 is a **byte offset into a word table**, not a command index requiring multiplication.

Second-stage dispatch uses `302E 0008 303B 0006 4EFB 0002`, or the variant with D1 (`323B` and `4EFB 1002`). The actor's native `$08(A6)` selects a phase index; phase **zero** selects the verified phase-zero body.

**Important PC arithmetic:** both indexed JMP variants resolve their displacement relative to the first word after the JMP extension. For Ryu's D6=0A case, the table word at `0337D2` holds `05CA`. The target is `0337C8 + 05CA = 033D92`, not `033D90`. The Stage 87.174 synthetic test explicitly checks this off-by-two behavior.

### Caveats and safety

- Static evidence connects five particular matcher sites, D6 assignments, mode-2A assignments, both dispatch levels, original action/selector opcodes, and ROM script pointers. It **does not demonstrate the entire CPU call chain in a live emulator, every branch guard, meter eligibility, or incoming-partner tag state**.
- Ryu D6=0A selects action `2E08` by default, or `2E0C` if the actor byte `$A4(A6)` equals `04`. The two selectors resolve to **different** scripts and must not be flattened into one move-name mapping.
- Ryu D6=00, audited earlier in 87.173, is a different command state. It must not be mislabeled as the proven Shinkuu Hadoken path.
- The original code at `01038E` is a script/action dispatcher, **not a callable super-input recognition function**. The future DHC bridge must preserve controller edges and allow the native character handler to run, not force `D6`, `2E`, or a ROM script pointer.
- User's 29,827-frame recording is preserved; no new F9 recording was required. Stable, TEST 87.167 executable, replay, build cache, BAT launchers, and gameplay headers remain untouched.

## Run read-only tests and analysis

From `F:\xmvsf` using the existing Python installation and the private local dump files:

```bat
py -3 dev\dhc\tests\test_dhc_d6_to_2e_174.py
py -3 dev\dhc\tools\ANALYZE_68000_D6_TO_2E_PATHS.py --opcodes local_rom_dump\xmvsfu_decrypted_opcodes_host_swapped.bin --rom local_rom_dump\xmvsfu_program_data_host_swapped.bin --canonical-json test\87.167\decomp\68000_command_descriptor_audit.json --alternate-csv test\87.167\decomp\68000_alternate_matchers.csv --out test\87.167\decomp\stage174
```

Input catalog paths can be set to wherever earlier derived JSON/CSV files live. Synthetic tests do not require any ROM files. There is no new batch launcher.

## Next gate for a playable DHC bridge

Implement a **read-only live native observer first** in a separate TEST development version. At the incoming actor's frame boundary, log actor ID/team/point status, actual native input (`C0`, `C2`, `C6`, `C8`, `CA`), command-history pointer/state, matcher return, `$D6`, `$A0`, `$08`, and subsequent real `2E` selector/script. Confirm the character handler naturally reaches the proven chain at tag-in. Only after that should DHC inject any real controller **input sequence**, never script/action state. A successful static path alone is not an engine feature.
