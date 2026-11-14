# XMVSF 87.360 — native full-Hyper DHC acceptance

## Actual observed evidence

The new observer(20261114-162352).zip has original incoming Charlie QCB+KKK D6=06 at frame 2758 followed by native 2C24 at 2759. It does not include Chun-Li as a point fighter, so it does not directly establish her late window.

Earlier real observer(9).zip does: Chun-Li QCF+KKK D6=08 remains in the original native 2A/2E Hyper through 2E0A, 2E0E, 2E10 for as long as 536 observed game frames (15379 through 15914). In the same capture 26 rejected button events for Chun-Li show activeHyper=1, window=0, eligible=0 despite living partner, stock=3, and gate221, gate257, gate500A all zero. The script-bounded 87.189 2E tracker is losing the continuing real native super.

## Source change

The original native Hyper state itself owns the DHC acceptance lifetime. Current native mode must be 2Axx, action 2Cxx/2Exx, and original D6 nonzero. This original 68000 action/controller state spans all original native root/continuation actions; it naturally ends on native Hyper exit. This replaces the artificial narrow animation-stage/next-script window for ALL fighters. The old tracker is still maintained for diagnostics and the existing separate pre-CPU 3P shield; it no longer vetoes the legitimate native input window merely because 2E0A, 2E0E or 2E10 advances scripts.

Normal fighting remains excluded: no native DHC motion parsing outside original native 2A/2C or 2A/2E, and all old input history is cleared every non-Hyper frame. An actual incoming fighter motion and real PP/KK/KKK chord, living reserve, meter, and native tag-control guards are still necessary. The bridge invokes the original 68000 exit/swap and real original D6 handler; no synthetic move/action, input alias, controller 8, forced tag or arbitrary stopwatch.

Charlie QCB+KKK continues using the facing-locked detector introduced in 87.358/359; no new Charlie lock. Chun-Li D6=08 QCF+KKK is available at any point while her native Senretsukyaku is still executing under original 2A/2E. No special-case duration hardcoded.

NATIVE_360_HYPER_INPUT logs genuine PP/KK inputs during native Hyper when Charlie/Chun-Li is active or incoming, including exact action, mode, D6, old script window, new native lifetime window, eligibility and other gate flags. This is bounded to real Hyper button edges and never collects normal-play inputs.

Existing standalone local-only fixture now verifies the 536-frame span of Chun-Li's original 2E stages, Charlie 2C family, negative normal action/controller cases, late incoming Chun-Li QCF+KKK and Charlie QCB+KKK actual D6, without changing prior tested input mappings. No extra mandatory build gates, GitHub Actions, or Stable/Version 3 modifications. The native Windows build and gameplay must still be exercised with BUILD_TEST.bat and RUN_TEST.bat.
