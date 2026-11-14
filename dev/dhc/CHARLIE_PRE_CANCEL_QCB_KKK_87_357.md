# XMVSF 87.357 — Charlie QCB+KKK input / native DHC authorization

Observer(10).zip documents one real incoming Charlie QCB+KKK -> native D6=06 -> 2C24 at frame 2819, proving the exact original US command and Hyper handler do exist. The capture also has 41 rejected button events, 30 with Charlie as incoming reserve, but displayed rejected masks are punches, not KKK. That does not mean his other failed KKK inputs were ever observed by this logger: the earlier gate skips those events entirely.

The excessive restriction in 87.356 was the EXTRA Charlie-only native ROM tracker live / previous-frame / exact D6 requirement before even RECOGNIZING QCB directions, over and above the existing final M68K DHC cancellation authorization. In 87.357 Charlie can enter his native QCB+KKK during any outgoing original 2A/2C or 2A/2E Hyper state. No normal play sniffing and no arbitrary DHC shortcuts.

When the actual physical Down, Down-Back, Back + new KKK completes inside that Hyper but the DHC cancel flags are not yet eligible, retain one pending Charlie D6=06 request, bound to the same outgoing native D6. Button release does not erase it. There is no invented 10-, 30-, or 50-frame deadline: native Hyper exit clears it, changed actor clears it, and a changed original outgoing D6 invalidates it. The ORIGINAL native cancellation, stock, live reserve, original 68000 tag swap and ROM route remain required before Charlie can actually enter. Do not intercept buttons for a pending uncommitted request.

Existing observer TSV now gains two bounded events: NATIVE_357_CHARLIE_QCB_KKK_CUE_HELD when physical QCB+KKK is entered before eligibility; NATIVE_357_CHARLIE_QCB_KKK_DEFERRED_CLAIM when native Hyper cancellation authorizes that already-real command. Continue to require NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED and MORPHUS_349_ACTUAL_US_2C_HYPER for an actual successful DHC.

Only Charlie's incoming cue changes; Sabretooth, Chun-Li, Stable, Version 3 and normal gameplay are untouched. A local-only C++11 regression fixture tests both directions, KKK release across 55 native Hyper samples before eligibility, stock and native cancel authorization, changed outgoing D6 and normal-play reset. No added build gates, GitHub Actions or Stable overwrite. Windows build/in-game behavior not run here.

Build/test from Windows: BUILD_TEST.bat; RUN_TEST.bat xmvsfu; Version 4 with existing four opt-in flags. Inspect test/87.357/observer/dhc_unified_274.tsv for two new events and authentic original tag/2C follow-through.
