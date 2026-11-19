// SPDX-License-Identifier: GPL-3.0-only
//
// Street Fighter Alpha: names for what is known of the game's memory and routines, each found by experiment in
// this game. Its variables are reached from A5 = FF8000, as in Alpha 2, but most are elsewhere: Alpha 2 is not
// made from this game's program, so nothing is taken from there unchecked.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>

namespace sfa {

using cps2recomp::M68K;

// --- where the game is ---

// Words from A5, each a step (in twos) of the one before, as in Alpha 2: the first is 2 once a game is being
// played (and the board's start and its show step through it before: title.cpp); the next 2 while the players
// choose their characters and 4 during the match; then 4 while a round is on, and under that 2 while it is being
// fought (0 its introduction). Traced with --watch.
constexpr uint32_t kMode = 0xFF8000, kGameStep = 0xFF8004, kMatchStep = 0xFF8008, kRoundStep = 0xFF800C;
enum : uint16_t { kGameChoosing = 2, kGameMatch = 4, kMatchRound = 4, kRoundFought = 2 };

// The round's clock: seconds left, a byte (99 at a round's start, 004378; a second off at 004B3A, and below
// nothing time is up). Found by RAM dumps against the clock shown (91, 87, 83, 79) and --watch: FF365D is the
// picture's copy of it, in BCD. Alpha 2's is at FF8109.
constexpr uint32_t kRoundClock = 0xFFAE09;

// Credits paid for and not yet used, a byte (found by RAM dumps after one, two and three coins put in: 0, 1, 2,
// 3; the board's settings memory starts at one coin a credit). Alpha 2 keeps its own at FF8076.
constexpr uint32_t kCredits = 0xFF8170;

// Who is in: a byte, bit 0 player 1, bit 1 player 2 (RAM dumps before and after each Start: 0, 1, 3; copied from
// FF81A8, the title's, as a game begins: 0040D6). And who has chosen a character, the same bits (0191E0: the player
// that chose added in; RAM dumps as player 1 and then player 2 chose: 0, 1, 3).
constexpr uint32_t kPlayersIn = 0xFF818D, kPlayersChosen = 0xFF81AC;

// The stage a match is fought on: a word, the stage's number doubled. A stage is a character's (stage n is
// character n's): the match is made at 006F6E (the computer's next opponent for one player), which takes the stage
// from the second player's character (006FD0). Each picture seen with the word held at each number.
constexpr uint32_t kStage = 0xFFAE00;
constexpr int kStages = 13;
// Who a player is (a player's byte kCharacter): each number seen in a fight with the byte held at it. The same
// order as Alpha 2's first thirteen. Akuma, M. Bison and Dan are the hidden three.
constexpr int kCharacters = 13;
inline constexpr const char *kCharacterNames[kCharacters] = {"Ryu",  "Ken",  "Akuma", "Charlie",  "Chun-Li", "Adon", "Sodom",
                                                             "Guy",  "Birdie", "Rose", "M. Bison", "Sagat",   "Dan"};
// The stages by whose they are and what they show (several are the same place at another time of day)
inline constexpr const char *kStageNames[kStages] = {
    "Ryu's (the shopping street)",   "Ken's (the night market)",     "Akuma's (the Great Wall at dusk)", "Charlie's (the crowd)",
    "Chun-Li's (the Great Wall)",    "Adon's (the reclining Buddha)", "Sodom's (the rail yard)",          "Guy's (the street at dusk)",
    "Birdie's (the ruins at night)", "Rose's (the ruins)",           "M. Bison's (the rail yard at dusk)", "Sagat's (the Buddha)",
    "Dan's (the Buddha at sunset)"};
// Each stage's tune (CPS2RECOMP_SOUNDTRACK_LOG over a fight on each), and the character choice's and the versus
// screen's
inline constexpr uint16_t kStageTunes[kStages] = {0x01, 0x02, 0x0B, 0x09, 0x03, 0x06, 0x05, 0x08, 0x07, 0x0A, 0x0C, 0x04, 0x32};
constexpr uint16_t kTuneChoose = 0x0E, kTuneVersus = 0x0F;

// The sound queue: a ring of sixteen-byte requests at A5-59C2, the word at A5-49C2 the place of the next (012BEE puts
// one in; 012B3E sends the oldest to the sound CPU), as Alpha 2's; FF05 asks for the music to stop before another
constexpr uint32_t kSoundQueue = 0xFF263E, kSoundQueueEnd = 0xFF363E;
constexpr uint16_t kTuneOff = 0xFF05;

// The camera: the picture's left edge in the stage (a word of the stage's own scroll screen's record, FF8280), with
// every place the game keeps it across, its own among them (all found moving together in RAM dumps as the players
// walked to one end: the scroll screens' routine 01A4AE copies it). 256 to 640 across on every stage seen.
constexpr uint32_t kCameraX = 0xFF8290, kCameraY = 0xFF8294;
inline constexpr uint32_t kCameraCopies[] = {0xFF8290, 0xFF802E, 0xFF82A0, 0xFF82A8};
constexpr int kCameraLeft = 256, kCameraRight = 640;
// Where the players stand at a round's start, and the walls (player 2 stopped at 1000 walking back to the right end;
// the left as far from the camera's end), as Alpha 2's
constexpr uint16_t kStartX[2] = {544, 736};
constexpr int kStageLeft = 280, kStageRight = 1000;

// The game's loop of notices, introduction, title, demonstrations and rankings (kMode, traced with no coin in; the
// steps between are fades): 0C the QSound notice, 0E the maker's, 10 the introduction, in which the step under it
// (kGameStep) goes 2 a demonstration fight, 4 the introduction's pictures, 6 another fight, 8 and 0A more pictures and
// 0C the title; 12-16 the score ranking, 1C-20 the notices again, 28 the introduction again (other fights), 2C the
// versus ranking; then 0 and on from the start.
enum : uint16_t { kIntro = 0x10, kSecondIntro = 0x28, kLoopEnd = 0x30 };
enum : uint16_t { kIntroFight = 0x02, kIntroPictures = 0x04, kIntroFight2 = 0x06, kIntroMore = 0x08 };

// The round's result: bit 0 set when player 2 is knocked out (its health below nothing), bit 1 player 1 (00F4A6, after
// the hit tests): 1 player 1 has won, 2 player 2, 3 neither
constexpr uint32_t kRoundResult = 0xFFAE0C;

// --- the players ---

// Each player's record, 0x400 long, the first at FF8400 (as in Alpha 2); a third, FF8C00, in the hidden three-player
// fight
constexpr uint32_t kPlayer1 = 0xFF8400, kPlayerSize = 0x400;
inline uint32_t Player(int n) {
    return kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}
// Four bytes of steps, each a step of the one before (RAM traced through a fight of known input: standing, crouching,
// walking, jumping, attacking, guarding, being hit and knocked down). In a fight the first is 2; the second says who
// is in charge of the player: 0 the player (kAction then saying what it is doing), 2 a hit (being hit or guarding one:
// "reeling"), 0E the round's introduction, more for being thrown and other things done to it. As in Alpha 2.
constexpr uint32_t kState = 0x04, kControl = 0x05, kAction = 0x06, kActionStep = 0x07;
enum : uint8_t { kControlOwn = 0x00, kControlReeling = 0x02 };
enum : uint8_t { kActionStand = 0x00, kActionCrouch = 0x02, kActionWalk = 0x04, kActionJump = 0x06, kActionAttack = 0x0A, kActionGuardPose = 0x0C };
constexpr uint32_t kFacesRight = 0x0B; // byte: not zero when facing right (a box's offset across is kept as if facing left: 0104B8)
constexpr uint32_t kColourSet = 0x0D;  // byte: the first of the player's colour sets in the palette buffer
constexpr uint32_t kX = 0x10;          // word: where it stands across the stage (a long with 16 bits of fraction)
constexpr uint32_t kY = 0x14;          // word: height, upwards; kGround on the floor
constexpr uint16_t kGround = 0x28;
constexpr uint32_t kFrame = 0x20;      // long: the animation frame being shown (in the ROM)
constexpr uint32_t kFrameTime = 0x24;  // word: frames it is still shown for
constexpr uint32_t kOpponent = 0x2A;   // word: the other player's record (FFxxxx: movea.w, 01EEEE)
constexpr uint32_t kInAir = 0x2F;      // byte: not zero while in the air (01 a jump, FF thrown up by a hit)
// Longs, 16 bits of fraction: the speeds across and up, and what is taken off them each step (RAM dumps through a
// jump: up 0x4A000 as it leaves the floor)
constexpr uint32_t kSpeedX = 0x30, kSpeedY = 0x34, kPullX = 0x38, kPullY = 0x3C;
// Health: what the player has (a hit takes it off here at once; below nothing: knocked out) and what the bar shows (it
// follows a little later). Alpha 2's are at +50 and +52.
constexpr uint32_t kHealth = 0x40, kHealthShown = 0x42;
constexpr uint16_t kHealthFull = 0x90;
constexpr uint32_t kGuardedHit = 0x45; // byte: FF when the last hit was guarded (00F792), else what kind of hit
constexpr uint32_t kHitStop = 0x4F;    // byte: frames the player stands frozen for while a hit lands
constexpr uint32_t kComboTaken = 0x57; // byte: hits of the combo the other player is landing (named in the FBNeo
                                       // training script for this set; counted up through hits here)
constexpr uint32_t kCharacter = 0xA2;  // byte: who the player is (kCharacterNames); the fight's tables are by it (01CE2A)
constexpr uint32_t kSpecialGauge = 0xBE; // word: the super combo gauge, 0x90 a level (FBNeo's script; it grows with hits here)
constexpr uint32_t kComputer = 0xC5;   // byte: 1 while the game plays this player (the fight step goes to the computer's
                                       // routine for it, 01CE68; found 1 for the opponent in a one-player game, 0 in a two-player one)
constexpr uint32_t kColourChoice = 0xC8; // byte: which set of colours: 0 a punch chose the character, 1 a kick (01920A)
// The stick as the player holds it, by its facing (bit 0 back, 1 forward, 2 down, 3 up): bit 0 is set while it
// holds away from the other (and can guard)
constexpr uint32_t kStick = 0x109;
// Boxes (the hit test 00F4C0, the boxes' test 0104B8, the push 00F304). The animation frame names the boxes in use:
// three it can be hit in (bytes kFrameHead, kFrameBody, kFrameLegs into the tables at the record's longs kHeadBoxes,
// kBodyBoxes, kLegBoxes; four bytes a box), its attack (kFrameAttack into kAttackBoxes, sixteen bytes an attack: the
// box, then what the attack does) and its push box (kFramePush into kPushBoxes). A box is four bytes: its middle from
// where the player stands (signed: across, turned about when facing right; up), half its width and half its height.
constexpr uint32_t kHeadBoxes = 0x110, kBodyBoxes = 0x114, kLegBoxes = 0x118, kAttackBoxes = 0x11C, kPushBoxes = 0x120; // longs
constexpr uint32_t kFrameHead = 0x08, kFrameBody = 0x09, kFrameLegs = 0x0A, kFrameAttack = 0x0B, kFramePush = 0x0C;    // bytes
constexpr uint32_t kBoxSize = 4, kAttackBoxSize = 0x10;
// The records of what the players throw: a pool of eight of 0x80 bytes from FF9000 (01B848 steps them; 0226B2, the
// guard test, goes through them), +00 1 for one in use; their attacks' table is named by the long at +50
constexpr uint32_t kThrown = 0xFF9000, kThrownSize = 0x80, kThrownTables = 0x50;
constexpr int kThrownRecords = 8;
constexpr uint32_t kHalfWidth = 0x12A; // word: half the player's width (what the picture's edges keep in: 01EEFE)
constexpr uint32_t kKnocksDown = 0x134; // byte: set by a hit that knocks the player down (a sweep's)
// The stun: what has built up, and the frames until it goes down again (a hit adds to both)
constexpr uint32_t kStun = 0x137, kStunTimer = 0x136;

// The control panel as the game read it this frame, from A5: a word a player, buttons above (punches in bits 8-10,
// kicks in 12-14) and the stick below (bit 0 right, 1 left, 2 down, 3 up). Found holding right, down, up and buttons.
constexpr uint32_t kControls[2] = {0xFF8050, 0xFF8054};

// --- routines (addresses in the program) ---

// A player's fight step goes on by kControl, once a step for each player: a person's (01CE40 on, the fight's step of
// 01CCF4's state table) and the game's computer player's (021DE6 on, of 021DCA's: 01CCDC goes there for one)
constexpr uint32_t kPlayerDispatch = 0x01CE48, kComputerDispatch = 0x021DF4;
constexpr uint32_t kHitTaken = 0x00F638;       // an attack has reached the other (A6 the attacker, A1 the other, A3 the
                                               // attack), after 00F646 has made it a hit or a guarded one

// The player is free to do anything: in charge of itself, and standing, crouching, walking or in its guarding pose
inline bool CanAct(uint32_t player) {
    using namespace cps2recomp;
    const uint8_t action = Ram8(player + kAction);
    return Ram8(player + kControl) == kControlOwn &&
           (action == kActionStand || action == kActionCrouch || action == kActionWalk || action == kActionGuardPose);
}

// --- the picture ---

// Objects by what they are (for Video::hideObject; found from the object lists of fights on every stage and of the
// attract mode's, by their colour sets, places and tiles): sets 00-07 are player 1's and 08-0F player 2's (kColourSet:
// the fighter, what it throws), 10-17 the sparks and dust of hits, 18-19 the stage's own pieces, 1A-1E the panel (the
// gauges at the bottom, tiles from 1ED50, and words such as FIRST ATTACK), and in 1F the players' shadows (tiles
// 1B7A0-1B7AF, on the floor's line) with more of the panel's words. The bars, names and clock at the top are a scroll
// screen's, not objects.
inline bool IsShadow(uint32_t tile, uint16_t attributes) {
    return (attributes & 0x1F) == 0x1F && tile >= 0x1B7A0 && tile < 0x1B7B0;
}
inline bool IsStagePiece(uint16_t attributes) {
    return (attributes & 0x1F) == 0x18 || (attributes & 0x1F) == 0x19;
}
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return set >= 0x1A && !IsShadow(tile, attributes);
}
inline bool NotTheStage(uint32_t tile, uint16_t attributes) {
    return !IsStagePiece(attributes);
}
// The scroll screens the stage viewer shows: all but the first (8-pixel tiles), which holds the panel's bars
constexpr unsigned kLayersWithoutPanel = 0xF & ~2u; // (bit 0 the objects, 1-3 the scroll screens)

// --- for hooks ---

inline void SetLow16(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFF0000) | (value & 0xFFFF);
}
inline void SetLow8(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFFFF00) | (value & 0xFF);
}

} // namespace sfa

namespace cps2recomp {
// The wide picture (widescreen.cpp): the columns it adds on each side (0: none), how far right the stage is drawn in it,
// how far short of each end the camera stops (64, or 0: played as on the board), and a spare's frame drawn as wide as
// the port's (and the port's machine back)
int WideSide();
int WideStage();
int WideMargin();
void WideEnds(int &left, int &right);
void WideForSpare(Machine &machine);
void WideForPort(Machine &machine);
} // namespace cps2recomp
