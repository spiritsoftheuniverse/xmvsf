// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the computer player, written again as C++ (rewrites: lift.h). It is Alpha 2's design, and
// Vampire Savior's before it (games/sfa2/src/computer.cpp; PROGRESS.md, "The computer player: scripts"): the computer
// plays a fighter by running scripts from the ROM, a set of them for each character (tools/aiscripts.py sfa prints
// them). A script is a list of instructions, each a word (an operation in the high byte, its kind in the low) and the
// words it takes. An instruction that does something with the fighter (a wait, a walk, a jump, an attack) is started,
// and then looked at every frame until it is over; the ones that decide (an if, a go to, a change of what may break
// in) are done at once, and the next one is taken.
//
// There are four banks of scripts, each with a table of them by character:
//   0  plans: chosen by the record's kPlan
//   2  tactics: by the plan, one of 32 slots at random (of three tiers of 32)
//   4  reactions: by the other's attack (its animation frame's kind; a projectile of the other's out, the second),
//      one of 32 slots at random; started when the other attacks (and a chance allows it)
//   6  answers: the one the game chose (kAnswer; Reactions below)
// Each bank keeps where its script has got to, so a plan broken off by a reaction goes on where it was.
//
// Alpha 1's is the first of the Alphas', and has less than Alpha 2's: one if, fewer waits and tests, a jump of one
// kind. Its think time is a character's own by level, and its tables of the characters' routines are read through
// the program counter (deciphered: CodeLong).
//
// The computer never reads the other player's controls: it sees the other's attack the frame it is out, and its
// projectiles, not the buttons.
#include "sfa.h"

#include <array>

namespace cps2recomp {

namespace {
    using namespace sfa;

    // The routines here, and the game's that they use
    constexpr uint32_t kChance = 0x0227F6, kComputerThinks = 0x022810, kBrokenOff = 0x02348A;
    constexpr uint32_t kComputerReactions = 0x023B78, kAnswerNow = 0x023CC6;
    constexpr uint32_t kRandom = 0x016462;        // a made-up byte in D0
    constexpr uint32_t kAttackComing = 0x0226B2;  // is an attack of the other side's coming? (in the flags)
    constexpr uint32_t kAttackAnswer = 0x02191A;  // a normal at the other's attack, by which of its boxes reach (kStance...)

    // The scripts' tables, by character (a long each): the four banks'. And, read through the program counter, the
    // characters' own tests of whether a special can be done now (a routine each) and their think times (a table each)
    constexpr uint32_t kPlans = 0x023E2A, kTactics = 0x023E5E, kReactionScripts = 0x023E92, kAnswers = 0x023EC6;
    constexpr uint32_t kCanDoSpecial = 0x023A3E, kThinkByCharacter = 0x0239FC;
    // A think time's part at random: one of 16 (signed words)
    constexpr uint32_t kThinkRandom = 0x023EFA;
    // Chance tables: a long for each level, of which a random bit says yes or no (Chance). kCharacterChances by
    // character (128 bytes each): whether to break in; its first character's also whether to Alpha counter. For
    // everyone: the others.
    constexpr uint32_t kPlanOrTactic = 0x11FB18, kBrokenOffChances = 0x11FB98, kGoToChances = 0x120498;
    constexpr uint32_t kCharacterChances = 0x11FC18;
    // Projectiles by a class (bytes): 16 for each kind of projectile, by its step (tests 30 and 32)
    constexpr uint32_t kProjectileClasses = 0x0231C4;

    constexpr int32_t kNoComputer = 0x2E0E;    // from A5, a byte: not zero: the computer does nothing

    // The computer's part of its player's record (from A6): Alpha 2's fields
    constexpr uint32_t kChosen = 0x200;        // byte: 0 until the bank's script is chosen, then 2
    constexpr uint32_t kStarted = 0x201;       // byte: 0 until the next instruction is started, then 2
    constexpr uint32_t kWaiting = 0x202;       // word: what the instruction started waits for (Going); 0 when over
    constexpr uint32_t kBank = 0x204;          // byte: 0 plans, 2 tactics, 4 reactions, 6 answers
    constexpr uint32_t kPlan = 0x205;          // byte: which plan (an offset in the character's table)
    constexpr uint32_t kIfBrokenOff = 0x206;   // word: what the script does when broken off: 0 nothing, 2 back to the
                                               // plan, 4 one of the two by a chance (BrokenOff)
    constexpr uint32_t kOperation = 0x208, kKind = 0x209; // bytes: the instruction (its word)
    constexpr uint32_t kOperands = 0x20A;      // words: an instruction's words, kept (up to three)
    constexpr uint32_t kMoveWhen = 0x211;      // byte: a jump's way word's low byte: when its move comes
    constexpr uint32_t kHeld1 = 0x212, kHeld2 = 0x213; // bytes: a move word's bits 1 and 2 (a command normal's: a throw's)
    constexpr uint32_t kWay = 0x214;           // byte: a walk's 0 forward, 1 back; a jump's 1 forward, 0 straight up, FF back
    constexpr uint32_t kChainable = 0x215;     // byte: a normal of kind 02 (it goes on while this is set)
    constexpr uint32_t kCount = 0x216;         // word: frames left (a wait, a walk)
    constexpr uint32_t kGap = 0x218;           // word: the gap between the two players
    constexpr uint32_t kTier = 0x21A;          // byte: 0, 20h or 40h: which 32 of a table's slots
    constexpr uint32_t kAlphaCounter = 0x21B;  // byte: a guard may Alpha counter
    constexpr uint32_t kKeptIfBrokenOff = 0x21C; // word: the plan's kIfBrokenOff while a tactic is gone to
    constexpr uint32_t kDepth = 0x21E;         // byte: how many ifs the interpreter is inside
    constexpr uint32_t kPlaces = 0x220;        // longs: where each bank's script has got to, by kBank (0x220 + kBank * 2)
    constexpr uint32_t kPlace = 0x230;         // long: where the interpreter is
    constexpr uint32_t kPlanStart = 0x234;     // long
    constexpr uint32_t kAnswer = 0x23C;        // byte: the answer chosen (bank 6's script)
    constexpr uint32_t kMayBreakIn = 0x242;    // word: the answers that may break in (a bit each; Reactions)
    constexpr uint32_t kSlot = 0x247;          // byte: the slot the script was chosen from
    constexpr uint32_t kUnbreakable = 0x24B;   // byte: not zero: nothing breaks in
    constexpr uint32_t kMayAlphaCounter = 0x24F; // byte: not zero: a guard of kind 02 may Alpha counter
    constexpr uint32_t kLevel = 0xC7;          // byte: how well the computer plays, 0-31

