# XMVSF 87.142: ROM-based super and tag detection

The user-provided 87.141.zip records 2,021 native combat frames and a P1
point/reserve swap at frame 2992. F10 did record real data: the native TSV
ends with 2,021 frames and one tag event.

We inspected the user's project-local xmvsfu CPS-2 program ROM using the
project's own cps2_crpt.cpp decryption and Musashi disassembler. This branch
contains NO ROM data, decrypted binary, encryption key, or extracted graphics.

## Correct native facing

The character byte at FFxxxx4B is 1 = RIGHT, 0 = LEFT, not the inverse
proxy previously used. The native initialization routine at 0x00FF0E-FF1A
sets the P1-side face bit to 1 and the opposite-side bit to 0. Observed
fighter positions support this. All three live chat facing interpretations
were corrected; raw CPS2 physical direction bits remain unchanged.

## Actual script-selector code

The decrypted 68000 routine at PC 0x01038E:

1. Stores D0/D1 in actor offsets +0x60/+0x61 (action/subaction).
2. Reads the even native fighter ID at +0x52.
3. Looks up a 32-bit fighter-directory pointer in unencrypted game ROM at
   0x08A946 + fighterID*2.
4. Reads the indexed action's relative 16-bit offset, then the indexed
   subaction's offset, yielding the first native animation-script record.
5. Transfers to the native animation setup routine at 0x001AE6.

The current actor's live script pointer is in +0x34. The first script
record's animation marker at offset +6 matches anim42 from the capture.

| Fighter | Action +60/+61 | First native script | Anim | Captured event |
|---|---|---|---|---|
| Ryu 12 | 2E08 | 0D3070 | BDA0 | Super entry: frames 2010/2295/2541/2788 |
| Ken 14 | 2C04 | 0D5E2E | EE56 | Super entry: frame 3135 |
| Ken 14 | 2C00 | 0D5C46 | EE56 | Super entry: frame 3247 |
| Ken 14 | 2E00 | 0D6722 | EF18 | Distinct super entry: frame 3302 |
| Ryu 12 | 3200 | 0D1DAC | A5C2 | Outgoing normal tag: frame 2978 |
| Ken 14 | 2A0C | 0D5702 | EB0A | Incoming normal tag: frame 2992 |

All six offsets were independently resolved in the game's ROM table and
matched the real native +0x34 script pointer and anim42. These prove script
allocation, not yet the named move for each of Ken's variants. The captured
Ken supers used punch inputs; do not guess which script is his kick super.

The old +A0==2A proxy was too broad: the normal tag at frame 2978 also
used modeA0=2A00. Stage 87.142 therefore requires a validated ROM script
entry to open a Ryu/Ken first-super command window. Other fighters still
use the expressly provisional fallback until examined individually.

## Native partner allocation path

- FF4000/FF4400: P1/P2 point actors; FF4800/FF4C00: reserve actors.
- Fighter ID at object +0x52. Partner pointer at point +0x224.
- PC 0x00FD0C / 0x00FD30 initializes P1 point FF4000 and its pointer
  FF4224 -> FF4800.
- PC 0x010000..0x010014 allocates point/reserve ID and partner pointer.
- PC 0x01147E obtains the partner via +0x224, checks partner +0x210,
  temporarily swaps A6 to the partner and calls 0x01038E to select its
  native animation. Calling context is NOT YET established as DHC.
- PC 0x013A2C checks partner status and invokes the native command
  matcher at 0x013C30. This path can select action 3202; it is different
  from the captured normal tag 3200 and must not be conflated.
- PC 0x011B16 selects action 3200 for a native script path. Caller tracing
  is required before classifying it as a universal tag trigger.
- At frame 2992 the ROM swaps P1 point ID 12->14 and reserve ID 14->12,
  with Ken's actual 2A0C incoming tag script at 0D5702.

## Implementation and TEST

The experimental header xmvsf_stage87_142_rom_dispatch.h now reproduces
the ROM data-table lookup on the actual loaded CpsRom bytes. It compares
the lookup result against each live actor's script pointer and reports
separate ROM_SUPER_SCRIPT, ROM_OUTGOING_TAG_SCRIPT, ROM_INCOMING_TAG_SCRIPT
and ROM_POINT_RESERVE_SWAP events. It also reads and reports partner +0x224.

The CPS2 real-frame source passes these events to the existing 3-second
Warcraft-style chat. For Ryu/Ken, the ROM-confirmed super-script event
replaces the generic +A0==2A first-super proxy. No code, meter, input,
fighter position, sprite, animation or tag state is modified.

Pull feature/dhc-mvc2-stage87; run the existing root BUILD_TEST.bat and
RUN_TEST.bat. The latter starts only runtime/stable/XMVSF_Test_87_142.exe.

F:\xmvsf\test\87.142\rom_dispatch.tsv is automatically written when real
native script or point/reserve changes are observed.
F:\xmvsf\test\87.142\super_chat.tsv records the chat.
F10 starts/stops additional native/OAM captures in the same folder.
Shift+G shows chat history. F9 replay and Shift+F10 quickload remain.

NEXT: identify the actual 68000 call sites and gates for the native partner
handoff, super action allocation, portrait timing, and first active frame.
Neither a DHC jump nor a native super dispatch is enabled yet. Only
isolated C++11 tests have run on GitHub; Windows game compilation and
the real runtime trace require local verification.
