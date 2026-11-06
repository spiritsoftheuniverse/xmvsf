# XMVSF 87.190 — continuous native 2E super lifetime, not arbitrary timers

Source: the user's actual F9 and native diagnostic archive 87.189.zip.

## Proven baseline

- 9 completed Ryu->Ken native DHCs, including 4 entered from air Shinkuu (`2E0C` at frames 2149, 2683, 4779, 5775). All reached native Ryu 3200, ROM-owned fighter swap, kickless Ken arrival and Ken 2E00 Shoryureppa.
- 5 ground DHCs likewise reached real Ken 2E00. No wrong native dispatch or post-DHC controller error was recorded among these nine.
- One separate air attack command at frame 4409 was declined because Ryu's ROM input history already contained the punch edge. Keep that safety preflight.
- Two commands at frames 4449/4476 were in air 2E0E/step06 (ROM follow-up/recovery), NOT in proven 2E0C/step04 native tag acceptance. They were blocked; do not automatically mark them safe by pretending every 2E+2 action is recovery.

## New finding: Ryu Shinkuu Tatsumaki

- Frame 8286: original root `2E00` script `0D2CFC`, native D6=0C (not Hadoken's 0A).
- Frames 8347..8350: same original `2E00` step04 attack.
- Frame 8352: original `2E00` step04 script rewinds back to `0D2CFC`. The previous 87.189 tracker incorrectly closed the super at this frame, although the ROM kept acting.
- Frame 8354: original next action `2E02`, step06, script `0D2DB0`; it loops repeatedly through `0D2DC0/D0/E0`. This is an ATTACKING follow-up, NOT proof of recovery.
- Frame 8447: `2E10`/step08 script `0D2DF4`; 8466: `2E04`/step0A script `0D2E38`; 8472: `2E06`/step0C script `0D2E7C`. Genuine super ownership ends when actor leaves the original family at 8476. The 9628 and 9843 captures reproduce the same sequence.

## Implemented

- `XM87189Family::Tracker` retains root ATTACK through script rewinds (including root script itself at step04) and caches each observed 2E subaction script boundary from the fighter's original ROM table. The next ROM script address is selected by address, not incorrectly inferred from selector order.
- Native `2E` stage transitions are a CONTINUATION until actual action/controller exit. They remain recognized in the super input observer, but are NOT silently tagged via unproven controller-8 logic.
- A queued DHC gesture in such an unverified follow-up is explicitly logged and discarded. It cannot be carried over into another action and then falsely cause a late tag.
- Stage changes and root rewinds are visible in the existing super-chat TSV, so another recording can show the exact point where each super remains native and when it truly exits.
- The original proven Ryu->Ken controller, 3P-to-2P normalization, ROM 3200 tag, Ken kickless landing/native super, F9 recording, and 87.189 experiment toggle are not changed.

## Verification contract

- C++11 synthetic-ROM regression uses the observed real 2E00 root, root script rewind at 8352, the four subsequent 2E stages, and verified native exit at 8476. It also retains ground/air and Ken root checks.
- New build/test version is 87.190, isolated under test/87.190. Root batch files retain their original names. Protected Stable and all prior TEST executables remain unchanged.
- Replaying old 87.189 F9 data with 87.190 should retain all nine DHC completions, make Tatsu root rewind at 8352/9693/9908 nonterminal, and keep 4409 (already-consumed input) and 4449/4476 (unverified 2E0E) from arming incorrectly.
- No new safe timing window has been guessed or enabled. The next live input experiment should probe fresh DHC chords during `2E00`/step04 and, separately, during sequenced follow-up steps while recording original 68000 controller and input history, before widening execution.

## Scope

Native family detection is multi-fighter and generic. Executable DHC remains only the ROM-proven Ryu->Ken tag chain. This does not claim arbitrary characters, arbitrary supers or arbitrary recovery frames are all native-tag-safe.