    // The rest of a player's record that the computer reads or sets
    constexpr uint32_t kWalkingBack = 0x2E;    // byte: 1 walking back, 0 forward (a jump's: 1 forward, 0 straight up, FF back)
    constexpr uint32_t kFloor = 0x54;          // word: the height of the floor under the player
    constexpr uint32_t kJuggle = 0x56;         // bytes, two: the hits taken in the air (cleared out of a reeling)
    constexpr uint32_t kCanChain = 0x58;       // byte (cleared as an attack starts)
    constexpr uint32_t kReelKind = 0x44;       // byte
    constexpr uint32_t kReeling = 0xB5;        // byte: 2 for a special started from a reeling (kState 02020400)
    constexpr uint32_t kStance = 0x100, kButton = 0x101, kStrength = 0x102; // bytes: a normal (0, 2, 4)
    constexpr uint32_t kProjectile = 0x124;    // word: the player's projectile's record (FFxxxx), 0 none
    constexpr uint32_t kCrouching = 0x130;     // byte
    constexpr uint32_t kFaceTo = 0x131;        // byte: the way the player should face (to kFacesRight)
    constexpr uint32_t kAttackOut = 0x132;     // byte: an attack of the player's is out
    constexpr uint32_t kMoveNumber = 0x133;    // byte: which special move
    constexpr uint32_t kDizzyAfter = 0x13B;    // byte: stunned once it is up
    constexpr uint32_t kAtEdge = 0x13C;        // byte: 1 at the picture's left edge, 2 at its right
    constexpr uint32_t kReelTimers = 0x160;    // long: the reeling's (cleared out of a reeling)
    constexpr uint32_t kProjectileOut = 0x238; // byte: Charlie's projectile out (test 22)
    constexpr uint32_t kNoThrow = 0x241;       // byte: steps in which the player cannot be thrown: 5 out of a reeling
    constexpr uint32_t kThrownAttack = 0x17;   // in an animation frame, a byte: below zero, not an attack to answer
    constexpr uint32_t kHurt = 0x08;           // in an animation frame, a long: the boxes it can be hit in (high three bytes)
    constexpr uint32_t kProjectileState = 0x04, kProjectileKind = 0x02, kProjectileStep = 0x2E; // bytes of a projectile's
    constexpr uint8_t kCharlie = 3;            // (test 22: his projectile)
    constexpr uint8_t kActionStunned = 0x12;

    // The values of kState the computer sets: the player standing, crouching, walking, jumping, attacking...
    constexpr uint32_t kStand = 0x02000000, kCrouch = 0x02000200, kStandUp = 0x02000204, kWalk = 0x02000400;
    constexpr uint32_t kJump = 0x0200060E, kNormal = 0x02000A00, kSpecialMove = 0x02000E00, kSuperCombo = 0x02001000;
    constexpr uint32_t kGuard = 0x02000C08, kReelingState = 0x02020400, kLyingDown = 0x02020202;

    uint8_t Mem8(M68K &c, uint32_t address) {
        return static_cast<uint8_t>(c.Read(address, 0));
    }
    uint16_t Mem16(M68K &c, uint32_t address) {
        return static_cast<uint16_t>(c.Read(address, 1));
    }
    uint32_t Mem32(M68K &c, uint32_t address) {
        return c.Read(address, 2);
    }
    // The ROM as the program reads it where it names a table by its distance from the instruction (deciphered: not
    // what Mem32 sees at the same address)
    uint32_t CodeLong(M68K &c, uint32_t address) {
        return c.ReadProgram(address, 2);
    }
    uint32_t Var(M68K &c, int32_t offset) {
        return (c.a[5] + static_cast<uint32_t>(offset)) & 0xFFFFFF;
    }

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
    uint32_t SignWord(uint32_t value) {
        return static_cast<uint32_t>(static_cast<int16_t>(value));
    }
    uint32_t SignByte(uint32_t value) {
        return static_cast<uint32_t>(static_cast<int8_t>(value));
    }
    // The other player's record (in A4, as the game has it)
    uint32_t Other(M68K &c) {
        c.a[4] = SignWord(Word(c, kOpponent));
        return c.a[4] & 0xFFFFFF;
    }
    // The other's projectile, if it has one (in A4, the record; D0 its word): 0 if not
    uint32_t OthersProjectile(M68K &c) {
        const uint32_t other = Other(c);
        SetLow16(c.d[0], Ram16(other + kProjectile));
        if (!(c.d[0] & 0xFFFF)) {
            return 0;
        }
        c.a[4] = SignWord(c.d[0]);
        return c.a[4] & 0xFFFFFF;
    }
    bool InTheAir(uint32_t player) {
        return Ram8(player + kInAir) == 1;
    }

