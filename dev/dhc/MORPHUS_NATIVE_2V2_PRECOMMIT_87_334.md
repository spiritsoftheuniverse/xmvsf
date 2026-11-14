# XMVSF 87.334 — original-input-backed 2v2 Hyper precommit

Branch: `feature/dhc-morphus-precommit-87-334`, forked from the verified US-native ABI 87.333. Version 3, Stable, F9 replay and the existing Windows warm build policy are preserved.

**Source:** Daniel Plas Rivera / Morphus56k, `XMvsSF-3v3-DHC-v1.1.1`, `source/engine/dhc.s`, `dhc_table.inc`, `dhc_commands.json`.

## The important correction

Morphus' 3v3 DHC motion assignments are **not the original fighter Hyper inputs**. For example, Ken's DHC QCF assignment selects Shoryureppa, but his native QCF+KK command selects Shinryuken. Our old 87.329 approximate motion-only filter cannot serve as a definitive native +D6 chooser for a conventional 2v2 DHC.

87.334 therefore builds a **separate table of 34 direct native command mappings** from the donor's `native_command` data: fighter ID, actual motion, PP/KK button family, and +D6 Hyper ID. For example:

- Ken QCF+PP = D6 `0C`; Ken QCF+KK = D6 `0E`.
- Ryu QCF+PP = D6 `0A`; Ryu QCB+KK = D6 `0C`.
- Magneto QCF+PP = D6 `04`; QCF+KK = D6 `06`.

This restricted table covers only the command forms the current 87.192 motion parser can express. Nontrivial source inputs (Cyclops F,DF,D; Zangief 360; Akuma's SGS; airborne Hypers) are **explicitly rejected, not synthesized**. The catalog still requires live US-ROM execution verification before native code can consume a staged +D6.

## What changed in running Version-4 code

- `xmvsf_stage87_334_morphus_2v2_precommit.h`: command mapping and a single-use, side-and-character-bound pending transaction. Strict same-family two-button filter; mixed punches/kicks are refused.
- `xmvsf_stage87_274_unified_dhc.h`: with `XM87334_MORPHUS_PRECOMMIT=1`, a proposed reserve Hyper is validated **before** the existing native controller-8 exit. It never spends stock, invents a 2E action, or overrides the ongoing outgoing Hyper. Genuine point/reserve swap verifies pending identity; incoming original-ROM 2E acceptance checks and logs the D6 value independently.
- `dev/dhc/tests/test_morphus_native_precommit_334.cpp`: exact source-form tests including the Ken QCF+PP vs QCF+KK distinction, wrong-side/wrong-identity protection, source unsupported inputs, no second consumption.
- Local `BUILD_TEST.bat`, `RUN_TEST.bat`, and shell builder version advanced to **87.334**, no additional build checks.

All new behavior is opt-in and only occurs inside the existing Version-4 experimental unified DHC mode. Enable with `XM87274_UNIFIED_DHC=1` and `XM87334_MORPHUS_PRECOMMIT=1`. It remains disabled by default.

## Scope not yet completed

The incoming direct native M68K Hyper commit is **not yet installed**. The 87.274 executor still tags, waits for natural reserve neutral, then replays the original Hyper input. The new D6 selection is host-side precommit evidence, not the final execution method.

We have verified US native hook sites `$011DBE` (incoming) and `$013AB6` (outgoing), but Morphus' original commit relies on helper calls and original team-native state beyond those hooks. EURO `$10380` corresponds to verified US `$1038E`, but EURO `$125D6` and `$4BF54` haven't been positively identified in the US ROM. The original voice JMP also changes nonuniformly (`$78BA4` EURO versus `$78BB6` US), so globally adjusting addresses remains unsafe.

Native Windows builds, actual gameplay, original-US native D6 coverage, one-stock debit, post-freeze timing and the no-kick entry remain unverified. Do **not** call this final DHC.

Local standalone C++11 source-form tests of the command resolver/pending policy have passed in the work container; the complete TEST binary has not been compiled here.
