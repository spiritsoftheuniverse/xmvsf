#ifndef XMVSF_STAGE87_140_SUPER_CHAT_H
#define XMVSF_STAGE87_140_SUPER_CHAT_H
// 87.140 experimental DHC input parser diagnostics. Read-only.
// Logs native CPS2 controller bits, the FIRST SUPER *candidate* action, the
// real point/reserve IDs+script, and matches generic motion/chord patterns.
// These patterns are NOT an invented fighter-specific verified super list.
// The ROM-specific DHC super registry remains unpopulated until demonstrated.
#include "../../intf/input/sdl/xmvsf_stage87_137_dhc_input_history.h"
#include "../../burn/drv/capcom/xmvsf_stage87_143_super_queue.h" // prebuilt match-start descriptor queue
#include "../../burn/drv/capcom/xmvsf_stage87_177_dhc_punch_chords.h" // 87.177 exact 2P/3P DHC-only gate
#include "../../burn/drv/capcom/xmvsf_stage87_184_super_action_lifetime.h" // ROM action beginning, stages, exact exit
#include "../../burn/drv/capcom/xmvsf_stage87_185_action_gate.h" // recognize all original action stages, separate native execution
#include "../../burn/drv/capcom/xmvsf_stage87_189_native_super_family.h" // ROM-owned generic 2E lifecycle
#include "../../burn/drv/capcom/xmvsf_stage87_192_reserve_super_ownership.h" // captured reserve input
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
    if(d&&f)return 3u;
    if(d&&back)return 1u;
    if(d)return 2u;
    if(f)return 6u;
    if(back)return 4u;
    if(u&&f)return 9u;
    if(u&&back)return 7u;
    if(u)return 8u;
    return 5u; // number-pad notation
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
        if(dir==last)continue;
        last=dir;
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
    static const unsigned dqcf[]={2,3,6,2,3,6},dqcb[]={2,1,4,2,1,4};
    // Diagnose ALL standard special/super motion families, not only the
    // presently ROM-verified Ken/Ryu registry. This never injects inputs.
    if(sequence(h,right,dqcf,6u,now,60u))return "2QCF";
    if(sequence(h,right,dqcb,6u,now,60u))return "2QCB";
    // longest motions first; one readable best candidate per attack chord.
    if(sequence(h,right,hcf,5u,now))return "HCF";
    if(sequence(h,right,hcb,5u,now))return "HCB";
    if(sequence(h,right,dpf,3u,now))return "DPF";
    if(sequence(h,right,dpb,3u,now))return "DPB";
    if(sequence(h,right,qcf,3u,now))return "QCF";
    if(sequence(h,right,qcb,3u,now))return "QCB";
    return nullptr;
}
// One authoritative per-fighter command catalog: the match-start queue
// populates from this source, and the input matcher selects its entries.
using XM87143Queue::Known;
typedef XM87143Queue::Known ListedSuper;
static inline const ListedSuper* known(unsigned id,unsigned index){
    return XM87143Queue::definition(id,index);
}
static inline const ListedSuper* matchListed(unsigned fighter,
        const XM87137Input::History& h,bool facingRight,unsigned now,
        unsigned chordMask,bool allowTriplePunch=false){
    static const unsigned qcf[]={2,3,6},qcb[]={2,1,4};
    for(unsigned i=0;i<4u;++i){
        const ListedSuper* m=known(fighter,i);
        if(!m)continue;
        // 87.177 DHC host gesture: all three punch buttons OR any exact
        // punch pair is valid. Original 3P is dash outside DHC; the
        // dedicated pre-CPU input shield prevents that dash on handoff.
        // Preserve exact TWO kicks; never treat 3K as superjump or accept
        // mixed P+K. The native Ken 68000 input still receives 2P.
        if(!XM87177Chords::valid(m->kicks,chordMask,
               allowTriplePunch && m->fighter==0x14u))continue;
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
    unsigned lastNamedMatchFrame[2]={~0u,~0u};
    const ListedSuper* lastNamedMatch[2]={nullptr,nullptr};
    unsigned lastReserveMatchId[2]={0u,0u};
    bool open=false,ready[2]={false,false},candidate[2]={false,false};
    unsigned oldA0[2]={0,0},oldStock[2]={0,0},openAt[2]={0,0};
    unsigned lastCharacter[2]={0,0};
    XM87137Input::History keys[2];
    XM87184Life::Tracker shinkuu[2];
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
               unsigned p1buttons,unsigned p2buttons,
               unsigned nativeSuperScriptMask=0u,
               const std::uint8_t*rom=nullptr,unsigned romSize=0u){
        if(!ram||lastFrame==frame)return;
        if(lastFrame!=~0u && frame<lastFrame){
            ready[0]=ready[1]=candidate[0]=candidate[1]=false;
            keys[0].clear();keys[1].clear();
            shinkuu[0].reset();shinkuu[1].reset();
            XM87189Family::state().reset();
            XM87192Owner::state().reset();
        }
        lastFrame=frame;
        const bool combat=b(ram,0x5000u)==8u;
        if(!combat){
            ready[0]=ready[1]=candidate[0]=candidate[1]=false;
            keys[0].clear();keys[1].clear();
            shinkuu[0].reset();shinkuu[1].reset();
            XM87189Family::state().reset();
            XM87192Owner::state().reset();
            XM87143Queue::state().discard(0u);
            XM87143Queue::state().discard(1u);
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
                shinkuu[side].reset();
                XM87143Queue::state().discard(side);
                XM87192Owner::state().clear(side);
            }
            // 87.184: original ROM-owned action lifetime, independent
            // of elapsed frame counts or the short tag-controller timer.
            // Original Shinkuu action: 2E08/step02 root 0D3070,
            // 2E08/step04 active 0D30B0..0D30E0, then
            // 2E0A/step06 recovery 0D30F4..0D3194.
            // A DIFFERENT action is the exact lifetime end.
            const XM87184Life::Sample shinkuuNow={
                fighter,w(ram,point+0x60u),w(ram,point+0xA0u),
                w(ram,point+0x08u),
                (w(ram,point+0x34u)<<16u)|w(ram,point+0x36u),
                b(ram,point+0xD6u)};
            const XM87184Life::Event lifeEvent=
                side==0u&&fighter==0x12u?
                  shinkuu[side].update(frame,shinkuuNow):
                  XM87184Life::NOTHING;
            // The original 2E ROM root / native step controller is the
            // shared inlet. No move-specific air action checks here.
            auto& nativeFamily=XM87189Family::state().side[side];
            const XM87189Family::Event familyEvent=
                nativeFamily.update(frame,shinkuuNow,rom,romSize);
            if(familyEvent==XM87189Family::ENTER){
                char msg[CHAT_LENGTH];
                std::snprintf(msg,sizeof(msg),
                    "DHC_189_NATIVE_2E_ROOT_ROM fighter=%02X action=%04X script=%06X",
                    fighter,nativeFamily.rootAction,nativeFamily.rootScript);
                log(frame,MATCH,msg);
                // Observed native ROM roots are learned from the actor
                // that REALLY owns the point controller. This is a
                // candidate until the reserve's own input is proven.
                const char* owned=std::getenv("XM87192_TEST_RESERVE_OWNERSHIP");
                if(owned && std::strcmp(owned,"1")==0)
                    XM87192Owner::state().observeNatural(
                        side,frame,shinkuuNow,rom,romSize);
            }else if(familyEvent==XM87189Family::REPEATED_ROOT_SCRIPT){
                log(frame,MATCH,
                    "DHC_190_NATIVE_2E_ROOT_SCRIPT_REWIND_STILL_SUPER_ATTACK");
            }else if(familyEvent==XM87189Family::CONTINUATION_BEGIN ||
                     familyEvent==XM87189Family::CONTINUATION_STAGE_CHANGE){
                char msg[CHAT_LENGTH];
                std::snprintf(msg,sizeof(msg),
                    "DHC_190_ROM_2E_SUBACTION_CONTINUES_SUPER action=%04X step=%u root=%04X",
                    shinkuuNow.action,shinkuuNow.step,nativeFamily.rootAction);
                log(frame,MATCH,msg);
            }else if(familyEvent==XM87189Family::ACTION_EXIT){
                log(frame,SYSTEM,"DHC_190_ROM_2E_SUPER_FAMILY_TRUE_NATIVE_EXIT");
            }
            if(lifeEvent==XM87184Life::ENTER)
                log(frame,MATCH,"DHC_184_RYU_SHINKUU_ACTION_ENTER_2E08_0D3070");
            else if(lifeEvent==XM87184Life::ATTACK_BEGIN)
                log(frame,MATCH,"DHC_184_RYU_SHINKUU_ATTACK_PHASE_2E08_STEP04");
            else if(lifeEvent==XM87184Life::RECOVERY_BEGIN)
                log(frame,MATCH,"DHC_184_RYU_SHINKUU_RECOVERY_PHASE_2E0A_STEP06");
            else if(lifeEvent==XM87184Life::ACTION_EXIT)
                log(frame,SYSTEM,"DHC_184_RYU_SHINKUU_ACTION_EXIT_NATIVE");
            if(side==0u && fighter==0x12u && candidate[side] &&
               !shinkuu[side].live && !nativeFamily.live){
                candidate[side]=false;
                XM87143Queue::state().discard(side);
                log(frame,SYSTEM,
                    "DHC_184_CANDIDATE_CLOSED_AT_NATIVE_ACTION_EXIT");
            }
            const bool actionEdge=a0==0x2Au&&oldA0[side]!=0x2Au;
            const bool meterDebit=stock<oldStock[side]&&stock<=3u&&oldStock[side]<=3u;
            // Never label 2A as VERIFIED super. Intro suppression in 87.63
            // gives useful correlated evidence on Stage 05 but not universal.
            // XM87.142: For Ryu/Ken we have actual native 68000 action
            // table+script resolution. Ignore the old +A0==2A proxy here:
            // it also fires for NORMAL TAG (captured at 87.141 frame 2978).
            // Other fighters retain the openly provisional fallback until
            // their own ROM script mapping has been captured and verified.
            const bool mappedNative= fighter==0x12u || fighter==0x14u;
            const bool nativeStart=(nativeSuperScriptMask&(1u<<side))!=0u;
            const bool familyRoot=familyEvent==XM87189Family::ENTER;
            const bool openedNow=!candidate[side] &&
                (fighter==0x12u ?
                    ((nativeStart && lifeEvent==XM87184Life::ENTER) ||
                     familyRoot) :
                    (mappedNative?(nativeStart||familyRoot):
                                  (familyRoot||actionEdge||meterDebit)));
            if(openedNow){
                candidate[side]=true;openAt[side]=frame;
                // The first-super dispatch may occur while an old motion or
                // attack chord is still buffered. A DHC command must begin
                // AFTER this super has actually started, not on the same
                // ROM entry frame. The 87.142 capture caught this at 2964.
                keys[side].clear();
                lastAttackFrame[side]=~0u;lastMoveFrame[side]=~0u;
                lastNamedMatchFrame[side]=~0u;
                lastNamedMatch[side]=nullptr;
                char m[CHAT_LENGTH];
                std::snprintf(m,sizeof(m),"P%u FIRST SUPER? point=%02X a0=%02X meter %u>%u",
                    side+1u,fighter,a0,oldStock[side],stock);
                log(frame,ALERT,m);
                if(nativeStart)
                    log(frame,MATCH,"NATIVE 68000 ROM-TABLE SCRIPT VERIFIED (NOT YET DHC EXECUTION)");
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
            if(candidate[side] && !openedNow){
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
                   (buttons[side]&0x3Fu)!=0u){
                    const bool facingRight=(b(ram,point+0x4Bu)&1u)!=0u; // 87.142 trace verified: 1=RIGHT
                    // Do not confuse whole-action lifetime with the
                    // native tag execution window. Startup is tracked,
                    // but it is not yet proven safe for controller 8.
                    // 87.184 can TEST the genuine 2E0A recovery stage
                    // only before its terminal 0D3194 script.
                    const bool playableRyuKen=side==0u &&
                        fighter==0x12u && bench==0x14u;
                    // 87.184 conflated *recognizing the user's command*
                    // with *whether the native controller 8 can safely
                    // tag right now*. This silently rejected 3P/2P
                    // gestures during the real 2E08 startup.
                    // Recognize intent for the whole native action,
                    // excluding terminal recovery, then apply the
                    // independent safety gate in XM87144.
                    const bool nativeWindow=playableRyuKen &&
                        ((shinkuu[side].live &&
                          XM87185Gate::canRecognize(shinkuuNow)) ||
                         nativeFamily.recognizes());
                    // 87.192 reserve-owned gesture, no Ken move list.
                    // Only the incoming actor's ORIGINAL ROM input
                    // interpreter may ultimately classify it as super.
                    const char* ownedSetting=std::getenv(
                        "XM87192_TEST_RESERVE_OWNERSHIP");
                    const bool ownedMode=ownedSetting &&
                        std::strcmp(ownedSetting,"1")==0;
                    const bool ownedReady=ownedMode && playableRyuKen &&
                        nativeWindow && pressed &&
                        XM87192Owner::state().propose(
                            side,bench,frame,
                            XM87192Owner::parse(
                                motion(keys[side],facingRight,frame)),
                            buttons[side]);
                    const ListedSuper* found=ownedMode?nullptr:
                        ((!playableRyuKen || nativeWindow)?
                        matchListed(bench,keys[side],facingRight,frame,
                                    buttons[side],nativeWindow):nullptr);
                    char text[CHAT_LENGTH];
                    if(ownedReady){
                        const auto& proposal=XM87192Owner::state().pending[side];
                        std::snprintf(text,sizeof(text),
                           "DHC_192_RESERVE_OWNS_INPUT id=%02X motion=%u buttons=%02X ROM_SUPER_PENDING",
                            bench,unsigned(proposal.motion),proposal.buttonMask);
                        log(frame,MATCH,text);
                        // Capture the inactive actor's actual native state
                        // BEFORE our tag writer runs. We cannot assume its
                        // 68000 input parser is active just because the
                        // actor exists in reserve RAM.
                        std::snprintf(text,sizeof(text),
                            "DHC_192_INACTIVE_PRETAG act=%04X mode=%04X step=%04X D6=%02X script=%06X",
                            w(ram,reserve+0x60u),w(ram,reserve+0xA0u),
                            w(ram,reserve+0x08u),b(ram,reserve+0xD6u),
                            unsigned((std::uint32_t(w(ram,reserve+0x34u))<<16u)|
                                      w(ram,reserve+0x36u)));
                        log(frame,SYSTEM,text);
                        lastMoveFrame[side]=lastAttackFrame[side];
                    }
                    if(found && playableRyuKen &&
                       (XM87184Life::identify(shinkuuNow)==XM87184Life::STARTUP ||
                        nativeFamily.phase==XM87189Family::STARTUP))
                        log(frame,SYSTEM,
                            "DHC_185_STARTUP_INPUT_RECOGNIZED_NATIVE_TAG_NOT_YET_VERIFIED");
                    if(found && lastMoveFrame[side]!=lastAttackFrame[side]){
                        // Pressing the third kick a frame after LP+MP/ LK+MK
                        // is still one chord (frame 4674 and 4675 in trace).
                        const bool repeat=lastNamedMatch[side]==found &&
                            lastNamedMatchFrame[side]!=~0u &&
                            frame>=lastNamedMatchFrame[side] &&
                            frame-lastNamedMatchFrame[side]<12u;
                        if(!repeat){
                            const XM87143Queue::Entry* queued=
                                XM87143Queue::state().select(
                                    side,bench,found->command,frame);
                            // The prepared queue, not a synthesized action,
                            // supplies DHC's selected super. Never dispatch
                            // a native action through this diagnostic path.
                            std::snprintf(text,sizeof(text),
                                "P%u RESERVE %02X MATCH %s (%s+%s) [INPUT ONLY]",
                                side+1u,bench,found->name,
                                found->back?"QCB":"QCF",found->kicks?"KK":((buttons[side]&7u)==7u?"3P":"2P"));
                            log(frame,MATCH,text);
                            if(queued){
                                unsigned ready=0u;
                                for(unsigned k=0;k<queued->count;++k)
                                    if(queued->scripts[k].resolved)++ready;
                                std::snprintf(text,sizeof(text),
                                    "P%u DHC QUEUED %s: ROM script options=%u/%u; HANDOFF PENDING",
                                    side+1u,found->name,ready,queued->count);
                                log(frame,ready?MATCH:SYSTEM,text);
                            }else if(XM87143Queue::state().prepared){
                                log(frame,ALERT,"DHC SELECT REJECTED: NO MATCH-START QUEUE ENTRY");
                            }
                            lastNamedMatch[side]=found;
                            lastNamedMatchFrame[side]=frame;
                            lastReserveMatchId[side]=bench;
                        }
                        lastMoveFrame[side]=lastAttackFrame[side];
                    }else if(pressed && !found && !ownedReady){
                        // Record EVERY fresh attack button/chord (LP through HK,
                        // mixed P+K, and single/2/3-button chords). Unknown
                        // fighter motions are attempts, NOT authorization to
                        // drive a guessed native action or spend meter.
                        const char* got=motion(keys[side],facingRight,frame);
                        char held[48];chord(buttons[side],held,sizeof(held));
                        if(got){
                            std::snprintf(text,sizeof(text),
                                "P%u DHC INPUT ATTEMPT reserve=%02X %s+%s; NO VERIFIED SUPER MAP",
                                side+1u,bench,got,held);
                            log(frame,MATCH,text);
                        }else{
                            std::snprintf(text,sizeof(text),
                                "P%u DHC INPUT ATTEMPT reserve=%02X %s; NO MOTION/MAP",
                                side+1u,bench,held);
                            log(frame,SYSTEM,text);
                        }
                    }
                }
                // For Ryu, the exact 2E08->2E0A->normal action
                // lifetime above owns closure. Do NOT substitute an
                // arbitrary 200-frame chat timeout. Other fighters
                // retain the provisional legacy observation policy.
                if(fighter!=0x12u &&
                   ((frame-openAt[side]>200u)||
                    (frame-openAt[side]>15u && a0!=0x2Au &&
                     a0!=0x32u && a0!=0x06u))){
                    candidate[side]=false;
                    XM87143Queue::state().discard(side);
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
