# XMVSF TEST 87.248 — controller hotplug and menu→game consistency

## Source findings

The user reported controllers working in the menu but stopping after launching CPS-2 gameplay, and initially invalid controllers remaining unusable until restart.

1. The SDL2 gameplay backend enumerated pads only at SDLinpInit and retained a fixed nJoystickCount, JoyList[] and GCList[]. Failed opens and late connections had no retry.
2. The backend disabled SDL joystick/controller events with SDL_IGNORE despite using a persistent menu in the same process.
3. Boot, Training, Arcade, and Versus menus each opened/polled handles without consistent retries on late device readiness.
4. The global menu→game input handoff could hold every control blocked forever if a different device or axis never returned to neutral.
5. Persisted GUID+ordinal assignments cannot by themselves identify an unchanged physical pad after another identical pad disconnects and SDL compacts indices.

## TEST 87.248 changes

- During gameplay, reconcile live SDL instance IDs on input frames, reopen changed slots immediately, retry failed opens every 250 ms and retry missing SDL GameController mappings after a raw joystick becomes available. All reads validate the attached handle. Slots are consistently capped at eight.
- Keep SDL Joystick and GameController events enabled for persistent menu operation.
- Recheck boot-menu devices every 100 ms and Training/Arcade/Versus menu devices every 150 ms; initial failures and late wakeups no longer require re-entering the menu.
- Seed held buttons when a menu pad reconnects, preventing an automatic Accept/Back; require release and a new press.
- Preserve assigned P1/P2 live SDL instance IDs at runtime, then use saved GUID/name matching to recover an unclaimed device after reconnect. Prevent an unchanged P2 from becoming P1 when another identical pad disconnects; do not change persisted controller map. Keyboard/unassigned changes release the previous live owner.
- Bound the global input handoff to a maximum of 400 ms, while preserving its normal release+80ms behavior.
- Diagnostic logs on changes: XM87.248 PAD_OPEN, PAD_LOST, PAD_MAPPING_READY and PAD_ROUTE saved_p=… device=… instance=… ready=…. device=-1 indicates no saved-device match; ready=0 with an enumerated device indicates SDL handle not yet attached.
- The local regression test dev/dhc/tests/test_controller_reconnect_248.py guards all of these source paths during BUILD_TEST.bat. No GitHub Actions.

## Local validation checklist

1. Start with a saved P1 pad ready. Navigate boot with A/B, enter Arcade/Training, and verify D-pad, six attacks, Start and Coin work immediately after handoff.
2. Start without the assigned pad, launch with keyboard, then plug in/wake the saved pad during the SAME running match; its inputs must work without restarting.
3. Unplug and reconnect P1 during gameplay; verify PAD_LOST, PAD_OPEN and PAD_ROUTE log transitions and that button polling resumes.
4. Connect two identical-model saved pads, unplug P1 and leave P2 attached. P2 must remain P2 after index compaction; reconnect P1 and verify ownership.
5. Hold A across reconnect in Training, Arcade and Versus overlays. No phantom menu choice; release and press again to control the menu.
6. Return to boot, start another session, and verify the original saved mappings remain unchanged.
7. Hold an unrelated input throughout menu→game handoff. The all-input latch must not remain blocked beyond 400 ms.

Verification: source checks only. Actual Windows MSYS2 build and SDL/XInput/USB/Bluetooth gameplay are not yet verified. Stable, prior TEST executables and build caches are untouched.