#ifndef XMVSF_STAGE87_127_STORM_VOICE_MUTE_H
#define XMVSF_STAGE87_127_STORM_VOICE_MUTE_H
// Stage 87.127: suppress ONLY the capture-confirmed Storm attack voice sample.
// 87.126 recording: channel 12 address 6508, volume register 66, phase 63.
// Input/action confirmation: ground HK=1204; air HP=1804; air MP=180C/180E.
// The DSP/QSound voice register mapping is verified in qs_c.cpp.
// XM87.129: recorded crouch HP uses action 1404 and the SAME channel-12
// PCM sample 6508. Unlike standing HP (1004), it must be muted.
// Keep native commands, hit noises, other channels, and all game RAM intact.
// TEST 87.223: apply in ALL rule modes; allow a bounded QSound Z80 delay
// after the observed 68000 action, but never mute an unrelated sample.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87127Voice {
static inline unsigned byteAt(const std::uint8_t* r,unsigned p){return r[(p&0xffffu)^1u];}
static inline unsigned wordAt(const std::uint8_t* r,unsigned p){return (byteAt(r,p)<<8)|byteAt(r,p+1);}
static inline bool stormTargetAction(const std::uint8_t* r,unsigned base){
    const unsigned id=wordAt(r,base+0x52u);
    const unsigned bank=wordAt(r,base+0x40u);
    const unsigned action=wordAt(r,base+0x60u);
    const unsigned y=wordAt(r,base+0x10u);
    if(id!=0x0004u || bank!=0x0022u)return false;
    // Native normal-air action 1802 is regular horizontal MP; 1A02 is its
    // super-jump family. Directional MP variants 180C/180E were already muted.
    // 1808/1A08 are medium KICKS and must remain audible.
    return action==0x1404u || // XM87.129: Storm crouching HP; NOT 1004 standing HP
           (action==0x1204u && y>=472u) ||
           ((action==0x1802u || action==0x1A02u ||
             action==0x1804u || action==0x180Cu || action==0x180Eu) && y<472u);
}
struct Filter {
    bool muted12=false,initialized=false,enabled=true;
    unsigned lastFrame=~0u,count=0,lastStormAttackFrame=~0u;
    unsigned lastStormActor=~0u;
    std::FILE* log=nullptr;
    ~Filter(){if(log)std::fclose(log);}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* flag=std::getenv("XM87127_STORM_VOICE_MUTE");
        enabled=!flag || std::strcmp(flag,"0")!=0;
        const char* path=std::getenv("XM87127_STORM_VOICE_LOG_FILE");
        if(path && *path){log=std::fopen(path,"wb");if(log)std::fputs("frame	register	original	filtered	muted12	reason\n",log);}
        std::fprintf(stderr,"XM87.128 STORM_HORIZONTAL_MP_MUTE XM87.129 STORM_CROUCH_HP_VOICE_MUTE STORM_VOICE_MUTE_READY enabled=%u targeted_pcm_channel=12 sample=6508\n",unsigned(enabled));
    }
    void reset(){muted12=false;lastFrame=~0u;count=0;
        lastStormAttackFrame=~0u;lastStormActor=~0u;}
    // Z80 QSound can dispatch a voice command AFTER the 68000 attack pose
    // has advanced. Capture the confirmed source actor independently of
    // Classic/Tournament on committed native frames, not inside a mode gate.
    void observe(unsigned frame,const std::uint8_t* ram,bool xmvsf){
        if(!xmvsf || !ram || (byteAt(ram,0x5000u)!=8u && byteAt(ram,0x5001u)!=8u)){
            lastStormAttackFrame=~0u;lastStormActor=~0u;return;
        }
        for(unsigned i=0;i<2u;++i){
            const unsigned base=i?0x4400u:0x4000u;
            if(stormTargetAction(ram,base)){
                lastStormAttackFrame=frame;lastStormActor=i;return;
            }
        }
        if(frame<lastStormAttackFrame || frame-lastStormAttackFrame>12u){
            lastStormAttackFrame=~0u;lastStormActor=~0u;
        }
    }
    unsigned transform(unsigned frame,unsigned reg,unsigned value,const std::uint8_t* ram){
        init();
        if(!enabled)return value;
        // The channel's sample-position register is 0x61. Its setup always
        // precedes a new PCM voice's 0x66 volume command in the capture.
        // Releasing on every new sample prevents silencing reused channels.
        if(reg==0x61u){
            const bool combat=ram && (byteAt(ram,0x5000u)==8u || byteAt(ram,0x5001u)==8u);
            const bool direct=combat && (stormTargetAction(ram,0x4000u)||
                                         stormTargetAction(ram,0x4400u));
            const bool delayed=combat && lastStormActor<2u &&
                frame>=lastStormAttackFrame && frame-lastStormAttackFrame<=12u &&
                wordAt(ram,(lastStormActor?0x4400u:0x4000u)+0x52u)==0x0004u;
            const bool newMute=(value==0x6508u && (direct || delayed));
            if(newMute!=muted12){
                muted12=newMute;
                if(log && count++<15000u){std::fprintf(log,"%u	%02X	%04X	%04X	%u	SAMPLE_%s\n",frame,reg,value,value,unsigned(muted12),muted12?"MUTED":"REPLACED");std::fflush(log);}
            }
            return value;
        }
        if(reg==0x66u && muted12 && value!=0u){
            if(log && count++<15000u){std::fprintf(log,"%u	%02X	%04X	0000	1	STORM_VOICE_VOLUME_SUPPRESSED\n",frame,reg,value);std::fflush(log);}
            return 0u;
        }
        return value;
    }
};
static inline Filter& state(){static Filter s;return s;}
} // XM87127Voice
#endif
