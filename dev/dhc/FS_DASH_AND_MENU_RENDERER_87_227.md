# TEST 87.227 — repeat flying-screen dash and black menu return

## Evidence supplied in 87.226.zip (8 November 2026)

The uploaded `observer/dash_startup.tsv` has 1,056 data rows. At **frame 16,508** Charlie (native actor ID `0020`, ground dash `1400`) presented invalid 16.16 world X **`704E7800`** despite a normal `00000000` X velocity. TEST 87.226 logged **one** `FS_DASH_INVALID_WORLD_X_RECOVERED` and then **497** `FS_DASH_REENTRY_SUPPRESSED` entries, consecutively from frame 16,509 through 17,005. Each entry found the *same* invalid X, the *same* 1400 dash mode, the same `000EC944` ROM animation script and timer 6. The old 87.221 36f watchdog was not involved in this particular capture: the 87.226 recovery was already active but the original 68000 kept reinstating bad state every frame.

This is evidence that the existing **post-CPU scalar restoration alone cannot eliminate the CPU-owned dash controller state**, not evidence of a specific executed native opcode. The supplied ZIP does not contain frontend SDL renderer lifecycle logs; the menu black screen itself is a user-observed symptom, not conclusively established by the CPS2 observer.

## Flying-screen dash correction

In `xmvsf_stage87_220_zero_dash_startup.h`, retain the original 87.220 zero-delay dash startup and 87.221 36f watchdog. The 87.226 quick invalid-X repair continues first.

When the *same* native actor repeatedly reenters the same bad dash, use a bounded stronger recovery after four consecutive repeat frames:

- Capture a **real native neutral** `0x000..0x0FF` movement/controller/animation actor slice, only from that fighter while grounded, with native neutral action, zero X velocity, valid ROM animation scripts and a plausible world X.
- On the fourth same-actor dash reentry, replace the corrupted movement/controller slice with that fighter's own captured native neutral slice, then keep the most recent valid position and zero residual X movement. This includes native animation state that the 87.226 scalar correction left behind.
- Never copy the actor's `+0x100..` storage, including life at `+0x210`, or rewrite the opponent, ROM instructions, menu mode, or Charlie's attack velocity/FS rules. Require actor identity and a native neutral snapshot at most 600 frames old; no invented fallback script.
- Afterward, remain ready to restore if original 68000 repopulates the invalid state. Reattempt the native controller restore only every 16 repeat frames; throttle log spam. Once the ROM itself remains out of dash for three consecutive frames, allow ordinary dashing again.

A new `FS_DASH_NATIVE_CONTROLLER_RESTORED` decision indicates a stronger restore was actually attempted. The 87.227 ROM-free test checks bad-X recovery, four repeated entries, native ROM/controller state restored, `+0x210` HP unchanged, and ordinary dashes after rearming.

This is an experimental corrective recovery. A native Windows run is necessary to establish whether it ends the symptom.

## Native 68000 source proof (for potential further recurrence)

The existing opt-in `XM87224_CHARLIE_M68K_TRACE` now automatically captures **up to four executed CPU frames** during 87.220 *recoveryActive* as well as Charlie's original air-heavy native reference. It includes the fighter's X (`+0C`), dash mode (`+A0`), original PC, A6, and native control state before and after actual 68000 writes. In dash mode it prioritizes position/mode/controller changes rather than rapid gravity updates. This logger is **read-only** and its extra columns are appended to the existing TSV schema.

Path: `test/87.227/observer/charlie_native_68000_pc_writes.tsv`. Filter `trace_kind=FS_DASH`. This will identify actual native writers if the repaired controller is still overwritten; static opcode searches alone cannot do that.

## Return-to-menu black screen

The persistent SDL window used to survive the return while the gameplay backend destroyed its `SDL_Renderer`. The menu then created a *new* renderer on the still-visible window, introducing an unnecessary GPU-context teardown during every in-process return.

In `vid_sdl2.cpp`, on `xm26PersistentShell && xm26ReturnToBoot`, release the original gameplay-owned textures, video compositor, and cached resources, **but retain the real SDL renderer with the window**. Reset target, logical canvas, viewport, clip, and blend mode to the native 960x540 menu defaults. Boot selector can then adopt it through `SDL_GetRenderer(window)`. On actual app exit/other modes, normal renderer destruction is unchanged.

In `run.cpp` the earlier return-to-menu path also deliberately cleared the game backbuffer to pure black and presented it. That submitted black frame is removed. The boot menu now takes responsibility for the first new presentation on the retained renderer, rather than inheriting a freshly presented black frame.

The transition logs `XM87.227 MENU_RENDERER_HANDOFF` from gameplay exit and `XM87.227 MENU_RENDERER_LIVE returned=1 adopted=1` from the boot selector, alongside the renderer backend and output size. These allow diagnosing a future black screen without guessing based on emulated-game observer files.

The first-window centering fix from 87.226 remains intact; the boot menu is never repositioned during an in-process return.

## Balance and build safety

**No edits to Charlie's air HP or HK behavior, Storm/Cyclops flying-screen variants, opponent flight, stun/dizzy, or grounded/OTG permissions.** Preserve the user's confirmed 87.226 Charlie balance.

New branch: `feature/fs-dash-controller-menu-renderer-87-227`. Local build target: `runtime/stable/XMVSF_Test_87_227.exe`. The new unit `test_flying_screen_dash_controller_227.cpp` joins the cached 87.220, 87.221, 87.222, 87.223, and 87.226 units. No GitHub Actions, no broad clean, no overwrite of Stable or old TEST builds. The Windows executable and real gameplay/menu-return behavior are not verified until local BUILD_TEST/RUN_TEST.
