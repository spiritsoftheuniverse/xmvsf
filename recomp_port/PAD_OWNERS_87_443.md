# 87.443 - player ownership, remembered profiles and no hold cycling

## Menu input

The native home menu, native Options, and upstream engineering settings
all use an input rising-edge gate. Holding a direction/confirm/back button
cannot move repeatedly. The user must release and press again, independent
of frame rate. Gameplay held buttons and charge moves are unaffected.

## P1/P2 controller ownership

Recomp previously used gamepads[0] for player 1 and gamepads[1] for player 2.
87.443 routes each player via a saved identity, following the working SDL2
Stage13 and Stage87.248 controller design. A controller cannot quietly
switch players when another controller disconnects.

The Controller tab now has:
- P1/P2 selection by Left/Right.
- First row: device assignment. Confirm cycles connected hardware,
  NOT ASSIGNED and KEYBOARD, persisting the choice in settings.ini.
- Second row: choose whether to edit keyboard or pad/raw joystick inputs.
- Remaining 14 rows: LP MP HP LK MK HK 3P 3K COIN START LEFT RIGHT UP DOWN.

Live SDL instance ID pins each assigned player while connected. On reconnect
we match serial if supplied, else GUID+occurrence, else actual OS joystick
name+occurrence. Both players' live IDs are reserved first, so identical
sticks cannot steal one another's identity after hotplug.

Unassigned P3/P4 co-op modes retain other unclaimed gamepads.

## Names and arcade sticks

Read SDL_GetJoystickName on the underlying SDL_Joystick first, before
SDL_GetGamepadName. If the Windows/SDL joystick name is Fighter PI, that
is what our UI displays; do not invent generic XInput labels.

Joystick devices that SDL3 does not recognize as gamepads are also
enumerated, opened, shown by name and made assignable. A raw joystick
uses its hats and first two axes for ordinary movement, with input
capture for raw buttons, hats and axes through the Controller tab.

## Persistent per-device memory

Device assignments live under sections xmvsf_player1 / xmvsf_player2
in the same recomp/work/xmvsf/settings.ini used by RECOMP_RUN.bat.
Each physical gamepad GUID+occurrence has its own xmvsf_profile_* section
with the corresponding 14 mappings. The original keyboard1/keyboard2
and gamepad1/gamepad2 settings remain compatible.

On reassignment or reconnect, the saved profile is restored automatically.
The previous profile remains stored if its pad disconnects. Same name
and ordinal can recover an older profile when a driver changes GUID.

Only recorded raw button/axis/hat mappings are used for unknown HID
arcade sticks. No imaginary mapping is generated. Hardware serial may
be absent, in which case GUID/name+ordinal and live instance apply.

## Build

New: recomp_port/xmvsf_pad_owners.h, apply_pad_owners.py,
and tests/test_pad_owners.py. The installer runs after the native
Options installer, with regression checks before incremental Ninja.
Older patchers recognize the newer frontend and do not reinstall
obsolete rendering/input blocks. No translation refresh or GitHub Actions.

The native Windows build and Fighter PI enumeration are not yet verified.
