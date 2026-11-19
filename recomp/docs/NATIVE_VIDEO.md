# The picture at the game's level

Today the 68000's code is translated ahead of time, and the sound is native for games with QSound driver 2.01b
(`sound/driver.cpp`, checked write for write against the Z80 with `--sound check`). The picture is not native yet:
`video/video.cpp` draws from the board's object memory, its CPS-A and CPS-B registers, and palette memory. That is a
model of the board's video chips, and a port should not ship one.

The plan: the games' drawing code hands a native interface each frame's objects, scroll layers, layer order and
palettes. A native renderer on SDL_GPU draws from that interface. The board model stays only as the reference, and
two checks run beside it every frame:

- **(a) the interface:** the records the rewritten game code hands over equal the records decoded from what the board
  model was given (`--video check`);
- **(b) the pixels:** a CPU rasterisation of those records equals `video.cpp`'s picture.

This page has the inventory the work starts from, the interface, the checks and the order of work. The first slice,
the palette's upload, is done: [below](#the-first-slice-the-palettes-upload).

All of it is written once, in the toolkit. The interface, the renderer, the decoder and the checks are in `video/`.
Capcom's video library, which the games share, is rewritten once in `video/lifts.cpp`. A game's `game.cfg` gives only where its copy of each part is. For a
routine it adds a CRC-32 of the routine's instruction words with their operands left out, which says the copy is the
version the toolkit has; a single instruction is checked to be the move the toolkit expects. Per-game code is only for a game whose drawing really differs; the games that need it are named
below.

## Finding out (`CPS2RECOMP_VIDEO_CENSUS`)

`CPS2RECOMP_VIDEO_CENSUS=FILE` with `--cpu blocks` (the interpreter, doing what translated code does) writes, at the
end of a run, every instruction that read or wrote the video hardware. Each line has the instruction's address, the
part reached, `W` or `R`, how often, the addresses it reached, and the interrupt masks it ran under. Mask 1 is the
main loop, 4 is the vertical blank's interrupt (level 2), 10 is the raster interrupt's (level 4), and 80 is start-up.

The parts are `O` object memory, `V` video RAM, `A` CPS-A, `B` CPS-B, `R` the object registers at 400000, and `K`
the object bank at 8040E0.

The census's output is the game's, so it stays outside the repository, like `work/`. Grouping the lines by routine
takes a few lines of Python over `work/<id>/program.lst`.

## The inventory

### Where the video hardware is written, in every game

The games run the same frame in the same order. Their main loop draws: it writes the next frame's object list into
the object bank not on screen, writes the scroll screens' maps and the row-scroll table in video RAM, and writes
colours into palette buffers in video RAM. It also sets *shadow* words in work RAM: scroll positions, layer
control, the palette buffer and its pages. Then the vertical blank's interrupt hands the frame to the board in one
place: the object bank swapped, the shadows copied to the CPS-A and CPS-B registers and the object registers, and
the palette's base written last (writing it makes the chip take the palette). Nothing in video memory is read back
as anything but the game's own data: the games read video RAM only to fade and change colours they wrote, and never
read object memory after start-up.

So the boundary is clear. **Video RAM (maps, row scroll, palette buffers) and object memory are the game's data
structures**, kept where the 68000 has them, as work RAM is. **What goes native is the board:** the registers'
meanings, the latching at the vertical blank, and the drawing. The game-side records are made where the game hands
the frame over, from the shadows the game keeps. They are not made from the registers.

### The families (static comparison of all 22 games, census of the representatives)

Each game's interrupt handlers (the vectors at 68 and 70) and the routines they call were compared by shape, with
operands left out. The library comes in these versions.

The **census** column says which game was run (attract mode and its ladder movie); the other games of a family
were only compared by shape.

| Family | Games | Frame's commit (vertical blank) | Palette's upload | Object list | Mid-frame changes | Census |
| --- | --- | --- | --- | --- | --- | --- |
| **Alpha** | sfa, sfa2, sfz2al, sfa3, pocket_fighter, puzzle_fighter; vampire_savior, vsav2, vhunt2, vsav_definitive | shadows at A5+40.. to the registers; scrolls (sfa2 0014CC); layer control and object registers (sfa2 0013CE) | **one routine, the same version in all ten** (sfa2 00123E, vsav 001762; CRC 770DFAA6) | main loop, the bank not shown; a list builder and its piece writers | none seen (the Vampire games set the raster counters and have a handler, 001372, not reached in these runs) | sfa2, vampire_savior |
| **Marvel** | xmcota, msh, xmvsf, mshvsf, mvsc | one long routine (msh 000584, 000800, 00088E) | inline: msh and xmcota a constant buffer on the frames a counter says; mshvsf, mvsc, xmvsf a variable | main loop; the pieces' tables read from video RAM | **yes**: msh's raster interrupt (0007B6) changes scrolls 2-3 and the layer control | msh |
| **SF2** | ssf2t, hsf2 | 001A6C, 001AD4 | inline: a variable, or the constant 9240 on some screens | **main loop and interrupt**, a copy in video RAM, read back, the bank swapped from the main loop too | none seen | ssf2t |
| **Darkstalkers** | dstlk, nwarr | 0007D4, 0041C4, 0043A2 | the Alpha shape without the next buffer (00089E: two instructions) | main loop; the bank swapped from the main loop too (0017F4, 001834); a piece writer reads video RAM | none | dstlk |
| **AvsP** | avsp | 000E0E, 001214, 0012EC | inline (000EC4) | main loop | none | avsp (attract only) |
| **Giga Wing** | gigawing | 000A7C, 000AD8, 0038AE, 00393C | inline (000B30), the pages after it | main loop | none seen (it sets the raster counters) | gigawing |
| **CPS-1** | dino, punisher | another board: objects in video RAM, latched by the CPS-A's base (`LatchObjectsCps1`), no object bank | a constant (9140) | | | not run |
| **Classic CPS-1** | sf2ce, sf2hf, ghouls | as the CPS-1's, its tiles through the graphics mapper (`gfx_map`) | as the CPS-1's | | | sf2ce, sf2hf, ghouls (their smoke movies) |

Code of a game's own (or a family's) will be needed for: **SSF2T and HSF2**'s object list (two writers, read back),
**Darkstalkers'** bank swapped from the main loop, **Marvel**'s mid-frame changes, and the **CPS-1** games (another
board). The rest fits a library rewritten once with each game's addresses from `game.cfg`.

### Census: Street Fighter Alpha 2 (attract mode 4,000 frames, the ladder 4,800)

- **CPS-A, CPS-B, object registers, object bank:** written only in the vertical blank's interrupt (mask 4): the
  frame's commit at 001150 and its callees 00123E (palette), 0013CE (layers, object registers), 0014CC (scrolls).
  The start-up routines 00091A, 000968, 0009C0 and 000B48 set them once.
- **Object memory:** written only from the main loop, only the bank not on screen. 0127A4 starts the frame's list
  (two entries cleared, 0x380 entries at most) and calls the drawers by priority group. A dozen routines
  (0129B6-012FFE) put an object's pieces in the list, one entry each; 018820 writes the list's end.
- **Scroll maps:** about 15 routines in the main loop, from 0115FC to 06468E (stage tiles as the camera moves, text,
  the panel).
- **Row scroll:** 928000-92BFFF, written at start-up and by 0121CC and 0121F4.
- **Palette buffers:** about 40 routines (colours put, fades read and written) in 90C000-90CFFF and the other
  buffers the game switches between (92A0.., 9280.., 91C0..).
- **Raster interrupt:** not used.

### Census: Marvel Super Heroes (attract mode 4,000 frames, the ladder 6,000)

- **Registers:** the frame's commit in the vertical blank (000584, 000800, 00088E, 000742), as in Alpha but one long
  routine. The object bank is swapped as the interrupt starts (00053E).
- **The raster interrupt is used:** 0007B6 writes scroll 2 and 3's registers and the layer control part way down the
  picture (381 writes over the run: some screens only). The main loop sets the raster counters (CPS-B 4E-52) for it.
- **Object memory:** from the main loop, the bank not on screen. Some thirty piece writers from 019B02 to 01A5F6.
  They **read their pieces' tables out of video RAM** (924000-925FFF, 928000-92A0xx): the game keeps data there that
  the board never shows.
- **Palette:** the upload is only on frames when a counter says so (00066E): a constant buffer (9140), the pages
  always 3F.

### Census: Super Street Fighter II Turbo (attract mode 4,000 frames, the ladder 6,000)

The exception:

- **Object memory is written from the main loop and in the vertical blank's interrupt.** About thirty main-loop
  routines write each piece twice, to object memory and to a copy in video RAM (910000-910644). The interrupt
  (001D5C, 001D8E) copies that copy into the object memory's top part (708FB0-7095F4).
