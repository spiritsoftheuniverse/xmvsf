#ifndef XMVSF_STAGE87_370_AKUMA_AIR_SUPER_STARTUP_H
#define XMVSF_STAGE87_370_AKUMA_AIR_SUPER_STARTUP_H
// 87.370 Version-4 native Akuma air-super startup trim.
//
// F10 TEST 87.368: Akuma (native ID 1E) entered action 2E06,
// mode 2A28, D6=0A and step08=2 at frame 7618 with ROM script
// 0EB87A/animation 59FA. The native startup reached script 0EB8BA,
// animation 64D4 at frame 7640; native 2E08/step 4 began at 7691.
// Trim ONLY the 22-frame lead-in to that already observed pose.
// Do not jump to 2E08, forge a projectile, mutate the meter, cancel
// the native super freeze, or alter any action, mode, step or D6.
//
// This is a TEST gameplay experiment: the ROM record is checked
// at runtime, and can be disabled with XM87370_AKUMA_AIR_SUPER_FAST=0.
// Original Version 3 and non-US XMVSF remain byte-for-byte unchanged.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87370Akuma {
inline void putW(std::uint8_t* ram,unsigned p,unsigned v){
    ram[(p&65535u)^1u]=std::uint8_t(v>>8u);
    ram[((p+1u)&65535u)^1u]=std::uint8_t(v);
}
inline void putL(std::uint8_t* ram,unsigned p,unsigned v){
    putW(ram,p,v>>16u);
    putW(ram,p+2u,v);
}
inline bool enabled(){
    const char* flag=std::getenv("XM87370_AKUMA_AIR_SUPER_FAST");
    return !flag || std::strcmp(flag,"0")!=0;
}
inline bool beforeCpu(std::uint8_t* ram,const std::uint8_t* rom,
                      unsigned romSize,unsigned frame,const char* driver,
                      bool version4,bool runahead){
    if(!enabled() || !version4 || runahead || !ram || !rom ||
       !driver || std::strcmp(driver,"xmvsfu")!=0)return false;
    const unsigned rootScript=0x0EB87Au;
    const unsigned chargeScript=0x0EB8BAu;
    const auto root=XM87142::resolve(rom,romSize,0x1Eu,0x2Eu,0x06u);
    if(!root.valid || root.script!=rootScript ||
       root.animation!=0x59FAu || chargeScript+16u>romSize ||
       XM87142::romW(rom,romSize,chargeScript+6u)!=0x64D4u)
        return false;
    // 16-byte original ROM animation records: +2 is the native
    // frame duration, +6 is the native sprite animation index.
    const unsigned chargeDuration=
        XM87142::romW(rom,romSize,chargeScript+2u);
    if(chargeDuration<1u || chargeDuration>16u)return false;
    for(unsigned side=0u;side<2u;++side){
        const unsigned p=0x4000u+side*0x400u;
        const unsigned y=XM87142::ramW(ram,p+0x10u);
        const unsigned floor=XM87142::ramW(ram,p+0x254u);
        // Only Akuma as the actually active point fighter, truly
        // airborne, executing this exact ORIGINAL native 2E06 root.
        if(!XM87142::ramW(ram,p) ||
           XM87142::ramW(ram,p+0x52u)!=0x1Eu ||
           XM87142::ramW(ram,p+0x60u)!=0x2E06u ||
           XM87142::ramW(ram,p+0xA0u)!=0x2A28u ||
           XM87142::ramW(ram,p+0x08u)!=2u ||
           XM87142::ramB(ram,p+0xD6u)!=0x0Au ||
           XM87142::ramL(ram,p+0x34u)!=rootScript ||
           XM87142::ramL(ram,p+0x38u)!=rootScript+16u ||
           XM87142::ramW(ram,p+0x42u)!=0x59FAu ||
           XM87142::ramW(ram,p+0x3Eu)==0u ||
           floor<24u || floor>=2048u || y+24u>=floor)
            continue;
        // Preserve the native controller. The normal 68000 animation
        // reader will execute the final startup pose and subsequently
        // dispatch its original charge/freeze/attack progression.
        putL(ram,p+0x34u,chargeScript);
        putL(ram,p+0x38u,chargeScript+16u);
        putW(ram,p+0x3Eu,chargeDuration);
        putW(ram,p+0x42u,0x64D4u);
        std::fprintf(stderr,
            "XM87.370 AKUMA_AIR_SUPER_STARTUP_TRIM frame=%u side=%u "
            "action=2E06 native_script=%06X->%06X pose=64D4 "
            "rom_duration=%u action_step_meter_untouched=1\n",
            frame,side+1u,rootScript,chargeScript,chargeDuration);
        return true;
    }
    return false;
}
} // namespace XM87370Akuma
#endif
