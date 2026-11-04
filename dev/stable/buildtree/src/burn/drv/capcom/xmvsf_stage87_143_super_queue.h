#ifndef XMVSF_STAGE87_143_SUPER_QUEUE_H
#define XMVSF_STAGE87_143_SUPER_QUEUE_H
// Stage 87.143: prepare every KNOWN super for both selected fighters at the
// start of combat, then select an entry (not a proxy proposal) at DHC time.
// This is a read-only preparation/selection stage. ROM dispatch and tag-out/
// tag-in must be invoked only after validating their native calling context.
// Never mistake a resolved script for a callable 68000 function: the caller
// and game-state setup are not captured by a ROM pointer alone.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
namespace XM87143Queue {
enum Command { QCF_PP=1, QCB_KK=2, QCF_KK=3 };
struct Known {
    unsigned fighter;
    Command command;
    const char* name;
    bool kicks,back;
    unsigned actions[2],selectors[2],expected[2];
    unsigned alternatives;
};
// Input assignments supplied by user; action-script associations are only
// proven where an exact active script was observed, not yet callable DHC
// entry points. Ken's two 2C variants must remain separate candidates.
static inline const Known* definition(unsigned id,unsigned index){
    static const Known entries[]={
        {0x12u,QCF_PP,"RYU SHINKUU HADOKEN",false,false,
           {0x2Eu,0u},{0x08u,0u},{0xD3070u,0u},1u},
        {0x12u,QCB_KK,"RYU SHINKUU TATSUMAKI",true,true,
           {0u,0u},{0u,0u},{0u,0u},0u},
        {0x14u,QCF_PP,"KEN SHORYUREPPA",false,false,
           {0x2Cu,0x2Cu},{0x00u,0x04u},{0xD5C46u,0xD5E2Eu},2u},
        {0x14u,QCF_KK,"KEN SHINRYUKEN",true,false,
           {0x2Eu,0u},{0x00u,0u},{0xD6722u,0u},1u}
    };
    unsigned seen=0;
    for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);++i){
        if(entries[i].fighter==id){
            if(seen==index)return &entries[i];
            ++seen;
        }
    }
    return nullptr;
}
struct Script {
    unsigned action=0,selector=0,script=0,animation=0;
    bool resolved=false;
};
struct Entry {
    const Known* known=nullptr;
    Script scripts[2];
    unsigned count=0;
    bool occupied=false;
};
struct MatchQueue {
    bool prepared=false;
    unsigned roster[2][2]={{0u,0u},{0u,0u}};
    Entry supers[2][2][4];
    unsigned generation=0,initializedAt=0;
    static bool idValid(unsigned id){return id<=0x40u && (id&1u)==0;}
    void reset(){
        prepared=false;initializedAt=0;
        for(unsigned s=0;s<2u;++s)for(unsigned c=0;c<2u;++c){
            roster[s][c]=0u;
            for(unsigned k=0;k<4u;++k)supers[s][c][k]=Entry();
        }
    }
    // Called every combat frame, but rebuilds ONLY when selected roster
    // changes (not on normal point/reserve ownership swap).
    bool prepare(const std::uint8_t* ram,const std::uint8_t* rom,
                 unsigned romSize,unsigned frame){
        if(!ram||!rom||XM87142::ramB(ram,0x5000u)!=8u){
            if(prepared)reset();
            return false;
        }
        const unsigned ids[2][2]={
            {XM87142::ramW(ram,0x4052u),XM87142::ramW(ram,0x4852u)},
            {XM87142::ramW(ram,0x4452u),XM87142::ramW(ram,0x4C52u)}
        };
        for(unsigned s=0;s<2u;++s)
            if(!idValid(ids[s][0])||!idValid(ids[s][1])||
               ids[s][0]==ids[s][1])return false;
        bool same=prepared;
        if(same)for(unsigned s=0;s<2u;++s){
            // Swap-in-place leaves the same two selected fighters.
            if(!((roster[s][0]==ids[s][0]&&roster[s][1]==ids[s][1])||
                 (roster[s][0]==ids[s][1]&&roster[s][1]==ids[s][0])))
                same=false;
        }
        if(same)return false;
        reset();
        for(unsigned s=0;s<2u;++s)for(unsigned c=0;c<2u;++c){
            const unsigned id=ids[s][c];
            roster[s][c]=id;
            for(unsigned k=0;k<4u;++k){
                const Known* known=definition(id,k);
                if(!known)break; // unmapped character stays empty, not a guess
                Entry& e=supers[s][c][k];e.occupied=true;e.known=known;
                for(unsigned j=0;j<known->alternatives && j<2u;++j){
                    const auto lookup=XM87142::resolve(
                        rom,romSize,id,known->actions[j],known->selectors[j]);
                    Script& script=e.scripts[e.count++];
                    script.action=known->actions[j];
                    script.selector=known->selectors[j];
                    script.script=lookup.script;
                    script.animation=lookup.animation;
                    script.resolved=lookup.valid && lookup.script==known->expected[j];
                }
            }
        }
        prepared=true;initializedAt=frame;++generation;
        return true;
    }
    const Entry* find(unsigned side,unsigned fighter,Command command)const{
        if(!prepared||side>=2u)return nullptr;
        for(unsigned c=0;c<2u;++c){
            if(roster[side][c]!=fighter)continue;
            for(unsigned k=0;k<4u;++k){
                const Entry& e=supers[side][c][k];
                if(e.occupied && e.known && e.known->command==command)return &e;
            }
        }
        return nullptr;
    }
    unsigned count(unsigned side,unsigned fighter)const{
        if(!prepared||side>=2u)return 0u;
        for(unsigned c=0;c<2u;++c)if(roster[side][c]==fighter){
            unsigned n=0u;
            for(unsigned k=0;k<4u;++k)if(supers[side][c][k].occupied)++n;
            return n;
        }
        return 0u;
    }
};
static inline MatchQueue& state(){static MatchQueue q;return q;}
} // namespace XM87143Queue
#endif
