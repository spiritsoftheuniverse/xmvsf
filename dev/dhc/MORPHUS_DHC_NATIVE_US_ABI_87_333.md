# 87.333 — verified US native DHC hook ABI, not yet a patched engine

**Branch**: feature/dhc-morphus-native-abi-87-333, from feature/dhc-morphus-context-87-332.

**Sources**: user's 87.332 \`morphus_dhc_context_332.tsv\`, original Morphus56k \`XMvsSF-3v3-DHC-v1.1.1\` ZIP (\`source/engine/dhc.s\`, \`native-hooks-2p.json\`), root repo README.

## Verified actual original US opcode map

| Role | Original EURO | US \`xmvsfu\` | Original US big-endian bytes |
| --- | ---: | ---: | --- |
| DHC outgoing chain entry | $013AA8 | **$013AB6** | \`0C2E000400A4\` |
| DHC outgoing next | $013AAE | **$013ABC** | \`674A0C2E000600A46742206E0224\` |
| Native incoming Hyper substitution | $011DB0 | **$011DBE** | \`302E0052D040\` |
| Native fall-through | $011DB6 | **$011DC4** | \`207B000A4E90\` |
| Original voice JMP (US) | $011DBC | **$011DCA** | \`4EF900078BB6\` |

The last US jump target is **$078BB6**, not simply EURO $078BA4 plus 14 (it differs by 18). Consequently it is unsafe to relocate all absolute JSR/JMP targets by adding 14. Original source \`dhc.s\` also calls EURO $125D6, $10380, $9E90, $4BF54. Their US call targets and calling contexts have NOT been verified.

The original US incoming hook exactly spans six bytes (\`MOVE.W $52(A6),D0\`, \`ADD.W D0,D0\`), preserving a clean fall-through to $11DC4 on all non-DHC actions. The outgoing six-byte hook is a native Hyper eligibility comparison (\`CMPI.B #4,$A4(A6)\`). Morphus' engine has chain ownership/state, native +D6 commands and direct incoming Hyper activation, not just a local tag animation patch.

## Code committed

- \`xmvsf_stage87_333_morphus_native_dhc_abi.h\`: static source-level guards at five original US native sites. Checks canonical M68K big-endian bytes against word-swapped CpsCode; does not modify CPS2 execution.
- \`dev/dhc/tests/test_morphus_native_dhc_abi_333.cpp\`: checks all five guarded sites, correct fall-through offsets, partial program size, and failure on tampered original bytes. Local standalone C++11 test; **not** wired into normal BUILD_TEST.
- Advance existing BUILD_TEST/RUN_TEST version from 87.332 to 87.333, with no additional build gates.
- No game behavior, Version 3, Stable, F9 recorder, existing warm buildtree, or GitHub Actions changed.

## DHC implementation design now anchored to original ROM

**Not implemented in this checkpoint:** code injection into the decrypted US opcode space, full incoming Hyper commit, native command parser and chain participation state. Build/Windows gameplay is unverified.

Port only the **2v2** parts of \`dhc.s\`, not Morphus' three-character team allocator:

1. Keep the 68000 input parser's native 14-input-sample completion rules and original D6 Hyper catalog. For 2v2 there is only one reserve. Reject unavailable commands *before any native swap*.
2. Native outgoing chain triggers only during a verified real Hyper active collision window (after freeze, within eight collision frames), on human point, with one additional level, and with one-chain reentrancy guard. Do not substitute fake action IDs or force charge/360 moves.
3. Reuse XMVSF's genuine tag-out / swap. Native incoming commit at US $11DBE runs **before** the ordinary tag-in startup, and transfers the prequalified D6 Hyper ID. Maintain A5=FF8000 and original register/stack preservation.
4. Let incoming fighter's original native 68000 Hyper startup own meter debit and superfreeze. Preserve outgoing projectiles and avoid clearing live hit objects.
5. Validate full ROM call-site mapping, original opcode guards, Version-4-only activation, native frame/replay acceptance for both sides and all selectable supers, no duplicate Hyper or input leak.

**No new 87.334 diagnostic is requested from the user by this change.** The address hunting is resolved. Remaining work is the native 2v2 assembly/runtime integration and game proof, not another broad ROM signature sweep.
