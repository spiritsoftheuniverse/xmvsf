#ifndef XMVSF_STAGE87_137_DHC_COMMAND_QUEUE_H
#define XMVSF_STAGE87_137_DHC_COMMAND_QUEUE_H
// Stage87.137: DHC uses ONLY native-verified super commands belonging to
// the *reserve* fighter. No guessed character-move mappings are installed.
// This code parses forwarded controller history; it does NOT queue/replay
// physical input into the game or consume meter. Registry starts EMPTY.
#include "../stable/buildtree/src/intf/input/sdl/xmvsf_stage87_137_dhc_input_history.h"
#include <cstdint>
namespace XM87137Command {
enum Motion { NONE=0,QCF,QCB,DPF,DPB,HCF,HCB,DOUBLE_QCF,DOUBLE_QCB };
enum ButtonMode { EXACT=0, TWO_PUNCHES, TWO_KICKS, THREE_PUNCHES, THREE_KICKS };
struct Move {
    unsigned characterId=0,moveId=0;
    Motion motion=NONE;
    ButtonMode mode=EXACT;
    std::uint8_t required=0; // EXACT button set, LP=bit0 ... HK=bit5
    bool nativeVerified=false; // must be linked to ROM-accepted super script
};
struct Pattern {
    unsigned len=0;
    std::uint8_t dir[6]={};
};
enum Direction { INVALID=0, NEUTRAL=1, F=2, B=3, D=4, DF=5, DB=6, U=7, UF=8, UB=9 };
static inline std::uint8_t relative(std::uint8_t raw,bool rightFacing){
    const bool l=(raw&1u)!=0,r=(raw&2u)!=0,u=(raw&4u)!=0,d=(raw&8u)!=0;
    if((l&&r)||(u&&d))return INVALID;
    const bool f=rightFacing?r:l, b=rightFacing?l:r;
    if(d&&f)return DF;
    if(d&&b)return DB;
    if(u&&f)return UF;
    if(u&&b)return UB;
    if(f)return F;
    if(b)return B;
    if(d)return D;
    if(u)return U;
    return NEUTRAL;
}
static inline Pattern pattern(Motion m){
    Pattern p;
    switch(m){
    case QCF:        p.len=3;p.dir[0]=D; p.dir[1]=DF;p.dir[2]=F;break;
    case QCB:        p.len=3;p.dir[0]=D; p.dir[1]=DB;p.dir[2]=B;break;
    case DPF:        p.len=3;p.dir[0]=F; p.dir[1]=D; p.dir[2]=DF;break;
    case DPB:        p.len=3;p.dir[0]=B; p.dir[1]=D; p.dir[2]=DB;break;
    case HCF:        p.len=5;p.dir[0]=B; p.dir[1]=DB;p.dir[2]=D;p.dir[3]=DF;p.dir[4]=F;break;
    case HCB:        p.len=5;p.dir[0]=F; p.dir[1]=DF;p.dir[2]=D;p.dir[3]=DB;p.dir[4]=B;break;
    case DOUBLE_QCF:p.len=6;p.dir[0]=D;p.dir[1]=DF;p.dir[2]=F;
                    p.dir[3]=D;p.dir[4]=DF;p.dir[5]=F;break;
    case DOUBLE_QCB:p.len=6;p.dir[0]=D;p.dir[1]=DB;p.dir[2]=B;
                    p.dir[3]=D;p.dir[4]=DB;p.dir[5]=B;break;
    default:break;
    }
    return p;
}
static inline unsigned bits(std::uint8_t b){
    unsigned n=0;while(b){n+=b&1u;b=std::uint8_t(b>>1u);}return n;
}
static inline bool buttons(const XM87137Input::Sample& s,const Move& m){
    const unsigned mask=m.mode==TWO_PUNCHES || m.mode==THREE_PUNCHES?0x07u:
      m.mode==TWO_KICKS || m.mode==THREE_KICKS?0x38u:m.required;
    if(!(s.pressed&mask))return false; // no retrigger from held keys
    const unsigned amount=bits(std::uint8_t(s.buttons&mask));
    if(m.mode==TWO_PUNCHES||m.mode==TWO_KICKS)return amount>=2u;
    if(m.mode==THREE_PUNCHES||m.mode==THREE_KICKS)return amount>=3u;
    return mask!=0 && (s.buttons&mask)==mask;
}
static inline bool motion(const XM87137Input::History& h,Motion m,
                          bool facingRight,unsigned now,unsigned endGap=6u,
                          unsigned stepGap=10u,unsigned window=42u){
    if(m==NONE)return true;
    const Pattern p=pattern(m);
    if(!p.len || !h.count)return false;
    unsigned match=p.len, previous=now;
    std::uint8_t last=INVALID;
    for(unsigned ago=0;ago<h.count;++ago){
        XM87137Input::Sample s;
        if(!h.at(ago,s)||s.frame>now)continue;
        if(now-s.frame>window)break;
        const std::uint8_t dir=relative(s.direction,facingRight);
        if(dir==last)continue;
        last=dir;
        if(dir==p.dir[match-1u]){
            if(match==p.len && now-s.frame>endGap)return false;
            if(match<p.len && previous-s.frame>stepGap)return false;
            previous=s.frame;
            --match;
            if(!match)return true;
        }
    }
    return false;
}
struct Registry {
    Move known[96]={};
    unsigned count=0;
    bool add(Move m){
        if(!m.nativeVerified || !m.moveId || m.characterId>0x40u ||
           (m.characterId&1u) || count>=96u)return false;
        if(m.motion!=NONE && !pattern(m.motion).len)return false;
        if(m.mode==EXACT && !m.required)return false;
        for(unsigned i=0;i<count;++i){
            if(known[i].characterId==m.characterId && known[i].moveId==m.moveId)
                return false;
        }
        known[count++]=m;return true;
    }
    // 0 denotes "no unambiguous verified reserve super". No guessed fallback.
    unsigned resolve(const XM87137Input::History& h,unsigned reserveId,
                     bool facingRight)const{
        XM87137Input::Sample newest;
        if(!h.at(0,newest) || !newest.pressed)return 0u;
        unsigned found=0;
        for(unsigned i=0;i<count;++i){
            const Move& m=known[i];
            if(m.characterId!=reserveId || !buttons(newest,m) ||
               !motion(h,m.motion,facingRight,newest.frame))continue;
            if(found && found!=m.moveId)return 0u; // ambiguous: fail closed
            found=m.moveId;
        }
        return found;
    }
};
struct Queue {
    unsigned lastReadFrame[2]={~0u,~0u};
    bool consumed[2]={false,false};
    void reset(unsigned side){
        if(side<2u){lastReadFrame[side]=~0u;consumed[side]=false;}
    }
    unsigned poll(unsigned side,bool outgoingNativeSuperVerified,
                  unsigned reserveId,bool facingRight,
                  const XM87137Input::History& hist,const Registry& registry){
        if(side>=2u||consumed[side]||!outgoingNativeSuperVerified)return 0u;
        XM87137Input::Sample latest;
        if(!hist.at(0,latest)||latest.frame==lastReadFrame[side])return 0u;
        lastReadFrame[side]=latest.frame;
        const unsigned move=registry.resolve(hist,reserveId,facingRight);
        if(move)consumed[side]=true;
        return move;
    }
};
} // namespace XM87137Command
#endif
