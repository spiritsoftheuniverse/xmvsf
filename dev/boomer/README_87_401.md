# TEST 87.401 — Boomer Kuwanger rotor + Triblaster GIF effects

Branch: feature/boomer-enemies-87-401
Base: feature/native-f6-screenshots (accepted 87.399 HUD timing preserved).

## Authored assets

Four GIFs are now tracked together under _art/boomerkuwanger/:

- rotor.gif — brought in from the updated main-branch Git blob
- triblaster.gif — retained from the F6 branch
- triblaster_shot.gif — brought in from main
- triblaster_bullet.gif — brought in from main

Existing authored a.png, b.png and c.png remain unmodified.
SDL_image's multi-frame GIF decoder loads each animation once into
frame textures, using the GIF's per-frame delays converted to native
60-Hz game frames. No GIF decoding or new texture creation per paint.
Animated GIF loading requires SDL2_image 2.6 or newer; the build fails
explicitly with an upgrade message if the system headers are older,
rather than silently showing unchanged background sprites.

## Painter Z order

1. C — distant shaft, original slow parallax
2. Rotors — between C and B, middle parallax, draw over C/under B
3. B — unchanged split left/right walls
4. Triblasters, blasts and bullets — the B wall motion layer
5. A — unchanged platform
6. Original stun, native fighters and flash overlays

No changes to CPS2 emulation, collision, knockdown, HUD, audio or timing.

## Rotor (rotor.gif)

- New spawn X is random in the 384px screen range 150–220.
- Spawns begin off the top edge, descend toward a random hover level,
  bob by up to 3px, then drift downward as the elevator scrolls.
- Middle parallax rate is 20px/sec, between C's 12px/sec and
  B's 30px/sec. Horizontal camera parallax is 60%, between 30% and 100%.
- A deterministic per-stage PRNG chooses target active population 1–3
  and spawn positions. The state never permits more than 3 live rotors.
- When a rotor exits the bottom, a new offscreen spawn may be scheduled.
- All positions are based on absolute game frame, not SDL paint count;
  sprite animations remain consistent with display skips and replay.

## Triblaster (triblaster.gif)

- One per side maximum, with independent top-edge entry and respawns.
- Right-side original anchor X=277, left-side horizontally mirrored X=105.
- X follows the camera and each B wall's saved spread.
- Descends on the same 30px/sec absolute native-frame clock as wall B.
- All source GIF frames play in order. On completing one full animation,
  the next cycle flips vertically; next cycle returns to original,
  alternating downward/upward bullet direction.

One-based authored GIF frame numbers:

- Frame 2 (index 1): spawn triblaster_bullet.gif at local X+27, Y+47.
  It fires downward by default, reversed upward on the flipped cycle.
- Frame 11 (index 10): spawn triblaster_shot.gif at local X+8, Y+26.
  It fires left by default. The mirrored side fires right.
- The local attachment coordinates are mirror-transformed on both axes,
  including the projectile's actual GIF width and height.

Both projectiles inherit the B wall's downward drift. Their own
speed is 60px/sec (1px/game-frame), exactly twice the 30px/sec elevator.
A downward bullet therefore gains its 60px/sec firing velocity *plus*
the existing 30px/sec elevator scroll in screen coordinates.
The horizontally travelling shot gains 30px/sec vertical elevator drift.
Projectile sprites animate using their source GIFs and are bounded by
a 24-projectile maximum; sprites are culled outside the viewport.

## Validation

TEST 87.401 is local only. Run:

    dev\boomer\RUN_TEST_BOOMER_401.bat
    BUILD_TEST.bat
    RUN_TEST.bat

The standalone C++ test checks source-parallax motion, attachment flips,
default leftward/rightward travel and the down/up bullet velocity.
BUILD_TEST.bat checks the four GIFs exist BEFORE compiling.
The native build refuses to publish a new TEST binary unless
"XM87.401 BOOMER_ENEMIES_READY" is present in the linked EXE, in addition
to the retained 87.399 HUD and 87.400 F6 screenshot proof markers.

At runtime inspect "XM87.401 GIF_READY", "ROTOR_SPAWN",
"TRIBLASTER_SPAWN" and "TRIBLASTER_FIRE" in diagnostics.
The GIF_READY log records each asset's actual decoded frame count;
if triblaster.gif has fewer than 11 frames, a warning explicitly
identifies the incompatibility.

F6 captures native-size 384x224 stills to the existing screenshots folder
and can be used to visually check attachment coordinates.

Source integration/asset presence verified through GitHub. A native
Windows compile and actual rendered GIF appearance have not been verified
in this environment.
