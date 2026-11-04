# Stage 87.147 — controller accepted, Hadoken still owns action

## Evidence from uploaded 87.146.zip

The user reports: Ryu still fires Hadoken instead of leaving for Ken's
queued Shoryureppa; repeated attempts now work, so the global lockout is
fixed. Inspection of the native TSV corroborates both findings.

| Frame | P1 point / reserve | Actor action | Mode A0 | 256/259 | Tag timer 4E | Result |
|---|---|---|---|---|---|---|
| 1777 | Ryu 12 / Ken 14 | 2E08 | 002A | 08/08 | 0008 | DHC queue requested native tag controller |
| 1778 | Ryu 12 / Ken 14 | **2E08** | **2A00** | 00/08 | **0040** | ROM accepted controller and restarted Ryu's Hadoken script D3070 |
| 1778–1789 | Ryu / Ken | 2E08 | 2A00 | 00/08 | 0040 | Super remains owner; no tag |
| 1790 | Ryu / Ken | 2E08 | 2A28 | 00/00 | 0040 | Controller timeout restores native mode; retry unlocked |
| 2364–2377 | Ryu / Ken | same | same | same | same | Second rejected attempt |
| 2755–2768 | Ryu / Ken | same | same | same | same | Third rejected attempt |
| **2886** | Ryu / Ken | **3200** | **2A00** | **00/00** | **001E** | *Successful ordinary native tag* |
| **2900** | Ken 14 / Ryu 12 | 0000 | 2A00 | — | — | Actual ROM point/reserve identity swap |
| **2901** | Ken / Ryu | 2A0C | 2A00 | — | — | Real incoming airborne tag script D5702 |

The DHC controller routine really ran. The previous hypothesis that
controller 8 was never entered is false. However, the ROM's live super
action selector was not transitioned, so super action 2E08 resumed even
in the tag mode. On the real normal tag, native action 3200 and its
30-tick timer 001E are established.

Ryu's native action 2E08 and script D3070 vs genuine native 3200/D1DAC
are confirmed by the independent ROM-script event stream. Input matching
and prepared Shoryureppa queue are also confirmed in super_chat.tsv.

**There is no evidence of successful DHC handoff in 87.146.**

## Stage 87.147 — DHC-only bridge between two native mechanisms

1. Accept a reserve Ken QCF+PP or QCF+KK command during a ROM-verified
   Ryu first super, and request controller 8 as before. Only P1
   Ryu point / Ken reserve, and only in TEST.
2. Let the ROM consume controller 8 (256: 08→00) and switch +A0
   from pre-tag 002A to tag mode 2A00. Do NOT spoof this acceptance.
3. If +A0==2A00 and Ryu's action is still 2E08, initialize the
   native outgoing action 3200/script D1DAC using the real loaded
   CpsRom fighter script table, with native tag timer +4E=001E,
   scheduler +06=0, +08=2, +0A=0 and status +259=0.
   This is *only* performed after genuine controller acceptance
   and a genuine queued DHC; ordinary tag never enters the path.
4. Observe whether the ROM keeps 3200 and swaps point/reserve IDs
   within the existing 60-frame window. If action ownership reverts
   to Ryu 2E08, log ROM_REASSERTED_RYU_HADOKEN_AFTER_TAG_3200
   instead of waiting for a vague timeout.
5. Only if the ROM performs the real identity swap, continue with
   native Ken flight, DHC-specific kick suppression and queued
   Shoryureppa on landing. No synthetic fighter-swap is added.

This is a controlled combined native-controller+script experiment.
The ROM may depend on further state that we have not yet identified.
It is **not** proven as full DHC, and does not yet spend an additional
meter, skip portrait, freeze startup, release on first active frame,
nor generalize across the whole roster.

## Test

Pull feature/dhc-mvc2-stage87. Run the unchanged root launchers
BUILD_TEST.bat and RUN_TEST.bat. The new separate dedicated binary is
F:\xmvsf\runtime\stable\XMVSF_Test_87_147.exe.

On P1 select Ryu as point, Ken as reserve. During Ryu's real
Shinkuu Hadoken input Ken's QCF+PP (Shoryureppa). Also do a
normal tag for a control reference. Optionally try Ken QCF+KK.
The failed-attempt retry fix remains active.

Send these files from F:\xmvsf\test\87.147:
- dhc_native_handoff.tsv — native controller and tag action state;
- rom_dispatch.tsv — ROM-verified super/tag/swap transitions;
- super_chat.tsv — selected reserve command and visible state messages.

Important outcomes:
DHC_ROM_TAG_3200_AFTER_CONTROLLER_ACCEPT: bridge set a real outgoing
tag script with native timer.
ROM_REASSERTED_RYU_HADOKEN_AFTER_TAG_3200: ROM super controller still
overrides the outgoing action; further native state/callsite analysis
needed, not another input proxy.
REAL_ROM_POINT_RESERVE_SWAP: native fighter ownership actually changed.
DHC_NATIVE_SUPER_REPLACES_KICK__UNVERIFIED_FX: incoming queued Ken
startup attempted after native landing (not proof of attack hitboxes).

Isolated GitHub C++11 tests simulate ROM controller acceptance and
verify script/timer/partner invariants; Windows gameplay remains
unverified. Stable remains untouched.