    // The next word of the script
    uint16_t Operand(M68K &c) {
        const uint16_t word = Mem16(c, c.a[0]);
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
    // A routine of the game's whose answer is in the Z flag
    bool Asks(M68K &c, uint32_t routine) {
        Call(c, routine);
        return !c.flagZ;
    }

    // 0227F6. A chance: one of the 32 bits of the table at A1's long for the computer's level, at random
    bool Chance(M68K &c) {
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x1F);
        SetLow8(c.d[1], Byte(c, kLevel));
        const uint16_t level = static_cast<uint16_t>(SignByte(c.d[1]));
        SetLow16(c.d[1], static_cast<uint16_t>(level << 2));
        c.flagX = (level >> 14) & 1;
        c.d[1] = Mem32(c, c.a[1] + SignWord(c.d[1]));
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

    // 023A72. A move (the word in D1) unpacked into what a player's buttons would give: bits 1 to 3 held (a command
    // normal's; bit 3 is also the way); then bit 0 clear, a normal (its stance, punch or kick, strength, a nibble
    // each); set, a special (its number, and punch or kick and strength)
    void UnpackMove(M68K &c) {
        uint16_t move = c.d[1] & 0xFFFF;
        SetByte(c, kHeld1, (move >> 1) & 1);
        SetByte(c, kHeld2, (move >> 2) & 1);
        SetByte(c, kWay, (move >> 3) & 1);
        uint8_t last;
        if (!(move & 1)) {
            move >>= 4;
            SetByte(c, kStance, move & 0xF);
            move >>= 4;
            SetByte(c, kButton, move & 0xF);
            c.flagX = (move >> 3) & 1;
            move >>= 4;
            last = move & 0xF;
            SetByte(c, kStrength, last);
        } else {
            move >>= 3;
            SetByte(c, kMoveNumber, move & 0x1E);
            move >>= 5;
            SetByte(c, kButton, move & 0xF);
            c.flagX = (move >> 3) & 1;
            move >>= 4;
            last = move & 0xF;
            SetByte(c, kStrength, last);
        }
        SetLow16(c.d[1], move);
        // (the flags as the routine's last instruction leaves them)
        c.flagN = false;
        c.flagZ = last == 0;
        c.flagV = c.flagC = false;
    }
    void UnpackMove(M68K &c, uint16_t move) {
        SetLow16(c.d[1], move);
        UnpackMove(c);
    }

    // 023A30. Can the character do the special unpacked now? (its own routine says, in the flags)
    bool CanDoSpecial(M68K &c) {
        c.d[0] = static_cast<uint8_t>(Byte(c, kCharacter) << 2);
        c.a[1] = CodeLong(c, kCanDoSpecial + c.d[0]);
        Call(c, c.a[1]);
        return !c.flagZ;
    }

    // 023DFE. Is the other open: on the floor, in its own control, and in an animation that can be hit?
    bool OpenFrom(M68K &c, uint32_t other) {
        if (Ram16(other + kState) != 0x0200 || Ram8(other + kInAir)) {
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
    // 023DF2. ... and not just out of a reeling (kNoThrow)
    bool OpenNotJustUp(M68K &c) {
        const uint32_t other = Other(c);
        if (Ram8(other + kNoThrow)) {
            Answer(c, false);
            return false;
        }
        return OpenFrom(c, other);
    }

    // 0232C4. How far across the projectile A4 (its frame at A1) is from the computer: from its attack box's near
    // side (D1; a box's bytes, as Alpha 1 keeps them)
    uint16_t ProjectileGap(M68K &c) {
        c.d[0] = static_cast<uint32_t>(Mem8(c, c.a[1] + kFrameAttack)) << 4;
        const uint32_t projectile = c.a[4] & 0xFFFFFF;
        c.a[1] = Ram32(projectile + kThrownTables);
        SetLow16(c.d[1], Mem16(c, c.a[1] + 2));
        c.a[1] += SignWord(c.d[1]);
        c.a[1] += SignWord(c.d[0]);
        uint16_t edge = static_cast<uint16_t>(-static_cast<int16_t>(static_cast<int8_t>(Mem8(c, c.a[1] + 2))));
        uint16_t across = static_cast<uint16_t>(static_cast<int8_t>(Mem8(c, c.a[1])));
        if (Ram8(projectile + kFacesRight)) {
            across = static_cast<uint16_t>(-across);
            edge = static_cast<uint16_t>(-edge);
        }
        across = static_cast<uint16_t>(across + edge + Ram16(projectile + kX));
        SetLow16(c.d[0], across);
        uint16_t gap = static_cast<uint16_t>(Word(c, kX) - across);
        if (gap & 0x8000) {
            gap = static_cast<uint16_t>(-gap);
        }
        SetLow16(c.d[1], gap);
        return gap;
    }
    // The class of the other's projectile (tests 30 and 32; A4 the projectile)
    uint8_t ProjectileClass(M68K &c) {
        const uint32_t projectile = c.a[4] & 0xFFFFFF;
        c.d[0] = static_cast<uint32_t>(Ram8(projectile + kProjectileKind)) << 4;
        c.a[1] = kProjectileClasses + c.d[0];
        c.d[0] = Ram8(projectile + kProjectileStep) >> 1;
        SetLow8(c.d[0], Mem8(c, c.a[1] + c.d[0]));
        return c.d[0] & 0xFF;
    }

    // --- choosing a script ---

    // A script's table: the character's (D0 the character by four), then the entry at `offset` (a signed word
    // offset from the table)
    void CharacterTable(M68K &c, uint32_t tables) {
        c.a[0] = tables;
        SetLow8(c.d[0], Byte(c, kCharacter));
        SetLow16(c.d[0], static_cast<uint16_t>(SignByte(c.d[0]) << 2));
        c.a[0] = Mem32(c, c.a[0] + SignWord(c.d[0]));
    }
    void Entry(M68K &c) {
        SetLow16(c.d[1], Mem16(c, c.a[0] + SignWord(c.d[0])));
        c.a[0] += SignWord(c.d[1]);
    }
    // A script begins with what it does when broken off; A0 is left at its first instruction
    void Begin(M68K &c, uint32_t place) {
        SetWord(c, kIfBrokenOff, Operand(c));
        SetLong(c, place, c.a[0]);
        SetByte(c, kDepth, 0);
    }
    // One of 32 slots at random (of the tier's), naming a script of the table at A0 (its offsets after the slots)
    void FromSlots(M68K &c) {
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x1F);
        SetLow8(c.d[0], c.d[0] + Byte(c, kTier));
        SetLow8(c.d[1], Mem8(c, c.a[0] + SignWord(c.d[0])));
        SetByte(c, kSlot, c.d[1] & 0xFF);
        SetLow16(c.d[1], static_cast<uint16_t>(SignByte(c.d[1]) * 2 + 0x60));
        SetLow16(c.d[1], Mem16(c, c.a[0] + SignWord(c.d[1])));
        c.a[0] += SignWord(c.d[1]);
    }

    // 02389E. The plan kPlan
    void ChoosePlan(M68K &c) {
        SetByte(c, kSlot, 0);
        SetByte(c, kUnbreakable, 0);
        CharacterTable(c, kPlans);
        SetLow8(c.d[0], Byte(c, kPlan));
        Entry(c);
        SetWord(c, kIfBrokenOff, Operand(c));
        SetLong(c, kPlaces + 0, c.a[0]);
        SetLong(c, kPlanStart, c.a[0]);
        SetByte(c, kDepth, 0);
    }
    // 0238D4. A tactic of the plan's, at random
    void ChooseTactic(M68K &c) {
        SetByte(c, kUnbreakable, 0);
        CharacterTable(c, kTactics);
        SetLow8(c.d[0], Byte(c, kPlan));
        Entry(c);
        FromSlots(c);
        Begin(c, kPlaces + 4);
    }
    // 023928. A reaction to the other's attack: the table by the kind of its frame (by a projectile of its, if it has
    // one), a slot at random
    void ChooseReaction(M68K &c) {
        SetByte(c, kUnbreakable, 0);
        CharacterTable(c, kReactionScripts);
        const uint32_t other = Other(c);
        c.d[0] = 2;
        if (!Ram16(other + kProjectile)) {
            c.a[4] = Ram32(other + kFrame);
            SetLow8(c.d[0], Mem8(c, c.a[4] + kThrownAttack));
            SetLow16(c.d[0], static_cast<uint16_t>(SignByte(c.d[0]) * 2));
        }
        Entry(c);
        FromSlots(c);
        Begin(c, kPlaces + 8);
    }
    // 023990. The answer kAnswer
    void ChooseAnswer(M68K &c) {
        SetByte(c, kUnbreakable, 0);
        CharacterTable(c, kAnswers);
        c.d[0] = Byte(c, kAnswer);
        SetByte(c, kSlot, c.d[0] & 0xFF);
        c.d[0] *= 2;
        Entry(c);
        Begin(c, kPlaces + 12);
    }

    // 0239C6. A think time to kCount: the character's own for the level, and one of 16 at random (not below 0)
    void Think(M68K &c) {
        c.d[0] = static_cast<uint8_t>(Byte(c, kCharacter) << 2);
        c.a[1] = CodeLong(c, kThinkByCharacter + c.d[0]);
        SetLow8(c.d[0], Byte(c, kLevel) * 2);
        SetWord(c, kCount, Mem16(c, c.a[1] + SignWord(c.d[0])));
        Random(c);
        SetLow16(c.d[0], c.d[0] & 0x1E);
        c.a[1] = kThinkRandom;
        SetLow16(c.d[0], Mem16(c, c.a[1] + SignWord(c.d[0])));
        SetWord(c, kCount, Word(c, kCount) + (c.d[0] & 0xFFFF));
        if (Word(c, kCount) & 0x8000) {
            SetWord(c, kCount, 0);
        }
    }

    // --- the instructions ---

    // The words each instruction takes after its own, by operation (and kind): what the skipping steps over
    constexpr uint8_t kWaitWords[17] = {1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 2, 2, 0};
    constexpr uint8_t kWalkWords[8] = {1, 1, 1, 1, 1, 2, 2, 2};
    // and the if's tests (by the test's number, in twos)
    constexpr uint8_t kTestWords[27] = {1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1};
    enum Operation : uint8_t {
        kWaits = 0x00, kWalks = 0x02, kJumps = 0x04, kNormalMove = 0x06, kSpecialMoves = 0x08, kSuperCombos = 0x0A,
        kIf = 0x0C, kGoToTactic = 0x0E, kGuards = 0x10, kBreakIn = 0x12, kEnd = 0xFF,
    };
    // The if: 00 if (and a test), 02 else, 04 endif
    enum IfKind : uint8_t { kIfTest = 0, kElse = 2, kEndIf = 4 };

    // 023CD4. Steps over instructions to the if's else or endif, at the depth in D6 (as the game's, the words stepped
    // over each go through D1: the last one is left there)
    void Skip(M68K &c) {
        for (;;) {
            c.d[0] = Mem8(c, c.a[0]);
            c.a[0] += 1;
            const uint8_t operation = c.d[0] & 0xFF;
            SetLow8(c.d[0], Mem8(c, c.a[0]));
            c.a[0] += 1;
            const uint8_t kind = c.d[0] & 0xFF;
            // (no script ends inside an if: aiscripts.py --check)
            uint32_t words = 0;
            switch (operation) {
            case kWaits: words = kWaitWords[kind / 2]; break;
            case kWalks: words = kWalkWords[kind / 2]; break;
            case kJumps: words = 3; break;
            case kNormalMove: case kSpecialMoves: case kSuperCombos: words = 1; break;
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
            default: break; // kGoToTactic, kGuards
            }
            for (uint32_t n = 0; n < words; n++) {
                SetLow16(c.d[1], Operand(c));
            }
        }
    }

    // 022B9E. A wait begins: the player stands (if it is not already)
    bool Stands(M68K &c) {
        c.d[1] = Ram32(Me(c) + kState) & 0xFFFFFF00;
        if (c.d[1] != kStand) {
            SetLong(c, kState, kStand);
        }
        return false;
    }
    bool Starts(M68K &c, uint16_t waiting, uint32_t state) {
        SetWord(c, kWaiting, waiting);
        SetLong(c, kState, state);
        return false;
    }
    // A normal attack started (its move unpacked already: kStance...)
    void StartsNormal(M68K &c, uint16_t waiting) {
        Starts(c, waiting, kNormal);
        SetByte(c, kAttackOut, 1);
        SetByte(c, kChainable, 0);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
    }

    // Each instruction: true when it was done at once (and the next is to be taken), false when it has started
    // something that goes on (Going looks at it each frame)

    // 02296A. Operation 00: waits, and standing or crouching
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
            SetLow16(c.d[0], Operand(c));
            SetWord(c, kOperands, c.d[0] & 0xFFFF);
            if (!InTheAir(other)) {
                return true;
            }
            SetLow16(c.d[0], (c.d[0] & 0xFFFF) + Ram16(other + kFloor));
            if ((c.d[0] & 0xFFFF) >= Ram16(other + kY) && (Ram16(other + kSpeedY) & 0x8000)) {
                return true;
            }
            return Starts(c, 0x0008, kStand);
        }
        case 0x0A: // a normal at the other's attack, if it is attacking
            if (!Asks(c, kAttackAnswer)) {
                return true;
            }
            StartsNormal(c, 0x0600);
            return false;
        case 0x0C: // stand so many frames, or a normal at the other's attack as soon as it attacks
            Starts(c, 0x000C, kStand);
            SetWord(c, kCount, Operand(c));
            if (Asks(c, kAttackAnswer)) {
                StartsNormal(c, 0x0600);
            }
            return false;
        case 0x0E: { // while the other's projectile is out, farther than a gap
            Other(c);
            SetWord(c, kOperands, Operand(c));
            other = c.a[4] & 0xFFFFFF;
            SetLow16(c.d[1], Ram16(other + kProjectile));
            if (!(c.d[1] & 0xFFFF)) {
                return true;
            }
            c.a[4] = SignWord(c.d[1]);
            if (Ram8((c.a[4] & 0xFFFFFF) + kProjectileState) != 2) {
                return true;
            }
            c.a[1] = Ram32((c.a[4] & 0xFFFFFF) + kFrame);
            const uint16_t gap = ProjectileGap(c);
            SetLow16(c.d[0], Word(c, kOperands));
            if (gap <= Word(c, kOperands)) {
                return true;
            }
            return Starts(c, 0x000E, kStand);
        }
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
        case 0x12: // while the computer's projectile is out
            if (!Word(c, kProjectile)) {
                return true;
            }
            SetWord(c, kWaiting, 0x0012);
            return Stands(c);
        case 0x14: // until the other is open
            if (Open(c)) {
                return true;
            }
            SetWord(c, kWaiting, 0x0014);
            return Stands(c);
        case 0x18: // a frame
            SetWord(c, kCount, 1);
            SetWord(c, kWaiting, 0x0010);
            return Stands(c);
        case 0x1A: // so many frames, not to be broken in on
            SetWord(c, kCount, Operand(c));
            SetWord(c, kWaiting, 0x0010);
            return Stands(c);
        case 0x20: // until the other is open (and not just up)
            if (OpenNotJustUp(c)) {
                return true;
            }
            SetWord(c, kWaiting, 0x001A);
            return Stands(c);
        default: { // 0x1C, 0x1E: for so many frames while farther than a gap (1C), or nearer (1E)
            SetLow16(c.d[1], Operand(c));
            SetWord(c, kOperands, c.d[1] & 0xFFFF);
            SetWord(c, kCount, Operand(c));
            SetLow16(c.d[1], (c.d[1] & 0xFFFF) - Word(c, kGap));
            const bool farther = (c.d[1] & 0x8000) != 0;
            if (kind == 0x1C ? !farther : farther) {
                return true;
            }
            return Starts(c, kind == 0x1C ? 0x0016 : 0x0018, kStand);
        }
        }
    }

