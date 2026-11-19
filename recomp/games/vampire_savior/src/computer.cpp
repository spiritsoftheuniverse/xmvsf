// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Vampire Savior: the computer player, written again as C++ (rewrites: lift.h). The computer plays a fighter by
// running scripts from the ROM, a set of them for each character (tools/aiscripts.py prints them; PROGRESS.md,
// "The computer player: scripts"). A script is a list of instructions, each a word (an operation in the high byte,
// its kind in the low) and the words it takes. An instruction that does something with the fighter (a wait, a
// walk, a jump, an attack) is started, and then looked at every frame until it is over; the ones that decide
// (an if, a go to, a change of what may break in) are done at once, and the next one is taken.
//
// There are four banks of scripts, each with a table of them by character:
//   0  plans: chosen by the record's kPlan
//   2  tactics: by the plan, one of 32 slots at random (of three tiers of 32)
//   4  reactions: one of 32 slots at random; started when the other player attacks (and a chance allows it)
//   6  answers: the one the game chose (kAnswer; Reactions below)
// Each bank keeps where its script has got to, so a plan broken off by a reaction goes on where it was.
//
// The computer never reads the other player's controls: it sees the other's attack the frame it is out, and its
// projectiles, not the buttons.
#include "../../../fight/battle.h"
#include "vsav.h"

#include <array>
#include <cstdio>
#include <cstdlib>

namespace cps2recomp {

namespace {
    using namespace vsav;

    // The routines here, and the game's that they use
    constexpr uint32_t kComputerPlays = At(0x02B7FA), kAnswerNow = At(0x02D09A), kBrokenOff = At(0x02C7C0);
    constexpr uint32_t kReactions = At(0x02CF74);
    constexpr uint32_t kUnpackMove = At(0x02CE44), kUnpackChainMove = At(0x02CEB0);
    constexpr uint32_t kChance = At(0x02D62A), kAttackComing = At(0x02D43C);
    constexpr uint32_t kJumpMoveNow = At(0x02D374), kSecondMoveNow = At(0x02D3F2), kGuardPose = At(0x02D50C);
    constexpr uint32_t kSetGuardsWithMove = At(0x02D544), kAtEdgeJump = At(0x02D568), kGuardAnyway = At(0x02D5B2);
    constexpr uint32_t kNextChainMove = At(0x02D5E6);
    constexpr uint32_t kRandom = At(0x014E8A);         // a made-up byte in D0
    constexpr uint32_t kRecover = At(0x026058);        // the player's bytes for a hit's reeling cleared
    constexpr uint32_t kSetAnimation = At(0x027EC8);   // the player's animation numbered D0

    // The scripts' tables, by character (a long each): the four banks', and the characters' own test of whether a
    // special can be done now (a routine each)
    constexpr uint32_t kPlans = At(0x0BF01A), kTactics = At(0x0BF09A), kReactionScripts = At(0x0BF11A);
    constexpr uint32_t kAnswers = At(0x0BF19A), kCanDoSpecial = At(0x0BF21A);
    // Chance tables: a long for each level, of which a random bit says yes or no (Chance). By character (128
    // bytes each): kGuardChances, whether to guard or react. For everyone: the others.
    constexpr uint32_t kPlanOrTactic = At(0x0BB480), kBrokenOffChances = At(0x0BB500), kGuardChances = At(0x0BB580);
    constexpr uint32_t kGoToChances = At(0x0BC780), kGuardAnywayChances = At(0x0BC680);
    constexpr uint32_t kGuardsWithMoveTable = At(0x0BC880); // by character (32 bytes), by level: a byte
    constexpr uint32_t kChains = At(0x0BCD20);              // lists of moves for a chain: word offsets, then bytes, FF ends
    // A wait's think time: a base by level, the character's own, and one of 32 at random (words)
    constexpr uint32_t kThinkByLevel = At(0x02D2B8), kThinkByCharacter = At(0x02D2F8), kThinkRandom = At(0x02D278);

    constexpr int32_t kNoComputer = AtA5(0x10E); // from A5, a byte: not zero: the computer does nothing (the round has ended)
    constexpr int32_t kGameMode = AtA5(0x90);    // from A5, a word: the kind of game (0, 2: the usual; 4, 8, C: no think time)
    constexpr int32_t kFrameCount = AtA5(0xB4);  // from A5, a byte counting frames
    constexpr int32_t kThrownFromA5 = AtA5(0x1400); // kThrown, from A5

    // The computer's part of its player's record (from A6)
    constexpr uint32_t kChosen = 0x200;        // byte: 0 until the bank's script is chosen, then 2
    constexpr uint32_t kStarted = 0x201;       // byte: 0 until the next instruction is started, then 2
    constexpr uint32_t kWaiting = 0x202;       // word: what the instruction started waits for (Going); 0 when over
    constexpr uint32_t kBank = 0x204;          // byte: 0 plans, 2 tactics, 4 reactions, 6 answers
    constexpr uint32_t kPlan = 0x205;          // byte: which plan (an offset in the character's table)
    constexpr uint32_t kIfBrokenOff = 0x206;   // word: what the script does when broken off: 0 nothing, 2 back to the
                                               // plan, 4 one of the two by a chance (BrokenOff)
    constexpr uint32_t kCount = 0x208;         // word: frames left (a wait, a walk)
    constexpr uint32_t kTier = 0x20A;          // byte: 0, 20h or 40h: which 32 of a table's slots
    constexpr uint32_t kWay = 0x20B;           // byte: 0 forward, 1 back (a walk, a jump)
    constexpr uint32_t kReactionTable = 0x20E; // byte: which of the reactions' tables (always the first)
    constexpr uint32_t kPlaces = 0x210;        // longs: where each bank's script has got to, by kBank (0x210 + kBank * 2)
    constexpr uint32_t kPlace = 0x220;         // long: where the interpreter is
    constexpr uint32_t kPlanStart = 0x224;     // long
    constexpr uint32_t kMayBreakIn = 0x228;    // word: the answers that may break in (a bit each; Reactions)
    constexpr uint32_t kSlot = 0x22A;          // byte: the slot the script was chosen from
    constexpr uint32_t kAnswer = 0x22B;        // byte: the answer chosen (bank 6's script)
    constexpr uint32_t kDepth = 0x22C;         // byte: how many ifs the interpreter is inside
    constexpr uint32_t kGuardThenMove = 0x22D; // byte: a guard with a move after it is going on (not broken in on)
    constexpr uint32_t kGuardedAnyway = 0x22E; // byte
    constexpr uint32_t kVariant = 0x22F;       // byte: of a move
    constexpr uint32_t kChain = 0x230;         // byte: 0, 1, or FF: a chain whose moves come from kChains
    constexpr uint32_t kUnbreakable = 0x231;   // byte: not zero: nothing breaks in
    constexpr uint32_t kMoveWhen = 0x232;      // byte: when a jump's move comes: 0, 2 at a gap, 4 after kCount frames
    constexpr uint32_t kGuardsWithMove = 0x233; // byte: from kGuardsWithMoveTable by level
    constexpr uint32_t kKeptIfBrokenOff = 0x234; // word: the plan's kIfBrokenOff while a tactic is gone to
    constexpr uint32_t kChainAt = 0x236, kChainNumber = 0x238; // words
    constexpr uint32_t kSecondMove = 0x23A;    // byte: a jump's move is its second (kind 02)
    constexpr uint32_t kTwoJumps = 0x23B;      // byte
    constexpr uint32_t kSecondJump = 0x23C;    // word
    constexpr uint32_t kChainedJump = 0x23E;   // byte
    constexpr uint32_t kStockMove = 0x23F;     // byte
    constexpr uint32_t kOperation = 0x240, kKind = 0x241; // bytes: the instruction (its word)
    constexpr uint32_t kOperands = 0x242;      // words: an instruction's words, kept (up to five)
    constexpr uint32_t kLevel = 0x3B3;         // byte: how well the computer plays, 0-31

    // The rest of a player's record that the computer reads or sets
    constexpr uint32_t kGoing = 0x0A;          // byte: the way the player moves: 0 forward, 1 back, FF up
    constexpr uint32_t kAnimationOver = 0x21;  // byte: bit 0 set when the animation has come to its end
    constexpr uint32_t kCanChain = 0x39;       // byte
    constexpr uint32_t kFloor = 0x3A;          // word: the height of the floor under the player
    constexpr uint32_t kStance = 0x100, kButton = 0x101, kStrength = 0x102; // bytes: a move (0, 2, 4)
    constexpr uint32_t kDirection = 0x103, kDirection2 = 0x104;            // bytes
    constexpr uint32_t kSpecial = 0x106;       // byte: the special's number
    constexpr uint32_t kDashForward = 0x113, kDashBack = 0x114, kDashing = 0x115; // bytes
    constexpr uint32_t kAtEdge = 0x116;        // byte: 1 at the picture's left edge, 2 at its right
    constexpr uint32_t kAttackSeen = 0x11A;    // byte: (the other's) cleared when the computer's script is broken off
    constexpr uint32_t kDown = 0x11B;          // byte: knocked down
    constexpr uint32_t kGettingUp = 0x145;     // byte
    constexpr uint32_t kOpenFlag = 0x11E;      // byte: not zero: not open
    constexpr uint32_t kFaceTo = 0x120;        // byte: the way the player should face (to kFacesRight)
    constexpr uint32_t kGap = 0x130;           // word: the gap between the two players (less their widths)
    constexpr uint32_t kHeldDown = 0x140, kRecovered = 0x143, kRecoverTimer = 0x174; // bytes
    constexpr uint32_t kLive = 0x149;          // bytes, five: how many of the player's projectiles of each kind are out
    constexpr uint32_t kThrowing = 0x154;      // byte
    constexpr uint32_t kNoGauge = 0x199;       // byte
    constexpr uint32_t kLowGuard = 0x1BA;      // byte
    constexpr uint32_t kCounter1BC = 0x1BC;    // byte (one character's)
    constexpr uint32_t kMoveStrength = 0x17;   // in an animation frame, a byte: below zero, not an attack to answer
    constexpr uint32_t kThrownAttack = 0xA2;   // in a thing thrown's record, a byte: not zero, harmless
    constexpr uint32_t kHurt = 0x94;           // in an animation frame, a long: the boxes it can be hit in

