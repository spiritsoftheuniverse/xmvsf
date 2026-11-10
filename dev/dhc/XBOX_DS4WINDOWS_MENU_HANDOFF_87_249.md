# XMVSF TEST 87.249 — Xbox / DS4Windows Arcade input handoff

## Issue and root cause

User confirms Arcade Fighter Pi controller works but Xbox gamepads and DS4Windows can navigate the native boot menu and become completely unresponsive after entering Arcade. This existed prior to TEST 87.248 and is not evidence that the new hotplug code caused it.

The boot menu handles SDL_CONTROLLERBUTTONDOWN and raw SDL_JOYBUTTONDOWN A/B events independently of the saved player assignment. CPS-2, however, used three separate saved-profile-only gates:

1. xm131ApplyGameInputMappings() installed P1 virtual CPS-2 input codes only when mapping.player[0].active was saved.
2. SDLinpState() only called xm13ReadInput() if xm13Config.player[xm14Source].active, otherwise returned xm15SyntheticButton() = 0 for every control.
3. xm13ReadInput() rejected inactive saved profiles and otherwise read device-specific raw SDL_Joystick button indices. A different GameController used for menu Accept could remain bound to the old saved Fighter Pi profile.

These differences explain a responsive menu and dead Arcade gameplay for a controller that has no matching saved P1 profile. The user has not supplied an 87.248 PAD_ROUTE log to verify whether their specific Xbox profile was unassigned or simply mismatched, so this is a source-grounded failure path, not a claim about their actual INI contents.

## Implemented

- On actual boot-menu Accept (SDL_CONTROLLERBUTTONDOWN or SDL_JOYBUTTONDOWN), preserve the originating SDL joystick instance ID as P1's temporary owner for the next game. Reset this selection whenever the persistent boot menu reopens.
- Only automatically claim SDL GameController-classified pads. Keep the existing native Fighter Pi raw joystick path and its saved custom bindings untouched.
- Use the menu-selected SDL instance before saved GUID/ordinal lookup, with GUID/name fallback if the driver reconnects and changes its instance ID.
- Use the same tested saved-or-session policy at all three gates: GameInp virtual mapping installation, the SDLinpState callback and the Stage13 per-player button reader. Otherwise the game would still silently discard the input at one of these layers.
- When this session controller differs from the saved P1 hardware or P1 was unassigned, read semantic SDL GameController buttons: LP=X, MP=Y, HP=LB, LK=A, MK=B, HK=RB, Coin=Back, Start=Start; directions use the existing D-pad/axis/hat union. Exact saved-device profiles keep custom binding indices.
- When P1's saved keyboard mapping exists and the user uses an Xbox pad, allow mapped keyboard controls to remain functional alongside the temporary semantic pad.
- If P1 was unavailable when the game started, a subsequently valid unclaimed Xbox/DS4Windows pad may be adopted through a NEW A press while Arcade remains open. A held A at the moment the device first appears is seeded and does not trigger unintended gameplay. Exclude a working P1, any assigned P2 and deterministic replay. Refresh CPS-2 GameInp virtual mappings once on successful claim; no ROM writes or fixed startup wait.
- The user's controller INI is NEVER rewritten simply for launching Arcade or adopting a controller. The menu-selected instance is session-scoped.
- Read-only diagnostics: XM87.249 MENU_CONTROLLER_SELECTED, PAD_GAME_OWNER, PAD_LATE_CLAIM; existing PAD_OPEN/LOST and PAD_ROUTE from 87.248 remain.
- Source integration test test_controller_session_249.py and directly compiled pure policy test test_controller_session_249.cpp added to local BUILD_TEST.bat / BUILD_TEST.sh test stage.

## Required local runtime checks

1. Connect Xbox/DS4Windows WITHOUT a saved P1 profile, use A in the boot menu to select Arcade, and test movement plus all six attacks. The Stage13.1 input log should report menu-P1=1 and a full P1 virtual switch mapping.
2. With an existing Fighter Pi profile saved for P1, use Xbox A to select Arcade. The Xbox should temporarily drive P1 while its physical instance exists, without overwriting the Fighter Pi INI.
3. Return to boot, select with Fighter Pi and verify the original raw controller remains usable and unmodified.
4. Launch with keyboard and no working gamepad. Wake/connect an Xbox/DS4Windows controller AFTER the match is running, release the initial held buttons, press A, then verify P1 controls without restarting.
5. Test two controllers, ensuring live P2 is never stolen for automatic P1 adoption. Verify custom bindings on the exact saved controller are unchanged.
6. In case of failure, inspect PAD_ROUTE and 87.249 MENU_CONTROLLER_SELECTED/PAD_GAME_OWNER/PAD_LATE_CLAIM messages together with xmvsf_stage13_1_input.log. They distinguish missing virtual GameInp mapping, wrong physical SDL instance, and semantic button delivery.

Status: changes committed in TEST 87.249 on feature/menu-intro-reveal-audio-87-235. Pure routing policy compiled and its 14 assertions executed in a minimal Linux C++ harness; repository-local Windows C++ tests and actual XInput/DS4Windows gameplay are not yet verified. Stable and previous TEST builds are untouched; GitHub is source control only.