    // 022BBA. Operation 02: walks: forward or back to a gap (00, 02), to a gap either way (04), for so many
    // frames (06, 08), and for so many frames or to a gap (0A, 0C, 0E)
    bool Walk(M68K &c, uint8_t kind) {
        SetLow16(c.d[1], Operand(c));
        if (kind == 0x06 || kind == 0x08) {
            SetWord(c, kCount, c.d[1] & 0xFFFF);
        } else {
            if (kind >= 0x0A) {
                SetWord(c, kCount, Operand(c));
            }
            SetWord(c, kOperands, c.d[1] & 0xFFFF);
            SetLow16(c.d[1], (c.d[1] & 0xFFFF) - Word(c, kGap));
        }
        const int16_t beyond = static_cast<int16_t>(c.d[1]);
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
            SetLow16(c.d[1], (c.d[1] & 0xFFFF) + 3);
            if ((c.d[1] & 0xFFFF) < 6) {
                return true;
            }
            break;
        case 0x08:
            way = 1;
            break;
        }
        SetWord(c, kWaiting, static_cast<uint16_t>(0x0200 | kind));
        SetLong(c, kState, kWalk);
        SetByte(c, kWalkingBack, way);
        SetByte(c, kWay, way);
        if (kind == 0x04 || kind == 0x0E) {
            SetLow16(c.d[1], Word(c, kOperands));
            if (Word(c, kOperands) >= Word(c, kGap)) {
                SetByte(c, kWalkingBack, 1);
                SetByte(c, kWay, 1);
            }
        }
        return false;
    }

    // 022D5C. Operation 04: a jump with a move: the move, the way word (the way in its high byte, and when the move
    // comes in its low: 4, after so many frames), the frames
    bool Jump(M68K &c) {
        Starts(c, 0x0400, kJump);
        for (int n = 0; n < 3; n++) {
            SetWord(c, kOperands + 2u * n, Operand(c));
        }
        UnpackMove(c, Word(c, kOperands));
        const uint16_t way = Word(c, kOperands + 2);
        SetByte(c, kWay, way >> 8);
        SetByte(c, kMoveWhen, way & 0xFF);
        SetLow16(c.d[0], way & 0xFE);
        if ((c.d[0] & 0xFFFF) == 4) {
            SetWord(c, kCount, Word(c, kOperands + 4));
        }
        return false;
    }

    // 022DAE. Operation 06: a normal (kind 02: it goes on while kChainable)
    bool NormalMove(M68K &c, uint8_t kind) {
        Starts(c, kind ? 0x0602 : 0x0600, kNormal);
        SetByte(c, kAttackOut, 1);
        SetByte(c, kChainable, kind ? 1 : 0);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
        SetByte(c, kCanChain, 0);
        SetLow16(c.d[1], Operand(c));
        SetWord(c, kOperands, c.d[1] & 0xFFFF);
        UnpackMove(c);
        return false;
    }

    // 022E18, 022E6C. Operations 08 and 0A: a special, or a super combo, if the character can do it now (from a
    // reeling, the reeling's own mark)
    bool SpecialMove(M68K &c, uint16_t waiting, uint32_t state) {
        SetLow16(c.d[1], Operand(c));
        SetWord(c, kOperands, c.d[1] & 0xFFFF);
        UnpackMove(c);
        if (!CanDoSpecial(c)) {
            return true;
        }
        if (Ram32(Me(c) + kState) == kReelingState) {
            SetByte(c, kReeling, 2);
        }
        Starts(c, waiting, state);
        SetByte(c, kAttackOut, 1);
        SetByte(c, kFacesRight, Byte(c, kFaceTo));
        SetByte(c, kCanChain, 0);
        SetByte(c, kCrouching, 0);
        return false;
    }

    // 022ED2. The if's test (the word in D0, read): true if it passes (the instructions after it are run), false if not
    // (they are stepped over)
    bool Test(M68K &c) {
        const uint16_t test = c.d[0] & 0xFFFF;
        uint32_t other;
        switch (test) {
        case 0x00: // the gap at most
            SetWord(c, kOperands + 2, Operand(c));
            SetLow16(c.d[1], Word(c, kGap));
            return Word(c, kGap) <= Word(c, kOperands + 2);
        case 0x02: // the gap at least
            SetWord(c, kOperands + 2, Operand(c));
            SetLow16(c.d[1], Word(c, kGap));
            return Word(c, kGap) >= Word(c, kOperands + 2);
        case 0x04: // one in two
            Random(c);
            SetLow16(c.d[0], c.d[0] & 1);
            return (c.d[0] & 0xFFFF) == 0;
        case 0x06: // the super combo gauge at least
            SetLow16(c.d[1], Operand(c));
            return (c.d[1] & 0xFFFF) <= Word(c, kSpecialGauge);
        case 0x08: // the other in the air
            return InTheAir(Other(c));
        case 0x0A: // the other crouching
            return Ram8(Other(c) + kCrouching) != 0;
        case 0x0C: // the other knocked down
            return Ram8(Other(c) + kKnocksDown) != 0;
        case 0x0E: { // the other's projectile out
            const uint32_t projectile = OthersProjectile(c);
            return projectile && Ram8(projectile + kProjectileState) == 2;
        }
        case 0x10: case 0x12: { // the other's projectile nearer than a gap (10; false with none), or not (12)
            SetWord(c, kOperands + 2, Operand(c));
            const uint32_t projectile = OthersProjectile(c);
            if (!projectile || !Ram8(projectile + kProjectileState)) {
                return test == 0x12;
            }
            c.a[1] = Ram32(projectile + kFrame);
            const uint16_t gap = ProjectileGap(c);
            SetLow16(c.d[0], Word(c, kOperands + 2));
            return test == 0x10 ? gap <= Word(c, kOperands + 2) : gap >= Word(c, kOperands + 2);
        }
        case 0x14: // an attack coming
            return Asks(c, kAttackComing);
        case 0x16: case 0x18: case 0x1A: { // the other jumping forward, straight up, back (its +2E 1, 0, FF)
            other = Other(c);
            const uint8_t way = test == 0x16 ? 1 : test == 0x18 ? 0 : 0xFF;
            return InTheAir(other) && Ram8(other + kWalkingBack) == way;
        }
        case 0x1C: // the other open
            return Open(c);
        case 0x1E: case 0x20: // the level at most, at least
            SetWord(c, kOperands + 2, Operand(c));
            c.d[0] = Byte(c, kLevel);
            c.d[1] = Word(c, kOperands + 2) & 0x1F;
            return test == 0x1E ? c.d[0] <= c.d[1] : c.d[0] >= c.d[1];
        case 0x22: // none of the computer's projectiles out (Charlie's: his own mark too)
            return !Word(c, kProjectile) && (Byte(c, kCharacter) != kCharlie || !Byte(c, kProjectileOut));
        case 0x24: // the other's health below
            SetLow16(c.d[1], Operand(c));
            return (c.d[1] & 0xFFFF) > Ram16(Other(c) + kHealth);
        case 0x26: // the computer's health above the other's
            SetLow16(c.d[1], Word(c, kHealth));
            return (c.d[1] & 0xFFFF) > Ram16(Other(c) + kHealth);
        case 0x28: // the other rising
            other = Other(c);
            return InTheAir(other) && !(Ram32(other + kSpeedY) & 0x80000000);
        case 0x2A: // the other not rising
            other = Other(c);
            return !InTheAir(other) || (Ram32(other + kSpeedY) & 0x80000000);
        case 0x2C: case 0x2E: { // the other below a height, above
            SetLow16(c.d[1], Operand(c));
            other = Other(c);
            SetLow16(c.d[1], (c.d[1] & 0xFFFF) + Ram16(other + kFloor));
            const uint16_t height = c.d[1] & 0xFFFF;
            return test == 0x2C ? height >= Ram16(other + kY) : height <= Ram16(other + kY);
        }
        case 0x30: { // no projectile of the other's out, or one of class 0 or 4
            if (!OthersProjectile(c)) {
                return true;
            }
            const uint8_t kind = ProjectileClass(c);
            return kind == 0 || kind == 4;
        }
        case 0x32: // the other's projectile out, of class 3 or more
            return OthersProjectile(c) && ProjectileClass(c) > 2;
        default: // 0x34: the computer's health below
            SetLow16(c.d[1], Operand(c));
            return (c.d[1] & 0xFFFF) > Word(c, kHealth);
        }
    }

    // 022EC0. Operation 0C: if (00, and a test), else (02), endif (04)
    bool If(M68K &c, uint8_t kind) {
        if (kind == kIfTest) {
            SetByte(c, kDepth, Byte(c, kDepth) + 1);
            SetLow16(c.d[0], Operand(c));
            SetWord(c, kOperands, c.d[0] & 0xFFFF);
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

    // 023318. Operation 0E: go to a tactic, and come back to the plan after it (02: by a chance)
    bool GoToTactic(M68K &c, uint8_t kind) {
        if (kind == 2 && !Chance(c, kGoToChances)) {
            return true;
        }
        SetWord(c, kKeptIfBrokenOff, Word(c, kIfBrokenOff));
        SetByte(c, kBank, 2);
        SetLong(c, kPlaces, c.a[0]);
        ChooseTactic(c);
        return true;
    }

    // 023352. Operation 10: guard, if an attack is coming (02: and it may Alpha counter, from the tiers above the
    // first, as the first character's chance says: the game's table for Ryu, whoever it is)
    bool Guard(M68K &c, uint8_t kind) {
        if (!Asks(c, kAttackComing)) {
            return true;
        }
        Starts(c, 0x0C00, kGuard);
        SetByte(c, kAlphaCounter, kind);
        if (!kind) {
            return false;
        }
        if (!Byte(c, kTier) || !Byte(c, kMayAlphaCounter) || !Chance(c, kCharacterChances)) {
            SetByte(c, kAlphaCounter, 0);
        }
        return false;
    }

    // 023392. Operation 12: above a level, let answers break in (00), or keep them out (02) (the bits of a word);
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

    void End(M68K &c);

    // 02294A. The instruction in kOperation, and the ones after it that are done at once, to one that goes on
    void Run(M68K &c) {
        for (;;) {
            const uint8_t kind = Byte(c, kKind);
            bool next;
            switch (Byte(c, kOperation)) {
            case kWaits: next = Wait(c, kind); break;
            case kWalks: next = Walk(c, kind); break;
            case kJumps: next = Jump(c); break;
            case kNormalMove: next = NormalMove(c, kind); break;
            case kSpecialMoves: next = SpecialMove(c, 0x0800, kSpecialMove); break;
            case kSuperCombos: next = SpecialMove(c, 0x0A00, kSuperCombo); break;
            case kIf: next = If(c, kind); break;
            case kGoToTactic: next = GoToTactic(c, kind); break;
            case kGuards: next = Guard(c, kind); break;
            default: next = LetBreakIn(c, kind); break; // kBreakIn
            }
            if (!next) {
                return;
            }
            // 0233DE. The next instruction
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
    // 023452. Back to the plan where it was (its own kIfBrokenOff again); at its end, the plan from its start: true if
    // the end is to be done
    bool BackToPlan(M68K &c) {
        SetByte(c, kBank, 0);
        SetWord(c, kIfBrokenOff, Word(c, kKeptIfBrokenOff));
        Resume(c, kPlaces + 0);
        if (Byte(c, kOperation) != kEnd) {
            return false;
        }
        ChoosePlan(c);
        return true;
    }

    // 0233FA. A script's end (FF): 00 a tactic at random; 02 back to the plan; 04 one of the two, by a chance
    void End(M68K &c) {
        for (;;) {
            uint8_t how = Byte(c, kKind);
            if (how == 4) {
                how = Chance(c, kPlanOrTactic) ? 2 : 0;
            }
            if (how == 0) { // 02340C
                if (!Byte(c, kBank)) {
                    ChoosePlan(c);
                }
                SetByte(c, kBank, 2);
                ChooseTactic(c);
                Resume(c, kPlaces + 4);
                return;
            }
            if (!Byte(c, kBank)) { // 02342E: the plan again from its start
                SetByte(c, kBank, 0);
                ChoosePlan(c);
                Resume(c, kPlaces + 0);
                if (Byte(c, kOperation) != kEnd) {
                    return;
                }
                continue;
            }
            if (!BackToPlan(c)) {
                return;
            }
        }
    }

    // 022916. The bank's next instruction started (and those done at once before it)
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

    // 023894. The instruction is over: the next is started next frame
    void Over(M68K &c) {
        SetByte(c, kStarted, 0);
        SetWord(c, kWaiting, 0);
    }

    // 023AF0. May a reaction break in? The other has a projectile out or an attack out (that is one to answer); the
    // computer stands free; the character's chance says so
    bool MayBreakIn(M68K &c) {
        if (Byte(c, kUnbreakable) || Byte(c, kAlphaCounter)) {
            Answer(c, false);
            return false;
        }
        const uint32_t other = Other(c);
        if (!Ram16(other + kProjectile) && !Ram8(other + kAttackOut)) {
            Answer(c, false);
            return false;
        }
        c.a[3] = Ram32(other + kFrame);
        if (Mem8(c, c.a[3] + kThrownAttack) & 0x80) {
            Answer(c, false);
            return false;
        }
        const uint8_t action = Byte(c, kAction);
        if (Word(c, kState) != 0x0200 || action == 0x0A || action == 0x0E || action == 0x10 || action == 0x12) {
            Answer(c, false);
            return false;
        }
        c.a[1] = kCharacterChances;
        SetLow8(c.d[0], Byte(c, kCharacter));
        SetLow16(c.d[0], static_cast<uint16_t>(SignByte(c.d[0]) << 7));
        c.a[1] += SignWord(c.d[0]);
        const bool yes = Chance(c);
        Answer(c, yes);
        return yes;
    }

    void ComputerThinks(M68K &c);

    // 023B6A. A reaction breaks in: bank 4 from its start
    void React(M68K &c) {
        SetByte(c, kBank, 4);
        SetLong(c, kChosen, 0);
        ComputerThinks(c);
    }

    // A count of frames down: true while some are left (as the game's, a count already at 0 goes round to FFFF)
    bool CountDown(M68K &c) {
        SetWord(c, kCount, Word(c, kCount) - 1);
        return Word(c, kCount) != 0;
    }

    // The waits of operation 00 (by the kind kWaiting names): true while it goes on
    bool Waiting(M68K &c, uint8_t kind) {
        uint32_t other;
        switch (kind) {
        case 0x00: case 0x10: // frames (0x10: not to be broken in on)
            return CountDown(c);
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
            return false;
        case 0x0C: // frames, or a normal at the other's attack
            if (!CountDown(c)) {
                return false;
            }
            if (Asks(c, kAttackAnswer)) {
                StartsNormal(c, 0x0600);
            }
            return true;
        case 0x0E: { // while the other's projectile is out farther than the gap
            other = Other(c);
            SetLow16(c.d[1], Ram16(other + kProjectile));
            if (!(c.d[1] & 0xFFFF)) {
                return false;
            }
            c.a[4] = SignWord(c.d[1]);
            c.a[1] = Ram32((c.a[4] & 0xFFFFFF) + kFrame);
            const uint16_t gap = ProjectileGap(c);
            SetLow16(c.d[0], Word(c, kOperands));
            return gap > Word(c, kOperands);
        }
        case 0x12:
            return Word(c, kProjectile) != 0;
        case 0x14:
            return !Open(c);
        case 0x16: case 0x18:
            if (!CountDown(c)) {
                return false;
            }
            SetLow16(c.d[0], Word(c, kOperands));
            return kind == 0x16 ? Word(c, kOperands) < Word(c, kGap) : Word(c, kOperands) >= Word(c, kGap);
        default: // 0x1A
            return !OpenNotJustUp(c);
        }
    }

    // The walks (kind as Walk's): true while it goes on
    bool Walking(M68K &c, uint8_t kind) {
        if (!Byte(c, kAction)) {
            return false;
        }
        // (stopped by the picture's edge, walking back)
        const bool stopped = Byte(c, kWalkingBack) && Byte(c, kAtEdge);
        if (kind >= 0x06) { // (the walks for so many frames)
            if (!Word(c, kCount)) {
                return false;
            }
            SetWord(c, kCount, Word(c, kCount) - 1);
        }
        switch (kind) {
        case 0x00: case 0x0A:
            SetLow16(c.d[0], Word(c, kOperands));
            if (Word(c, kOperands) >= Word(c, kGap)) {
                return false;
            }
            return !stopped;
        case 0x02: case 0x0C:
            SetLow16(c.d[0], Word(c, kOperands));
            if (Word(c, kOperands) < Word(c, kGap)) {
                return false;
            }
            return !stopped;
        case 0x06: case 0x08:
            return !stopped;
        default: { // 0x04, 0x0E: to the gap, turning about as it is passed
            if (Byte(c, kAtEdge)) {
                return false;
            }
            SetLow16(c.d[1], Word(c, kOperands) - Word(c, kGap) + 3);
            if ((c.d[1] & 0xFFFF) < 6) {
                return false;
            }
            SetLow16(c.d[0], Word(c, kOperands));
            SetByte(c, kWay, Word(c, kOperands) >= Word(c, kGap) ? 1 : 0);
            return true;
        }
        }
    }

    // 0234C0. The instruction going on, a frame of it
    void Going(M68K &c) {
        const uint8_t group = Byte(c, kWaiting), kind = Byte(c, kWaiting + 1);
        bool on;
        switch (group) {
        case 0x00:
            // (all but a few may be broken in on, by a reaction: bank 4 from its start)
            if (kind != 0x0A && kind != 0x10 && MayBreakIn(c)) {
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
        case 0x06: // a normal (02: while kChainable)
            on = Byte(c, kAction) && (kind != 2 || Byte(c, kChainable));
            break;
        case 0x0C: // a guard, while it is held
            on = Word(c, kState) != 0x0200 || Byte(c, kAction);
            break;
        default: // a jump, a special, a super combo: to the move's end
            on = Byte(c, kAction) != 0;
            break;
        }
        if (!on) {
            Over(c);
        }
    }

    // 022810. A frame of the computer's play (the player at A6). The bank's script is chosen if it is not yet; the
    // next instruction started if the last is over; and the one going on looked at
    void ComputerThinks(M68K &c) {
        if (Ram8(Var(c, kNoComputer))) {
            return;
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

    // 023CC6. The answer the game chose (Reactions), from its start
    void AnswerNow(M68K &c) {
        SetByte(c, kBank, 6);
        SetLong(c, kChosen, 0);
        ComputerThinks(c);
    }

    // 02348A. The script broken off (the player hit, thrown...): by its kIfBrokenOff, the plan goes on where it was
    void BrokenOff(M68K &c) {
        const uint16_t how = Word(c, kIfBrokenOff);
        if (how == 0 || (how == 4 && Chance(c, kBrokenOffChances))) {
            Over(c);
            return;
        }
        SetByte(c, kBank, 0);
        SetByte(c, kDepth, 0);
        SetWord(c, kIfBrokenOff, Word(c, kKeptIfBrokenOff));
        if (BackToPlan(c)) {
            End(c);
        }
    }

    // 023C6E. The computer standing free (in its own control, not attacking, jumping or stunned)
    bool StandsFree(M68K &c) {
        const uint8_t action = Byte(c, kAction);
        const bool yes = Word(c, kState) == 0x0200 && action != 0x06 && action != 0x0A && action != 0x0E && action != 0x10 &&
                         action != 0x12;
        Answer(c, yes);
        return yes;
    }
    // Whether the other's animation frame can be hit (its kHurt boxes; D0)
    bool OthersFrameHittable(M68K &c) {
        Other(c);
        c.a[4] = Ram32((c.a[4] & 0xFFFFFF) + kFrame);
        c.d[0] = Mem32(c, c.a[4] + kHurt) & 0xFFFFFF00;
        return c.d[0] != 0;
    }

    // 023B78. The answers that may break in (kMayBreakIn): is one to be given now? Its number to kAnswer (AnswerNow
    // runs it). 0: the other attacking near, on the floor; 1: reeling (kState 02020400) near an other that can be
    // hit: out of it at once; 2: the other lying down near; 3: the other stunned (or to be, once it is up)
    void Reactions(M68K &c) {
        SetLow16(c.d[5], Word(c, kMayBreakIn));
        const uint16_t bits = c.d[5] & 0xFFFF;
        if (!bits || Byte(c, kBank) == 6) {
            Answer(c, false);
            return;
        }
        bool yes = false;
        c.d[6] = 0;
        if ((bits & 1) && StandsFree(c)) { // 023C2C
            const uint32_t other = Other(c);
            yes = !Ram8(other + kInAir) && Word(c, kGap) < 0x30 && Ram8(other + kAttackOut) && Ram16(other + kState) == 0x0200 &&
                  OthersFrameHittable(c);
        }
        if (!yes) {
            c.d[6] = 1;
            if ((bits & 2) && Ram32(Me(c) + kState) == kReelingState && Word(c, kGap) < 0x40 && OthersFrameHittable(c)) {
                SetByte(c, kJuggle, 0);
                SetByte(c, kJuggle + 1, 0);
                SetLong(c, kReelTimers, 0);
                SetByte(c, kNoThrow, 5);
                SetByte(c, kFacesRight, Byte(c, kFaceTo));
                yes = true;
            }
        }
        if (!yes) {
            c.d[6] = 2;
            if ((bits & 4) && Word(c, kGap) < 0x60 && StandsFree(c)) { // 023CA2
                const uint32_t other = Other(c);
                yes = Ram32(other + kState) == kLyingDown && Ram8(other + kReelKind) == 4 && Ram8(other + kKnocksDown);
            }
        }
        if (!yes) {
            c.d[6] = 3;
            if ((bits & 8) && StandsFree(c)) {
                const uint32_t other = Other(c);
                yes = Ram8(other + kDizzyAfter) ||
                      (Ram16(other + kState) == 0x0200 && Ram8(other + kAction) == kActionStunned);
            }
        }
        if (yes) {
            SetByte(c, kAnswer, c.d[6] & 0xFF);
        }
        Answer(c, yes);
    }
} // namespace

void RegisterComputer() {
    // (the last number is the time a call is charged: about the game's own on average, as the check measured it)
    AddRewrite(kComputerThinks, "ComputerThinks", ComputerThinks, kFreeD0 | kFreeD1 | kFreeFlags, 467);
    AddRewrite(kAnswerNow, "AnswerNow", AnswerNow, kFreeD0 | kFreeD1 | kFreeFlags, 3416);
    AddRewrite(kBrokenOff, "BrokenOff", BrokenOff, kFreeD0 | kFreeD1 | kFreeFlags, 218);
    // (both callers set D0 and D1 before they read them: 021DF4, and AnswerNow)
    AddRewrite(kComputerReactions, "Reactions", Reactions, kFreeD0 | kFreeD1, 375);
    AddRewrite(kChance, "Chance", [](M68K &c) { Chance(c); }, 0, 78);
}

} // namespace cps2recomp
