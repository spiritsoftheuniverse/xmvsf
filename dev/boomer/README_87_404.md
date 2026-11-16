# TEST 87.404 — Boomer camera anchoring and 1–0 layer audit

Base: accepted TEST 87.403 with corrected SDL_image GIF type. Stage-specific
code only; no ROM changes, HUD reveal timer changes, F6 hotkey changes,
controller/punch input changes, or Stable overwrite.

## 87.403 symptom and coordinates

The old Triblaster X was `105/277 + (192-cameraX) +/- wallSpread`.
That made both sprites appear near the center at a centered camera,
with an additional split-wall spread translation even though their
intended final placement was at the OUTER corners of the 768px stage.

New world-space left sprite X = 106.
New world-space original/right sprite X = 768 - 106 = 662.

`screenX = worldX - nativeCameraX`:

| Native camera X | Mirrored/left screen X | Original/right screen X |
|---:|---:|---:|
| 0 (fully left) | **106** | 662 (offscreen) |
| 192 (middle) | -86 (offscreen) | 470 (offscreen) |
| 384 (fully right) | -278 (offscreen) | **278** |

Both anchors are the sprite's TOP-LEFT point. The corner inset is
106 pixels from the screen edge to that point; right X=278 means
384-278=106. Projectiles retain their exact GIF-local launch offsets,
become WORLD-space entities, and use the same X camera transform.
The saved B wallSpread is no longer added to the projectile/Triblaster
X twice. The B wall PNG positions and saved placement remain unchanged.

### Vertical camera motion

The native stage camera Y changes during jumping. The 87.403 GIFs used
screen-space Y while B's authored wall was drawn with
`stageTop=224-471+256-nativeCameraY`. This made the sprites slide
relative to the actual elevator wall when the player jumped.

Now the first actual Boomer frame records a local `cameraAnchorY`.
On each paint, `stageYShift = cameraAnchorY - nativeCameraY`.
That equals the wall B's change in `stageTop` and is applied equally
to Triblasters, both projectiles and rotors. Their initial screen-space
entry still begins above the top edge; they then remain attached
to their scenery plane through jumps. Rotor drift 20px/sec, B 30px/sec,
and GIF animation timing are preserved.

## Native keyboard layer filtering (Boomer stage only)

Top-row **1–0** and keypad **1–0** are one-shot press toggles:

| Key | Boomer compositor source |
|---|---|
| 1 | Distant background C |
| 2 | Rotors |
| 3 | B split walls (both sides) |
| 4 | Triblasters (both sides) |
| 5 | Triblaster shot blasts |
| 6 | Triblaster downward/upward bullets |
| 7 | Foreground/platform A |
| 8 | Stun HUD |
| 9 | Native fighters and flash effects |
| **0** | **Scenery-only mode**: show C, B, A; hide everything else |

Press 0 again to restore the individual 1–9 selections. In
scenery-only mode, even an independently disabled scenery layer is
temporarily shown so F6 captures the full background. Independent
1–9 selections are preserved while 0 is active, and reappear when
0 is switched off. On a new stage generation/replay rewind, filters
reset to all-on; a previous clean capture cannot leave fighters
invisible in a subsequent match.

In clean mode the final SDL presentation also hides the floating
frame-data text, player HUD, training/versus UI, and F9/F10/chat status
notices. F6 is unchanged and saves a clean native-size **384x224** PNG to
`F:\xmvsf\screenshots` when launched by RUN_TEST.bat.

Wily's previous 1–9 audit remains its own feature; the Boomer control
does not affect other stages. These toggles are paint-only; the original
CPS2 game still runs and accepts normal physical button inputs.
The original authored A/B/C PNGs are untouched: any imagery baked
directly into those PNGs is not separately removable.

## Validation

Run locally:

```powershell
git fetch origin
git switch feature/boomer-camera-layers-87-404
git pull --ff-only
.\BUILD_TEST.bat
.\RUN_TEST.bat
```

`BUILD_TEST.bat` first checks the original SDL_image GIF loader in a
fast C++ syntax-only test, then compiles/runs pure C++ assertions for:
- Left camera=0 -> mirrored X106; right camera=384 -> original X278
- Relative camera displacement and exact native B wall Y offset on jump
- 1–9 independent layer selections and key 0 clean scenery mask
- Existing 2x projectile velocity, frame parity and sprite attachment flips

The builder still preserves the previous linked 87.399 HUD, 87.400 F6
and 87.401 GIF readiness source proofs, plus a new 87.404 linked
`XM87.404 BOOMER_LAYERS_0` proof. It will not publish a stale EXE.

The uploaded 87.403 ZIP contains F9 v2 replay and F10 diagnostic files,
not an F6 screenshot; no claim of native Windows visual acceptance is
made from that archive. Use 0, then F6 on a left/right corner and
during a jump to inspect stage/sprite separation. TEST 87.404 is
source-complete but still requires the local native compilation and
visual check.