    // The values of kState the computer sets: the player standing, crouching, walking, jumping, attacking...
    constexpr uint32_t kStand = 0x02000000, kCrouch = 0x02000200, kStandUp = 0x02000204, kWalk = 0x02000400;
    constexpr uint32_t kJump = 0x0200060E, kJumpAway = 0x02000608, kDashJump = 0x02000E00;
    constexpr uint32_t kNormal = 0x02000A00, kSpecialMove = 0x02000E00, kStockSpecial = 0x02001000;
    constexpr uint32_t kStockSpecial2 = 0x02001200, kDash = 0x02001400, kGuard = 0x02000C08;

    uint32_t Me(M68K &c) {
        return c.a[6] & 0xFFFFFF;
    }
    uint8_t Byte(M68K &c, uint32_t field) {
        return Ram8(Me(c) + field);
    }
    uint16_t Word(M68K &c, uint32_t field) {
        return Ram16(Me(c) + field);
    }
    void SetByte(M68K &c, uint32_t field, uint8_t value) {
        SetRam8(Me(c) + field, value);
    }
    void SetWord(M68K &c, uint32_t field, uint16_t value) {
        SetRam16(Me(c) + field, value);
    }
    void SetLong(M68K &c, uint32_t field, uint32_t value) {
        SetRam32(Me(c) + field, value);
    }
    // The other player's record (in A4, as the game has it)
    uint32_t Other(M68K &c) {
        c.a[4] = static_cast<uint32_t>(static_cast<int16_t>(Word(c, kOpponent)));
        return c.a[4] & 0xFFFFFF;
    }
    int Character(M68K &c) {
        return static_cast<int8_t>(Byte(c, kCharacter));
    }
    bool InTheAir(uint32_t player) {
        return Ram8(player + kAirborne) == 1;
    }
    // Projectiles of the player's out, of any kind
    bool ProjectilesOut(uint32_t player) {
        uint8_t any = 0;
        for (uint32_t kind = 0; kind < 5; kind++) {
            any |= Ram8(player + kLive + kind);
        }
        return any != 0;
    }
    // --- the modes: the port's own tactics ---
    //
    // The game's scripts know nothing of the toolkit's modes (fight/battle.h): a tag, or a ring out, where only the
    // stage's ends count. For those the port has tactics of its own, written in the same instructions and two of its
    // own, kept at addresses of their own (kModeScripts: no place of the board's). A tactic is chosen as the game's are
    // (ChooseTactic), one of these in its place when the mode and the fight call for it (ModeTactic), as often as the
    // difficulty says. Its instructions are the computer's own walks, jumps, attacks and guarding, so that the
    // reactions break in on them as on the game's.
    constexpr uint32_t kModeScripts = 0x80000000;
    constexpr uint16_t kTagInstruction = 0x1C00; // (operation 1C: the toolkit is asked for a tag)
    // (moves: a jumping heavy kick; a standing heavy punch held 2, a throw as the game's scripts have theirs, and held
    // 1, a throw that sends the other behind (out of a corner: more than twice as often out of it as the jump and the
    // walk alone, counted); a crouching heavy kick, a sweep, a knock-down; a jump's way word forward with its move
    // after so many frames)
    constexpr uint16_t kJumpKick = 0x004C, kThrow = 0x0240, kEscapeThrow = 0x0140, kSweep = 0x004A, kForwardThenMove = 0x0004;
    constexpr std::array<uint16_t, 64> kModeWords = {
        // 0: the ring out, its back near its end: out of the corner, over the other (near), or walking out of the
        // end's way
        0x0002,
        0x1000,                                   // guard if an attack is coming (by the character's chance)
        0x0C00, 0x0000, 40,                       // if gap <= 40
        0x0600, kEscapeThrow,                     //   throw (the other the other way)
        0x0C02,                                   // else
        0x0C00, 0x0000, 90,                       //   if gap <= 90
        0x0400, kJumpKick, kForwardThenMove, 14,  //     jump forward: jumping heavy kick after 14 frames
        0x0C02,                                   //   else
        0x020A, 50, 30,                           //     walk forward until gap <= 50, at most 30 frames
        0x0C04,                                   //   endif
        0x0C04,                                   // endif
        0xFF02,                                   // back to the plan
        // 22: the ring out, the other's back near its end: a throw or a sweep (a knock-down carries it over)
        0x0002,
        0x1000,                                   // guard if an attack is coming
        0x0C00, 0x0000, 40,                       // if gap <= 40
        0x0C00, 0x0004,                           //   if one time in 2
        0x0600, kThrow,                           //     throw
        0x0C02,                                   //   else
        0x0600, kSweep,                           //     sweep
        0x0C04,                                   //   endif
        0x0C02,                                   // else
        0x020A, 36, 40,                           //   walk forward until gap <= 36, at most 40 frames
        0x0600, kSweep,                           //   sweep
        0x0C04,                                   // endif
        0xFF02,                                   // back to the plan
        // 43: the tag style, wanting to go off: a tag while the other is down or far, or after walking away from it
        0x0002,
        0x0C00, 0x000C,                           // if the other lying down
        kTagInstruction,                          //   tag
        0x0C02,                                   // else
        0x0C00, 0x0002, 160,                      //   if gap >= 160
        kTagInstruction,                          //     tag
        0x0C02,                                   //   else
        0x1000,                                   //     guard if an attack is coming
        0x020C, 160, 50,                          //     walk back while nearer than 160, at most 50 frames
        kTagInstruction,                          //     tag
        0x0C04,                                   //   endif
        0x0C04,                                   // endif
        0xFF02,                                   // back to the plan
    };
    enum ModeTacticAt : uint32_t { kOutOfTheCorner = 0, kKnockOver = 22, kTagOut = 43 };
    // (each begins after the end of the one before)
    static_assert(kModeWords[kKnockOver - 1] == 0xFF02 && kModeWords[kTagOut - 1] == 0xFF02 && kModeWords[60] == 0xFF02);
    // When the ring out's tactics are wanted: within 150 pixels of one's own end (and nearer it than the other is
    // to its own), the other within 100 of its end
    constexpr int kNearOwnEnd = 150, kNearTheirEnd = 100;

    // A word of a script: the game's (in its ROM) or the port's
    uint16_t ScriptWord(M68K &c, uint32_t address) {
        if (address & kModeScripts) {
            const uint32_t at = (address & ~kModeScripts) / 2;
            return at < kModeWords.size() ? kModeWords[at] : 0xFF02;
        }
        return Mem16(c, address);
    }
    uint8_t ScriptByte(M68K &c, uint32_t address) {
        const uint16_t word = ScriptWord(c, address & ~1u);
        return static_cast<uint8_t>(address & 1 ? word : word >> 8);
    }
    // The next word of the script
    uint16_t Operand(M68K &c) {
        const uint16_t word = ScriptWord(c, c.a[0]);
        c.a[0] += 2;
        return word;
    }
    // A routine's answer, as the game gives it: in D0 (moveq), and so in the flags
    void Answer(M68K &c, bool yes) {
        c.d[0] = yes ? 1 : 0;
        c.flagN = c.flagV = c.flagC = false;
        c.flagZ = !yes;
    }
    void Random(M68K &c) {
        Call(c, kRandom);
    }
    // 16-bit sums and differences as the 68000 makes them, with the extend flag they leave (for routines whose
    // callers are given all the flags)
    uint16_t Add16(M68K &c, uint16_t a, uint16_t b) {
        c.flagX = a + b > 0xFFFF;
        return static_cast<uint16_t>(a + b);
    }
    uint16_t Sub16(M68K &c, uint16_t a, uint16_t b) {
        c.flagX = a < b;
        return static_cast<uint16_t>(a - b);
    }

    // 02D62A. A chance: one of the 32 bits of the table at A1's long for the computer's level, at random
    bool Chance(M68K &c) {
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x1F);
        const uint16_t level = static_cast<uint16_t>(static_cast<int8_t>(Byte(c, kLevel)));
        SetLow16(c.d[1], static_cast<uint16_t>(level << 2));
        c.flagX = (level >> 14) & 1;
        c.d[1] = Mem32(c, c.a[1] + static_cast<uint32_t>(static_cast<int16_t>(c.d[1])));
        const bool yes = (c.d[1] >> (c.d[0] & 31)) & 1;
        c.flagN = (c.d[1] >> 31) != 0;
        c.flagV = c.flagC = false;
        c.flagZ = !yes;
        return yes;
    }
    bool Chance(M68K &c, uint32_t table) {
        c.a[1] = table;
        return Chance(c);
    }
    // ... from the character's own table of 128 bytes
    bool CharacterChance(M68K &c, uint32_t tables) {
        SetLow16(c.d[0], static_cast<uint16_t>(Character(c) << 7));
        c.a[1] = tables + static_cast<uint32_t>(static_cast<int16_t>(c.d[0]));
        return Chance(c);
    }

    // 02CE44. A move (the word in D1) unpacked into what a player's buttons would give: bit 0 clear, a normal
    // (its stance, punch or kick, strength, the directions held, a variant); set, a special (its number, its
    // strength, a variant)
    void UnpackMove(M68K &c) {
        uint32_t move = c.d[1] & 0xFFFF;
        uint8_t last;
        if (!(move & 1)) {
            SetByte(c, kStance, move & 6);
            move >>= 2;
            SetByte(c, kButton, move & 6);
            move >>= 2;
            SetByte(c, kStrength, move & 6);
            move >>= 4;
            SetByte(c, kDirection, move & 3);
            SetByte(c, kDirection2, move & 0xC);
            c.flagX = (move >> 3) & 1;
            move >>= 4;
            last = move & 0xF;
        } else {
            SetByte(c, kSpecial, move & 0x1E);
            move >>= 4;
            SetByte(c, kStrength, move & 6);
            c.flagX = (move >> 3) & 1;
            move >>= 4;
            last = move & 0xFF;
        }
        SetLow16(c.d[1], move);
        SetByte(c, kVariant, last);
        // (the flags as the routine's last instruction leaves them)
        c.flagN = (last & 0x80) != 0;
        c.flagZ = last == 0;
        c.flagV = c.flagC = false;
    }
    void UnpackMove(M68K &c, uint16_t move) {
        SetLow16(c.d[1], move);
        UnpackMove(c);
    }
    // 02CEB0. One of a chain's moves (the byte in D1): its strength, punch or kick, and stance
    void UnpackChainMove(M68K &c) {
        uint8_t move = c.d[1] & 0xFF;
        SetByte(c, kStrength, move & 6);
        move >>= 3;
        SetByte(c, kButton, move & 2);
        c.flagX = (move >> 1) & 1;
        move >>= 2;
        SetByte(c, kStance, move & 6);
        SetLow8(c.d[1], move);
        c.flagN = false;
        c.flagZ = (move & 6) == 0;
        c.flagV = c.flagC = false;
    }
    // The next move of the chain kChainNumber, in D1 (A1 left at the chain's list)
    void NextOfChain(M68K &c, uint32_t &list) {
        list = kChains;
        SetLow16(c.d[0], Word(c, kChainNumber));
        list += static_cast<uint32_t>(static_cast<int16_t>(Mem16(c, list + static_cast<uint32_t>(static_cast<int16_t>(c.d[0])))));
        SetLow16(c.d[0], Word(c, kChainAt));
        SetWord(c, kChainAt, Add16(c, Word(c, kChainAt), 1));
        SetLow8(c.d[1], Mem8(c, list + static_cast<uint32_t>(static_cast<int16_t>(c.d[0]))));
    }

