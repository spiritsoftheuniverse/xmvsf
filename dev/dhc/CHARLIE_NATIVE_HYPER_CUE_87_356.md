# XMVSF TEST 87.356 — Charlie QCB+KKK native Super cue gate

87.355 opened Charlie QCB+KKK only once the outgoing super's final DHC eligibility gate was true. 87.354 had correctly stopped all normal-play DHC sniffing to prevent an unwanted Chun-Li entry during Charlie's Flash Kick. Charlie's short QCB input may begin while the original 68000 Hyper is already running but transiently fails the more restrictive DHC cancel gates. Those first D/DB steps were cleared and the final KKK could not match.

The earlier observer(9).zip, recorded before these revisions, contains 26 Charlie-reserve rejected chord events with activeHyper=1 but window=0. This is historical evidence of the gate timing, not a live reproduction of 87.355 and not proof that all 26 inputs formed valid QCB commands.

87.356: for incoming Charlie only, permit native QCB input history during the SAME actor's ROM-verified original Super (2A mode, original 2C/2E action, tracked live actor/D6 and contiguous prior frame), even if the final DHC cancellation gate is not yet satisfied. Never claim, suppress input, tag or change the reserve until the original eligible predicate is TRUE. A fresh KKK edge, or that edge held for at most 10 native samples, and the completed genuine QCB must still agree.

For other reserve fighters, the 87.355 gate remains unchanged, including Sabretooth's already working HCB+KKK. The original native D6 table remains unchanged. Outside an original verified Hyper, native DHC input history is cleared every normal-play frame and the full-round DHC input monitor stays disabled.

Existing C++11 local fixture adds both facings: D and DB during native Hyper with eligible=false; Back+KKK still cannot swap; full eligibility the next frame allows Charlie D6=06, while pre-Hyper held KKK and normal-play history remain rejected. No new mandatory build gates, GitHub Actions, Stable, or Version 3 changes.

Only source and launcher version move to 87.356. Local Windows BUILD_TEST.bat and in-game native ROM are not executed here. Verify with deliberate Charlie QCB+KKK during outgoing Hyper and a separate no-input Charlie Flash Kick case.
