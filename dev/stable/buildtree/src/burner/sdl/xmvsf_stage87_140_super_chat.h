#ifndef XMVSF_STAGE87_140_SUPER_CHAT_H
#define XMVSF_STAGE87_140_SUPER_CHAT_H
// 87.140 experimental DHC input parser diagnostics. Read-only.
// Logs native CPS2 controller bits, the FIRST SUPER *candidate* action, the
// real point/reserve IDs+script, and matches generic motion/chord patterns.
// These patterns are NOT an invented fighter-specific verified super list.
// The ROM-specific DHC super registry remains unpopulated until demonstrated.
#include "../../intf/input/sdl/xmvsf_stage87_137_dhc_input_history.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87140Chat {
enum { CHAT_CAPACITY=96, CHAT_LENGTH=124 };
enum Kind { SYSTEM=0, INPUT=1, MATCH=2, ALERT=3 };
struct Line {
    unsigned frame=0,kind=0;
    char text[CHAT_LENGTH]={};
};
static inline unsigned b(const std::uint8_t* ram,unsigned a){return ram[(a&65535u)^1u];}
static inline unsigned w(const std::uint8_t* ram,unsigned a){return (b(ram,a)<<8)|b(ram,a+1u);}
static inline unsigned bc(unsigned m){unsigned n=0;for(;m;m>>=1u)n+=m&1u;return n;}
static inline unsigned relative(std::uint8_t raw,bool faceRight) {
    const bool l=(raw&1u)!=0,r=(raw&2u)!=0,u=(raw&4u)!=0,d=(raw&8u)!=0;
    if((l&&r)||(u&&d))return 0u;
    const bool f=faceRight?r:l,back=faceRight?l:r;
    if(d&&f)return 3u; if(d&&back)return 1u;
    if(d)return 2u;if(f)return 6u;if(back)return 4u;
    if(u&&f)return 9u;if(u&&back)return 7u;
    if(u)return 8u;return 5u; // number-pad notation
}
static inline const char* dirName(unsigned n){
    switch(n){case 1:return "DB";case 2:return "D";case 3:return "DF";
    case 4:return "B";case 5:return "N";case 6:return "F";
    case 7:return "UB";case 8:return "U";case 9:return "UF";
    default:return "?";}
}
static inline const char* physicalDirection(unsigned raw){
    switch(raw&15u){
    case 0:return "NEUTRAL";case 1:return "RIGHT";case 2:return "LEFT";
    case 4:return "DOWN";case 5:return "DOWN+RIGHT";case 6:return "DOWN+LEFT";
    case 8:return "UP";case 9:return "UP+RIGHT";case 10:return "UP+LEFT";
    default:return "MULTI";
    }
}
static inline void chord(unsigned mask,char* out,unsigned size){
    static const char* words[6]={"LP","MP","HP","LK","MK","HK"};
    if(!size)return;
    out[0]=0;unsigned n=0;
    for(unsigned i=0;i<6u;++i)if(mask&(1u<<i)){
        int wrote=std::snprintf(out+n,size-n,"%s%s",n?"+":"",words[i]);
        if(wrote<0 || unsigned(wrote)>=size-n)break;
        n+=unsigned(wrote);
    }
    if(!n)std::snprintf(out,size,"-");
}
static inline bool sequence(const XM87137Input::History& h,bool right,
                            const unsigned* required,unsigned length,
                            unsigned now,unsigned window=40u){
    if(!length)return false;
    unsigned need=length;
    unsigned prior=now;
    unsigned last=~0u;
    for(unsigned ago=0;ago<h.count;++ago){
        XM87137Input::Sample x;
        if(!h.at(ago,x)||x.frame>now)continue;
        if(now-x.frame>window)break;
        const unsigned dir=relative(x.direction,right);
        if(dir==last)continue;last=dir;
        if(dir==required[need-1u]){
            if(need==length && now-x.frame>7u)return false;
            if(need<length && prior-x.frame>12u)return false;
            prior=x.frame;
            if(--need==0)return true;
        }
    }
    return false;
}
static inline const char* motion(const XM87137Input::History& h,bool right,unsigned now){
    static const unsigned qcf[]={2,3,6},qcb[]={2,1,4};
    static const unsigned dpf[]={6,2,3},dpb[]={4,2,1};
    static const unsigned hcf[]={4,1,2,3,6},hcb[]={6,3,2,1,4};
    // longest motions first; one readable best candidate per attack chord.
    if(sequence(h,right,hcf,5u,now))return "HCF";
    if(sequence(h,right,hcb,5u,now))return "HCB";
    if(sequence(h,right,dpf,3u,now))return "DPF";
    if(sequence(h,right,dpb,3u,now))return "DPB";
    if(sequence(h,right,qcf,3u,now))return "QCF";
    if(sequence(h,right,qcb,3u,now))return "QCB";
    return nullptr;
}
// Stage 87.141 user-confirmed COMMAND LIST, not an ROM action/dispatch table.
// Ryu and Ken native fighter IDs from observed point/reserve slots.
// First super's point facing controls relative input directions.
struct ListedSuper {unsigned fighter;bool kicks;bool back;const char* name;};
static inline const ListedSuper* known(unsigned id,unsigned index){
    static const ListedSuper moves[]={
      {0x12u,false,false,"RYU SHINKUU HADOKEN"},
      {0x12u,true,true,"RYU SHINKUU TATSUMAKI"},
      {0x14u,false,false,"KEN SHORYUREPPA"},
      {0x14u,true,false,"KEN SHINRYUKEN"}
    };
    unsigned found=0;
    for(unsigned i=0;i<sizeof(moves)/sizeof(moves[0]);++i)
        if(moves[i].fighter==id && found++==index)return &moves[i];
    return nullptr;
}
static inline const ListedSuper* matchListed(unsigned fighter,
        const XM87137Input::History& h,bool facingRight,unsigned now,unsigned chordMask){
    static const unsigned qcf[]={2,3,6},qcb[]={2,1,4};
    for(unsigned i=0;i<2u;++i){
        const ListedSuper* m=known(fighter,i);
        if(!m)continue;
        const unsigned mask=m->kicks?(chordMask>>3u)&7u:chordMask&7u;
        if(bc(mask)<2u)continue;
        // Anchor to the CURRENT final cardinal direction. Do not reach into
        // an earlier QCB/QCF while the latest stick position is DF/DB from
        // another command; that was the confusing Ryu KK diagnostic.
        const unsigned final=relative(h.direction,facingRight);
        if(final!=(m->back?4u:6u))continue;
        if(sequence(h,facingRight,m->back?qcb:qcf,3u,now,40u))
            return m;
    }
    return nullptr;
}
struct State {
    Line lines[CHAT_CAPACITY]={};
    unsigned write=0,count=0,lastFrame=~0u;
    unsigned lastMsgFrame=0, chatMessages=0, lastPlayerMask[2]={0,0};
    unsigned lastDirectionMask[2]={0,0};
    unsigned lastAttackFrame[2]={~0u,~0u},lastMoveFrame[2]={~0u,~0u};
    bool open=false,ready[2]={false,false},candidate[2]={false,false};
    unsigned oldA0[2]={0,0},oldStock[2]={0,0},openAt[2]={0,0};
    unsigned lastCharacter[2]={0,0};
    XM87137Input::History keys[2];
    std::FILE* file=nullptr;
    bool checkedFile=false;
    ~State(){if(file){std::fflush(file);std::fclose(file);}}
    void log(unsigned now,Kind type,const char* text){
        Line& l=lines[write];l.frame=now;l.kind=type;
        std::snprintf(l.text,sizeof(l.text),"%s",text?text:"");
        write=(write+1u)%CHAT_CAPACITY;
        if(count<CHAT_CAPACITY)++count;
        if(!checkedFile){
            checkedFile=true;
            const char* path=std::getenv("XM87140_CHAT_LOG");
            if(path&&*path){
                file=std::fopen(path,"ab");
                if(file)std::fputs("frame\tkind\tmessage\n",file);
            }
        }
        if(file){
            // Text is generated internally; strip tabs/newlines defensively.
            char safe[CHAT_LENGTH];std::snprintf(safe,sizeof(safe),"%s",l.text);
            for(char* q=safe;*q;++q)if(*q=='\t'||*q=='\n'||*q=='\r')*q=' ';
            std::fprintf(file,"%u\t%u\t%s\n",now,unsigned(type),safe);
            std::fflush(file);
        }
        ++chatMessages;lastMsgFrame=now;
    }
    void toggle(){open=!open;}
    unsigned visibleLines(unsigned frame)const{
        if(open)return count<12u?count:12u;
        unsigned n=0;
        for(unsigned i=0;i<count&&i<5u;++i){
            const Line& l=lines[(write+CHAT_CAPACITY-1u-i)%CHAT_CAPACITY];
            if(frame>=l.frame&&frame-l.frame<180u)++n;
            else break;
        }
        return n;
    }
    static std::uint8_t nativeDirections(unsigned raw){
        // CPS2 port order RIGHT,LEFT,DOWN,UP, verified in d_cps2.cpp.
        std::uint8_t mapped=0;
        if(raw&1u)mapped|=2u;  // right
        if(raw&2u)mapped|=1u;  // left
        if(raw&4u)mapped|=8u;  // down
        if(raw&8u)mapped|=4u;  // up
        return mapped;
    }
    void updateKey(unsigned side,unsigned frame,unsigned dir,unsigned buttons){
        const std::uint8_t decoded=nativeDirections(dir);
        for(unsigned bit=0;bit<4u;++bit)
            keys[side].mapped(bit,(decoded&(1u<<bit))!=0,frame);
        for(unsigned bit=0;bit<6u;++bit)
            keys[side].mapped(0x80u+bit,(buttons&(1u<<bit))!=0,frame);
    }
    void frame(const std::uint8_t* ram,unsigned frame,
               unsigned p1directions,unsigned p2directions,
               unsigned p1buttons,unsigned p2buttons){
        if(!ram||lastFrame==frame)return;
        if(lastFrame!=~0u && frame<lastFrame){
            ready[0]=ready[1]=candidate[0]=candidate[1]=false;
            keys[0].clear();keys[1].clear();
        }
        lastFrame=frame;
        const bool combat=b(ram,0x5000u)==8u;
        if(!combat){
            ready[0]=ready[1]=candidate[0]=candidate[1]=false;
            keys[0].clear();keys[1].clear();
            return;
        }
        const unsigned directions[2]={p1directions,p2directions};
        const unsigned buttons[2]={p1buttons,p2buttons};
        for(unsigned side=0;side<2u;++side){
            updateKey(side,frame,directions[side],buttons[side]);
            const unsigned point=0x4000u+side*0x400u;
            const unsigned reserve=0x4800u+side*0x400u;
            const unsigned a0=b(ram,point+0xA0u); // ACTION candidate, NOT super definition
            const unsigned stock=b(ram,0x4214u+side*0x400u);
            const unsigned fighter=w(ram,point+0x52u),bench=w(ram,reserve+0x52u);
            if(!ready[side]||fighter!=lastCharacter[side]){
                ready[side]=true;oldA0[side]=a0;oldStock[side]=stock;
                lastCharacter[side]=fighter;candidate[side]=false;
            }
            const bool actionEdge=a0==0x2Au&&oldA0[side]!=0x2Au;
            const bool meterDebit=stock<oldStock[side]&&stock<=3u&&oldStock[side]<=3u;
            // Never label 2A as VERIFIED super. Intro suppression in 87.63
            // gives useful correlated evidence on Stage 05 but not universal.
            if(!candidate[side] && (actionEdge||meterDebit)){
                candidate[side]=true;openAt[side]=frame;
                char m[CHAT_LENGTH];
                std::snprintf(m,sizeof(m),"P%u FIRST SUPER? point=%02X a0=%02X meter %u>%u",
                    side+1u,fighter,a0,oldStock[side],stock);
                log(frame,ALERT,m);
                std::snprintf(m,sizeof(m),"P%u RESERVE %02X act=%04X anim=%04X script=%08X",
                    side+1u,bench,w(ram,reserve+0x60u),w(ram,reserve+0x42u),
                    unsigned((std::uint32_t(w(ram,reserve+0x34u))<<16u)|
                            w(ram,reserve+0x36u)));
                log(frame,SYSTEM,m);
                // These are user-confirmed move commands, NOT ROM script IDs.
                for(unsigned mi=0;mi<2u;++mi){
                    const ListedSuper* listed=known(bench,mi);
                    if(!listed)break;
                    std::snprintf(m,sizeof(m),"P%u RESERVE %02X %s = %s+%s [INPUT MAP]",
                        side+1u,bench,listed->name,listed->back?"QCB":"QCF",listed->kicks?"KK":"PP");
                    log(frame,SYSTEM,m);
                }
                if(!known(bench,0u))
                    log(frame,SYSTEM,"NO COMMAND MAP FOR RESERVE ID; ROM MOVE IDS NOT YET VERIFIED");
            }
            const unsigned pressed=buttons[side] & ~lastPlayerMask[side];
            const unsigned rawD=directions[side];
            const bool directionEdge=rawD!=lastDirectionMask[side];
            if(candidate[side]){
                if(directionEdge){
                    char text[CHAT_LENGTH];
                    const bool facingRight=(b(ram,point+0x4Bu)&1u)!=0u; // 87.142 trace verified: 1=RIGHT
                    const unsigned rel=relative(keys[side].direction,facingRight);
                    std::snprintf(text,sizeof(text),"P%u DIR %s abs=%s raw=%X face=%s",
                        side+1u,dirName(rel),physicalDirection(rawD),rawD,
                        facingRight?"RIGHT":"LEFT");
                    log(frame,INPUT,text);
                }
                if(pressed){
                    char text[CHAT_LENGTH],held[48];
                    chord(buttons[side],held,sizeof(held));
                    const bool facingRight=(b(ram,point+0x4Bu)&1u)!=0u; // 87.142 trace verified: 1=RIGHT
                    const unsigned rel=relative(keys[side].direction,facingRight);
                    std::snprintf(text,sizeof(text),
                        "P%u INPUT %s dir=%s(%u) abs=%s raw=%X superA0=%02X",
                        side+1u,held,dirName(rel),rel,physicalDirection(rawD),rawD,a0);
                    log(frame,INPUT,text);
                    lastAttackFrame[side]=frame;
                }
                // A button may precede final F/B directional edge by up to
                // four frames; keep the chord while waiting for completion.
                if((pressed || directionEdge) && lastAttackFrame[side]!=~0u &&
                   frame>=lastAttackFrame[side] && frame-lastAttackFrame[side]<=4u &&
                   (bc(buttons[side]&7u)>=2u || bc((buttons[side]>>3u)&7u)>=2u)){
                    const bool facingRight=(b(ram,point+0x4Bu)&1u)!=0u; // 87.142 trace verified: 1=RIGHT
                    const ListedSuper* found=matchListed(bench,keys[side],
                                          facingRight,frame,buttons[side]);
                    char text[CHAT_LENGTH];
                    if(found && lastMoveFrame[side]!=lastAttackFrame[side]){
                        std::snprintf(text,sizeof(text),
                            "P%u RESERVE %02X MATCH %s (%s+%s) [INPUT ONLY]",
                            side+1u,bench,found->name,
                            found->back?"QCB":"QCF",found->kicks?"KK":"PP");
                        log(frame,MATCH,text);
                        lastMoveFrame[side]=lastAttackFrame[side];
                    }else if(pressed && !found){
                        const char* got=motion(keys[side],facingRight,frame);
                        std::snprintf(text,sizeof(text),
                            "P%u RESERVE %02X INPUT%s%s; NO LISTED SUPER MATCH",
                            side+1u,bench,got?" ":"",got?got:"");
                        log(frame,SYSTEM,text);
                    }
                }
                // Candidate gate expires by native action change, but keep a
                // conservative window for move-specific attack scripts we
                // have not reverse-engineered. No gameplay DHC is triggered.
                if((frame-openAt[side]>200u)||
                   (frame-openAt[side]>15u && a0!=0x2Au &&
                    a0!=0x32u && a0!=0x06u)){
                    candidate[side]=false;
                    char text[CHAT_LENGTH];
                    std::snprintf(text,sizeof(text),"P%u SUPER WINDOW END candidate only",side+1u);
                    log(frame,SYSTEM,text);
                }
            }
            lastPlayerMask[side]=buttons[side];
            lastDirectionMask[side]=directions[side];
            oldA0[side]=a0;oldStock[side]=stock;
        }
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87140Chat
#endif