    // 02CE30. Can the character do the special unpacked now? (its own routine says, in the flags)
    bool CanDoSpecial(M68K &c) {
        c.d[0] = static_cast<uint8_t>(Byte(c, kCharacter) << 2);
        c.flagX = (Byte(c, kCharacter) >> 6) & 1;
        c.a[1] = MemPointer(c, kCanDoSpecial + c.d[0]);
        Call(c, c.a[1]);
        return !c.flagZ;
    }

    // 02D43C. Is an attack of the other side's coming? A thing thrown of the other side's with an attack out, or
    // the other player attacking near enough (or throwing)
    bool AttackComing(M68K &c) {
        const uint32_t me = Me(c);
        c.d[6] = 0x1F;
        c.a[4] = c.a[5] + static_cast<uint32_t>(kThrownFromA5); // (the things thrown: kThrown)
        for (;;) {
            const uint32_t thing = c.a[4] & 0xFFFFFF;
            if (Ram8(thing) == 1 && Ram8(thing + 1) && !Ram8(thing + kThrownAttack)) {
                SetLow8(c.d[0], Ram8(me + kSide));
                if (Ram8(me + kSide) != Ram8(thing + kSide)) {
                    c.a[1] = Ram32(thing + kFrame);
                    if (!(Mem8(c, c.a[1] + kMoveStrength) & 0x80)) {
                        Answer(c, true);
                        return true;
                    }
                }
            }
            c.a[4] += kThrownSize;
            SetLow16(c.d[6], (c.d[6] - 1) & 0xFFFF);
            if ((c.d[6] & 0xFFFF) == 0xFFFF) {
                break;
            }
        }
        const uint32_t other = Other(c);
        if (Ram8(me + kAirborne) && !Ram8(other + kAirborne)) {
            Answer(c, false);
            return false;
        }
        if (Ram8(other + kThrowing)) {
            Answer(c, true);
            return true;
        }
        c.a[3] = Ram32(other + kFrame);
        if ((Mem8(c, c.a[3] + kMoveStrength) & 0x80) || !Ram8(other + kAttacking)) {
            Answer(c, false);
            return false;
        }
        // The attack's boxes, frame by frame of the animation (a frame is 24 bytes), until one is near enough or
        // the animation's end: near enough is within half a box (and 80h) across
        c.a[2] = Ram32(other + kFrame);
        c.a[1] = Ram32(other + kAttackBoxes);
        for (;;) {
            SetLow8(c.d[0], Mem8(c, c.a[2] + kFrameAttack));
            if (c.d[0] & 0xFF) {
                const uint16_t number = family::kComingBoxesUnsigned ? static_cast<uint16_t>(c.d[0] & 0xFF)
                                                                     : static_cast<uint16_t>(static_cast<int8_t>(c.d[0]));
                SetLow16(c.d[0], static_cast<uint16_t>(number << 5));
                c.flagX = (number >> 11) & 1;
                c.a[1] += static_cast<uint32_t>(static_cast<int16_t>(c.d[0]));
                uint16_t across = Mem16(c, c.a[1]);
                if (Ram8(other + kFacesRight)) {
                    across = Sub16(c, 0, across);
                }
                across = Sub16(c, Add16(c, across, Ram16(other + kX)), Ram16(me + kX));
                if (across & 0x8000) {
                    across = Sub16(c, 0, across);
                }
                SetLow16(c.d[0], across);
                SetLow16(c.d[1], Add16(c, Mem16(c, c.a[1] + 4), 0x80));
                if ((c.d[0] & 0xFFFF) < (c.d[1] & 0xFFFF)) {
                    const bool yes = !Ram8(other + kAttackSeen);
                    Answer(c, yes);
                    return yes;
                }
            }
            SetLow16(c.d[0], Mem8(c, c.a[2] + 1) & 0xC0);
            if (c.d[0] & 0xFFFF) {
                break;
            }
            c.a[2] += 0x18;
        }
        // (at the animation's end: is the other near enough across?)
        SetLow16(c.d[0], Add16(c, Sub16(c, Ram16(me + kX), Ram16(other + kX)), family::kComingNear));
        const bool yes = (c.d[0] & 0xFFFF) < 2u * family::kComingNear;
        Answer(c, yes);
        return yes;
    }

    // 02D240. Is the other open: on the floor, in its own control, hittable, and in an animation that can be hit?
    bool OpenFrom(M68K &c, uint32_t other) {
        if (Ram16(other + kState) != 0x0200 || Ram8(other + kAirborne) || Ram8(other + kNotHit) || Ram8(other + kOpenFlag)) {
            Answer(c, false);
            return false;
        }
        c.a[1] = Ram32(other + kFrame);
        c.d[2] = Mem32(c, c.a[1] + kHurt) & 0xFFFFFF00;
        Answer(c, c.d[2] != 0);
        return c.d[2] != 0;
    }
    bool Open(M68K &c) {
        return OpenFrom(c, Other(c));
    }
    // 02D234. ... and not just out of a reeling early (Reactions' answer 1)
    bool OpenNotRecovered(M68K &c) {
        const uint32_t other = Other(c);
        if (Ram8(other + kRecovered)) {
            Answer(c, false);
            return false;
        }
        return OpenFrom(c, other);
    }

    // --- choosing a script ---

