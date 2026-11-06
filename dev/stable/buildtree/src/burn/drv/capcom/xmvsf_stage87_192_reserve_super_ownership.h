#ifndef XMVSF_STAGE87_192_RESERVE_SUPER_OWNERSHIP_H
#define XMVSF_STAGE87_192_RESERVE_SUPER_OWNERSHIP_H
// 87.192 EXPERIMENT: The RESERVE fighter owns a captured motion/chord.
// Do not choose a super by character ID, English move name, 2E selector,
// ROM address, guessed script index, or a fixed list of accepted supers.
// Replay this gesture only after the actual native fighter swap, so the
// incoming actor's OWN 68000 command parser chooses its action.
// Record/accept ONLY a genuine original-ROM 2E root/controlled action.
// A rejected command never masquerades as a super or enters the catalog.
// This is post-tag discovery; pre-tag reserve shadow CPU execution is NOT
// implemented. The real tag is not reversed if the reserve chooses a
// normal/special. Keep the feature TEST-only until that is solved.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include "xmvsf_stage87_189_native_super_family.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87192Owner {
enum Motion { NONE=0,QCF=1,QCB=2,DPF=3,DPB=4,HCF=5,HCB=6,
              DOUBLE_QCF=7,DOUBLE_QCB=8 };
static inline Motion parse(const char*name){
    if(!name)return NONE;
    if(!std::strcmp(name,"QCF"))return QCF;
    if(!std::strcmp(name,"QCB"))return QCB;
    if(!std::strcmp(name,"DPF"))return DPF;
    if(!std::strcmp(name,"DPB"))return DPB;
    if(!std::strcmp(name,"HCF"))return HCF;
    if(!std::strcmp(name,"HCB"))return HCB;
    if(!std::strcmp(name,"2QCF"))return DOUBLE_QCF;
    if(!std::strcmp(name,"2QCB"))return DOUBLE_QCB;
    return NONE;
}
static inline unsigned countBits(unsigned n){
    unsigned c=0;for(;n;n>>=1u)c+=n&1u;return c;
}
static inline unsigned canonicalButtons(unsigned mask){
    mask&=0x3Fu;
    // Original 3P host motion is a dash; original 2P is a proven
    // native super chord. Do not turn mixed P+K into an invented 2P.
    if(!(mask&0x38u) && countBits(mask&0x07u)>=2u)return 0x03u;
    if(!(mask&0x07u) && countBits(mask&0x38u)>=2u)return 0x18u;
    return mask;
}
struct Mask { bool active=false;unsigned directions=0u,attacks=0u; };
struct Proposal {
    bool valid=false;
    unsigned frame=0u,reserve=0u,buttonMask=0u;
    Motion motion=NONE;
    // Direction path uses relative D/DF/F etc; not a character-specific
    // move registry. The reserve ROM is the only super decoder.
    static unsigned length(Motion m){
        if(m==DOUBLE_QCF||m==DOUBLE_QCB)return 6u;
        if(m==HCF||m==HCB)return 5u;
        if(m!=NONE)return 3u;
        return 0u;
    }
    static unsigned direction(Motion m,unsigned idx){
        static const unsigned qcf[]={2u,3u,6u};
        static const unsigned qcb[]={2u,1u,4u};
        static const unsigned dpf[]={6u,2u,3u};
        static const unsigned dpb[]={4u,2u,1u};
        static const unsigned hcf[]={4u,1u,2u,3u,6u};
        static const unsigned hcb[]={6u,3u,2u,1u,4u};
        static const unsigned dqcf[]={2u,3u,6u,2u,3u,6u};
        static const unsigned dqcb[]={2u,1u,4u,2u,1u,4u};
        switch(m){
        case QCF:return qcf[idx%3u];case QCB:return qcb[idx%3u];
        case DPF:return dpf[idx%3u];case DPB:return dpb[idx%3u];
        case HCF:return hcf[idx%5u];case HCB:return hcb[idx%5u];
        case DOUBLE_QCF:return dqcf[idx%6u];
        case DOUBLE_QCB:return dqcb[idx%6u];
        default:return 0u;
        }
    }
    unsigned duration()const{return 2u*length(motion)+3u;}
    Mask input(unsigned t,bool facingRight)const{
        Mask r;
        const unsigned n=length(motion);
        if(!valid||!n||!buttonMask||t>=duration())return r;
        r.active=true;
        const unsigned step=t/2u<n?t/2u:n-1u;
        const unsigned dir=t<=2u*n?direction(motion,step):0u;
        const unsigned f=facingRight?1u:2u,b=facingRight?2u:1u;
        const unsigned side=dir==6u||dir==3u?f:
                            dir==4u||dir==1u?b:0u;
        const unsigned down=dir==2u||dir==3u||dir==1u?4u:0u;
        r.directions=side|down;
        // Preserve the empirically good 87.156 final cardinal-edge
        // timing: two frames per direction; press late and release.
        if(t>=2u*(n-1u)+1u && t<=2u*n)
            r.attacks=canonicalButtons(buttonMask);
        return r;
    }
};
struct Seen {
    unsigned fighter=0u,action=0u,script=0u,at=0u;
    Motion motion=NONE;unsigned buttons=0u;
};
struct State {
    Proposal pending[2];
    Seen learned[128];
    unsigned learnedCount=0u,lastFrame=~0u;
    std::FILE*log=nullptr;bool checkedLog=false;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void ensureLog(){
        if(checkedLog)return;
        checkedLog=true;
        const char*name=std::getenv("XM87192_RESERVE_SUPER_LOG");
        if(name&&*name){
            log=std::fopen(name,"wb");
            if(log)std::fputs(
               "frame\\tside\\treserve\\taction\\trom_root\\tmotion\\tbuttons\\tresult\\n",log);
        }
    }
    void rejected(unsigned side,unsigned now,
                  const Proposal&p,const XM87184Life::Sample&s){
        ensureLog();
        if(log){std::fprintf(log,
            "%u\\t%u\\t%02X\\t%04X\\t%06X\\t%u\\t%02X\\tNOT_NATIVE_SUPER\\n",
            now,side+1u,p.reserve,s.action,s.script,unsigned(p.motion),
            p.buttonMask);std::fflush(log);}
    }
    void reset(){
        pending[0]=pending[1]=Proposal();
        learnedCount=0u;lastFrame=~0u;
    }
    void clear(unsigned side){if(side<2u)pending[side]=Proposal();}
    bool propose(unsigned side,unsigned reserve,unsigned now,
                 Motion motion,unsigned buttons){
        if(side>=2u||reserve>0x40u||(reserve&1u)||motion==NONE||
           !lengthValid(motion)||!(buttons&0x3Fu))return false;
        Proposal p;p.valid=true;p.frame=now;p.reserve=reserve;
        p.motion=motion;p.buttonMask=buttons&0x3Fu;
        pending[side]=p;
        ensureLog();
        if(log){std::fprintf(log,
           "%u\\t%u\\t%02X\\t0000\\t000000\\t%u\\t%02X\\tINPUT_PROPOSED\\n",
           now,side+1u,reserve,unsigned(motion),p.buttonMask);
           std::fflush(log);}
        return true;
    }
    static bool lengthValid(Motion motion){
        return Proposal::length(motion)>0u;
    }
    bool fresh(unsigned side,unsigned reserve,unsigned now)const{
        if(side>=2u)return false;
        const Proposal& p=pending[side];
        return p.valid && p.reserve==reserve && p.frame==now;
    }
    // Native ROM verification is action- and actor-owned, not inferred
    // from a motion name. The ROM table MUST resolve the action's 2E
    // selector, its script must be in that original record span, and
    // the actor must actually be in a 2A controller with nonzero D6.
    static bool nativeSuper(const std::uint8_t*rom,unsigned size,
                            const XM87184Life::Sample&s,
                            unsigned*rootOut=nullptr){
        if((s.action&0xFF00u)!=0x2E00u||
           (s.mode&0xFF00u)!=0x2A00u||s.d6==0u||
           s.step<2u||(s.step&1u))return false;
        const auto r=XM87142::resolve(
            rom,size,s.fighter,0x2Eu,s.action&0xFFu);
        if(!r.valid)return false;
        const unsigned end=XM87189Family::Tracker::nextRomScript(
            rom,size,s.fighter,r.script);
        if(s.script<r.script || (s.script-r.script)%16u!=0u)return false;
        if(end && s.script>=end)return false;
        if(!end && (s.script>=size||size-s.script<16u))return false;
        if(rootOut)*rootOut=r.script;
        return true;
    }
    bool confirm(unsigned side,unsigned now,const Proposal&p,
                 const XM87184Life::Sample&s,
                 const std::uint8_t*rom,unsigned size){
        unsigned root=0u;
        if(!p.valid||p.reserve!=s.fighter||
           !nativeSuper(rom,size,s,&root))return false;
        ensureLog();
        for(unsigned i=0;i<learnedCount;++i)
            if(learned[i].fighter==s.fighter&&
               learned[i].action==s.action&&
               learned[i].script==root){
                if(log){std::fprintf(log,
                    "%u\t%u\t%02X\t%04X\t%06X\t%u\t%02X\tCONFIRMED_REPEAT\n",
                    now,side+1u,s.fighter,s.action,root,unsigned(p.motion),
                    p.buttonMask);std::fflush(log);}
                return true;
            }
        if(learnedCount<128u){
            Seen &item=learned[learnedCount++];
            item.fighter=s.fighter;item.action=s.action;item.script=root;
            item.at=now;item.motion=p.motion;item.buttons=p.buttonMask;
        }
        if(log){std::fprintf(log,
             "%u\t%u\t%02X\t%04X\t%06X\t%u\t%02X\tCONFIRMED_NEW\n",
             now,side+1u,s.fighter,s.action,root,unsigned(p.motion),
             p.buttonMask);std::fflush(log);}
        return true;
    }
};
inline State&state(){static State s;return s;}
} // namespace XM87192Owner
#endif
