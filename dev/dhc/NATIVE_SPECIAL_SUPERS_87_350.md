# XMVSF TEST 87.350 — real Raging Demon and Final Atomic Buster inputs

Branch: feature/dhc-morphus-precommit-87-334. The visible RUN_TEST.bat header and both existing local build scripts are TEST 87.350. No GitHub Actions, mandatory checks, or changes to Stable/Version 3.

## Grounded 87.349 result

The user's uploaded dhc_unified_274(3).tsv records 33 original outgoing native DHC tags, 33 native incoming swaps, 23 original 2C Hyper confirmations, and 10 original 2E confirmations. No post-swap failures are present. Several physical PP/KK attempts with a valid outgoing cancel window still fail to complete a recognized incoming motion; do not invent a universal motion alias.

The 87.349 match includes Storm/Sabretooth and Charlie/Bison, not Akuma or Zangief. Therefore new move activation cannot be claimed from this capture.

The ORIGINAL native_command fields in the user-supplied Morphus 3v3 donor archive source/engine/dhc_commands.json are:
- Akuma Shun Goku Satsu / Raging Demon: LP, LP, F, LK, HP -> native Hyper D6=10 (hex), chain-terminal grab. Donor-assigned DP is NOT the actual original input.
- Zangief Final Atomic Buster: 360+PP/PPP -> native Hyper D6=0A (hex), chain-terminal grab. Donor-assigned QCF is NOT the actual original input.

## 87.350 implementation

- Adds motion ID 9 to the exact native fighter-command table for Zangief original FAB; adds motion ID 10 for Akuma's full SGS sequence. Other fighter mappings remain exactly as before.
- Tracks both new commands in the same actual physical P1/P2 input history as the eight original directional motions. SGS requires two distinct LP rising edges with release between, relative Forward direction, LK edge and a final single HP edge, with bounded timing. It does not use generic PP/KK qualification for the last HP and does not accept a fake DP+PP substitute.
- Tracks original relative 8-way sector rotation, requiring a consistent clockwise or counterclockwise sweep of at least 270 degrees with both upper/lower hemisphere inputs and a fresh PP/PPP edge within a bounded completion interval. QCF+PPP alone is insufficient. Unlike other game's variants, the primary native command source specifically calls this 360+PP.
- Both recognizers can consume the final input ONLY inside a legitimate active outgoing Hyper cancel window, with real stock and a distinct valid reserve. They do not alter solo/no-living-reserve input or previous 2C/2E Super lifecycles.
- Selected original D6 is passed unchanged to the existing Morphus-derived M68K outgoing/incoming handoff, opponent-relative entry and 87.348 experimental native fighter dispatch. No synthetic controller 8, fabricated animation, controller remap or bogus ROM handler.
- New existing TSV events: NATIVE_350_RAGING_DEMON_LP_LP_F_LK_HP and NATIVE_350_FINAL_ATOMIC_BUSTER_360. An input-selection event is NOT proof of Hyper execution, grab collision or damage. Check the subsequent original native tag, incoming swap and 2C/2E execution event.

## Local gameplay test

In the same PowerShell window, set the four existing experiment flags to 1, run BUILD_TEST.bat then RUN_TEST.bat xmvsfu. Select Version 4.

As a two-character team, first place Akuma on the bench: during an outgoing Hyper input LP LP Forward LK HP using independent button taps. The final HP alone should select D6=10. Then place Zangief on the bench: spin the stick fully through the down and up quadrants, ending with PP/PPP; D6=0A should result. Test either controller/facing. Validate the actual original 68000 2C/2E execution, opponent capture range and game continuation. Repeat a verified Ryu/Ken Hyper for regression and ensure ordinary one-character team Supers are not intercepted.

Expected log: F:\xmvsf\test\87.350\observer\dhc_unified_274.tsv.

Standalone C++11 -Wall -Wextra -Werror sequence test passed on both facings and both spin directions. The committed source includes a distinct regression for original D6 IDs, physical Raging Demon steps and FAB rotation on both player sides. Full Windows BUILD_TEST.bat and gameplay for 87.350 have not been run by the assistant, so activation, damage, animation and any throw-specific interaction remain experimentally unverified.