    // A script's table: the character's, then the entry at `offset` (a signed word offset from the table)
    uint32_t Entry(M68K &c, uint32_t table, uint32_t offset) {
        return table + static_cast<uint32_t>(static_cast<int16_t>(Mem16(c, table + offset)));
    }
    // A script begins with what it does when broken off; A0 is left at its first instruction
    void Begin(M68K &c, uint32_t script, uint32_t place) {
        c.a[0] = script;
        SetWord(c, kIfBrokenOff, Operand(c));
        SetLong(c, place, c.a[0]);
    }
    // One of 32 slots at random (of the tier's), naming a script of the table at A0 (its offsets after the slots)
    void FromSlots(M68K &c) {
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x1F);
        SetLow8(c.d[0], c.d[0] + Byte(c, kTier));
        SetLow8(c.d[1], Mem8(c, c.a[0] + static_cast<uint32_t>(static_cast<int16_t>(c.d[0]))));
        SetByte(c, kSlot, c.d[1] & 0xFF);
        SetLow16(c.d[1], static_cast<uint16_t>(static_cast<int8_t>(c.d[1]) * 2 + 0x60));
        c.a[0] = Entry(c, c.a[0], c.d[1] & 0xFFFF);
    }

    // 02CCAE. The plan kPlan
    void ChoosePlan(M68K &c) {
        SetByte(c, kSlot, 0);
        SetByte(c, kUnbreakable, 0);
        c.a[0] = MemPointer(c, kPlans + static_cast<uint32_t>(Character(c) * 4));
        SetLow16(c.d[0], static_cast<uint16_t>(Character(c) * 4));
        SetLow8(c.d[0], Byte(c, kPlan));
        Begin(c, Entry(c, c.a[0], c.d[0] & 0xFFFF), kPlaces + 0);
        SetLong(c, kPlanStart, c.a[0]);
        SetByte(c, kDepth, 0);
    }
    // Which of the modes' tactics the fight calls for (-1: none, or no mode)
    int ModeSituation(M68K &c) {
        switch (ComputerModeOf(Me(c))) {
        case ComputerMode::kRingOut: {
            const int mine = ComputerBack(Me(c), false), theirs = ComputerBack(Me(c), true);
            if (mine < kNearOwnEnd && mine < theirs) {
                return kOutOfTheCorner;
            }
            if (theirs < kNearTheirEnd && theirs < mine) {
                return kKnockOver;
            }
            return -1;
        }
        case ComputerMode::kTag:
            return ComputerTagWanted(Me(c)) ? static_cast<int>(kTagOut) : -1;
        default:
            return -1;
        }
    }

    // 02CCE6. A tactic of the plan's, at random. (In the modes: one of the port's when the fight calls for it, from
    // four times in ten to nine times in ten by the difficulty)
    void ChooseTactic(M68K &c) {
        SetByte(c, kUnbreakable, 0);
        SetLow16(c.d[0], static_cast<uint16_t>(Character(c) * 4));
        c.a[0] = MemPointer(c, kTactics + (c.d[0] & 0xFFFF));
        SetLow8(c.d[0], Byte(c, kPlan));
        c.a[0] = Entry(c, c.a[0], c.d[0] & 0xFFFF);
        FromSlots(c);
        const int mode = ModeSituation(c);
        if (mode >= 0) {
            Random(c);
            if (static_cast<int>(c.d[0] & 0xFF) * 1000 / 256 < 400 + ComputerSkill() / 2) {
                c.a[0] = kModeScripts + 2u * static_cast<uint32_t>(mode);
                // (for tests, CPS2RECOMP_BATTLE_LOG as fight/battle.cpp's: each of the modes' tactics chosen)
                static const bool log = getenv("CPS2RECOMP_BATTLE_LOG") != nullptr;
                if (log) {
                    fprintf(stderr, "tactic side=%d %s gap=%d\n", Me(c) == Player(0) ? 0 : 1,
                            mode == kOutOfTheCorner ? "out of the corner" : mode == kKnockOver ? "knock over" : "tag out", Word(c, kGap));
                }
            }
        }
        Begin(c, c.a[0], kPlaces + 4);
        SetByte(c, kDepth, 0);
    }
    // 02CD3C. A reaction, at random
    void ChooseReaction(M68K &c) {
        SetByte(c, kUnbreakable, 0);
        c.a[0] = MemPointer(c, kReactionScripts + static_cast<uint32_t>(Character(c) * 4));
        Other(c);
        c.d[0] = 0;
        SetByte(c, kReactionTable, 0);
        c.a[0] = Entry(c, c.a[0], 0);
        FromSlots(c);
        Begin(c, c.a[0], kPlaces + 8);
        SetByte(c, kDepth, 0);
    }
    // 02CD98. The answer kAnswer
    void ChooseAnswer(M68K &c) {
        SetByte(c, kUnbreakable, 0);
        c.a[0] = MemPointer(c, kAnswers + static_cast<uint32_t>(Character(c) * 4));
        c.d[0] = Byte(c, kAnswer);
        SetByte(c, kSlot, c.d[0] & 0xFF);
        c.d[0] *= 2;
        Begin(c, Entry(c, c.a[0], c.d[0]), kPlaces + 12);
        SetByte(c, kDepth, 0);
    }

    // 02CDD0. A think time to kCount: the level's, and (in the usual games) the character's and one at random
    void Think(M68K &c) {
        c.d[0] = 0;
        c.a[1] = kThinkByLevel;
        SetLow8(c.d[0], Byte(c, kLevel) * 2);
        SetWord(c, kCount, Mem16(c, c.a[1] + (c.d[0] & 0xFFFF)));
        const uint16_t mode = Ram16(Var(c, kGameMode));
        if (mode == 8 || mode == 4 || mode == 0xC) {
            return;
        }
        c.d[0] = Byte(c, kCharacter) * 2u;
        c.a[1] = kThinkByCharacter;
        SetLow16(c.d[0], Mem16(c, c.a[1] + c.d[0]));
        SetWord(c, kCount, Word(c, kCount) + (c.d[0] & 0xFFFF));
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x3E);
        c.a[1] = kThinkRandom;
        SetLow16(c.d[0], Mem16(c, c.a[1] + (c.d[0] & 0xFFFF)));
        SetWord(c, kCount, Word(c, kCount) + (c.d[0] & 0xFFFF));
        if (Word(c, kCount) & 0x8000) {
            SetWord(c, kCount, 0);
        }
    }

    // --- the instructions ---

    // The words each instruction takes after its own, by operation (and kind): what Skip steps over
    constexpr uint8_t kWaitWords[19] = {1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 2, 2, 0, 0, 0};
    constexpr uint8_t kWalkWords[8] = {1, 1, 1, 1, 1, 2, 2, 2};
    constexpr uint8_t kJumpWords[6] = {3, 5, 4, 3, 3, 3};
    // and the if's tests (by the test's number, in twos)
    // (the last three are the port's: 44, 46, 48)
    constexpr uint8_t kTestWords[37] = {1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1,
                                        0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0};
    enum Operation : uint8_t {
        kWaits = 0x00, kWalks = 0x02, kJumps = 0x04, kNormalMove = 0x06, kSpecialMoves = 0x08, kStockMoves = 0x0A,
        kIf = 0x0C, kGoToTactic = 0x0E, kGuards = 0x10, kBreakIn = 0x12, kStockMoves2 = 0x14, kDashes = 0x16,
        kChainMoves = 0x18, kYield = 0x1A, kTag = 0x1C, kEnd = 0xFF,
    };
    enum IfKind : uint8_t { kIfTest = 0, kElse = 2, kEndIf = 4 };

    // 02D0A8. A failed if (or an else reached): the instructions stepped over, to the else or the endif that
    // closes it (D6: the depth it began at)
    void Skip(M68K &c) {
        for (;;) {
            c.d[0] = ScriptByte(c, c.a[0]);
            c.a[0] += 1;
            const uint8_t operation = c.d[0] & 0xFF;
            SetLow8(c.d[0], ScriptByte(c, c.a[0]));
            c.a[0] += 1;
            const uint8_t kind = c.d[0] & 0xFF;
            // (no script ends inside an if: aiscripts.py --check)
            uint32_t words = 0;
            switch (operation) {
            case kWaits: words = kWaitWords[kind / 2]; break;
            case kWalks: words = kWalkWords[kind / 2]; break;
            case kJumps: words = kJumpWords[kind / 2]; break;
            case kNormalMove: case kSpecialMoves: case kStockMoves: case kStockMoves2: case kChainMoves: words = 1; break;
            case kDashes: words = 3; break;
            case kGoToTactic: case kYield: case kTag: words = 0; break;
            case kGuards: words = kind == 2 ? 1 : 0; break;
            case kBreakIn: words = kind < 4 ? 2 : 0; break;
            case kIf:
                if (kind == kIfTest) {
                    SetByte(c, kDepth, Byte(c, kDepth) + 1);
                    SetLow16(c.d[0], Operand(c));
                    words = kTestWords[(c.d[0] & 0xFFFF) / 2];
                } else if (kind == kElse) {
                    if ((c.d[6] & 0xFF) == Byte(c, kDepth)) {
                        return;
                    }
                } else {
                    SetByte(c, kDepth, Byte(c, kDepth) - 1);
                    SetLow8(c.d[5], (c.d[6] & 0xFF) - Byte(c, kDepth));
                    if ((c.d[5] & 0xFF) == 1) {
                        return;
                    }
                }
                break;
            }
            c.a[0] += words * 2;
        }
    }

    // 02C718 (below)
    void End(M68K &c);

    // 02BB9C. A wait begins: the player stands (if it is not already, or in a 1A pose)
    bool Stands(M68K &c) {
        const uint32_t state = Ram32(Me(c) + kState) & 0xFFFFFF00;
        if (state != 0x02001A00 && state != kStand) {
            SetLong(c, kState, kStand);
        }
        return false;
    }
    bool Starts(M68K &c, uint16_t waiting, uint32_t state) {
        SetWord(c, kWaiting, waiting);
        SetLong(c, kState, state);
        return false;
    }

    // Each instruction: true when it was done at once (and the next is to be taken), false when it has started
    // something that goes on (Waits looks at it each frame)

    // 02B96A. Operation 00: waits, and standing or crouching
    bool Wait(M68K &c, uint8_t kind) {
        uint32_t other;
        switch (kind) {
        case 0x00: // stand, for so many frames
            SetWord(c, kWaiting, 0x0000);
            SetWord(c, kCount, Operand(c));
            SetLong(c, kState, kStand);
            return false;
        case 0x02: // crouch
            if (Byte(c, kCrouching)) {
                return true;
            }
            return Starts(c, 0x0002, kCrouch);
        case 0x04: // stand up
            if (!Byte(c, kCrouching)) {
                return true;
            }
            return Starts(c, 0x0004, kStandUp);
        case 0x06: // while the other is in the air
            if (!InTheAir(Other(c))) {
                return true;
            }
            return Starts(c, 0x0006, kStand);
        case 0x08: { // while the other is in the air, above a height or still rising
            other = Other(c);
            const uint16_t height = Operand(c);
            SetWord(c, kOperands, height);
            if (!InTheAir(other)) {
                return true;
            }
            if (static_cast<uint16_t>(height + Ram16(other + kFloor)) >= Ram16(other + kY) && (Ram16(other + kSpeedY) & 0x8000)) {
                return true;
            }
            return Starts(c, 0x0008, kStand);
        }
        case 0x0A: { // while a projectile of the computer's of a kind is out
            const uint16_t which = Operand(c);
            SetWord(c, kOperands, which);
            c.a[1] = c.a[6] + kLive;
            if (!Ram8((c.a[1] + static_cast<uint32_t>(static_cast<int16_t>(which))) & 0xFFFFFF)) {
                return true;
            }
            SetWord(c, kWaiting, 0x0010);
            return Stands(c);
        }
        case 0x0C:
            return true;
        case 0x0E: // while the other's projectiles are out
            other = Other(c);
            SetWord(c, kOperands, Operand(c));
            if (!ProjectilesOut(other)) {
                return true;
            }
            return Starts(c, 0x000E, kStand);
        case 0x10: // think (in a plan, unless it is the first: not to be broken in on)
            if (Word(c, kBank)) {
                Think(c);
                if (!Word(c, kCount)) {
                    return true;
                }
                SetWord(c, kWaiting, 0x0010);
                return Stands(c);
            }
            [[fallthrough]];
        case 0x16: // think
            Think(c);
            if (!Word(c, kCount)) {
                return true;
            }
            SetWord(c, kWaiting, 0x0000);
            return Stands(c);
        case 0x12: // while the computer's projectiles are out
            if (!ProjectilesOut(Me(c))) {
                return true;
            }
            SetWord(c, kWaiting, 0x0012);
            return Stands(c);
        case 0x14: { // until the other is open (or down and not getting up)
            other = Other(c);
            if (!(Ram8(other + kDown) && !Ram8(other + kGettingUp)) && Open(c)) {
                return true;
            }
            SetWord(c, kWaiting, 0x0014);
            return Stands(c);
        }
        case 0x18: // a frame
            SetWord(c, kCount, 1);
            SetWord(c, kWaiting, 0x0010);
            return Stands(c);
        case 0x1A: // so many frames, not to be broken in on
            SetWord(c, kCount, Operand(c));
            SetWord(c, kWaiting, 0x0010);
            return Stands(c);
        case 0x1C: case 0x1E: { // for so many frames while farther than a gap (1C), or nearer (1E)
            const uint16_t gap = Operand(c);
            SetWord(c, kOperands, gap);
            SetWord(c, kCount, Operand(c));
            const bool farther = static_cast<int16_t>(gap - Word(c, kGap)) < 0;
            if (kind == 0x1C ? !farther : farther) {
                return true;
            }
            return Starts(c, kind == 0x1C ? 0x0016 : 0x0018, kStand);
        }
        case 0x20: // until the other is open (and not just out of a reeling)
            if (OpenNotRecovered(c)) {
                return true;
            }
            SetWord(c, kWaiting, 0x001A);
            return Stands(c);
        case 0x22: // a move that takes a stock, once the player is free
            if (Byte(c, kStocks) < family::kStockMoveStocks || Byte(c, family::kNoStockMove)) {
                return true;
            }
            SetWord(c, kWaiting, 0x001C);
            SetByte(c, kStockMove, 1);
            return false;
        default: // 0x24
            SetByte(c, kCanChain, 0);
            SetWord(c, kWaiting, 0x000A);
            return Stands(c);
        }
    }

    // 02BBC0. Operation 02: walks: forward or back to a gap (00, 02), to a gap either way (04), for so many
    // frames (06, 08), and for so many frames or to a gap (0A, 0C, 0E)
    bool Walk(M68K &c, uint8_t kind) {
        uint16_t gap = 0;
        if (kind == 0x06 || kind == 0x08) {
            SetWord(c, kCount, Operand(c));
        } else {
            gap = Operand(c);
            if (kind >= 0x0A) {
                SetWord(c, kCount, Operand(c));
            }
            SetWord(c, kOperands, gap);
        }
        const int16_t beyond = static_cast<int16_t>(gap - Word(c, kGap));
        uint8_t way = 0;
        switch (kind) {
        case 0x00: case 0x0A: // forward, while farther than the gap
            if (beyond >= 0) {
                return true;
            }
            break;
        case 0x02: case 0x0C: // back, while nearer
            if (beyond < 0) {
                return true;
            }
            way = 1;
            break;
        case 0x04: case 0x0E: // to the gap, from either side (within three is there)
            if (static_cast<uint16_t>(beyond + 3) < 6) {
                return true;
            }
            break;
        case 0x08:
            way = 1;
            break;
        }
        SetWord(c, kWaiting, static_cast<uint16_t>(0x0200 | kind));
        SetLong(c, kState, kWalk);
        SetByte(c, kGoing, way);
        SetByte(c, kWay, way);
        if (kind == 0x04 || kind == 0x0E) {
            if (Word(c, kOperands) >= Word(c, kGap)) {
                SetByte(c, kGoing, 1);
                SetByte(c, kWay, 1);
            }
        }
        return false;
    }

    // A jump's way word: the way (high byte) and when its move comes (low byte; 4: after so many frames, the word given)
    void JumpWay(M68K &c, uint16_t way, uint16_t frames) {
        SetByte(c, kWay, way >> 8);
        SetByte(c, kMoveWhen, way & 0xFF);
        if ((way & 0xFE) == 4) {
            SetWord(c, kCount, frames);
        }
    }
    void KeepOperands(M68K &c, int words) {
        for (int n = 0; n < words; n++) {
            SetWord(c, kOperands + 2u * n, Operand(c));
        }
    }
    uint16_t Kept(M68K &c, int n) {
        return Word(c, kOperands + 2u * n);
    }

    // 02BD54. Operation 04: jumps, each with a move (and a way word: JumpWay)
    bool Jump(M68K &c, uint8_t kind) {
        SetWord(c, kWaiting, 0x0400);
        switch (kind) {
        case 0x00: // a jump with a move: the move, the way, the frames
            SetLong(c, kState, kJump);
            SetByte(c, kSecondMove, 0);
            SetByte(c, kChain, 0);
            KeepOperands(c, 3);
            UnpackMove(c, Kept(c, 0));
            JumpWay(c, Kept(c, 1), Kept(c, 2));
            break;
        case 0x02: // a jump whose move is the second: the way, the frames, the move, and two words more
            SetLong(c, kState, kJump);
            SetByte(c, kSecondMove, 1);
            SetByte(c, kChain, 0);
            KeepOperands(c, 5);
            UnpackMove(c, Kept(c, 2));
            JumpWay(c, Kept(c, 0), Kept(c, 1));
            break;
        case 0x04: // two jumps: the move, the way, the frames, the second jump
            SetLong(c, kState, kJump);
            SetByte(c, kSecondMove, 0);
            SetByte(c, kTwoJumps, 1);
            SetByte(c, kChain, 0);
            KeepOperands(c, 4);
            UnpackMove(c, Kept(c, 0));
            SetByte(c, kWay, Kept(c, 1) >> 8);
            SetByte(c, kMoveWhen, Kept(c, 1) & 0xFF);
            SetWord(c, kSecondJump, Kept(c, 3));
            if ((Kept(c, 1) & 0xFE) == 4) {
                SetWord(c, kCount, Kept(c, 2));
            }
            break;
        case 0x06: // a jump away
            SetLong(c, kState, kJumpAway);
            SetByte(c, kSecondMove, 0);
            SetByte(c, kChain, 0);
            KeepOperands(c, 3);
            UnpackMove(c, Kept(c, 0));
            JumpWay(c, Kept(c, 1), Kept(c, 2));
            break;
        case 0x08: // a dash's jump
            SetLong(c, kState, kDashJump);
            SetByte(c, kSpecial, 0x0A);
            SetByte(c, kSecondMove, 0);
            SetByte(c, kChain, 0);
            KeepOperands(c, 3);
            UnpackMove(c, Kept(c, 0));
            SetByte(c, kGoing, Kept(c, 1) >> 8);
            JumpWay(c, Kept(c, 1), Kept(c, 2));
            break;
        default: { // 0x0A: a jump into a chain: the chain, the way, the frames
            SetLong(c, kState, kJump);
            KeepOperands(c, 3);
            SetWord(c, kChainNumber, Kept(c, 0));
            SetByte(c, kSecondMove, 0);
            SetByte(c, kChain, 0xFF);
            SetByte(c, kChainedJump, 1);
            SetByte(c, kCanChain, 0);
            SetByte(c, kDirection, 0);
            SetByte(c, kDirection2, 0);
            SetWord(c, kChainAt, 0);
            NextOfChain(c, c.a[1]);
            UnpackChainMove(c);
            JumpWay(c, Kept(c, 1), Kept(c, 2));
            break;
        }
        }
        return false;
    }

    // The player attacks: the state, its kAttacking, and facing the other
    void Attacks(M68K &c, uint16_t waiting, uint32_t state) {
        SetWord(c, kWaiting, waiting);
        SetLong(c, kState, state);
        SetByte(c, kAttacking, 1);
    }

    // 02BFE2. Operation 06: a normal (kind 02: one a chain may follow)
    bool NormalMove(M68K &c, uint8_t kind) {
        Attacks(c, kind ? 0x0602 : 0x0600, kNormal);
        SetByte(c, kChain, kind ? 1 : 0);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
        SetByte(c, kCanChain, 0);
        const uint16_t move = Operand(c);
        SetWord(c, kOperands, move);
        UnpackMove(c, move);
        return false;
    }

    // 02C04C, 02C090, 02C5A2. Operations 08, 0A and 14: a special, if the character can do it now; 0A and 14 take
    // a stock of the gauge (and need one)
    bool SpecialMove(M68K &c, bool stock, uint16_t waiting, uint32_t state) {
        const uint16_t move = Operand(c);
        SetWord(c, kOperands, move);
        UnpackMove(c, move);
        if (stock && !Byte(c, kStocks)) {
            return true;
        }
        if (!CanDoSpecial(c)) {
            return true;
        }
        Attacks(c, waiting, state);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
        SetByte(c, kCanChain, 0);
        SetByte(c, kCrouching, 0);
        if (stock) {
            SetByte(c, kStocks, Byte(c, kStocks) - 1);
        }
        return false;
    }

    // 02C0F2. An if's test (the word after it): true if it passes (the instructions after it are run), false if
    // not (they are stepped over, to its else or endif)
    bool Test(M68K &c) {
        const uint16_t test = Operand(c);
        SetWord(c, kOperands, test);
        const uint32_t me = Me(c);
        uint32_t other;
        uint16_t value;
        switch (test) {
        case 0x00: // the gap at most
            SetWord(c, kOperands + 2, Operand(c));
            return Word(c, kGap) <= Word(c, kOperands + 2);
        case 0x02: // the gap at least
            SetWord(c, kOperands + 2, Operand(c));
            return Word(c, kGap) >= Word(c, kOperands + 2);
        case 0x04: // one in two
            Random(c);
            SetLow16(c.d[0], c.d[0] & 1);
            return (c.d[0] & 0xFFFF) == 0;
        case 0x06: // a stock of the gauge
            return Byte(c, kStocks) != 0;
        case 0x08: // the other in the air
            return InTheAir(Other(c));
        case 0x0A: // the other crouching
            return Ram8(Other(c) + kCrouching) != 0;
        case 0x0C: // the other lying down
            return Ram8(Other(c) + kLiesDown) != 0;
        case 0x0E: // the other's projectiles out
            return ProjectilesOut(Other(c));
        case 0x10: case 0x12: // (the same, with a word)
            SetWord(c, kOperands + 2, Operand(c));
            return ProjectilesOut(Other(c));
        case 0x14: // an attack coming
            return AttackComing(c);
        case 0x16: case 0x18: case 0x1A: { // the other jumping back, forward, straight up
            other = Other(c);
            const uint8_t way = test == 0x16 ? 1 : test == 0x18 ? 0 : 0xFF;
            return InTheAir(other) && Ram8(other + kGoing) == way;
        }
        case 0x1C: // the other open
            return Open(c);
        case 0x1E: case 0x20: // the level at most, at least
            SetWord(c, kOperands + 2, Operand(c));
            c.d[0] = Byte(c, kLevel);
            c.d[1] = Word(c, kOperands + 2) & 0x1F;
            return test == 0x1E ? c.d[0] <= c.d[1] : c.d[0] >= c.d[1];
        case 0x22: // none of the computer's projectiles out
            return !ProjectilesOut(me);
        case 0x24: // the other's health below
            value = Operand(c);
            return value > Ram16(Other(c) + kHealthCeiling);
        case 0x26: // the computer's health above the other's
            value = Word(c, kHealthCeiling);
            return value > Ram16(Other(c) + kHealthCeiling);
        case 0x28: // the other rising
            other = Other(c);
            return InTheAir(other) && !(Ram32(other + kSpeedY) & 0x80000000);
        case 0x2A: // the other not rising
            other = Other(c);
            return !InTheAir(other) || (Ram32(other + kSpeedY) & 0x80000000);
        case 0x2C: case 0x2E: { // the other below a height, above
            value = Operand(c);
            other = Other(c);
            const uint16_t height = static_cast<uint16_t>(value + Ram16(other + kFloor));
            return test == 0x2C ? height >= Ram16(other + kY) : height <= Ram16(other + kY);
        }
        case 0x30: // none of the other's projectiles out
            return !ProjectilesOut(Other(c));
        case 0x32:
            return ProjectilesOut(Other(c));
        case 0x34: // the computer's health below
            value = Operand(c);
            return value > Word(c, kHealthCeiling);
        case 0x36: // one in four
            Random(c);
            SetLow16(c.d[0], c.d[0] & 3);
            return (c.d[0] & 0xFFFF) == 0;
        case 0x38: // the other is the character
            value = Operand(c);
            return (value & 0xFF) == Ram8(Other(c) + kCharacter);
        case 0x3A: // the computer is
            value = Operand(c);
            return (value & 0xFF) == Byte(c, kCharacter);
        case 0x3C:
            return Byte(c, kCanChain) != 0;
        case 0x3E: { // a chance of 16 (the bits of the word)
            Random(c);
            SetLow16(c.d[0], c.d[0] & 0xF);
            value = Operand(c);
            return (value >> (c.d[0] & 0xF)) & 1;
        }
        case 0x40: // a projectile of the other's of a kind out
            value = Operand(c);
            other = Other(c);
            c.a[4] += kLive;
            return Ram8((c.a[4] + static_cast<uint32_t>(static_cast<int16_t>(value))) & 0xFFFFFF) != 0;
        case 0x42: // stocks, so many at least
            value = Operand(c);
            return (value & 0xFF) <= Byte(c, kStocks);
        // The port's: in a ring out, the computer's back nearer its end than so many pixels, the other's; in the tag
        // style, the computer's fighter wanting to go off (fight/battle.h)
        case 0x44: case 0x46:
            value = Operand(c);
            return ComputerBack(me, test == 0x46) < value;
        default: // 0x48
            return ComputerTagWanted(me);
        }
    }

    // 02C0E0. Operation 0C: if (00, and a test), else (02), endif (04)
    bool If(M68K &c, uint8_t kind) {
        if (kind == kIfTest) {
            SetByte(c, kDepth, Byte(c, kDepth) + 1);
            if (Test(c)) {
                return true;
            }
        } else if (kind == kEndIf) {
            SetByte(c, kDepth, Byte(c, kDepth) - 1);
            return true;
        }
        // (a test failed, or the else after the instructions of one that passed)
        SetLow8(c.d[6], Byte(c, kDepth));
        Skip(c);
        return true;
    }

    // 02C47A. Operation 0E: go to a tactic, and come back to the plan after it (02: by a chance)
    bool GoToTactic(M68K &c, uint8_t kind) {
        if (kind == 2 && !Chance(c, kGoToChances) && ModeSituation(c) < 0) {
            return true; // (in a mode whose tactic the fight calls for, a tactic is gone to anyway)
        }
        SetWord(c, kKeptIfBrokenOff, Word(c, kIfBrokenOff));
        SetByte(c, kBank, 2);
        SetLong(c, kPlaces, c.a[0]);
        ChooseTactic(c);
        return true;
    }

    // 02C4B4. Operation 10: guard, if an attack is coming and the character's chance says so (02: with a move
    // after it)
    bool Guard(M68K &c, uint8_t kind) {
        uint16_t move = 0;
        if (kind == 0) {
            if (!AttackComing(c) || !CharacterChance(c, kGuardChances)) {
                return true;
            }
            Starts(c, 0x0C00, kGuard);
            SetByte(c, kGuardThenMove, 0);
            return false;
        }
        move = Operand(c);
        SetWord(c, kOperands, move);
        if (!AttackComing(c)) {
            return true;
        }
        Starts(c, 0x0C00, kGuard);
        SetByte(c, kGuardThenMove, Byte(c, kKind));
        if (!Byte(c, kTier) || !Byte(c, kGuardsWithMove)) {
            SetByte(c, kGuardThenMove, 0);
            return false;
        }
        if (CharacterChance(c, kGuardChances)) {
            UnpackMove(c, Word(c, kOperands));
        }
        return false;
    }

    // 02C556. Operation 12: above a level, let answers break in (00), or keep them out (02) (the bits of a word);
    // nothing breaks in (06), or it may again (04)
    bool LetBreakIn(M68K &c, uint8_t kind) {
        if (kind == 4) {
            SetByte(c, kUnbreakable, 0);
            return true;
        }
        if (kind == 6) {
            SetByte(c, kUnbreakable, Byte(c, kUnbreakable) + 1);
            return true;
        }
        SetLow16(c.d[1], Operand(c));
        SetLow16(c.d[2], Operand(c));
        if (kind == 2) {
            SetLow16(c.d[2], c.d[2] ^ 0xFFFF);
        }
        if ((c.d[1] & 0xFF) < Byte(c, kLevel)) {
            const uint16_t bits = c.d[2] & 0xFFFF;
            SetWord(c, kMayBreakIn, kind == 2 ? Word(c, kMayBreakIn) & bits : Word(c, kMayBreakIn) | bits);
        }
        return true;
    }

    // 02C5F2. Operation 16: a dash with a move: the move, the way, the frames
    bool DashMove(M68K &c) {
        Attacks(c, 0x1000, kDash);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
        SetByte(c, kCanChain, 0);
        SetByte(c, kCrouching, 0);
        SetByte(c, kDashing, 1);
        KeepOperands(c, 3);
        UnpackMove(c, Kept(c, 0));
        SetByte(c, kDashForward, 0);
        SetByte(c, kDashBack, 1);
        SetByte(c, kWay, Kept(c, 1) >> 8);
        if (!(Kept(c, 1) >> 8)) {
            SetByte(c, kDashForward, 1);
            SetByte(c, kDashBack, 0);
        }
        SetByte(c, kMoveWhen, Kept(c, 1) & 0xFF);
        if ((Kept(c, 1) & 0xFE) == 4) {
            SetWord(c, kCount, Kept(c, 2));
        }
        return false;
    }

    // 02C682. Operation 18: a chain, its moves from the list kChains names
    bool Chain(M68K &c) {
        SetWord(c, kOperands, Operand(c));
        SetWord(c, kChainAt, 0);
        SetWord(c, kChainNumber, Word(c, kOperands));
        Attacks(c, 0x1200, kNormal);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
        SetByte(c, kCanChain, 0);
        SetByte(c, kChain, 0xFF);
        SetByte(c, kDirection, 0);
        SetByte(c, kDirection2, 0);
        NextOfChain(c, c.a[1]);
        UnpackChainMove(c);
        return false;
    }

    // 02B942. The instruction in kOperation, and the ones after it that are done at once, to one that goes on
    void Run(M68K &c) {
        for (;;) {
            const uint8_t kind = Byte(c, kKind);
            bool next;
            switch (Byte(c, kOperation)) {
            case kWaits: next = Wait(c, kind); break;
            case kWalks: next = Walk(c, kind); break;
            case kJumps: next = Jump(c, kind); break;
            case kNormalMove: next = NormalMove(c, kind); break;
            case kSpecialMoves: next = SpecialMove(c, false, 0x0800, kSpecialMove); break;
            case kStockMoves: next = SpecialMove(c, true, 0x0A00, kStockSpecial); break;
            case kIf: next = If(c, kind); break;
            case kGoToTactic: next = GoToTactic(c, kind); break;
            case kGuards: next = Guard(c, kind); break;
            case kBreakIn: next = LetBreakIn(c, kind); break;
            case kStockMoves2: next = SpecialMove(c, true, 0x0E00, kStockSpecial2); break;
            case kDashes: next = DashMove(c); break;
            case kChainMoves: next = Chain(c); break;
            case kTag: next = true; ComputerAsksTag(Me(c)); break; // (the port's: a tag, taken once the fighter can)
            default: next = false; break; // kYield: nothing more this frame
            }
            if (!next) {
                return;
            }
            // 02C6FC. The next instruction
            c.d[0] = 0;
            SetWord(c, kOperation, Operand(c));
            SetLong(c, kPlace, c.a[0]);
            if (Byte(c, kOperation) == kEnd) {
                End(c);
            }
        }
    }

    // The bank's script from where it was: A0, and its instruction taken
    void Resume(M68K &c, uint32_t place) {
        c.a[0] = Ram32(Me(c) + place);
        SetLong(c, kPlace, c.a[0]);
        SetWord(c, kOperation, Operand(c));
    }

    // 02C718. A script's end (FF): 00 a tactic at random; 02 back to the plan (in the usual games; in others, as
    // 04); 04 one of the two, by a chance
    void End(M68K &c) {
        for (;;) {
            uint8_t how = Byte(c, kKind);
            if (how == 2 && Ram16(Var(c, kGameMode)) != 0 && Ram16(Var(c, kGameMode)) != 2) {
                how = 4;
            }
            if (how == 4) {
                c.a[1] = kPlanOrTactic;
                how = Chance(c) ? 2 : 0;
            }
            if (how == 0) { // 02C72A
                if (!Byte(c, kBank)) {
                    ChoosePlan(c);
                }
                SetByte(c, kBank, 2);
                ChooseTactic(c);
                Resume(c, kPlaces + 4);
                return;
            }
            if (!Byte(c, kBank)) { // 02C760: the plan again from its start
                SetByte(c, kBank, 0);
                ChoosePlan(c);
                Resume(c, kPlaces + 0);
                if (Byte(c, kOperation) != kEnd) {
                    return;
                }
                continue;
            }
            // 02C784: back to the plan where it was
            SetByte(c, kBank, 0);
            SetWord(c, kIfBrokenOff, Word(c, kKeptIfBrokenOff));
            Resume(c, kPlaces + 0);
            if (Byte(c, kOperation) != kEnd) {
                return;
            }
            ChoosePlan(c);
        }
    }

    // 02B90E. The bank's next instruction started (and those done at once before it)
    void Step(M68K &c) {
        c.a[0] = Ram32(Me(c) + kPlace);
        SetWord(c, kOperation, Operand(c));
        if (Byte(c, kOperation) == kEnd) {
            End(c);
        }
        Run(c);
        c.d[0] = Byte(c, kBank);
        SetLong(c, kPlace, c.a[0]);
        SetLong(c, kPlaces + 2 * c.d[0], c.a[0]);
    }

    // --- an instruction going on, each frame ---

    // 02CCA4. The instruction is over: the next is started next frame
    void Over(M68K &c) {
        SetByte(c, kStarted, 0);
        SetWord(c, kWaiting, 0);
    }

    // 02CED4. May a reaction break in? The other has projectiles out, or an attack out (that is one to answer);
    // the computer stands free; the character's chance says so
    bool MayBreakIn(M68K &c) {
        if (Byte(c, kUnbreakable) || Byte(c, kGuardThenMove)) {
            Answer(c, false);
            return false;
        }
        const uint32_t other = Other(c);
        if (!ProjectilesOut(other)) {
            if (!Ram8(other + kAttacking)) {
                Answer(c, false);
                return false;
            }
            c.a[3] = Ram32(other + kFrame);
            if (Mem8(c, c.a[3] + kMoveStrength) & 0x80) {
                Answer(c, false);
                return false;
            }
        }
        const uint8_t action = Byte(c, kAction);
        if (Word(c, kState) != 0x0200 || action == 0x0A || action == 0x0E || action == 0x10 || action == 0x12 || action == 0x14) {
            Answer(c, false);
            return false;
        }
        const bool yes = CharacterChance(c, kGuardChances);
        Answer(c, yes);
        return yes;
    }

    void ComputerPlays(M68K &c);

    // 02CF66. A reaction breaks in: bank 4 from its start
    void React(M68K &c) {
        SetByte(c, kBank, 4);
        SetLong(c, kChosen, 0);
        ComputerPlays(c);
    }

    // The waits of operation 00 (by the kind kWaiting names): true while it goes on
    bool Waiting(M68K &c, uint8_t kind) {
        uint32_t other;
        switch (kind) {
        case 0x00: case 0x10: // frames (0x10: not to be broken in on)
            if (!Word(c, kCount)) {
                return false;
            }
            SetWord(c, kCount, Word(c, kCount) - 1);
            return Word(c, kCount) != 0;
        case 0x02: // crouching
            return Byte(c, kAction) && !Byte(c, kCrouching);
        case 0x04: // standing up
            return Byte(c, kAction) && Byte(c, kCrouching);
        case 0x06:
            return InTheAir(Other(c));
        case 0x08:
            other = Other(c);
            if (!InTheAir(other)) {
                return false;
            }
            SetLow16(c.d[0], Word(c, kOperands) + Ram16(other + kFloor));
            return (c.d[0] & 0xFFFF) < Ram16(other + kY) || !(Ram16(other + kSpeedY) & 0x8000);
        case 0x0A:
            SetLow16(c.d[1], Word(c, kOperands));
            c.a[1] = c.a[6] + kLive;
            return Ram8((c.a[1] + static_cast<uint32_t>(static_cast<int16_t>(c.d[1]))) & 0xFFFFFF) != 0;
        case 0x0C:
            return false;
        case 0x0E:
            return ProjectilesOut(Other(c));
        case 0x12:
            return ProjectilesOut(Me(c));
        case 0x14:
            other = Other(c);
            if (Ram8(other + kDown) && !Ram8(other + kGettingUp)) {
                return false;
            }
            return !Open(c);
        case 0x16: case 0x18:
            if (!Word(c, kCount)) {
                return false;
            }
            SetWord(c, kCount, Word(c, kCount) - 1);
            if (!Word(c, kCount)) {
                return false;
            }
            return kind == 0x16 ? Word(c, kOperands) < Word(c, kGap) : Word(c, kOperands) >= Word(c, kGap);
        case 0x1A:
            return !OpenNotRecovered(c);
        default: // 0x1C
            return Byte(c, kStockMove) || Byte(c, kAction);
        }
    }

    // The walks (kind as Walk's): true while it goes on
    bool Walking(M68K &c, uint8_t kind) {
        if (!Byte(c, kAction)) {
            return false;
        }
        const bool stopped = Byte(c, kAtEdge) != 0;
        if (kind >= 0x06) {
            if (!Word(c, kCount)) {
                return false;
            }
            SetWord(c, kCount, Word(c, kCount) - 1);
        }
        switch (kind) {
        case 0x00: case 0x0A:
            if (Word(c, kOperands) >= Word(c, kGap)) {
                return false;
            }
            return !(Byte(c, kGoing) && stopped);
        case 0x02: case 0x0C:
            if (Word(c, kOperands) < Word(c, kGap)) {
                return false;
            }
            return !(Byte(c, kGoing) && stopped);
        case 0x06: case 0x08:
            return !(Byte(c, kGoing) && stopped);
        default: { // 0x04, 0x0E: to the gap, turning about as it is passed
            if (kind == 0x04 ? (Byte(c, kWay) && stopped) : stopped) {
                return false;
            }
            const uint16_t gap = Word(c, kOperands);
            if (static_cast<uint16_t>(gap - Word(c, kGap) + 3) < 6) {
                return false;
            }
            SetByte(c, kWay, gap >= Word(c, kGap) ? 1 : 0);
            return true;
        }
        }
    }

    // (the port's) In the modes, a wait or a walk of a plan's or of the game's tactic broken off for the modes' tactic the
    // fight calls for, seen on a frame in twelve or so (as often as the difficulty says, of those): it takes the
    // tactic's place, and goes back to the plan where it was when it ends
    bool ToModeTactic(M68K &c) {
        const uint8_t bank = Byte(c, kBank);
        if (bank > 2 || (bank == 2 && (Ram32(Me(c) + kPlaces + 4) & kModeScripts))) {
            return false;
        }
        const int mode = ModeSituation(c);
        if (mode < 0) {
            return false;
        }
        Random(c);
        if (static_cast<int>(c.d[0] & 0xFF) * 1000 / 256 * 12 >= 400 + ComputerSkill() / 2) {
            return false;
        }
        if (bank == 0) {
            SetWord(c, kKeptIfBrokenOff, Word(c, kIfBrokenOff));
            SetByte(c, kBank, 2);
        }
        Begin(c, kModeScripts + 2u * static_cast<uint32_t>(mode), kPlaces + 4);
        SetByte(c, kDepth, 0);
        static const bool log = getenv("CPS2RECOMP_BATTLE_LOG") != nullptr;
        if (log) {
            fprintf(stderr, "tactic side=%d %s gap=%d (broken off)\n", Me(c) == Player(0) ? 0 : 1,
                    mode == kOutOfTheCorner ? "out of the corner" : mode == kKnockOver ? "knock over" : "tag out", Word(c, kGap));
        }
        return true;
    }

    // 02C7F6. The instruction going on, a frame of it
    void Going(M68K &c) {
        const uint8_t group = Byte(c, kWaiting), kind = Byte(c, kWaiting + 1);
        if ((group == 0x00 && (kind == 0x00 || kind == 0x10)) || group == 0x02) {
            if (ToModeTactic(c)) {
                Over(c);
                return;
            }
        }
        bool on;
        switch (group) {
        case 0x00:
            // (all but a few may be broken in on, by a reaction: bank 4 from its start)
            if (kind != 0x0C && kind != 0x10 && kind != 0x1C && MayBreakIn(c)) {
                React(c);
                return;
            }
            on = Waiting(c, kind);
            break;
        case 0x02:
            if (MayBreakIn(c)) {
                React(c);
                return;
            }
            on = Walking(c, kind);
            break;
        case 0x04: // a jump, to its end (or to the start of a second, kind 0x0604)
            on = Byte(c, kAction) && !((Byte(c, kAnimationOver) & 1) && Ram32(Me(c) + kState) == 0x02000604);
            break;
        case 0x06: // a normal (02: while a chain may follow it)
            on = Byte(c, kAction) && (kind != 2 || Byte(c, kChain));
            break;
        case 0x0C: // a guard, while it is held
            on = Word(c, kState) != 0x0200 || Byte(c, kAction);
            break;
        default: // the rest: to the move's end
            on = Byte(c, kAction) != 0;
            break;
        }
        if (!on) {
            Over(c);
        }
    }

    // 02B7FA. A frame of the computer's play (the player at A6). The bank's script is chosen if it is not yet;
    // the next instruction started if the last is over; and the one going on looked at
    void ComputerPlays(M68K &c) {
        if (Ram8(Var(c, kNoComputer))) {
            return;
        }
        if (Word(c, kCount) >= 0x100) {
            SetWord(c, kCount, 0xFF);
        }
        const uint8_t bank = Byte(c, kBank);
        if (!Byte(c, kChosen)) {
            SetByte(c, kChosen, 2);
            switch (bank) {
            case 0: ChoosePlan(c); break;
            case 2: ChooseTactic(c); break;
            case 4: ChooseReaction(c); break;
            default: ChooseAnswer(c); break;
            }
        }
        if (!Byte(c, kStarted)) {
            SetByte(c, kStarted, 2);
            SetLong(c, kPlace, Ram32(Me(c) + kPlaces + 2u * bank));
            Step(c);
        }
        Going(c);
    }

    // 02D09A. The answer the game chose (Reactions), from its start
    void AnswerNow(M68K &c) {
        SetByte(c, kBank, 6);
        SetLong(c, kChosen, 0);
        ComputerPlays(c);
    }

    // 02C7C0. The script broken off (the player hit, thrown...): by its kIfBrokenOff, the plan goes on where it was
    void BrokenOff(M68K &c) {
        const uint16_t how = Word(c, kIfBrokenOff);
        if (how == 0 || (how == 4 && Chance(c, kBrokenOffChances))) {
            Over(c);
            return;
        }
        SetByte(c, kBank, 0);
        SetByte(c, kDepth, 0);
        SetWord(c, kIfBrokenOff, Word(c, kKeptIfBrokenOff));
        // (02C784)
        SetByte(c, kBank, 0);
        SetWord(c, kIfBrokenOff, Word(c, kKeptIfBrokenOff));
        Resume(c, kPlaces + 0);
        if (Byte(c, kOperation) == kEnd) {
            ChoosePlan(c);
            End(c);
        }
    }

    // 02D048. The computer standing free (standing, crouching, walking, or in a guarding pose)
    bool StandsFree(M68K &c) {
        const uint8_t action = Byte(c, kAction);
        const bool yes = Word(c, kState) == 0x0200 && (action == 0 || action == 2 || action == 4 || action == 0x0C);
        Answer(c, yes);
        return yes;
    }

    // 02CF74. The answers that may break in (kMayBreakIn): is one to be given now? Its number to kAnswer
    // (AnswerNow runs it). 0: the other attacking near, on the floor; 1: reeling (kState 02020400) near an other not
    // attacking: out of it at once (on every other frame); 2: the other knocked down near; 3: the other down
    void Reactions(M68K &c) {
        const uint32_t me = Me(c);
        SetLow16(c.d[5], Word(c, kMayBreakIn));
        const uint16_t bits = c.d[5] & 0xFFFF;
        if (!bits || !Byte(c, kTier) || Byte(c, kBank) == 6) {
            Answer(c, false);
            return;
        }
        bool yes = false;
        c.d[6] = 0;
        if (bits & 1) {
            if (StandsFree(c)) {
                const uint32_t other = Other(c);
                yes = !Ram8(other + kAirborne) && Word(c, kGap) < 0x30 && Ram8(other + kAttacking) && Ram16(other + kState) == 0x0200;
            }
            Answer(c, yes);
        }
        if (!yes) {
            c.d[6] = 1;
            if ((bits & 2) && Ram32(me + kState) == 0x02020400 && Word(c, kGap) < 0x40 && !Ram8(Other(c) + kAttacking) &&
                (Ram8(Var(c, kFrameCount)) & 1)) {
                Call(c, kRecover);
                SetByte(c, kRecovered, 5);
                c.flagX = Byte(c, kRecoverTimer) + 5 > 0xFF;
                SetByte(c, kRecoverTimer, Byte(c, kRecoverTimer) + 5);
                SetByte(c, kFacesRight, Byte(c, kFaceTo));
                yes = true;
            }
        }
        if (!yes) {
            c.d[6] = 2;
            if ((bits & 4) && Word(c, kGap) < 0x60) {
                if (StandsFree(c)) {
                    const uint32_t other = Other(c);
                    yes = Ram32(other + kState) == 0x02020202 && Ram8(other + kHeldDown) == 4 && Ram8(other + kLiesDown);
                }
                Answer(c, yes);
            }
        }
        if (!yes) {
            c.d[6] = 3;
            if ((bits & 8) && StandsFree(c)) {
                yes = Ram8(Other(c) + kDown) != 0;
            }
        }
        if (yes) {
            SetByte(c, kAnswer, c.d[6] & 0xFF);
        }
        Answer(c, yes);
    }

    // --- asked by the player's own routines while the computer's move goes on ---

    // (the two routines below go on by a table of the way word's kinds, through D1: what they leave there)
    constexpr uint16_t kWhenTable[3] = {0x06, 0x06, 0x12};

    // 02D374. Is it time for a jump's move? (by kMoveWhen: at a gap, or after kCount frames; and a special only
    // if the character can do it now, and a stock move only with a stock)
    void JumpMoveNow(M68K &c) {
        if (Byte(c, kSecondMove) || Ram8(Other(c) + kUnshakeable)) {
            Answer(c, false);
            return;
        }
        if (Byte(c, kCharacter) == 6 && Byte(c, kCounter1BC) >= 3) {
            Answer(c, false);
            return;
        }
        if (family::kJumpMoveWaitsToo >= 0 && Byte(c, kCharacter) == family::kJumpMoveWaitsToo && Byte(c, kCounter1BC)) {
            Answer(c, false);
            return;
        }
        SetLow16(c.d[0], Byte(c, kOperands + 3) & 0xFE); // (its way word's low byte)
        if (!(c.d[0] & 0xFFFF)) {
            Answer(c, false);
            return;
        }
        SetLow16(c.d[1], kWhenTable[(c.d[0] & 0xFFFF) / 2]);
        if ((c.d[0] & 0xFFFF) == 2) {
            SetLow16(c.d[1], Kept(c, 2));
            if ((c.d[1] & 0xFFFF) < Word(c, kGap)) {
                Answer(c, false);
                return;
            }
        } else if (Word(c, kCount)) {
            SetWord(c, kCount, Word(c, kCount) - 1);
            c.flagX = false;
            Answer(c, false);
            return;
        }
        if (!(Byte(c, kOperands + 1) & 1)) {
            Answer(c, true);
            return;
        }
        if (!CanDoSpecial(c)) {
            Answer(c, false);
            return;
        }
        if ((Byte(c, kNoGauge) || Byte(c, kStrength) == 6) && !Byte(c, kStocks)) {
            Answer(c, false);
            return;
        }
        Answer(c, true);
    }

    // 02D3F2. ... for a jump whose move is the second (by its own way word)
    void SecondMoveNow(M68K &c) {
        if (!Byte(c, kSecondMove) || Ram8(Other(c) + kUnshakeable)) {
            Answer(c, false);
            return;
        }
        SetLow16(c.d[0], Byte(c, kOperands + 1) & 0xFE);
        if (!(c.d[0] & 0xFFFF)) {
            Answer(c, false);
            return;
        }
        SetLow16(c.d[1], kWhenTable[(c.d[0] & 0xFFFF) / 2]);
        if ((c.d[0] & 0xFFFF) == 2) {
            SetLow16(c.d[1], Kept(c, 2));
            Answer(c, (c.d[1] & 0xFFFF) >= Word(c, kGap));
            return;
        }
        if (!Word(c, kCount)) {
            Answer(c, true);
            return;
        }
        SetWord(c, kCount, Word(c, kCount) - 1);
        c.flagX = false;
        Answer(c, false);
    }

    // (Vampire Savior 2's, 02CD38) Is the other's attack, the first of its animation's frames with one, a low one?
    bool LowAttack(M68K &c) {
        const uint32_t other = Other(c);
        c.a[2] = Ram32(other + kFrame);
        c.a[1] = Ram32(other + kAttackBoxes);
        for (;;) {
            SetLow8(c.d[3], Mem8(c, c.a[2] + kFrameAttack));
            if (c.d[3] & 0xFF) {
                SetLow16(c.d[3], (c.d[3] & 0xFF) << 5);
                c.a[1] += static_cast<uint32_t>(static_cast<int16_t>(c.d[3]));
                // (the low kinds of attack, by thirty-twos: 02 and 03, 38 and 39)
                uint8_t kind = Mem8(c, c.a[1] + kAttackKind);
                if (kind < 0x20) {
                    c.d[4] = 0xC;
                } else if ((kind -= 0x20) < 0x20) {
                    c.d[4] = 0x03000000;
                } else if ((kind -= 0x20) < 0x20) {
                    c.d[4] = 0;
                } else {
                    c.d[3] = 0;
                    c.flagX = false;
                    Answer(c, false);
                    return false;
                }
                SetLow8(c.d[3], kind);
                const bool yes = (c.d[4] >> (kind & 31)) & 1;
                c.flagN = c.flagV = c.flagC = c.flagX = false;
                c.flagZ = !yes;
                return yes;
            }
            SetLow16(c.d[3], Mem8(c, c.a[2] + 1) & 0xC0);
            if (c.d[3] & 0xFFFF) {
                c.d[3] = 0;
                return false;
            }
            c.a[2] += 0x18;
        }
    }

    // 02D50C. The guarding pose: crouching if the other is on the floor and guards low or crouches (or, in Vampire
    // Savior 2, attacks low)
    void GuardPose(M68K &c) {
        c.d[2] = 0;
        c.d[1] = 0x02000C00;
        c.d[0] = 0x13;
        const uint32_t other = Other(c);
        if (!Ram8(other + kAirborne) &&
            (Ram8(other + kLowGuard) || (family::kGuardsLowAttacks && LowAttack(c)) || Ram8(other + kCrouching))) {
            c.d[2] = 1;
            c.d[1] = 0x02000C04;
            c.d[0] = 0x16;
        }
        SetByte(c, kCrouching, c.d[2] & 0xFF);
        SetLong(c, kState, c.d[1]);
        Call(c, kSetAnimation);
    }

    // 02D544. Whether a guard has a move after it (kGuardsWithMove), by the character and level
    void SetGuardsWithMove(M68K &c) {
        SetByte(c, kGuardsWithMove, 0);
        c.d[0] = static_cast<uint32_t>(Byte(c, kCharacter)) << 5;
        c.flagX = false;
        c.a[0] = kGuardsWithMoveTable + c.d[0];
        c.d[0] = Byte(c, kLevel);
        const uint8_t value = Mem8(c, c.a[0] + c.d[0]);
        SetByte(c, kGuardsWithMove, value);
        c.flagN = (value & 0x80) != 0;
        c.flagZ = value == 0;
        c.flagV = c.flagC = false;
    }

    // 02D568. One character's (7) jump in a dash at the edge it is going towards, high enough
    void AtEdgeJump(M68K &c) {
        if (!(Byte(c, kMoveWhen) & 1) || Byte(c, kCharacter) != 7) {
            Answer(c, false);
            return;
        }
        SetLow16(c.d[0], Add16(c, Word(c, kFloor), 0x20));
        if ((c.d[0] & 0xFFFF) >= Word(c, kY)) {
            Answer(c, false);
            return;
        }
        SetLow8(c.d[1], Byte(c, kGoing));
        if (!(c.d[1] & 0xFF)) {
            Answer(c, false);
            return;
        }
        SetLow8(c.d[0], Byte(c, kAtEdge));
        const uint16_t speed = Word(c, kSpeedX);
        if (!(c.d[0] & 0xFF) || !speed) {
            Answer(c, false);
            return;
        }
        Answer(c, (c.d[0] & 0xFF) == ((speed & 0x8000) ? 1 : 2));
    }

    // 02D5B2. Guard anyway (once), by a chance, if an attack is coming
    void GuardAnyway(M68K &c) {
        if (Byte(c, kGuardedAnyway)) {
            Answer(c, false);
            return;
        }
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x1F);
        if (!Chance(c, kGuardAnywayChances) || !AttackComing(c)) {
            Answer(c, false);
            return;
        }
        SetByte(c, kGuardedAnyway, 1);
        c.flagN = c.flagZ = c.flagV = c.flagC = false;
    }

    // 02D5E6. A chain's next move, if one may follow (FF ends the list)
    void NextChainMove(M68K &c) {
        if (!Byte(c, kCanChain)) {
            Answer(c, false);
            return;
        }
        const int8_t chain = static_cast<int8_t>(Byte(c, kChain));
        if (chain < 0) {
            NextOfChain(c, c.a[0]);
            if (!(c.d[1] & 0x80)) {
                UnpackChainMove(c);
                SetByte(c, kCanChain, 0);
                Answer(c, true);
                return;
            }
        }
        SetByte(c, kChain, 0);
        Answer(c, false);
    }
} // namespace

