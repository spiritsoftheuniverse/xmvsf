#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 sunkpoet-lang and contributors. Part of cps2recomp.
"""Check computed-MMIO trial isolation with an original, synthetic 68000 program (no game files)."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ap = argparse.ArgumentParser(description=__doc__)
ap.add_argument('--output', type=Path, required=True)
ap.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
args = ap.parse_args()
root = Path(__file__).resolve().parents[1]
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=False)
# Reset vectors; MOVE.W #2,$1000; MOVE.W #3,$1002; MOVE.W $1004,D0; STOP #$2700.
program = bytes.fromhex('0000 2000 0000 0008 33FC 0002 0000 1000 33FC 0003 0000 1002 3039 0000 1004 4E72 2700')
(out/'program.bin').write_bytes(program)
(out/'entries.txt').write_text('000008\n', encoding='utf-8')
subprocess.run([sys.executable, str(root/'tools/recomp.py'), '--program', str(out/'program.bin'), '--entries', str(out/'entries.txt'), '--out', str(out/'gen')], check=True)
cpp = r"""
#include "runtime/native.h"
#include "runtime/game.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace cps2recomp;
namespace cps2recomp { Game& TheGame() { static Game game; return game; } }
struct Bus : M68KBus {
    std::array<uint8_t, 65536> bytes{};
    uint16_t left=7, right=11, savedLeft=0, savedRight=0;
    unsigned begins=0, ends=0, trials=0;
    uint16_t Read16(uint32_t a) override {
        if (a==0x1000) return left;
        if (a==0x1002) return right;
        if (a==0x1004) return left*right;
        return (bytes.at(a)<<8)|bytes.at(a+1);
    }
    uint8_t Read8(uint32_t a) override { return Read16(a&~1u) >> ((a&1)?0:8); }
    uint16_t Fetch16(uint32_t a) override { return Read16(a); }
    void Write16(uint32_t a,uint16_t v) override {
        if (a==0x1000) left=v;
        else if (a==0x1002) right=v;
        else throw std::runtime_error("unexpected synthetic I/O write");
    }
    void Write8(uint32_t,uint8_t) override { throw std::runtime_error("unexpected byte write"); }
    void BeginCheck() override { savedLeft=left; savedRight=right; ++begins; }
    void CheckWrite(uint32_t a,unsigned size,uint32_t v) override {
        if (size!=1) throw std::runtime_error("unexpected trial width");
        Write16(a,static_cast<uint16_t>(v)); ++trials;
    }
    void EndCheck() override { left=savedLeft; right=savedRight; ++ends; }
};
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    Bus bus;
    std::ifstream in(argv[1],std::ios::binary);
    in.read(reinterpret_cast<char*>(bus.bytes.data()),34);
    if(in.gcount()!=34) return 2;
    M68K cpu(bus); cpu.Reset(true); cpu.Run(100,M68K::Mode::Check);
    if (cpu.d[0]!=6 || bus.left!=2 || bus.right!=3 || bus.begins!=bus.ends || !bus.begins || bus.trials!=2)
        throw std::runtime_error("computed MMIO or trial restoration differs");
    std::cout << "PASS: computed MMIO writes are visible during trial and restored before reference\n";
}
"""
(out/'check.cpp').write_text(cpp,encoding='utf-8')
exe=out/('check-mmio.exe' if os.name=='nt' else 'check-mmio')
flags=['-std=c++20','-O1','-DCPS2RECOMP_CHECK','-I'+str(root),'-I'+str(root/'runtime'),'-I'+str(root/'machine'),'-I'+str(root/'recomp-common')]
if os.name=='nt': flags+=['-static']
subprocess.run([args.cxx,*flags,str(out/'check.cpp'),str(root/'machine/m68k.cpp'),str(root/'runtime/recomp.cpp'),*[str(p) for p in sorted((out/'gen').glob('native_*.cpp'))],'-o',str(exe)],check=True)
subprocess.run([str(exe),str(out/'program.bin')],check=True)
