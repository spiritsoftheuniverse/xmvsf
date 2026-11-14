# XMVSF 87.331 — fixed byte order for Morphus opcode mapping

**Branch:** `feature/dhc-morphus-endian-87-331`, forked from `feature/dhc-morphus-locator-87-330`. 
**Primary input:** actual uploaded `morphus_opcode_locator_330.tsv` (22-site, 1,048,576-byte `xmvsfu` decrypted CpsCode).

## Proven from the diagnostic

The 87.330 locator reported 0 hits at every site. However the six collision signatures have **exact first-six-byte agreement at their original European addresses after byte reversal of each 68000 16-bit word**:

- `$009EE6` expected `4A2E02176704`, observed `2E4A17020467`
- `$009F74` expected `4A2DC2176704`, observed `2D4A17C20467`
- `$009FB6` expected `4A2DCA176700`, observed `2D4A17CA0067`
- `$00A02E` likewise
- `$00A0C0` expected `4A2DC6176704`, observed `2D4A17C60467`
- `$00A102` expected `4A2DCE176704`, observed `2D4A17CE0467`

**Important:** only the first six bytes were logged in 87.330. The new 87.331 locator checks the **full expected signatures**, not merely these observed prefixes. The key `$011DB0` incoming Hyper and `$013AA8` DHC chain **still differ at their European positions even with proper word byte order**; their US equivalents are not identified by this evidence.

## Implementation

- New `xmvsf_stage87_331_morphus_endian_locator.h` checks `CpsCode[address+(k^1)]` against canonical big-endian M68K source pattern byte `k`.
- Scans 46 total hooks (22 original + 24 additional exact source guards from the supplied Morphus native-hooks-2p.json).
- Logs full expected-match status, candidate-count and up to 8 candidate addresses, and delta only when unique.
- Opt-in: `XM87331_MORPHUS_LOCATOR=1`; writes `test/87.331/observer/morphus_opcode_locator_331.tsv`.
- Standalone `dev/dhc/tests/test_morphus_endian_locator_331.cpp`; not part of the existing native BUILD_TEST pipeline.
- No opcode mutation, no change to DHC behavior, no new build gate, no Actions, no Stable overwrite. Version 3 remains untouched.

## Next

Collect the 87.331 TSV, isolate the incoming Hyper and DHC-chain **unique candidate sites**, and compare neighboring code / executed PC traces. Duplicates are explicitly not trusted. Even a unique signature is not permission to install the hook until surrounding 68000 instructions, original control flow, displaced instructions, stack/register/A5 invariants and hyper meter/cinematic entry are verified.
