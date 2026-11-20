#!/usr/bin/env python3
"""87.470 native second P1 pick changes input owner to dummy P2 immediately."""
from pathlib import Path
from version_guard import require_matching_versions

ROOT = Path(__file__).resolve().parents[2]
S = (ROOT / "recomp/fight/session.cpp").read_text(encoding="utf-8")
H = (ROOT / "recomp/fight/session.h").read_text(encoding="utf-8")
C = (ROOT / "recomp/games/xmvsf/src/choosing.cpp").read_text(encoding="utf-8")
T = (ROOT / "recomp/games/xmvsf/src/training.cpp").read_text(encoding="utf-8")
G = (ROOT / "recomp/games/xmvsf/src/game.cpp").read_text(encoding="utf-8")
F = (ROOT / "recomp/port/frontend_sdl.cpp").read_text(encoding="utf-8")
HOME = (ROOT / "recomp_port/xmvsf_home.h").read_text(encoding="utf-8")
TEXTURES = (ROOT / "recomp_port/xmvsf_menu_textures.h").read_text(encoding="utf-8")

def main() -> None:
    assert 'AddHook(HookAt(0x00B1E8), "Chosen", Chosen);' in C
    assert 'session::CharacterAccepted(n);' in C
    assert 'if (record != kChoosing && record != kChoosing + kChoosingSize)' in C
    assert '#include "session.h"' in C
    assert 'bool handoffAfterSecondPick = false;' in H
    assert 'd.handoffAfterSecondPick = true;' in T
    assert 'void CharacterAccepted(int player);' in H
    pick = S.split('void CharacterAccepted(int player) {', 1)[1].split('\n}\n', 1)[0]
    for token in ('player != 0', '!InSession()', '!modes::Choosing()',
                  'RunningAhead()', 'g_p1Accepted >= 2',
                  '++g_p1Accepted;', 'if (g_p1Accepted == 2)'):
        assert token in pick, token
    restart = S.split('void StartSession(Machine &machine) {', 1)[1].split('bool InSession()', 1)[0]
    for token in ('g_p1Accepted = 0;', 'g_p1AutoFrames = 0;',
                  'g_p1AutoFallback = false;', 'g_choosingTwo = false;', 'g_heldOver = 0;'):
        assert token in restart, token
    before = S.split('void BeforeFrame(Machine &machine) {', 1)[1].split('void Frame(Machine &machine)', 1)[0]
    for token in ('g_p1Accepted == 2', '(earlyOwner || d.chosen(0)) && !d.chosen(1)',
                  'machine.players[1] &= kStart | kCoin;',
                  'machine.players[1] = (machine.players[1] & (kStart | kCoin)) | (held & ~heldOver);',
                  'g_choosingTwo ? g_heldOver & held : held', 'g_p1AutoFrames % 12 == 4',
                  'g_p1AutoFrames > 240', 'machine.players[0] |= kPunch1;'):
        assert token in before, token
    # Avoid a late stall if P2 Dummy 2 finishes ahead of P1's native mode/speed.
    assert before.count('autoConfirmP1();') == 2
    assert 'if (earlyOwner && d.chosen && !d.chosen(0)) autoConfirmP1();' in before
    assert 'if (!RunningAhead() && ++g_p1AutoFrames > 240)' in before
    assert G.index('ChoosingBeforeFrame(machine);') < G.index('session::BeforeFrame(machine);')
    assert 'session::StartSession(*port.machine);' in F
    assert 'if (session::EnteringTraining()) {' in F
    assert require_matching_versions(HOME, TEXTURES, minimum=(87, 470)) >= (87, 470)
    assert 'films_.Draw(renderer, now, filmReveal);' in TEXTURES
    print('XMVSF 87.470 native P1 team handoff to hidden dummy P2 source PASS')

if __name__ == '__main__':
    main()