- **Object memory is read back** by the main loop (0C81E2, 0C83A4), and the object bank is swapped from the main loop
  too (0C682A, 0C69C2), as well as from the interrupt.
- **Palette:** a variable buffer, or the constant 9240 on some screens (001C5C, 001C6C).
- So its object list needs code of its own (HSF2's is the same program grown).

<!-- census: other families -->

### What each fighting game's stage code needs (for stages taken from one game into another)

The toolkit already shows other games' stages behind a game's fighters (`fight/guest_stages.h`; Vampire Savior
Definitive shows Darkstalkers' and Alpha 2's). It does that by running the guest's board beside the game's, drawing
the guest's whole picture, and laying the game's marked fighters and panel over it. With the interface, the guest's
code still runs on its own board (its CPU and RAM), but its picture is not drawn. Its stage's records (scroll
layers, row scroll, palette pages, stage objects) go into the game's frame as layers of their own.

What a stage needs. Two games are already described as guests (`games/<id>/guest.h`); the rest is to be found, one
game at a time, the same way: the census of a fight gives which scroll screens' maps and which palette buffers the
stage's code writes (by the CPS-A's bases at the time).

| | Scroll layers | Palettes | Stage objects | Camera | Music |
| --- | --- | --- | --- | --- | --- |
| Alpha 2 (`games/sfa2/guest.h`) | all three drawn today (`layers` 0xF); which carry the stage: to find | the scroll pages, and the object page's sets 27 and up | objects in colour sets 27 and up that are not the panel's tiles | asks where the players are (`Extents`, 01BFE0, 01C1D0); ends 256-640 | the stage's tune by number |
| Darkstalkers (`games/dstlk/guest.h`; Night Warriors at its own places) | all three today; to find | the scroll pages, the object page's sets 14 and up | colour sets 14 and up | an object stepped across and up (`Steps`, 0EDAF4, 0EDC1A); ends 256-640 | by number |
| Vampire Savior (`games/vampire_savior/guest.h`) | all three today; to find | the scroll pages, the object page's sets 22 and up | colour sets 22 and up but the players' shadows | asks where the players are (`Extents`, 01E8F4, 01E962); ends 256-640 | by number |
| Alpha 3 (`games/sfa3/guest.h`), alone | its stage's own, 017462 | the stage's (B718 and its objects) | colour sets 10-13 | asks where the players are (`Extents`, 018654, 01877C) | by stage (the character's theme) |
| Super Turbo (`games/ssf2t/guest.h`), alone | its stage's own, 0CC3EE | the stage's, faded in at once (00120A) | colour sets 0B and 1A-1F | an object stepped across and up (`Steps`, 0CC4E4, 0CC630); ends 0x100-0x280 | by stage |
| Alpha (`games/sfa/guest.h`), alone | its stage's own, 01A376 (not the first scroll screen: the panel's bars) | the stage's (011324, 011368, 0113A0) | colour sets 18 and 19 | asks where the players are (`Extents`, 01A9A8, 01AA2A) | by stage |
| Night Warriors (`games/nwarr/guest.h`), alone | Darkstalkers' at its own places | as Darkstalkers' | colour sets 14 and up | as Darkstalkers' (`Steps`, 0FF6C4, 0FF7EA) | by stage |
| Marvel Super Heroes (`games/msh/guest.h`), alone | its stage's own, 082346 | its own (007A1A) | colour sets 13 and up | its record set where its routines move it (`Place`, 082440, 0826AA), between the record's own ends | by stage |
| X-Men vs. Street Fighter (`games/xmvsf/guest.h`), alone | Marvel Super Heroes' at its own places | as Marvel Super Heroes' | colour sets 13 and up | `Place` (07025A, 0704D0) | a character's theme each (its stages have none) |
| Marvel Super Heroes vs. Street Fighter (`games/mshvsf/guest.h`), alone | X-Men vs. Street Fighter's at its own places | as X-Men vs. Street Fighter's | colour sets 1A and up | `Place` (0A1C94, 0A1F52), on the copy its step keeps (+76, +78; ends +68-6E) | a character's theme each |
| Marvel vs. Capcom (`games/mvsc/guest.h`), alone | Marvel Super Heroes vs. Street Fighter's at its own places | as theirs | colour sets 16-1D | `Place` (0CB2E2, 0CB560), on the copy | a character's theme each |
| X-Men: Children of the Atom (`games/xmcota/guest.h`), alone | its stage's own, 06EB00 | its own (003082, 003312) | colour sets 14 and up | `Place` (06EB84, 06ECAC), ends +70-76 | the stage's character's theme |
| Super Gem Fighter (`games/pocket_fighter/guest.h`), alone | its stage's own, 01E890 | its own (02790A on) | colour sets 1A and up but the shadows | asks where the players are (`Extents`, 01FAF0, 01FB66); ends 0-0x100 | by stage |
| Every other fighting game (sfz2al and the other Vampire games, whose stages are those above, pocket_fighter, xmcota) | to find | to find | its port's `notTheStage` where it has one | its port's camera facts (the widescreen and zoom features found them) | its port's `stageTunes` |

**A stage alone** (`Guest::StageAlone`, first Vampire Savior's): rather than a match played to a round on the guest's
board, the guest's board is started once and kept at the top of its main loop, and only its stage's code runs there:
the routines a round's first step calls to build the stage (Vampire Savior's 005A6E on: 016C4E, 00A4BA, 01C256,
01C2B0, 01C336, 017144, 004D60, 01C140, 02207E, 01DA3C, and its scroll screens switched on), then each frame the scroll
screens and the camera (01DA3C), the stage's pieces (07D240, 084738) and the object list (01ABC8); its frame interrupt
hands the frame over as the game's does, and the stage's tune is asked for in its sound ring. All twelve of its stages
are drawn so in Alpha 2's stage viewer; its music is the same, sample for sample, as from a match played to the round.

What a person playing sees first is whether the stage goes with the fight's camera, and the first stage-alone build did
not: its scroll screens' records were never put to work (a round's first step does that in its own lines, 005AE4, not
in a routine; `Guest::buildStep`), so the stage stood still. `--stage-check` now checks it each frame a borrowed stage
is shown: the guest's camera (`Guest::cameraAtX`, `cameraAtY`) against the place wanted for it, failing a run where it
is off by more than 8 pixels for more than 4 frames, or where no borrowed stage was shown. With it, three more things
were found and fixed: Vampire Savior's camera goes 5 pixels a frame at most (set each frame at 01DA82; raised to 16 for a
stage alone, within a strip of the 8-pixel scroll screen, `Guest::hooks`); a round's start puts the game's camera back
at once (the guest's scroll screens' records are started again, `Guest::roundStart`, and the camera brought to its place
in the same frame by its placing routine, `Guest::placing`); and the train's camera runs its arrival at a round's start
(01E098), which is passed over. All twelve stages in the viewer, Alpha 2's ladder and versus movies (widescreen too)
pass.

Darkstalkers' stages go alone too (`games/dstlk/guest.h`), on another kind of program: its game runs as tasks (start-up,
attract mode and a game each another), and a game's own loop (02E1AA, the modes at 02E1C0) is there only once a game is
begun. Its start-up is given a credit and Start (`Guest::beginAfter`) and stops at the character choice, where nothing
is chosen. What a match then does on its way to a round, the build does: the colour set every load of colours takes put
back to a match's (1390(A5): the character choice's 0C made the stages blue), every object put away (00521A: the
character choice's went on painting into the stages' maps), the stage (0059C6, 7734, 5C2C, 3EEE), the round's own task
(17CC with 0x804: its colours brought up), 5E90, 80CC, 76D8, 56516 (not 56B2 alone, which took an object for the
round's announcing and left it half made). Each frame, the stage's part of a fight's frame: two lists of objects (0D8C90:
Victor's lightning, Anakaris's fountain's colours; 0D8290: the sarcophagi), the camera and scroll screens (0ED9EC),
B925C, C05C8, EEECA, D310C and the object list (0017F4). Found with the call log (`CPS2RECOMP_CALL_LOG`, with
`CALL_TASK` and `CALL_ORDER`), the guest's memory against a match's, and by which code writes a colour set's sprites.
Victor's stage starts in its storm's colours until its lightning's first great strike, which comes when the game's own
random numbers say, as in a match. Checked: the twelve beside the match-played ones, frame for frame the same but for
their animations' and the lightning's moments; `--stage-check` passes on all twelve, Alpha 2's ladder and versus
(widescreen too) and Definitive's ladder with them (a stage built before it is shown is brought to the camera when it
comes into the picture); rollback the same.