void RegisterComputer() {
    // (the last number is the time a call is charged: about the game's own on average, as the check measured it;
    // a frame's play takes from under 300 cycles to some thousands, when a script is chosen and many instructions
    // are done at once)
    AddRewrite(kComputerPlays, "ComputerPlays", ComputerPlays, kFreeD0 | kFreeD1 | kFreeFlags, 520);
    AddRewrite(kAnswerNow, "AnswerNow", AnswerNow, kFreeD0 | kFreeD1 | kFreeFlags, 3400);
    AddRewrite(kBrokenOff, "BrokenOff", BrokenOff, kFreeD0 | kFreeD1 | kFreeFlags, 230);
    AddRewrite(kReactions, "Reactions", Reactions, 0, 150);
    AddRewrite(kUnpackMove, "UnpackMove", UnpackMove);
    AddRewrite(kUnpackChainMove, "UnpackChainMove", UnpackChainMove, 0, 118);
    AddRewrite(kChance, "Chance", [](M68K &c) { Chance(c); }, 0, 78);
    AddRewrite(kAttackComing, "AttackComing", [](M68K &c) { AttackComing(c); }, 0, 1200);
    AddRewrite(kJumpMoveNow, "JumpMoveNow", JumpMoveNow, 0, 170);
    AddRewrite(kSecondMoveNow, "SecondMoveNow", SecondMoveNow, 0, 30);
    AddRewrite(kGuardPose, "GuardPose", GuardPose, 0, 200);
    AddRewrite(kSetGuardsWithMove, "SetGuardsWithMove", SetGuardsWithMove, 0, 110);
    AddRewrite(kAtEdgeJump, "AtEdgeJump", AtEdgeJump);
    AddRewrite(kGuardAnyway, "GuardAnyway", GuardAnyway, 0, 500);
    AddRewrite(kNextChainMove, "NextChainMove", NextChainMove, 0, 40);
}

} // namespace cps2recomp
