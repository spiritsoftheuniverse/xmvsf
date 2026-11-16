# TEST 87.405 — Boomer Triblaster aiming and three distributed rotors

Branch: `feature/boomer-rotor-spread-87-405`.
Based on 87.404 camera-correct sprites, 1–9 layer filters and 0
scenery-only F6 capture. The 87.399 HUD timing, native controller,
DHC, ROM, Wily painter, and Stage A/B/C assets are unchanged.

## Triblaster adjustments

Both units retain their previous 768-pixel **stage-world** anchoring,
with no center-camera or saved wallSpread double offset:

| Unit | 87.404 world X | 87.405 world X | At camera corner |
|---|---:|---:|---|
| Mirrored left | 106 | **104** | cameraX=0 => screen X 104 |
| Original right | 662 | **628** | cameraX=384 => screen X 244 |

All shots and bullets remain anchored to the correct gun and move
with camera X and stage camera-Y delta as before.

## Upward shots only in the middle

The original alternate odd/even GIF-cycle up/down behavior remains,
but an odd cycle **may turn upward only when the Triblaster's sprite
center lies within the central vertical third of the native screen**:
Y 75 through Y 149 inclusive (384x224 viewport). Above or below
this band, bullet launches remain DOWNWARD; the normal horizontal
shot still fires left (original/right gun) or right (mirrored/left gun).

The gate is checked at the actual animation event frame (GIF frame
2 or 11) and includes stage/elevator motion and the live native
camera-Y offset. It also controls the Triblaster's vertical flip in
the renderer. The GIF frame order, 30px/sec wall scroll, and twice-
elevator projectile speed are unchanged.

## Three non-clustered rotors

The previous narrow X 150..220 spawn strip is removed.

There are now **three independent rotor slots** instead of randomly
choosing a 1..3 target population. Each chooses a random spawn X
from a broad third of the current 384px stage view, with a full-view
fallback when a third is blocked by a Triblaster.

Spacing constraints check the sprites' horizontal BOUNDS:
- At least 88px between rotor origin X values for ordinary narrow GIFs
  (larger sprite widths add extra separation).
- At least 38px visible empty space between a rotor and the bounds of
  either Triblaster at selection.
- The 60%-parallax rotor position is checked against the 100%-camera
  Triblaster position at current camera X **and both camera extremes**
  so normal left/right panning won't drag the rotor into a wall gun.

If a particularly crowded viewport or oversized GIF makes all
three placements impossible at once, a blocked slot delays spawning
rather than stacking sprites or overwriting wall art.

The source GIF animation, random vertical hover levels, deterministic
replay seed, 20px/sec rotor vertical drift, and B/C/A painter order
remain unchanged. No extra per-paint GIF decode is introduced.

## Build and local checks

```powershell
git fetch origin
git switch feature/boomer-rotor-spread-87-405
git pull --ff-only
.\BUILD_TEST.bat
.\RUN_TEST.bat
```

The TEST 87.405 local preflight compiles the SDL2_image GIF header,
then compiles and runs the native C++ math/geometry assertions for:
104 and 244 Triblaster corner positions; middle-third upward
limits; three separated rotors and gun exclusion across 12 camera
positions and both panning extremes; original 2x projectile speed;
original camera-Y wall lock; 1–9 / 0 Boomer layer audit.

The native linker must contain the **new**
`XM87.405 BOOMER_ENEMIES_READY` marker, as well as the retained
87.399/87.400/87.401/87.404 source signatures, before it may publish
`XMVSF_Test_87_405.exe`. No GitHub Actions and no Stable overwrite.

This repository edit has been source-checked and its geometry
constraints cross-checked, but **Windows native compilation and
visual acceptance are not performed in this remote editing session**.
For exact display validation, use F6 on both stage corners after
the TEST 87.405 build succeeds.