Alpha 2's stages go alone too (`games/sfa2/guest.h`). Its start-up and attract mode keep a loop of their own (0017F6,
the modes called at 0017FE; a game has another, 004AC0), which is where its stage runs. Its stage is built a step a
frame before the versus screen (by A5+10, 0051B8): the scroll screens switched on, 01A88C, 01B1DE, 01C31E and 01B7FC,
its colours (012608 with its registers, 012510 to 0125D8), 0188B8, then 019774 and 01EB00 (its objects); and, as a
round's start does in its own lines (002446), its scroll screens' records put to work. Each frame: its objects (01E308,
01E582, 01E640), its scroll screens and camera (01A7F6), its object list (01E6AE), its colours given over (0127A4). The
routines were found with the call log run on the game's own code (`--no-rewrites`: Alpha 2's port has many rewrites,
whose calls the interpreter does not see). Checked: all eighteen beside the match-played ones in Definitive's viewer
(the same but for their animations' moments), every one passing `--stage-check`, and a training session on Ken's stage
with both players walked to each end (no frame off); rollback the same; Definitive's hashes main's with the feature off.

Hosts: Vampire Savior, Vampire Savior 2 and Vampire Hunter 2 show Darkstalkers' and Alpha 2's stages too
(`games/vampire_savior/src/stages.cpp`, called from their family.h's `RegisterOwnSources`; Definitive keeps its own).
Their check found three more things: Alpha 2's camera goes 5 pixels a frame across and 1 up as constants in its code
(01B6A0 on: hooked, as Vampire Savior's is raised); what the port calls in a guest's loop took the guest's frame, so a
heavy frame (a stage's build) left Darkstalkers' loop a frame short and its camera a frame late (the calls now take none
of it); and the host's own stage under a borrowed one still moves the host's camera: the train's arrival and Fetus of
God's shake took it 32 pixels below where any other stage goes. So a stage chosen by name puts Feast of the Damned under
it, and the arcade's choice passes over those two. Every host's ladder passes `--stage-check`, plays as main's with the
feature off, and runs again the same (`--rollback-test`).

Super Turbo's stages go alone too (`games/ssf2t/guest.h`), in Definitive. Its start-up and attract mode keep a loop
(008FA0, the modes called at 008FA8). Its stage is built as a match's steps before a round build it (00D758 on: 003440,
003AAE, its colours 00258C, 0026FC, 002886, its pieces 068E96, 0C84AC, its scroll screens 0CC3EE); then, as a round's
fade-in does, its scroll screens switched on (bits 1-3 of 4E(A5), the word the frame interrupt gives the layer control)
and its colours, written dark, brought to full (00120A with D2 0x1000, fifteen times). Each frame: the object pool
(02822A, its stage's pieces among them), 0C8E1C, the scroll screens and camera (0CC3EE), 0BE518 and the sprite list
(0C66CA). The first build drew nothing at all and passed the check, which looked only at the camera: `--stage-check`
now also fails a run whose borrowed stage's picture is one colour for more than half a second. (`CPS2RECOMP_GUEST_DUMP=
FRAME:FILE` writes the guest's memory and video memory, to set beside a match's `--dump-ram`.) Checked: all sixteen
beside Super Turbo's own viewer (the same but for their animations' moments and the camera's place), every one passing
the check, a training session on two of them with both players walked to each end, and the music.

Darkstalkers' stages had no music: their build asked the sound program to stop everything (7734), and the frame
interrupt, sending that, empties the sound ring, the stage's tune asked for after it too. The build leaves it out now;
the tune is the first stage's, as in Darkstalkers' own viewer.

The survival, the team battle and the endless battle (its one stage) take other games' stages too, from the menu's Stage
page as every other fight does (`session::kStageFeature`: the game's own stages, then the others'): a match on one is
fought on the game's first stage, the other drawn behind it (`BattleStage`, read by the host's `wanted`). The endless battle's Shuffle and Scroll go on through the
game's own stages: each shows two stages at once, made on the game's spare boards.

Alpha's stages go alone too (`games/sfa/guest.h`). Its start-up and attract mode keep a loop (001996, the modes called
at 00199E). A match builds its stage by a task that wipes the picture over before the versus screen (009CC6 with D1
0x0C, a step a frame); the build does its steps at once: the memory of objects set out again (016170, as a match does
before its character choice: attract mode leaves the three pools' free lists used up, so the stage could make none of
its objects), the scroll screens' records put to work, the scroll screens (01A37C), 01A754, 011302, 0112D4, 019A6A,
019AE6, its colours (011324, 011368, 0113A0), its objects (01BC9A, a table by stage) and the layer words a round sets
(004874). Each frame: the three pools (01BBD6, 01BA48, 01BC08), the scroll screens and camera (01A376) and the sprite
list (011B3A), with the fight's frame counter stepped (004900) and two players standing either side of the picture's
middle: its cats walk about by where the players are, and with none walked to the stage's left end and away. Its sound
ring is at A5-59C2, FF263E (the port had FFA63E, which made the stage viewer's **Music** choice write into an object
pool and play nothing new; fixed). Checked: all thirteen beside Alpha's own viewer, every one passing the check, a
training session on two of them (rollback the same), the music.

Night Warriors' stages go alone as Darkstalkers' do (`games/nwarr/guest.h`): its program is Darkstalkers' grown, so
the description is Darkstalkers' at Night Warriors' places, found by setting the two programs' listings side by side
(the instructions around each place, with their numbers left out, matched where they are alike only once) and from
games/dstlk/family.h (its memory: the stage's block 0x600 lower). Checked: all fourteen beside its own viewer, every one
passing the check, a training session on two of them (rollback the same), the music.

Marvel Super Heroes' stages go alone too (`games/msh/guest.h`), with a third way of putting a guest's camera
(`Guest::Camera::Place`): its camera is a record whose place differs by stage (FF4980, FF4A00, FF4B00) and whose ends
are the record's own, so the routines that move it are hooked where they move it (082440 across, 0826AA up and down,
after their checks), the place set there and the routine returned from, the place wanted put as far between the
record's ends as the game's camera is between its own (`cameraRecord`, `endLeft` and the others; its place up and
down counts down the picture, `placeDown`). Its game is a task, its loop (007EB0) there once a game is begun, as
Darkstalkers'. The build: the lists of objects emptied (0035BE), the pools set out (003844, 003656), the round's words
(00C7E4), its colours (007A1A), the stage (082192), 004A3A, the colours brought up (007890). Each frame: the lists of
objects (01AEC8, 01AF96, 01B0A4: its pieces, the city street's people, the lake's posts), its scroll screens and
camera (082346), the object list (01B34E) and the frame's end (019B02: the lists of objects to be drawn, which each
piece puts itself on, made empty; left full they grew down over the lists of objects in 500 frames and the board
went astray). Stark Enterprises opens each round with its camera sliding down from the sky and its machinery coming
in: played through at once when its stage is built. Checked: all twelve beside its own viewer (the same but for their
animations' moments: the lift's buildings and the neon city's skyline go by, the gem chamber's colours turn), every
one passing the check, a training session on two (rollback the same), the music.

X-Men vs. Street Fighter's stages go alone as Marvel Super Heroes' do (`games/xmvsf/guest.h`): the same engine, so
the description is Marvel Super Heroes' at this game's places, by the two listings set side by side and the match's
steps and the fight's frame read beside each other. Its stages have no tunes of their own (a round plays a character's
theme): each is given one. Checked: all eleven beside its own viewer (the same but for their animations' moments: the
space base's lights go round, the tree's leaves), every one passing the check, a training session on two (rollback
the same), the music.

Marvel Super Heroes vs. Street Fighter's stages go alone as X-Men vs. Street Fighter's do (`games/mshvsf/guest.h`),
but its camera's step works on a copy of the place (+76 and +78), kept between ends of its own (+68 to +6E) and put
back by the stage's code: the copy is what is set. Definitive now shows nine games' stages, past the eight a game could
have (`kMostGuests`, now sixteen): the ninth was left out without a word, and the check's "no frames shown" found it.
Checked: all twelve beside its own viewer (the same but for their animations' moments: the desert base sinks into its
hangar and on to its alarm, as in the game), every one passing the check, a training session on two (rollback the
same), the music.

Marvel vs. Capcom's stages go alone as Marvel Super Heroes vs. Street Fighter's do (`games/mvsc/guest.h`). One more
thing a stage reads that only a round runs: the canyon turns to dusk and to night as the round's clock (A5-3FF8) runs
below 60 and 30 (0CDD26), and with nothing running it, it read 0, night from the start; it is set full after the build.
(A guest's clock could follow the game's round's, the canyon darkening with it: not done.) Checked: all ten beside its
own viewer, every one passing the check, a training session on two (rollback the same), the music.

X-Men: Children of the Atom's stages go alone too (`games/xmcota/guest.h`), the engine's first game, read beside
Marvel Super Heroes'. Its stages wait for a round in more ways: Avalon's camera stays at the top until a round begins,
then comes down (07399A: sent to the floor at once, as when that opening is passed over); the Danger Room shows a
simulation chosen by a piece that goes by the players' states (062686), so with none it was the bare room (its first,
the jungle, is given at once: 070E82). The sunset's sun glows with a round's fighters and is left without. 000D66, a
round's first step's clearing, waits for its sound program and is left out. Checked: all twelve beside its own viewer
(the same but for their animations' moments: the Savage Land's sky, the frozen sea's voyage, and the sunset's glow),
every one passing the check, a training session on two (rollback the same), the music.

Super Gem Fighter's stages go alone as Vampire Savior's do (`games/pocket_fighter/guest.h`): the same engine, read
beside it, its build a round's first two steps (005A0E on) and its frame a fight's (007DDE on), the pools of objects
but the players'. Checked: all eight beside its own viewer, every one passing the check, a training session on two
(rollback the same), the music. With it, every fighting game in the collection with stages of its own is a guest
(Hyper Street Fighter II's are Super Turbo's, Zero 2 Alpha's Alpha 2's, Vampire Savior 2's and Vampire Hunter 2's
Vampire Savior's).

Hosts: Vampire Savior Definitive, Vampire Savior, Vampire Savior 2, Vampire Hunter 2, Alpha 2, Zero 2 Alpha, Alpha 3
and Alpha show every other fighting game's stages, from one list (`fight/all_guests.h`: each takes it without its own, so
the stages keep the numbers they had). Alpha 3's stage viewer reaches its round through the character choice, its
camera 768 up on the way: its borrowed stage is shown once the round is on. Alpha's panel (bars, names, clock) is a
scroll screen, not objects: a host says which of its scroll screens are its panel (`Host::panelLayers`), drawn marked
over the borrowed stage as its objects are. Every host's ladder plays as main's with the feature off, runs again the
same (`--rollback-test`) and passes the check.

Played, the borrowed stages showed what the checks had let through, plain to a person: the fighters drawn over pieces
that are in front of them in their own game (Fetus of God's bulbs), Alpha 3's stages without any of their objects (the
castle, the trees, the grass: its frame called neither the pools of objects nor the object list, 020C4A; and its own
viewer left objects out altogether, so the comparison had missed it), pieces standing still that go by the fight's
frame counter (Vampire Savior's searchlights at the Tower of Arrogance), and the Marvel stages' wide cameras racing
across the middle. Now:

- **In front of the fighters.** Each game's object list routine puts its draw lists in turn, and the fighters' among
  them (`Guest::fightersDrawn`: where they are in, the next entry's place in an address register). The stage's objects
  put in after them are marked (bit 12 of the entry's first word, which the board does not read), and the picture's
  pixels they show at have 0x40 in the top byte (`Video::markFront`; with `frontLevel`, a scroll screen's pixels an
  object of the fighters' level is behind too). The game's objects are marked as before, and its panel's 0x40 too
  (`Video::panelObject`, the host's `panel`): put together, the stage's front pixels go over the fighters and under the
  panel. Both pictures do it, the CPU's and the GPU's (the marks in the alpha, put together in the frame's last pass: colours.frag's `stage`).
- **The fight's frame counter** a step a frame (`Guest::frameCounter`, B4(A5) in Vampire Savior's and the Alphas').
- **The camera** goes as far as the game's from the middle (`Across`): a stage wider than the game's goes no faster,
  its ends beyond the game's not reached; one narrower goes as far between its ends as the game's between its own. A
  Marvel stage's place by its record's own ends is taken from the game's camera the same way. The old way spread the
  game's camera over the guest's whole width, three times as fast on the skull canyon, and its steps past 24 pixels
  started the guest's round over each frame (its pieces running fast with it).
- **Alpha 3** runs its pools of objects (033696, 033522, 0336D6, 0335E0) and its object list each frame, and its own
  stage viewer shows the stage's objects (the panel and fighters left out as in a match's).

And a check that looks at the picture as a person does (`--stage-look FILE`, `tools/stage_look.py`): the stage viewer of
the game showing another's stage and of that stage's own game each record their stage's picture, the camera held still
in the middle and at each end with the stick; where both held it at the same place, every pixel of the shown stage is to
be a colour the own game has there at some moment, its picture is to change about as much from frame to frame, and its
camera is to go no faster. `tools/stage_look.py sweep` does it for every stage of the guests named (the first sweep:
most alike; it found Super Turbo's breakable crate and barrel at Guile's and Ken's ends missing, as
Night Warriors' lamp post at Jon Talbain's (what makes them not yet found: not the frame's routines, which the guests
all run); a stage edge column
not painted at Blanka's right end; and own viewers that are wrong themselves, Darkstalkers' and Marvel Super Heroes'
showing the fighters at a round's start, X-Men: Children of the Atom's sunset moving on with its clock).

What this asks of the interface: **a frame holds layers from more than one producer** (the game and a guest). Each
producer has its own palettes, its own tile graphics, its own priorities, and its own scroll and row scroll. The
frame's layer order says how one producer's layers go between the other's: the guest's stage behind the game's
fighters, shadows and panel.

## The interface (`video/native.h`)

Made for a GPU to draw from: flat arrays of fixed size, uploaded as buffers. There is nothing to walk and no pointers
into the 68000's memory.

```cpp
struct Producer {                  // the game, or a guest whose stage is shown
    Graphics *tiles;               // its graphics ROMs as an indexed texture (once, at load)
    uint16_t palette[6][512];      // after this frame's uploads (the colour words; the shader makes RGB)
    struct Layer {                 // scroll 1-3
        uint8_t tileSize;          // 8, 16, 32
        bool on;
        uint16_t scrollX, scrollY;
        const uint16_t *map;       // 64 x 64 entries of (tile, attributes): a GPU buffer, re-sent when written
        bool rowScroll; uint16_t rowOffset; const uint16_t *rows; // 1,024 words (scroll 2)
    } layers[3];
    struct Object { int16_t x, y; uint32_t tile; uint16_t attributes; uint8_t level; } objects[1024];
    int objectCount;
    uint16_t layerControl;         // order and enables
    uint16_t priorityMasks[4];     // which pens of a scroll screen are over the objects of each level
};
struct Band { int firstLine; /* the registers above that changed from this line */ };
struct Frame {
    Producer producers[2];         // [0] the game; [1] a guest stage, or none
    struct Slot { uint8_t producer, layer; } order[8]; // back to front; a guest's layers between the game's
    Band bands[8]; int bandCount;  // raster effects: what changed part way down
    int side;                      // widescreen columns each side (Video::wide)
};
```

On the GPU, each scroll layer and the object list are instanced quads, one per tile, with the tile number, place,
flips and colour set in the instance. The fragment shader reads the indexed tile texture and the palette (a
512 x 12 texture: two producers' six pages). Priority between objects and scroll pens (the CPS-2's masks) is a
per-pixel decision, so each layer writes its pen's priority group to a stencil or depth value, and the objects test
against it. Row scroll is a per-line offset buffer for the scroll 2 pass, and bands are a scissor per run of lines.

The **CPU rasteriser** draws the same `Frame`, line by line, for check (b). It is the reference against
`video.cpp`. It does not draw what the player sees.

Done so far: `PaletteUpload` (one upload: the pages taken, the buffer, the colours), `ObjectList` (the list the board
takes for a frame: its 1,024 entries, the priority control and the offsets) and `Layers` (the registers a line is drawn
with: the maps, the scrolls, the row scroll, the layer control, the objects' offsets).

## The checks

- **(a)** `--video check`. Each lift hands the interface its record, then makes the board writes the game's code made.
  The board model decodes the same record from its registers and memories at the moment it acts on them
  (`video.cpp`'s reading of them). The two must be equal. A record only the board made comes from game code not
  native yet, and is counted with the first frame it came in. Any of those, a record only the game's side gave, or one
  that differs, is a failure (exit 6). Each machine has its own interface (`Machine::nativeVideo`, reached by the
  lifts through the CPU): the spare and guest boards run the same code, and only the port's own machine is checked. This does not run with `--check-rewrites`, which runs the original as well and so
  makes every upload twice.
- **(b)** the CPU rasteriser's picture (`video/native_render.h`) against `video.cpp`'s, every line, with
  `--video check`: [below](#check-b-the-picture-from-the-records).
- The rewrites among the lifts are checked against the game's code by `--check-rewrites` and `make check-rewrites`.
  The rest are **watches** (`lift.h`, `AddWatch`): told before an instruction runs, they only look. Unlike a hook, a
  watch is not a place where translated code starts a block, so it is not a place where an interrupt can be taken:
  the translated code is the same, but for the call (`tools/recomp.py --watches`). Hooks were tried first, and in
  Super Turbo the hooks at its main loop's bank swaps moved where the vertical blank came, and the game went
  differently from frame 5,464 of its ladder. For both, a run's frame hashes (`--hash-log`) must be the same as a
  build's without them.
- The game's side keeps state of its own (what the game last said to each register, the list it handed over). It is
  not yet in the save state, so a check run is to start from the program's start, not from a loaded state or a
  rollback.
- **Board timing:** with `arcade_timing`, rewrites run only when a game's features need them, and then without the
  bus's waits (`runtime/recomp.cpp`), as for every rewrite. A video lift that is to run always must count the waits
  of the instructions it replaces. That comes before the native renderer is the default.

## The first slice: the palette's upload

The upload is what each family hands the board at one or two instructions. Two kinds of lift cover them all.

**The Alpha library's commit routine**, in the vertical blank's interrupt, is rewritten once (`CommitPalette`):

```
move.w pages(a5),<CPS-B palette control>   the pages the chip is to take
move.w base(a5),<CPS-A palette base>       where from; writing it makes the chip take them
move.w next(a5),base(a5)                   next frame's buffer, as the game asked this frame
rts
```

Its operands (the three variables, the two registers) are read from the game's own copy, which `game.cfg` names with
its version: `video_palette_commit = 00123E:770DFAA6`. At start-up the toolkit checks the CRC.

**Everywhere else** the upload is one instruction inside a bigger routine (the other families' commits, every game's
start-up). Each is a `move` to the register. A watch there works out, from the CPU as the instruction finds it, what
the instruction writes and where, and tells the game's side. The game's side keeps what the game has said to each
register, and makes the upload's record when the palette's base is written: from the game's buffer and the pages it
last asked for. `game.cfg` lists the instructions:

```
video_writes = 0009D0 0009D8 000B7E 000BB4 ...
```

At start-up the toolkit reads each instruction and takes it only if it is a `move.b` or `move.w` it can read: from a
data register, an address register's place, a variable or a number; to a fixed address that is a video register, or
to an address register's place, which is looked at as it runs. When the frame's commit is rewritten whole (step 3
below), these watches become part of it.

`build.mk` gives the translator the rewrites' addresses as hooks (`video_hooks.txt`) and the instructions as watches
(`video_watches.txt`).

The two kinds were first put in eight games, one or two of each family's version. Results, with `--video check`
over attract mode (4,000 frames) and the ladder movie (6,000; Giga Wing's with `free_play,invincible`, the rest
`free_play,no_damage,one_hit`), and the ladder's frame hashes against a build without the lifts:

| Game | Lifts | Attract mode: uploads the same | The ladder: uploads the same | Frame hashes |
| --- | --- | --- | --- | --- |
| sfa2 | commit routine + start-up hooks | 3,941 | 5,939 | the same, 6,000 |
| vampire_savior | commit routine + start-up hooks | 3,941 | 5,941 | the same, 6,000 |
| dstlk | hooks (two-instruction commit) | 3,957 | 5,957 | the same, 6,000 |
| msh | hooks (constant buffer, on demand) | 1,516 | 3,060 | the same, 6,000 |
| mshvsf | hooks (variable buffer) | 3,926 | 5,926 | the same, 6,000 |
| ssf2t | hooks (variable or constant) | 3,968 | 5,968 | the same, 6,000 |
| avsp | hooks | 3,934 | (no ladder movie) | attract mode the same, 4,000 |
| gigawing | hooks | 3,734 | 5,826 | the same, 6,000 |

In every run none differed, none came only from the game's side, and none was made by code that is not native (the
start-up's uploads included). The builds without the lifts were this branch's before them (sfa2, vampire_savior,
dstlk, msh, ssf2t) and the `widescreen-stage-ends` branch's (avsp, gigawing, mshvsf), whose sfa2 gives this branch's
hashes, all 6,000 frames.

Before the hooks, the commit routine alone (sfa2, Vampire Savior): `--check-rewrites` over the ladders, 4,738 and
5,939 calls, 0 differed, the cycles exact (68 a call). The frame hashes with and without it
(`CPS2RECOMP_SKIP_REWRITES=CommitPalette`) were the same, all 4,800 and 6,000.

## The second slice: the object list

The board draws a frame from the object list it takes as the vertical blank starts: the 1,024 entries of the bank of
object memory the bank register names, with the object registers' priority control and offsets. The game fills one
bank through its window at 708000, while the other is shown, and then names the bank it filled.

The game's side is told by watches at every instruction that writes the object registers (400000-40000B) or the
bank (8040E0), the same `video_writes` list. When the game names the other bank it hands over the list it filled: the
1,024 entries in its window at 708000, at that moment. Naming the bank already shown, it keeps the list it handed over
last. At each frame's start the board's list (`DecodeObjects`: the bank latched, the registers) must equal the game's
(the list handed over, the registers as the game last set them). A list the board takes before the game has named a
bank at all (the first frames, as the program starts) is counted, not compared.

The instructions are the census's and the listing's (every `move` to a fixed object register or the bank). Games
whose object list needs code of their own (above) pass as well: what they do between the hand-over and the next
frame's start does not change the list the board takes.

Results, the same runs as the palette's:

| Game | Attract mode: lists the same (before the game's first) | The ladder: lists the same (before) | Frame hashes |
| --- | --- | --- | --- |
| sfa2 | 3,999 (0) | 5,999 (0) | the same, 6,000 |
| vampire_savior | 3,999 (0) | 5,999 (0) | the same, 6,000 |
| dstlk | 3,955 (44) | 5,955 (44) | the same, 6,000 |
| msh | 3,930 (69) | 5,930 (69) | the same, 6,000 |
| mshvsf | 3,930 (69) | 5,930 (69) | the same, 6,000 |
| ssf2t | 3,998 (1) | 5,998 (1) | the same, 6,000 |
| avsp | 3,999 (0) | (no ladder movie) | attract mode the same, 4,000 |
| gigawing | 3,922 (77) | 5,922 (77) | the same, 6,000 |

None differed. The palette's uploads, now from the watches, are as in the table above, all the same.

## The third slice: the layers' registers

`video.cpp` reads the layers' registers on every line it draws: where each scroll screen's map is and where it is
scrolled to, the row scroll's table and place and whether it is on, the layer control (the order and which are on),
and the objects' offsets. A game can change them part way down the picture (Marvel Super Heroes does, from its raster
interrupt), so they are compared line by line: at each line the board draws, the registers it draws with
(`DecodeLayers`) must be what the game's side has the game last saying to each.

The game's side hears it from the same `video_writes` watches, now with every instruction that writes CPS-A or CPS-B
(the census's, of any form the watch reads, and the listing's `move`s and `clr`s to a fixed register; a watch reads
`move` of a long and `clr` too, which write two registers at once). Some games write registers from their own rewrites
(Alpha 2's `FrameRegisters`, the palette's commit): whatever a rewrite writes to a video register is the game's word by
being a rewrite's, and is told to the game's side as it is written (`M68K::nativeWrites`); the game's own code a
rewrite calls is not, its instructions are watched. The registers start as the board's do (the places of the maps it
starts with), until the game says otherwise.

Results, the same runs:

Every line drawn, 896,000 in each attract mode run (4,000 frames) and 1,344,000 in each ladder (6,000), was drawn with
the registers the game's side has, in all eight games (Marvel Super Heroes' lines below its raster interrupt's changes
among them); the palette's uploads and the object lists as before, all the same; the frame hashes the same as without
the lifts.

## Every CPS-2 game

The other thirteen CPS-2 games (sfa, sfz2al, sfa3, pocket_fighter, puzzle_fighter, vsav2, vhunt2, vsav_definitive,
hsf2, nwarr, xmcota, mvsc, xmvsf) got their lines from their listings alone: the Alpha games' commit routine (the same
version in each), and every `move` or `clr` to a fixed video register. With all three records checked over attract mode
and the ladder (Vampire Savior Definitive: attract mode; it has no ladder movie), every palette upload, every object
list and every line was the same from both sides, none was made by code not native, and the frame hashes were the same
as the `widescreen-stage-ends` builds' (without the lifts), all of each run's frames. So in all 21 CPS-2 games the
game's side now has every palette upload, every object list and every line's layers. The CPS-1 games (dino, punisher)
followed: [below](#the-cps-1-games).

## The CPS-1 games

Cadillacs and Dinosaurs (dino) and The Punisher (punisher) are on the CPS-1 board. They have no object bank and no
object registers: their object list is 256 entries in video RAM where CPS-A's first register says, and the board takes
it as the vertical blank starts (`Video::LatchObjectsCps1`). Their palette control and layer control are CPS-B
registers at other places, which `game.cfg`'s `cps_b` gives, and they are drawn with CPS-B's four priority masks: the
pens of a screen in front of the objects.

So for them the game's side makes the object list from video RAM where the game said the list is (its word to CPS-A's
first register), when the board takes it (`DecodeObjectsCps1`), and a line's layers have the four masks too. The
palette's upload is as on the CPS-2, with the board's palette control. Their `video_writes` are every `move` or `clr`
to a fixed video register in their listings.

Over attract mode (4,000 frames each), and The Punisher's ladder (25,000 frames, into its second stage, with its cheats:
`free_play,infinite_health,infinite_time,infinite_lives`), every palette upload, object list and line was the same from
both sides (The Punisher's ladder: 24,940 uploads, 24,999 lists, 5.6 million lines). The frame hashes were the same as
the `widescreen-stage-ends` builds' (without the lifts). The picture drawn from the records (check b) is the CPS-2's
only so far.

The classic CPS-1 games (sf2ce, sf2hf, ghouls: `board = cps1-classic`, #163) are drawn the same way, with one thing
more: their graphics mapper (a PAL) does not keep each kind of tile where its number says, and `game.cfg`'s `gfx_map`
gives its banks and ranges (machine/romset.h). video.cpp reads the ranges as it draws; the picture from the records and
the GPU's take them as a table (`GfxTable`: for each kind, objects and the three scroll screens, and each 0x100 of its
8-pixel units, a base and a mask; a range that is not whole steps makes none, and says so). An object's tiles are put
through it as the frame's list is taken (one it has nowhere gets a tile past the graphics' end, so not drawn); a scroll
screen's tile as it is drawn, on the GPU from the table after the lines' records (`L_GFX_ON`). Their `video_writes` are
the CPS-A's registers and the CPS-B's video ones (layer control, priority masks, palette control; not the CPS-B-21's
multiplier at 800140, which the game also writes): every `move` or `clr` to one in the listing, with what a census of
the smoke movie saw. Over each game's smoke movie (40,000 frames), every upload, list, line and picture line was the
same from both sides, and the GPU's first 6,000 frames the same as the board's.

## Check (b): the picture from the records

`video/native_render.h` draws the picture from the records alone: the palette as the game side's uploads made it,
the object list the game handed over, and on each line the layers' registers as the game set them. With them it
reads only the game's own data, the scroll screens' maps and the row scroll's table in video RAM, and the graphics
ROMs. It reads no register of the board's.

It is laid out the way a GPU will draw: once a frame, the object list becomes a list of tiles (`ObjectTile`: one 16x16
tile each, front to back, with its place before the offsets, tile, colour set, flips and level), which the GPU will
draw as instances. On each line the scroll screens' tiles and the objects' are put down in the layers' order, each
object pixel in front of or behind the screens' pens by its level. This one draws on the CPU, a line at a time, as the
reference the GPU renderer must equal.

What a game's sources set on the picture (`PictureOptions`, from `Video`'s members of the same names: a wider picture
and its columns, the screens and objects moved in it, the tiles beyond the board's columns that the game's sources
give, holes filled, objects hidden or the list changed, the marks a picture put together from two reads) it follows
as `video.cpp` does.

`--video check` compares each line it draws with the line `video.cpp` drew, pixel for pixel and mark for mark, at the
picture's width (`Video::ComparablePicture`: a CPS-2's picture from its own graphics). Lines before the game's first
object list are not compared.

Results:

Every line compared was the same in all 21 CPS-2 games: 895,962 in each Alpha-family attract mode run (4,000 frames),
1,343,962 in each ladder (6,000), and so on for each game. The lines not compared are those before the game's first
object list (38 in the Alpha games; up to 17,258, Giga Wing's 77 frames before its first). Two kinds were left out at
first: **Vampire Savior Definitive**, which draws some objects and tiles from Vampire Savior 2's graphics (its extra set:
now in the records, [below](#another-sets-graphics-vampire-savior-definitive)); and the CPS-1 games, another board. The
frame hashes were the same as without the lifts.

**With widescreen** (the ladder with `widescreen` on, in the 16 games that have it and a ladder movie: sfa, sfa2,
sfz2al, sfa3, pocket_fighter, vampire_savior, vsav2, vhunt2, hsf2, ssf2t, nwarr, xmcota, msh, xmvsf, mshvsf, mvsc), every
line compared was the same too, at the picture's wider width: 1,343,962 in each Alpha-family ladder, with the screens
and objects moved where each game's widescreen moves them, the tiles its sources give beyond the board's columns, and
holes filled.

## The GPU renderer: the plan

The records are already what the GPU needs, and `native_render.cpp` already draws the way the GPU will. On the GPU each
frame:

- **What it reads.** These are buffers, uploaded as they change:
  - the graphics ROMs as a storage buffer, once at load; the shader unpacks a tile row's four bit planes itself, as
    `Unpack` does, so there is no atlas to build;
  - the palette, 0xC00 colours made RGB, after each upload;
  - video RAM (0x30000 bytes, the game's maps and row-scroll table), the ranges written since the last frame;
  - the 224 lines' `Layers`;
  - the frame's `ObjectTile`s, as instances.
- **Pass 1, the scroll screens.** A fragment shader for every pixel reads its line's `Layers`, walks the three screens
  back to front in the layer control's order, and writes the pen and the screens' marks (a 16-bit target). Row scroll
  is a lookup in the table by line, and widescreen's moved screens are numbers per line in the same buffer. The tiles a
  game's sources give beyond the board's columns (`PictureOptions::tile`, C++ the shader cannot call) are asked for on
  the CPU, once a frame per screen and column, and sent as a small override map.
- **The maps change while the board draws.** The games' main loops write tiles into the maps (stage tiles as the camera
  moves, text) while the board is drawing the picture, and the board draws each line from the map as it is then. Drawn
  from the maps as they were at the frame's first line, the ladders differ from the board on 358 lines in sfa2, 1,577
  in msh, 10,747 in vampire_savior and 25,089 in ssf2t (of about 1.34 million each). With the row scroll's table read
  live and only the maps from the frame's start, the counts are the same, so it is the maps. So pass 1 is drawn **in
  bands**: the CPU keeps the frame's writes to video RAM with the line each came at. A new band starts at a line where
  a write to a map was made, and that band's writes are put into the GPU's copy before it is drawn, with a scissor for
  its lines.
- **Pass 2, the objects.** These are instanced 16x16 quads, each at a depth by its place in the list, depth-tested so
  that the front-most object pixel is the one left at each pixel, as on the board, whatever order they are drawn in.
  The colour pass then reads the marks from pass 1 and keeps either the object's pen or, where the object's level puts
  it behind those screens, the pen that is there. Either way the object has taken the pixel, so a masked object still
  hides the ones behind it (the CPS-2's way, and `DrawObjects`'s). `hideObject` and `object` (C++ again) are asked once
  a frame per entry.
- **Pass 3, the colours.** The pen goes through the palette into the picture texture, with the marks in a second
  target for the pictures that are put together from two (`markObjects`). The presenter then samples that texture as
  it samples today's upload.

Where pixels are still wanted on the CPU, the CPU rasteriser gives them. Those are the frame hashes, screenshots,
headless runs and the checks, the guest stages' picture put together (until item 8 does without it), and the two
presenters that are not SDL_GPU (SDL's renderer, and OpenGL for shader presets). A third check, `--video gpu-check`,
reads the GPU's picture back now and then and compares it with the CPU rasteriser's, as (b) compares that with
`video.cpp`'s.

The work: the three shaders and their buffers (`tools/gen_shaders.py` makes them for Metal, Vulkan and Direct3D 12 as it
does the presenter's) L; the frame's records kept for the GPU and the presenter taking its texture M; the read-back
check S.

## The GPU renderer, first slice (`port/gpu_picture.h`, `--video gpu-check`)

`port/gpu_picture.cpp` draws a frame's record (`NativeVideo::FrameData`, below) with SDL_GPU, in three passes whose
shaders are `video/shaders/`, compiled by `tools/gen_shaders.py` into `video/picture_shaders.inc` (SPIR-V and Metal):

1. **The objects** (`objects.vert`, `objects.frag`) as one instanced draw of the frame's `ObjectTile`s. Each tile's
   depth is its place in the list, front-most smallest, and the depth test is "less", so the front-most object pixel is
   the one left at each pixel, whatever order the GPU draws them in. A clear pixel writes nothing. Out: the pen, the
   level, and that an object is there.
2. **The scroll screens** (`layers.frag`), a pixel at a time, as `DrawScroll` does, in **bands**: before each band
   its writes to video RAM go up, and the pass draws its lines with a scissor. Out: the front pen and the screens'
   marks.
3. **The colours** (`colours.frag`), band by band with the band's palette: at each pixel, the object's pen where its
   level puts it in front of the screens' pens there (the masks, from the line's layer order and the priority
   control), else the screens' pen, then its colour.

A shader reads storage buffers (the graphics ROMs, video RAM, the lines' records, the tiles) or textures (the passes'
results, the lines, the palette), never both: SDL_GPU numbers a fragment shader's storage buffers after its uniform
buffers on Metal and after its textures in SPIR-V.

`--video gpu-check` does what `--video check` does, and also draws each frame on a GPU device of its own (SDL's
offscreen video driver: no window), reads it back and compares it with the board's lines. It draws frames whose
objects' offsets stay the same on every line, with what a game's sources set on the picture (below), and counts
other frames as not drawn there.

Results:

Over attract mode (4,000 frames) and the ladder (6,000) in the 20 CPS-2 games compared (all but Vampire Savior
Definitive, which draws from another set's graphics), every frame drawn on the GPU (Metal, on the Mac mini) was the
same as the board's, pixel for pixel: 3,999 and 5,999 in the Alpha games, and so on. None needed the CPU: the objects'
offsets stayed the same down every picture, Marvel Super Heroes' included. Taking objects away from the second pass
(a test of the check) made the frames with objects on the screen differ at once.

### A game's picture settings on the GPU (widescreen)

What a game's sources set on the picture (`PictureOptions`) is drawn on the GPU too, except the marks (pictures put
together from two) and layers left out. These settings are C++, asked as `video.cpp` draws each line and reading the
game's memory then, so they are asked on the CPU at the time the board would ask, and their answers go to the GPU with
the frame:

- **A wider picture** (`side`): the textures are as wide as the widest picture (512 columns), and a frame is drawn
  as wide as its own. The textures the presenter samples are made at the frame's width, which the presenter takes as
  the picture's.
- **Screens moved** (`scroll`, `rows`): where the board's own columns start for each screen, on each line, is in the
  line's record.
- **Tiles beyond the board's columns** (`tile`): asked on each line as the board draws it (`Raster::Overrides`,
  with the frame's record: `TileOverride`). The first pass takes a line's from a small list, in the map's place.
- **Objects moved and hidden** (`object`, `hideObject`): asked for each entry when the list is taken. A tile's move
  goes with it to the GPU; a hidden one is not sent.
- **Holes filled** (`fillHoles`): a pass between the scroll screens and the colours (`video/shaders/holes.frag`).
  Each pixel of the backdrop looks no further than the added side each way for the run it is in, and takes the
  mirrored pen as `FillHoles` does.

Results, the ladder with `widescreen` on:

In the 16 games with widescreen and a ladder movie (sfa, sfa2, sfz2al, sfa3, pocket_fighter, vampire_savior, vsav2,
vhunt2, hsf2, ssf2t, nwarr, xmcota, msh, xmvsf, mshvsf, mvsc), every frame drawn on the GPU was the same as the board's,
pixel for pixel. Between 2,873 (xmcota) and 4,101 (sfa) of each ladder's 6,000 frames were wider than the board's, with
the stages' tiles given beyond its columns, objects moved and holes filled. The frames not drawn there were only those
before a game's first object list. Without widescreen, as before (sfa2, msh, dstlk, avsp, gigawing).

### Another set's graphics (Vampire Savior Definitive)

Vampire Savior Definitive draws the three characters it takes from Vampire Savior 2 from that game's graphics. The
machine marks which object entries and which tiles of the maps are drawn from them (`Machine::objectsFromExtra`,
`scrollFromExtra`: what was written just after the other set's program was read). The records carry those marks:

- **The object list** has its entries' marks (`ObjectList::fromExtra`). The game's side takes them with the list it
  hands over, those of the bank it names, and they are compared with the board's at each frame's start, as the
  entries are.
- **The frame's record** has the tiles' marks as the frame began, and each write to a map in its log carries the mark
  it was given.
- The CPU rasteriser and the GPU draw each object and tile from the graphics its mark says. On the GPU, the other set's
  graphics are a second buffer, and the marks a buffer put up band by band as video RAM is.

Made-up play (`--auto`, seeds 1-6, 9,000 frames each, with `free_play`) picks the three often enough: 122 to 328 object
lists a run with entries drawn from the other set's graphics, 138 to 422 frames with its tiles. Every record, every line
of the picture drawn from the records and every frame drawn at once was the same as the board's; with
`--video gpu-check` (seeds 2 and 4), every frame drawn on the GPU too.

### The CPS-1 games' picture

Cadillacs and Dinosaurs and The Punisher are drawn from the records too, on the CPU (`Raster::LineCps1`) and on the
GPU. A CPS-1 draws by other rules (`Video::RenderLineCps1`):

- **The order:** the layer control's four places, back to front, take the objects (0) among the scroll screens, as
  they are, with no compacting. Each screen is on by its own bit of the layer control, which the board's `cps_b` gives.
- **The objects:** the first entry in front. A pixel once an object's stays its, and it shows unless the screen just
  behind the objects has a high pen there: one its tile's priority mask (CPS-B's four, in the line's record) puts in
  front of them. Screens after the objects in the order draw over them.
- **Tiles:** the scroll screens' tiles are numbered from the graphics' start, as the objects' are.

On the GPU the first pass gives two pens at each pixel: that of the screens behind the objects (and whether the one
just behind has a high pen there), and that of the screens in front of them, if one drew. The objects' pass places the
objects by the CPS-1's raster, and the colours' pass takes the screens in front if any drew, else the object unless
the pen behind is high, else the screens behind.

Results: every line and frame compared was the same as the board's. On the CPU that is Dino's attract mode (895,776
lines) and The Punisher's ladder (25,000 frames, 5,599,776 lines); on the GPU, Dino's attract mode (3,999 frames) and
The Punisher's ladder (24,999 frames). With them every game of the collection can be drawn on the GPU.

### Guest boards' own records (stages from another game)

A game that shows another game's stages runs that game's board beside its own (`runtime/guest.h`: Vampire Savior
Definitive's Darkstalkers and Alpha 2 stages). Each board is a machine with its own native interface, and now with its
own game's side too:

- **Watches on a guest's board.** A guest runs its own game's program, at that game's addresses, with hooks of its
  own. Its game's `video_writes` (and its palette commit's two moves to the registers, as a guest's board has no
  rewrites) go into the host's build (`GAME_GUEST_VIDEO`, from each guest's `game.cfg`), and are watched on the guest's
  board when it is loaded (`RegisterGuestVideoLifts`, `AddGuestWatches`). The CPU tells a guest's watches as it tells the
  game's (`ownWatched`). The moves are read from the guest's own program when they are registered, not as they run.
- **The list and the palette kept always.** A guest's matches are made on a spare board and put into the guest's board
  as a save state. So the game's side keeps its object list and palette whether or not anything records, and a spare's
  state has them right. That costs a frame a few thousand reads.
- **Checked.** `--video check` checks the guests' boards too, each report saying whose ("video check, dstlk").

In Vampire Savior Definitive's arcade mode with `dstlk_stages` (Vampire Savior's ladder movie, 8,000 frames), the
Darkstalkers board showed its stages for 4,229 frames. Every palette upload, object list and line of its, the picture
drawn from its records and every frame drawn at once were the same as its board's, and so were Definitive's own.

**On the GPU.** Each guest board has a GPU renderer of its own (its graphics, its palette), on the same device, and its
frames are recorded and kept. A frame of the game's with its objects alone (layer mask 1, marked: `GpuPicture::Over`)
is drawn with them as the colour times their alpha, and put over the guest's last frame; the readouts over that, as
always. The GPU follows the frame as the CPU put it together (`Machine::guestBehind`): a frame not put together over a
guest's is the CPU's. `--video gpu-check` checks a game's frame of objects alone by the board's marks, the guests'
frames against their boards, and the whole against the CPU's picture. Definitive's ladder with `dstlk_stages`
(8,000 frames): 3,922 frames of objects alone over Darkstalkers' stages, every one the same; 4,229 of the Darkstalkers
board's; the pictures put together within a step (13 at a stage's start or end the CPU's). In training on an Alpha 2
stage (9,000 frames): 5,650 over Alpha 2's stage, 8,995 of its board's, all the same.

### The GPU's picture on the screen (`--video gpu`)

With `--video gpu` the window shows the GPU's picture, where it can, when the presenter draws with SDL_GPU. The SDL
renderer's presenter and the OpenGL shader presets' keep the CPU's picture.

- **The record every frame.** `NativeVideo::recording` keeps the game's side's records without comparing them, and at
  each frame's end hands the frame over (`frameDrawn`). The GPU renderer keeps the last one (`GpuPicture::Keep`) and
  draws it when the presenter shows a frame, in the same command buffer (`Presenter::ShowDrawn`, `DrawKept`), into the
  texture the presenter samples in its upload's place. A frame the GPU does not draw yet (before the game's first
  object list, another set's graphics, a game's picture settings such as widescreen, objects' offsets changing down
  the picture) is shown from the CPU's picture, as before.
- **The game's side in the save state.** What the game has said to each register, the list it handed over and the
  palette and objects as its records have them are put in the save state with the machine (`NativeVideo::Serialize`).
  So online play's rollback and save states put them back too; a state is only read by the build that wrote it, so
  adding to it is allowed. With `--rollback-test` and `--video check`, every record is the same and the hashes are a
  plain run's. With the game's side left out of the state, 25 object lists and 149 picture lines of sfa2's versus movie
  differ.
- **The readouts over it.** The training mode's readouts, the hit boxes, the input display and the status lines are
  drawn as the CPU's picture has them, twice: over black and over white (`Port::Readouts`). What they leave of what is
  under them is the difference, so that gives each pixel's colour times its alpha, and its alpha, exactly, with no
  change to the drawing. A last pass (`video/shaders/shown.frag`) puts them over the GPU's picture. Zoomed, the two are
  stacked as the presenter takes them (the game's above, with the readouts below).
- **Checked.** `--video gpu-check` also draws the kept frame with the readouts over it and compares it with the CPU's
  picture with them (`Port::Shown`), within a step of a colour (the blend's rounding). In sfa2's training session
  (5,500 frames, 3,666 with readouts on the screen), every frame was within a step.
- **What stays on the CPU.** These go on using the CPU's picture: screenshots, frame hashes, online play's frames run
  again (not drawn), the model viewer, and the guest stages' pictures put together.

The window itself has not been looked at here (the checks run headless): `--video gpu` is for trying. Next: objects'
offsets that change down the picture and the marks, so that every frame is drawn there.

## The frame drawn at once (the GPU's data, checked on the CPU)

Before any shader, the GPU's frame is drawn on the CPU and checked. At a frame's first line the check keeps video RAM
and the palette as they are, then logs every write to video RAM and every palette upload after that, with the line it
came before. When the frame is over, it draws all 224 lines from that copy, putting each write in before the first
line it was seen on, with each line's layers and the frame's objects. That is the GPU's data for a frame (a copy at
its start, a log, the lines' layers, the objects), and its bands are where the log has writes the drawing reads.
`--video check` compares the frame with the board's lines.

A frame's bands are where it has writes into a map or the row scroll's table in use, or palette uploads, at a line
other than the first. Some frames need one at every line (a whole map rewritten while the picture is drawn, as
screens change); most need two or three.

Results:

Over attract mode and the ladder in the 21 CPS-2 games, every frame compared was the same as the board's: 3,999 an
attract mode run and 5,999 a ladder in the Alpha games, and so on (not compared: the frames before a game's first object
list). Bands a frame on average: 1.4 (dstlk's attract mode) to 5.6
(sfa3's), and 12.6 in Giga Wing's ladder, a shoot-'em-up that rewrites its maps as it scrolls.

## The machine's picture from the records (the default; `--video native`)

The machine's own picture (what the window shows when the GPU's is not used, screenshots, frame
hashes, the fighting modes' pictures put together) is drawn line by line by the CPU rasteriser from the game's records,
not by `video.cpp`. `video.cpp` still draws the frames before a game's first object list and the model viewer's probes.
The guest boards' pictures (stages from another game) are drawn from their own records the same way.

Results: in all 23 games, every frame's hash (the picture in it) was the same as with the board model's picture: the
ladder movies' 6,000 frames (8,000 in Vampire Savior Definitive with Darkstalkers' stages), attract mode's 4,000 in the
games without one (Alien vs. Predator, Cadillacs and Dinosaurs); sfa2's ladder in widescreen too.

It is the default: `--video board` draws the board model's picture instead, kept as the reference the checks compare
with. Before it was, the fighting modes' movies (ring out, tag battle) showed one frame that differed: a spare machine
reset over the port's state (the next match's stage, made while this one is fought) kept the old game's colours and
objects on the game's side, where the board's palette and object list start empty. `NativeVideo::Reset` now empties
them too. And Super Turbo's health bars, once damaged, write their empty pieces into the object bank already shown
(700000): their loops take their pointer 8000 back after the first piece. The board takes its list with those writes
in it, so the game's side watches them (`video_writes` in its game.cfg) and puts them into the list it was handed
(no ladder showed it: the ladder movies take no damage). With that, Alpha 2's, Vampire Savior's and Super Turbo's ring
out and tag battle movies, with and without widescreen, are the same as the board's frame for frame, as is every
game's ladder or attract mode.

The GPU's shaders read nothing beyond their buffers (Metal does not check): a map's or the row scroll's address past
video RAM's 0x30000 bytes reads 0, and the lines' buffer holds the two sizes after the records.

## Order of work

Sizes: S a session, M a few, L a week or more. Done:

- **The records** (palette uploads, object lists, the layers' registers line by line) in every game of the collection,
  CPS-1 games and guest boards among them, checked against the board's each frame (check a).
- **The picture from the records** on the CPU (check b) and drawn a frame at a time in bands, every game, with the
  games' picture settings (widescreen), another set's graphics, and the CPS-1's rules.
- **The SDL_GPU renderer**, every game, checked frame by frame against the board (`--video gpu-check`); in the
  window, the readouts over it, the game's side in the save state for online play's rollback; guest boards' stages under
  the game's fighters, each drawn from its own board's records on the same device and put together in one pass
  (in the game's frame's last pass, colours.frag reading the stage's picture: its pieces in front of the fighters, the game's panel over all).
- **The GPU picture by default** (the Display page's **Picture drawn by**, the GPU unless the CPU is chosen; `--video
  gpu`, or `native`/`cpu` and `board` for the CPU's), after it was looked at in the window. Frames it cannot draw are the
  CPU's: those put together on the CPU (`MarkPictureMade`: the spares, the endless battle, the colour editor), a shader
  preset's (OpenGL), and every frame where the window is not drawn with SDL_GPU (no usable device: SDL's renderer; a
  GLES window on Android) or the GPU's picture cannot be made there (it says so and goes on with the CPU's).
- **The palette commit's moves watched** as well as rewritten: when the board's timing has the rewrite stand aside,
  the game's own code runs it and the watches tell the native interface (`CPS2RECOMP_SKIP_REWRITES=CommitPalette`:
  every upload still the game's side's).
- **The machine's picture from the records**, the default (`--video board` for the board model's): the picture the
  frame hashes include drawn by the CPU rasteriser, every game's hashes the same as the board model's, the fighting
  modes' too.

Left:

1. **The fighting modes' pictures** (`fight/battle.cpp`: endless battle, ring out): the fighters over stages spare
   machines of the same game draw, with fades between stages and darkening. Spare machines as producers, as guest
   boards are, and the fades as a pass. M-L. Until then a frame whose picture was put together or changed after the
   machine drew it (a spare ran, `Give`, `Darken`, the infinite stage, the colour editor: `MarkPictureMade`,
   runtime/spare.h) is shown from the CPU's picture: the GPU had kept the last spare's frame (the fighters and HUD
   missing, seen in the window in endless battle). `--video gpu-check` compares only the frames the window shows from
   the GPU.
2. **Objects' offsets that change down the picture** on the GPU (none seen in any game's runs; such frames are the
   CPU's). S.
3. **Deeper lifts:** the records made from the game's own drawing code (the Alpha library's list builder and its
   piece writers, the frame's commit with sfa2's `FrameRegisters` moved to the toolkit) rather than at the register
   writes and the bank swap. M per family.
4. **Every fighting game's stages in every other's** through two producers (a guest's facts are written once per
   game, `games/<id>/guest.h`). Every fighting game with stages of its own is a guest, each run from its own stage code
   alone (no game played to a round). Its board draws no picture of its own: it keeps the records of each frame
   (`Machine::ownPicture`), and the game's renderer draws the guest's stage from them and lays the game's fighters and
   panel over it, the stage's front pieces over those: on the CPU (`NativeVideo::LastFrame`, `DrawFrame`: the frame at
   once, as the GPU's bands draw it; the frame hashes the same as when the guest's board drew its own) and on the GPU
   (the guest's stage drawn from its records, then the game's frame, whose last pass reads that stage and writes the
   picture: no pass or picture of its own for putting the two together). Left: the guest's screens and objects resolved
   with the game's in that same last pass, its palette beside the game's (each game's frame is drawn in bands where its
   own palette or maps change, and the two games' bands are not the same lines: the stage's finished picture is what
   lets the game's bands read it; the same picture either way); and hosts for the fighting games that are not yet (Super Turbo, Hyper Street Fighter II,
   the Marvel games, Gem Fighter, Darkstalkers and Night Warriors). M.
