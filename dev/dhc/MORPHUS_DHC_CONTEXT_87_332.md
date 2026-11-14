# XMVSF 87.332 — targeted native DHC opcode context

**Branch:** feature/dhc-morphus-context-87-332 (from feature/dhc-morphus-endian-87-331).

## What the actual 87.331 US diagnostic established

The uploaded \`morphus_opcode_locator_331.tsv\` contains 46 source signatures and 1,048,576 decrypted US program bytes scanned.

- 13/46 full European signatures match **at their original address**.
- 14 distinct patterns have **a single unique hit** in the entire program. Offsets: four +14, four +0, three +12, three +18.
- Other non-unique but relevant anchors support **+14** in the DHC neighborhood: team defeat/timeout/compare, round initialization, actor swap, side-0/side-1 initialization, and HUD copy.
- \`incoming_hyper_commit\` signature \`302E0052D040\` appears 63 times, and \`dhc_chain\` signature \`0C2E000400A4\` appears 159 times. The first eight hits shown by 87.331 cannot establish the correct location. The **candidate +14 addresses are only hypotheses**: \`$011DBE\`, \`$013AB6\`.
- \`snap_motion\` includes candidate \`$013C30\` (+14), but has seven global hits, so that also requires context.
- The source 2P DHC assembly \`source/engine/dhc.s\` documents original EURO hook sites \`$11DB0\` and \`$13AA8\`, native return \`$11DB6\`, and original game calls \`$125D6\`, \`$10380\`, \`$9E90\`, \`$4BF54\`. Those function addresses may differ in US. Don't reuse them without verification.

## Code change: read-only 80-byte windows at predicted sites

- New \`xmvsf_stage87_332_morphus_dhc_context.h\`.
- Opt-in \`XM87332_DHC_CONTEXT=1\` before running the existing local TEST executable.
- Logs full signature-match status at offsets +0,+12,+14,+18 and 80 actual decrypted M68K bytes around each probe (16 prior + 64 onward), normalized to big-endian. Exact instruction context can be disassembled offline.
- Focus sites include incoming Hyper commit, DHC chain, VC, motion snapshot, KO, input dispatcher, swap-adjacent controls, plus known +0/+12/+18 examples.
- Finds matches within ±128 bytes separately, rather than trusting the first eight global results. No CPU callbacks, ROM writes, original ROM modifications, A5 changes, additional gameplay build gates, GitHub Actions, cache deletion, or Stable updates.

## Local run

\`\`\`bat
set "XM87332_DHC_CONTEXT=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
\`\`\`

Upload \`test/87.332/observer/morphus_dhc_context_332.tsv\`.

A **MATCH** at +14 is valuable evidence, not authorization to patch. We still need exact M68K instruction boundaries, replaced-instruction replay, actor context, safe register/A5 preservation, native execution and the full two-sided DHC acceptance matrix. This checkpoint **does not claim completed DHC**.
