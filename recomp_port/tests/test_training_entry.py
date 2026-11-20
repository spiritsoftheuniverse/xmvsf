#!/usr/bin/env python3
"""87.467 Training disclaimer handoff: source-only local regression preflight."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SESSION = (ROOT / "recomp/fight/session.cpp").read_text(encoding="utf-8")
API = (ROOT / "recomp/fight/session.h").read_text(encoding="utf-8")
FRONT = (ROOT / "recomp/port/frontend_sdl.cpp").read_text(encoding="utf-8")
PORT = (ROOT / "recomp/port/main.cpp").read_text(encoding="utf-8")

def need(value: bool, what: str) -> None:
    assert value, "87.467 Training entry: " + what

def main() -> None:
    boot = SESSION.split("void Boot(Machine &machine) {", 1)[1].split("void StartOver(", 1)[0]
    need("if (modes::Choosing() || ++g_bootFrames > kBootMost)" in boot,
         "must wait for native character select, with bounded fallback")
    need("if (Ram8(m.playersIn) == 3 ||" not in boot,
         "must not finish solely because both players joined")
    need("} else if (Ram8(m.playersIn) == 3) {" in boot,
         "do not continue pressing Start after joining")
    need(boot.index("} else if (Ram8(m.playersIn) == 3)") <
         boot.index("} else if (Ram8(m.credits) < 2"),
         "joined guard must precede credit and Start injection")
    need("SetFastForward(false);" in boot and "constexpr int kBootMost = 4000;" in SESSION,
         "original safe timeout must stay")
    begin = SESSION.split("void StartSession(Machine &machine) {", 1)[1].split(
        "bool InSession()", 1)[0]
    need("SetFastForward(true);" in begin, "Training still uses native fast-forward")
    need("bool EnteringTraining();" in API, "frontend training-state API exists")
    need("bool EnteringTraining() {\n    return g_boot && InSession();\n}" in SESSION,
         "only Training auto-boot gates presentation")
    need("if (g_boot && !InSession())" in SESSION,
         "leaving Training cancels pending boot")
    need("for (int n = 0; n < 120 && FastForward()" in PORT,
         "reuse existing 120-frame fast-forward slices")
    need(FRONT.index("const bool ran = port.Tick();") <
         FRONT.index("if (session::EnteringTraining()) {") <
         FRONT.index("if (nativeCapture.Pending()) {\n            // One native game-frame screenshot"),
         "block intermediate video and F6 before capture/presentation")
    gate = FRONT.split("if (session::EnteringTraining()) {", 1)[1].split("\n        }", 1)[0]
    need("port.machine->audio.clear();" in gate and "continue;" in gate,
         "no intermediate audio or video")
    need(FRONT.count("if (session::EnteringTraining()) {") == 1,
         "exactly one startup gate")
    need("case xmvsf_home::Action::Training:\n                    stagePicker.Open();" in FRONT,
         "native stage picker still precedes Training reset")
    need("case xmvsf_home::Action::Arcade:" in FRONT and
         "case xmvsf_home::Action::Versus:" in FRONT,
         "other mode paths remain present")
    print("XMVSF 87.467 Training entry: skip disclaimer until native select PASS")

if __name__ == "__main__":
    main()
