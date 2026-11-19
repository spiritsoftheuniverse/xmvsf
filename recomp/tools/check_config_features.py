#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 sunkpoet-lang and contributors. Part of cps2recomp.
"""Compile and exercise the actual game.cfg Board/Cheats parsers without game files."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True, help='New directory for local test artifacts')
parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=False)
source = (root/'port/main.cpp').read_text(encoding='utf-8')
start = source.index("// The board's settings that game.cfg declares")
end = source.index("// Every game's features that come from the port itself", start)
fragment = source[start:end]
registration = source[source.index('void RegisterPortFeatures() {'):source.index('    // (run_ahead: Port::RunAhead)')]
fragment += registration.replace('void RegisterPortFeatures()', 'void RegisterDeclaredFeaturesForTest()') + '}\n'
header = r'''
#include "runtime/game.h"
#include "machine/machine.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <array>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#define GAME_BOARD_OPTION_1 "difficulty | Difficulty | A:0-2 | Test | Normal=3"
#define GAME_CHEAT_1 "energy | Energy | Test | when FF0000.b=1 | On: FF0002.w=100"
using namespace cps2recomp;
namespace cps2recomp {
std::map<std::string,int> testFeatures;
Game& TheGame() { static Game game; return game; }
int FeatureValue(const char* name) { return testFeatures[name]; }
uint8_t* Ram(uint32_t address) { static std::array<uint8_t,65536> ram{}; return ram.data()+(address & 0xFFFF); }
}
namespace port {
'''
tests = r'''
}
static void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
int main() {
    std::string error;
    { port::BoardOption option; require(port::ParseBoardOption("test | Test | A:0-2 | Test | Normal=3", option, error), "one-choice board option"); }
    { port::BoardOption option; require(port::ParseBoardOption("test | Test | C:6 | Test | On=1", option, error), "single-bit board option"); }
    for (const char* bits : {"D:0", "A:-1", "A:7-8", "A:4-2", "A:0-2junk", "A:0junk"}) {
        port::BoardOption option;
        auto line=std::string("test | Test | ")+bits+" | Test | On=1";
        require(!port::ParseBoardOption(line.c_str(),option,error),"invalid board range accepted");
    }
    { port::BoardOption option; require(!port::ParseBoardOption("test | Test | A:0 | Test | On=2",option,error),"oversized board value"); }
    for (const char* value : {"FF0001.w=1", "FFFFFE.l=1", "FEFFFF.b=1", "FF0000.q=1", "FF0000.b=100", "FF0000.b=1junk"}) {
        port::RamWrite write; require(!port::ParseRamWrite(value,write),"invalid RAM write accepted");
    }
    { port::RamWrite write; require(port::ParseRamWrite("FF0000.b=FF",write),"valid RAM byte"); }
    { port::Cheat cheat; require(port::ParseCheat("test | Test | Test | when FF0000.b=1 | On: FF0002.w=100",cheat,error),"valid conditional cheat"); }
    { port::Cheat cheat; require(!port::ParseCheat("test | Test | Test | when FF0000.b=1",cheat,error),"cheat without action"); }
    { port::Cheat cheat; require(!port::ParseCheat("test | Test | Test | On: FFFFFF.w=1",cheat,error),"cheat invalid write"); }
    TheGame().features.push_back({"difficulty","Board","Existing","Existing"});
    require(!port::ReadDeclaredFeatures(error),"duplicate with existing feature accepted");
    TheGame().features.clear();
    require(port::ReadDeclaredFeatures(error),"valid declarations rejected");
    for (bool existing : {false, true}) {
        TheGame().features.clear();
        TheGame().pages.clear();
        if (existing) {
            TheGame().pages.push_back({"Board", "", "Existing board"});
            TheGame().pages.push_back({"Cheats", "", "Existing cheats"});
        }
        require(port::ReadDeclaredFeatures(error), "declarations rejected during registration");
        port::RegisterDeclaredFeaturesForTest();
        require(TheGame().pages.size()==2, "Board/Cheats pages duplicated or missing");
        require(TheGame().features.size()==2, "declarative features duplicated or missing");
    }
    *Ram(0xFF0000)=0;
    testFeatures["energy"]=1;
    port::ApplyCheats();
    require(Ram16(0xFF0002)==0,"conditional cheat ran outside its condition");
    *Ram(0xFF0000)=1;
    port::ApplyCheats();
    require(Ram16(0xFF0002)==0x100,"conditional cheat did not apply");
    SetRam16(0xFF0002,0x77);
    testFeatures["energy"]=0;
    port::ApplyCheats();
    require(Ram16(0xFF0002)==0x77,"disabled cheat changed memory");
    std::cout << "PASS: Board/Cheats syntax, bounds, conditional writes and existing-registry collisions and page reuse\n";
    return 0;
}
'''
cpp = out/'configuration-parsers.cpp'
cpp.write_text(header+fragment+tests, encoding='utf-8')
exe = out/('configuration-parsers.exe' if os.name == 'nt' else 'configuration-parsers')
flags = ['-std=c++20', '-O1', '-ffunction-sections', '-fdata-sections', '-I'+str(root), '-I'+str(root/'machine'), '-I'+str(root/'recomp-common')]
flags += ['-Wl,-dead_strip'] if sys.platform == 'darwin' else ['-Wl,--gc-sections']
if os.name == 'nt': flags += ['-static']
subprocess.run([args.cxx, *flags, str(cpp), '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
