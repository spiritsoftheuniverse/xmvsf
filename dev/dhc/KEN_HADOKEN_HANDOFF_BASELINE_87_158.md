# 87.158 — Restore the actual Ken-Hadoken arrival, not a team Hyper Combo

**Source of truth:** `87.151.zip` from user, not a later archive in which Ken
only enters from a normal HP+HK tag. The immediate game objective is
**Ryu Shinkuu Hadoken → DHC tag-out while Ryu is still in super →
Ken arrives without an entry kick → Ken's chosen Shoryureppa or
Shinryuken**. A Variable Combination is not part of the sequence.

## Verified working 87.151 frames

| frame | point | native action | controller / script | actual result |
|-------|-------|---------------|---------------------|---------------|
| 1857 | Ryu 0012 | 2E08, A0=002A | +4E=001C (28), +256/+259=08 | tag controller armed *during Ryu super* |
| 1858 | Ryu 0012 | 3200, A0=2A00 | +D6=14, 0D1DAC | native Ryu tag-out accepted |
| 1872 | Ken 0014 | 2A0C | 0D5702 | point/reserve ownership swap |
| 1873 | Ken 0014 | 2A0C→2A08 | 0D567E, D6=12 | Ken incoming kick suppressed |
| 1875 | Ken 0014 | 2C00 | 0D5C46 | **KEN HADOKEN**; this is the original near-working end point |

Never call this a proven Ken hyper startup. The Ken Hadoken was a genuine
native special, and its successful *arrival timing* is what matters.

## What regressed

In 87.155-87.157, the active handler waited until Ryu's +4E reached 8,
based on a separate 87.150 observation, disregarding that the successful
Ken-Hadoken archive had already accepted +4E=28. 87.157 user capture:
at frame 1932 (+4E=37) and 2795 (+4E=39), Ken's super input was staged,
but the implementation waited until 1958 (+4E=8) and 2823 (+4E=7).
At those frames +256 was consumed, **A0 remained 002A**, so the ROM
did not select native tag action 3200. Both attempts timed out at
1971/2836; Ken never became active from the experimental DHC.

Adding a normal-tag sequence after the outgoing super finished was
NOT progress on this specific cancellation. It was an unrelated,
slower control proof, and must not be the baseline or active route.

## 87.158 scope

- Restore the 87.151 controller staging window +4E<=0x1C (28).
  When an input is queued earlier (e.g. at +4E=37), observe without
  mutation until +4E enters the known 28-frame point.
- Restore the original 87.151 accepted controller predicate:
  Ryu +A0=2A00, +256 consumed to 0, action=2E08.
  No posthoc dependency on extra +259/+D6 conditions in this route.
- Retain Ryu ROM 3200 script and controller-owned point/reserve swap.
  Retain first-frame Ken 2A0C→2A08 kickless suppression.
- **Replace only the unwanted downstream Ken Hadoken behavior.**
  The existing faster Ken QCF+2P / QCF+2K input attempt remains
  **EXPERIMENTAL** until a live 87.158 recording shows Ken actually
  entering action 2E00 / 0D6722 or 2E02 / 0D6976 promptly after arrival.
- No active team-hyper (Variable Combination), no inserted HP+HK
  normal tag, no guesses based on an archive with no Ken arrival.
- Keep `BUILD_TEST.bat`, `RUN_TEST.bat`, warm object cache, and Stable
  unchanged except that standard TEST launchers target 87.158.

## Live validation hierarchy

1. **First priority**: see `DHC_ROM_TAG_3200_AFTER_CONTROLLER_ACCEPT`,
   `REAL_ROM_POINT_RESERVE_SWAP` and
   `DHC_KEN_2A0C_ATTACK_SUPPRESSED_IMMEDIATE_2A08` in one attempt,
   matching 87.151. If any fail, do not diagnose the Ken super.
2. Then inspect whether the native QCF input is accepted. Desired Ken
   ROM actions are 2E00/D6722 (Shoryureppa) or 2E02/D6976
   (Shinryuken). A 2Cxx action is a special, not a successful hyper.
3. Confirm repeatable attempts, preserving Tournament rules and Stable.

GitHub isolated C++11 tests verify staged 28-frame controller and ROM
handoff against synthetic CPU-state samples only. They do not establish
that the Windows CPS2 game actually performed the cancellation.
