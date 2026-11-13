# XMVSF TEST 87.352 — native input rejections and Cyclops ricochet cleanup

Branch: feature/dhc-morphus-precommit-87-334. Original US xmvsfu, Version 4, and the same four native DHC opt-ins. Stable, Version 3, regular native Supers, and other fighters are unchanged. No new mandatory build tests or GitHub Actions.

## Ground truth: observer(7).zip

87.351 has 862 DHC rows, 11 real original incoming native handoffs (8 original 2C, 3 original 2E), and 175 NATIVE_342_COMMAND_NOT_CLAIMED rows. Of the latter, 22 occurred when the outgoing native Hyper window and DHC eligibility were both true. The capture specifically includes P1 Cyclops original D6=02, action 2C12, before an outgoing DHC into Sabretooth at frame 5390. All 11 successful inputs were mapped QCF; the observation does not prove every missing reverse/half-circle command identity.

Original X-Men vs Street Fighter M68K helper update at $010358 uses object +52 as maker fighter and +54 as its owning fighter slot (Daniel Plas Rivera, XMvsSF 3v3 technical writeup). The original owner slot may switch during DHC; an old ongoing helper can remain while the source Super is interrupted. Native helper identity/ownership in the current US revision must still be validated through gameplay.

## 87.352 experimental correction

On actual Version-4 native DHC CLAIMED while outgoing Cyclops is D6=02, action 2C, capture the already-linked native helper addresses whose +52 maker is Cyclops and +54 is the outgoing actor slot. This does not scan or suppress OAM artwork. Only after the native M68K COMMITTED original swap, watch the same linked objects. If still active at 210 emulated frames after DHC, with the same maker and ownership still attached to one of that fighter's slots, clear their active byte +03 and offensive box/gate IDs +70/+72/+82. Do not detach a linked object pointer, modify CpsCode, change a new fighter's projectiles, or interfere with normal solo Super. Abandon a rejected handoff, a vanished link, or a Cyclops return to point. No collision removal during the first 3.5 seconds.

Events in the existing unified TSV: NATIVE_352_CYCLOPS_SOB_HELPERS_TRACKED and NATIVE_352_CYCLOPS_SOB_ORPHAN_HELPERS_RETIRED. If tracked=0, this exact ownership path has not been observed and no FX is touched. The effect's actual disappearance is NOT proven by the synthetic test or the DHC actor observer.

The 87.351 capture has 22 eligible input chords without a recognized incoming motion. Instead of inventing native supers from arbitrary chords, the same rejected-command TSV now reports raw and relative direction, facing, and actual ROM gates +03/+221/+257/$FF500A so the next capture separates missing directional completion from original native eligibility.

Standalone C++11 -Wall -Wextra -Werror lifecycle regression passes both active-helper retirement and negative paths (unrelated fighter, recycled pointer, denied tag, returning owner). Full Windows build and gameplay have not been run here.

## Test

With the four native flags enabled, run BUILD_TEST.bat and RUN_TEST.bat xmvsfu, choose Version 4, DHC out of Cyclops original Super Optic Blast into Sabretooth. Observe whether stray ricochet effects disappear after the native DHC. Retry the previously missed PP/KK inputs; inspect test/87.352/observer/dhc_unified_274.tsv. If an effect still remains and tracked=0, the native object collection needs a new live capture before a different object is modified.
