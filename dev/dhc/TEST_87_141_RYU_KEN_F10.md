# XMVSF Stage 87.141 — Ryu/Ken super-input verification and F10 recorder repair

## What the supplied 87.140 transcript established

The previous `super_chat(1).tsv` contains point/reserve IDs
`0x12` / `0x14` and the reverse ordering. With Ryu as reserve (`0x12`)
it records a correctly formed `QCB+KK` at frame 4022, but elsewhere the
generic matcher labels unrelated kick sequences `DPB+KK` or `DPF+KK`.
That generic grammar was **not** a character-specific super list.

Stage 87.141 uses the following **user-confirmed input commands** instead:

| Reserve | Native ID | Named super | Command |
|---|---|---|---|
| Ryu | `0x12` | Shinkuu Hadoken | QCF+PP |
| Ryu | `0x12` | Shinkuu Tatsumaki | **QCB+KK** |
| Ken | `0x14` | Shoryureppa | QCF+PP |
| Ken | `0x14` | Shinryuken | QCF+KK |

For Ryu facing right, QCB is physically **Down, Down+Left, Left**. For
Ryu facing left, QCB is **Down, Down+Right, Right**. The chat now prints
both the physical direction (DOWN+LEFT/RIGHT) and the direction relative
to the active fighter's native facing flag (DB/DF), so a side change is
not confused with an inverted command.

The matcher requires the correct **final BACK** for Ryu QCB+KK, and
**final FORWARD** for a QCF command. A valid punch/kick chord pressed up
to four frames before the final cardinal direction may complete once
the direction arrives. Stale quarter-circle data and generic DP/HCF
guesses cannot identify a named super.

These are **input-map matches only**. No ROM-native super script ID
or DHC move dispatch has been verified or installed. No meter debit,
tag activation, scene freeze, or native action changes are done.

## F10 fix

Previously F10 was handled **after** the display, Training and Versus
SDL event handlers. On some sessions the key never reached the recorder.

Stage 87.141 handles F10 **before those consumers**. It writes a message
to the Warcraft-style chat when F10 starts/stops, or when it fails.

The F10 trace and OAM files now write a `# F10_START armed` marker and
flush immediately on startup. At stop they write a frame-count footer,
including `NO_COMBAT_FRAMES_SEEN` if zero actual combat frames arrived.
This distinguishes an unreceived key from a path failure or an invalid
combat-phase detector, without modifying native gameplay.

**Shift+F10** still loads the existing quick-save. **Shift+G** expands
the chat history; messages otherwise fade after three seconds. **F9**
continues recording replay inputs.

## How to test

1. In GitHub Desktop select `feature/dhc-mvc2-stage87` and Pull origin.
2. Run the same root `F:\xmvsf\BUILD_TEST.bat` and
   `F:\xmvsf\RUN_TEST.bat`. The latter launches only
   `F:\xmvsf\runtime\stable\XMVSF_Test_87_141.exe`.
3. In Training, use Ryu and Ken. Start a normal super with the point
   fighter; input the reserve fighter's QCF+PP/KK or QCB+KK command.
4. Open chat using Shift+G. The result must name the reserve fighter
   and the **specific** matching input-super entry, or state that no
   listed input command matched. It must not invent another super.
5. Press F10 during combat, verify an **F10 PROBE ON** line appears,
   reproduce your inputs, then press F10 to stop. Verify an
   **F10 PROBE STOP** line with native frame count.

The current TEST writes into `F:\xmvsf\test\87.141\`:

- `super_chat.tsv` — named input matches + event messages.
- `xm87139_probe_0001.tsv` — native input/action/ROM state and F10 markers.
- `xm87139_oam_0001.tsv` — native OAM around observed tags; will contain
  only markers/header if no tag occurs. Later probe sessions are numbered.

If F10 says `frames=0`, the recorder opened but the native-frame gate
wasn't satisfied; if it shows a file-open error, the diagnostic path
wasn't writable. Either outcome is actionable.

GitHub's isolated C++11 tests cover Ryu/Ken matches, both facings,
incorrect Ryu QCF+KK rejection, F10 immediate file creation, and
zero-frame markers. Native Windows build/gameplay remain user-side tests.
No original Stable executable or launcher was changed.
