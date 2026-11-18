#!/usr/bin/env python3
"""87.451: executable C++ capture gate test, plus Options integration guards."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "recomp_port" / "xmvsf_capture_lifecycle.h"
OPTIONS = ROOT / "recomp_port" / "xmvsf_options.h"
OWNER = ROOT / "recomp_port" / "xmvsf_pad_owners.h"

CPP = r'''
#include "xmvsf_capture_lifecycle.h"
#include <cassert>
using port::xmvsf_home::CaptureLifecycle;
int main() {
    CaptureLifecycle gate;
    assert(gate.MayNavigate());
    gate.Arm(100, true);
    assert(gate.Capturing());
    assert(gate.AwaitingActivatorRelease());
    assert(!gate.MayAccept(true)); // the original A cannot bind itself
    assert(gate.MayAccept(false)); // fast new input (<180ms) is accepted
    assert(!gate.Expired(250));
    gate.ActivatorReleased();
    assert(gate.MayAccept(true)); // a fresh A may be assigned
    gate.Finish(180);
    assert(!gate.Capturing() && !gate.MayNavigate());
    gate.MenuButtons(true, 240);
    assert(!gate.MayNavigate());
    gate.MenuButtons(false, 250);
    assert(gate.MayNavigate());
    gate.Arm(500, false);
    assert(gate.MayAccept(true));
    assert(!gate.Expired(12499));
    assert(gate.Expired(12500));
    gate.Cancel(12500);
    assert(!gate.Capturing() && !gate.MayNavigate());
    gate.MenuButtons(false, 12501);
    assert(gate.MayNavigate());
    gate.Arm(900, true);
    gate.Finish(1000);
    gate.MenuButtons(true, 2499);
    assert(!gate.MayNavigate());
    gate.MenuButtons(true, 2500); // stuck A cannot lock Options indefinitely
    assert(gate.MayNavigate());
    gate.Arm(3000, true);
    gate.Reset();
    assert(gate.MayNavigate() && !gate.Capturing());
}
'''


def run():
    options = OPTIONS.read_text(encoding="utf-8")
    owners = OWNER.read_text(encoding="utf-8")
    header = HEADER.read_text(encoding="utf-8")
    for required in (
        '#include "xmvsf_capture_lifecycle.h"',
        'lifecycle_.Arm(SDL_GetTicks(), !activators_.empty())',
        'lifecycle_.Finish(SDL_GetTicks())',
        'lifecycle_.Cancel(SDL_GetTicks())',
        'TickCapture(owners, table);',
        'ObserveActivatorRelease(ev);',
        'IsActivatorPress(ev, owners)',
        'SDL_EVENT_KEY_UP',
        'SDL_EVENT_GAMEPAD_BUTTON_UP',
        'SDL_EVENT_JOYSTICK_BUTTON_UP',
        'SDL_EVENT_WINDOW_FOCUS_LOST',
        'SDL_EVENT_JOYSTICK_REMOVED',
        'SDL_EVENT_GAMEPAD_REMOVED',
        'SDL_GAMEPAD_BUTTON_GUIDE',
        'CAPTURE TIMED OUT',
        'UNMAPPED BUTTON - TRY RAW INPUT OR ESC',
    ):
        assert required in options, f"missing lifecycle hook: {required}"
    for obsolete in ("captureAt_ + 180", "suppressUntil_", "bool capture_ ="):
        assert obsolete not in options, f"old capture timer: {obsolete}"
    assert "IsMappedConfirmPhysical(" in owners
    assert "IsRawMenuConfirmPhysical(" in owners
    assert "kMaxRearmMs = 1500" in header
    compiler = shutil.which("g++")
    if not compiler:
        raise RuntimeError("g++ required for real C++ capture-state regression")
    with tempfile.TemporaryDirectory(prefix="xmvsf-capture-") as folder:
        source = Path(folder, "gate.cpp")
        program = Path(folder, "gate_test.exe")
        source.write_text(CPP, encoding="utf-8")
        subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-I", str(HEADER.parent), str(source), "-o", str(program)],
                       check=True, timeout=30)
        subprocess.run([str(program)], check=True, timeout=10)
    print("XMVSF 87.451 capture lifecycle PASS: fast binding, A-release, timeout, "
          "stuck-confirm recovery, focus/disconnect hooks and menu rearm")


if __name__ == "__main__":
    run()
