# Stage 87.140 — first-super command responsiveness test

This is a **read-only UI/input test**, not a DHC move implementation.
It does not tag, freeze, recolor fighters, bypass super portraits or spend meter.

## Controls and presentation

- **Normal gameplay:** start an ordinary super on P1 or P2, then input another
  motion and attack combination while that first super is executing.
- **Warcraft-style chat:** bottom-left translucent blue/black combat log,
  gold system lines, blue delivered-input lines, green provisional command
  matches. The five newest entries fade and disappear at **180 native frames
  (3 seconds at 60fps)**. The final 60 frames fade gradually.
- **Shift+G:** expand or close a retained history of the last twelve entries.
  The shortcut is withheld from the ROM's keyboard inputs.
- **F10:** independent native/input/OAM TSV recorder from 87.139, if desired.
- **F9:** existing input replay recorder. **Shift+F10:** existing quick-load.

## How the chat knows when to listen

At the CPS-2 real-frame boundary, the diagnostic observes the P1/P2 action
byte +A0 and completed meter stocks at FF4214 / FF4614. An edge into
`+A0 == 0x2A` or a native stock decrease opens a bounded **FIRST SUPER?**
observation window. During that period it captures BOTH the direction and
six attack buttons from the real CPS2 input buffers for that same side,
including keyboard or controller inputs. These are the actual ROM-forwarded
bits, not only the SDL mapped-controller history.

The native action +A0=2A can also occur in non-super actions; **this is a
super CANDIDATE window, not an authoritative super declaration**. The probe
prints the point/reserve IDs, meter edge, and reserve's action, animation and
script pointer so that real ROM capture can establish the exact states.

The command matcher recognizes input sequences in the currently observed
forward-facing orientation: quarter-circle forward/backward, DP forward/backward,
and half-circle forward/backward plus two punches or two kicks. This is a
test **grammar**, not a character-specific ROM move list. On each attack
press, the screen reports what you pressed (LP/MP/HP/LK/MK/HK), current
direction and any pattern matched.

**The ROM-verified reserve super registry is still empty.** You will see
`ROM-VERIFIED RESERVE SUPER LIST: EMPTY` and
`MATCH QCF+PP (CANDIDATE NOT VERIFIED)` rather than a made-up move name or
assertion that the ROM accepted the intended super. Data from this TEST is
what lets us build actual character-ID-to-super-command mappings.

## Files and build

Select the experimental branch `feature/dhc-mvc2-stage87` in GitHub Desktop
and Fetch/Pull origin. Run only the established root launchers:

```bat
F:\xmvsf\BUILD_TEST.bat
F:\xmvsf\RUN_TEST.bat
```

New test executable:
`F:\xmvsf\runtime\stable\XMVSF_Test_87_140.exe`

Chat messages append automatically to:
`F:\xmvsf\test\87.140\super_chat.tsv`

For full native evidence, press F10 to record one test session and F10 again
to stop. The existing 87.139 probe folder remains:
`F:\xmvsf\test\87.139\`

**Reproduction:** P1 performs a native super. Before it ends, enter what
you believe is the reserve fighter's super (e.g. QCF+PP) at least once;
repeat with a different button family or direction. Watch the lower-left
chat and press Shift+G to review the messages. Feed back the chat TSV,
and, if possible, the F10 native trace.

There is no new root launcher name and no historical installer. The branch
uses the current warm object cache, changes four native translation units
only, publishes a separate 87.140 TEST, and protects Stable.
GitHub C++11 unit tests cover simulated input patterns and chat expiry;
full Windows native build and gameplay are still unverified.
