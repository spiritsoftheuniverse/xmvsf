#ifndef XMVSF_STAGE87_160_SUPER_DISCOVERY_H
#define XMVSF_STAGE87_160_SUPER_DISCOVERY_H
// Stage 87.160: READ-ONLY, roster-wide CPS2 ROM move-table discovery.
// Exhaustively inspect native action 0x2E selectors using the *original*
// 68000 ROM pointer tables, then record which script the live ROM executed.
// Input associations are temporal CANDIDATES until the 68000 command parser
// is independently decoded. No newly discovered move is enabled for DHC.
// Do not overwrite gameplay actors, projectiles, meter, or input ports.
#include "xmvsf_stage87_143_super_queue.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87160 {
static inline unsigned rb(const std::uint8_t* r,unsigned a){
    return XM87142::ramB(r,a);
}
static inline unsigned rw(const std::uint8_t* r,unsigned a){
    return XM87142::ramW(r,a);
}
static inline unsigned rl(const std::uint8_t* r,unsigned a){
    return XM87142::ramL(r,a);
}
struct Input {
    unsigned frame=0u,physicalDir=0u,buttons=0u,pressed=0u;
    bool faceRight=true,injected=false;
};
static inline unsigned relative(unsigned d,bool faceRight){
    const bool right=(d&1u)!=0u,left=(d&2u)!=0u;
    const bool down=(d&4u)!=0u,up=(d&8u)!=0u;
    if((right&&left)||(up&&down))return 5u;
    const bool forward=faceRight?right:left;
    const bool back=faceRight?left:right;
    if(down&&forward)return 3u;
    if(down&&back)return 1u;
    if(down)return 2u;
    if(up&&forward)return 9u;
    if(up&&back)return 7u;
    if(up)return 8u;
    if(forward)return 6u;
    if(back)return 4u;
    return 5u;
}
static inline const char* physicalChord(unsigned mask){
    switch(mask&0x3Fu){
    case 0x03u:return "LP+MP";
    case 0x05u:return "LP+HP";
    case 0x06u:return "MP+HP";
    case 0x18u:return "LK+MK";
    case 0x28u:return "LK+HK";
    case 0x30u:return "MK+HK";
    case 0x07u:return "3P";
    case 0x38u:return "3K";
    case 0x24u:return "HP+HK_TAG_OR_VARIABLE";
    case 0u:return "NONE";
    default:return "OTHER_CHORD";
    }
}
struct State {
    bool initialized=false,enabled=false,catalogDone=false,privateDumpDone=false;
    unsigned lastFrame=~0u,lastFighter[2]={~0u,~0u},
             lastAction[2]={~0u,~0u},lastScript[2]={~0u,~0u};
    unsigned previousButtons[2]={0u,0u};
    unsigned candidateCount=0u,observedCount=0u;
    unsigned lastObservedFighter[2]={0u,0u};
    unsigned lastObservedAction[2]={0u,0u};
    unsigned lastObservedScript[2]={0u,0u};
    Input history[2][64]={};
    unsigned head[2]={0u,0u},used[2]={0u,0u};
    std::FILE* catalog=nullptr;
    std::FILE* observations=nullptr;
    ~State(){
        if(catalog)std::fclose(catalog);
        if(observations)std::fclose(observations);
    }
    void init(){
        if(initialized)return;
        initialized=true;
        const char* setting=std::getenv("XM87160_DISCOVER");
        enabled=setting && std::strcmp(setting,"1")==0;
        if(!enabled)return;
        const char* path=std::getenv("XM87160_CATALOG_LOG");
        if(path&&*path){
            catalog=std::fopen(path,"wb");
            if(catalog)std::fputs(
                "fighter_id\taction\tselector\tscript\tanimation\tfirst_duration"
                "\tstatus\tknown_name\tverified_command\n",catalog);
        }
        const char* events=std::getenv("XM87160_OBSERVED_LOG");
        if(events&&*events){
            observations=std::fopen(events,"wb");
            if(observations)std::fputs(
                "frame\tside\tfighter_id\treserve_id\taction\tselector"
                "\tnative_script\ttable_script\tanimation\trom_match"
                "\tlast_input_frame\tlast_input_mask\tlast_input_chord"
                "\tmotion_hint\tinput_origin\tclassification\tmotion_path"
                 "\tmovie_input_frame\n",
                observations);
        }
    }
    static const XM87143Queue::Known* label(unsigned fighter,
            unsigned action,unsigned selector,unsigned script){
        for(unsigned i=0u;i<4u;++i){
            const XM87143Queue::Known* k=XM87143Queue::definition(fighter,i);
            if(!k)break;
            for(unsigned j=0u;j<k->alternatives && j<2u;++j)
                if(k->actions[j]==action && k->selectors[j]==selector &&
                   k->expected[j]==script)return k;
        }
        return nullptr;
    }
    // Actions are offset pairs. The closest later valid action section
    // bounds 2E's selector table; if none exists, cap at 40 slots and
    // leave every entry explicitly UNVERIFIED. ROM pointer validity is
    // evidence of a script, NOT evidence of a hyper or its command.
    static unsigned selectorLimit(const std::uint8_t* rom,unsigned size,
                                   unsigned fighter){
        const unsigned table=XM87142::romL(rom,size,0x8A946u+fighter*2u);
        if(table>=size || size-table<0x38u)return 0u;
        const unsigned at=XM87142::romW(rom,size,table+0x2Eu);
        if(!at || at>size-table)return 0u;
        const unsigned section=table+at;
        unsigned next=0u;
        for(unsigned act=0x30u;act<=0x34u;act+=2u){
            const unsigned delta=XM87142::romW(rom,size,table+act);
            if(!delta || delta>size-table)continue;
            const unsigned p=table+delta;
            if(p>section && (!next || p<next))next=p;
        }
        if(!next)return 0x50u; // capped, never claim table completeness
        const unsigned span=next-section;
        return span<0x50u?(span&~1u):0x50u;
    }
    // Private, local-only export of decoded CPS2 program words and ROM data.
    // Both buffers are from the running game; the binary files are excluded
    // from Git and are NEVER included in diagnostics ZIPs automatically.
    // This is needed to disassemble command-recognition 68000 instructions,
    // rather than incorrectly treating animation scripts as input commands.
    void exportPrivateCode(const std::uint8_t* code,unsigned codeLen,
                           const std::uint8_t* rom,unsigned romLen){
        init();
        if(!enabled||privateDumpDone||!code||!rom ||
           codeLen<0x1000u||codeLen>0x2000000u ||
           romLen<0x1000u||romLen>0x2000000u)return;
        const char* dest=std::getenv("XM87160_PRIVATE_ROM_DIR");
        if(!dest||!*dest)return;
        privateDumpDone=true;
        const std::uint8_t* bytes[2]={code,rom};
        const unsigned lengths[2]={codeLen,romLen};
        const char* names[2]={
           "xmvsfu_decrypted_opcodes_host_swapped.bin",
           "xmvsfu_program_data_host_swapped.bin"
        };
        for(unsigned i=0u;i<2u;++i){
            char path[2048];
            const int n=std::snprintf(path,sizeof(path),"%s/%s",dest,names[i]);
            if(n<0 || unsigned(n)>=sizeof(path))continue;
            // Never overwrite a successfully captured existing ROM file.
            std::FILE* exists=std::fopen(path,"rb");
            if(exists){std::fclose(exists);continue;}
            std::FILE* out=std::fopen(path,"wb");
            if(!out)continue;
            const size_t wrote=std::fwrite(bytes[i],1,lengths[i],out);
            std::fclose(out);
            if(wrote!=lengths[i])std::remove(path);
        }
    }
    void discover(const std::uint8_t* rom,unsigned size){
        init();
        if(!enabled||catalogDone||!rom||size<0x8AA08u)return;
        catalogDone=true;
        for(unsigned fighter=0u;fighter<=0x40u;fighter+=2u){
            const unsigned limit=selectorLimit(rom,size,fighter);
            for(unsigned selector=0u;selector<limit;selector+=2u){
                const XM87142::Lookup x=XM87142::resolve(
                    rom,size,fighter,0x2Eu,selector);
                if(!x.valid || x.script<=x.actionTable)continue;
                const unsigned duration=XM87142::romW(rom,size,x.script+2u);
                if(!duration || duration>=0x8000u)continue;
                const XM87143Queue::Known* known=
                    label(fighter,0x2Eu,selector,x.script);
                const bool teamOnly=fighter==0x14u &&
                    selector==0x06u && x.script==0xD6B3Eu;
                ++candidateCount;
                if(catalog){
                    std::fprintf(catalog,
                        "%02X\t2E\t%02X\t%06X\t%04X\t%u\t%s\t%s\t%s\n",
                        fighter,selector,x.script,x.animation,duration,
                        teamOnly?"EXCLUDE_VARIABLE_COMBINATION":
                        known?"VERIFIED_CAPTURE":"UNVERIFIED_ROM_2E_SCRIPT",
                        known?known->name:
                        teamOnly?"KEN_VARIABLE_COMBINATION":"UNKNOWN",
                        known?(known->back?"QCB+KK":
                            known->kicks?"QCF+KK":"QCF+PP"):"UNDECODED");
                }
            }
        }
        if(catalog)std::fflush(catalog);
    }
    void sample(unsigned side,unsigned now,unsigned dir,unsigned buttons,
                bool faceRight,bool injected){
        Input& e=history[side][head[side]];
        e.frame=now;e.physicalDir=dir;e.buttons=buttons&0x3Fu;
        e.pressed=e.buttons & ~previousButtons[side];
        e.faceRight=faceRight;e.injected=injected;
        previousButtons[side]=e.buttons;
        head[side]=(head[side]+1u)%64u;
        if(used[side]<64u)++used[side];
    }
    Input recentButton(unsigned side,unsigned now)const{
        Input result;
        for(unsigned ago=0u;ago<used[side];++ago){
            const Input& e=history[side][(head[side]+64u-1u-ago)%64u];
            if(e.frame>now)continue;
            if(now-e.frame>18u)break;
            if(e.pressed){result=e;break;}
        }
        return result;
    }
    // Report historical button/motion correlations as HINTS. The actual
    // command decoder (decrypted 68000 code) must validate them before DHC.
    const char* motionHint(unsigned side,unsigned now,unsigned anchor)const{
        if(!anchor || now<anchor || now-anchor>18u)return "UNKNOWN";
        unsigned changes[64]={},count=0,prev=~0u;
        // Chronological snapshots across a rolling 48-frame window.
        for(unsigned ago=used[side];ago>0u;--ago){
            const Input& e=history[side][(head[side]+64u-ago)%64u];
            if(e.frame+48u<anchor || e.frame>anchor)continue;
            if(e.injected)continue;
            const unsigned dir=relative(e.physicalDir,e.faceRight);
            if(dir==prev)continue;
            prev=dir;
            changes[count++]=dir;
        }
        static const unsigned qcf[]={2u,3u,6u};
        static const unsigned qcb[]={2u,1u,4u};
        static const unsigned dp[]={6u,2u,3u};
        static const unsigned hcf[]={4u,1u,2u,3u,6u};
        static const unsigned hcb[]={6u,3u,2u,1u,4u};
        struct Pattern { const unsigned* v;unsigned size;const char* name; };
        const Pattern patterns[]={{hcf,5u,"HCF"},{hcb,5u,"HCB"},
                                  {dp,3u,"DP"},{qcf,3u,"QCF"},
                                  {qcb,3u,"QCB"}};
        for(unsigned i=0u;i<sizeof(patterns)/sizeof(patterns[0]);++i){
            unsigned matched=0u;
            for(unsigned j=0u;j<count;++j)
                if(changes[j]==patterns[i].v[matched]){
                    if(++matched==patterns[i].size)return patterns[i].name;
                }
        }
        return "UNKNOWN";
    }
    // Preserve the exact recent direction trail instead of silently
    // treating an older unrelated back/forward input as an HCF/HCB.
    // This is observation only; no new DHC command can be dispatched.
    void directionPath(unsigned side,unsigned anchor,char* out,unsigned cap)const{
        if(!out||!cap)return;
        out[0]=0;
        if(!anchor)return;
        unsigned changes[64]={},count=0,previous=~0u;
        for(unsigned ago=used[side];ago>0u;--ago){
            const Input& e=history[side][(head[side]+64u-ago)%64u];
            if(e.frame>anchor || e.frame+24u<anchor || e.injected)continue;
            const unsigned dir=relative(e.physicalDir,e.faceRight);
            if(dir==previous)continue;
            previous=dir;
            changes[count++]=dir;
        }
        const unsigned first=count>14u?count-14u:0u;
        unsigned n=0u;
        for(unsigned i=first;i<count;++i){
            const int written=std::snprintf(out+n,cap-n,
                                           "%s%u",n?">":"",changes[i]);
            if(written<0 || unsigned(written)>=cap-n)break;
            n+=unsigned(written);
        }
    }
    void frame(const std::uint8_t* ram,const std::uint8_t* rom,unsigned size,
               unsigned now,unsigned p1Dir,unsigned p2Dir,
               unsigned p1Buttons,unsigned p2Buttons,bool p1Injected,
               unsigned movieFrame=~0u){
        discover(rom,size);
        if(!enabled||!ram||rb(ram,0x5000u)!=8u || now==lastFrame)return;
        if(lastFrame!=~0u && now<lastFrame){
            for(unsigned s=0;s<2u;++s){
                lastFighter[s]=lastAction[s]=lastScript[s]=~0u;
                previousButtons[s]=used[s]=head[s]=0u;
            }
        }
        lastFrame=now;
        const unsigned directions[2]={p1Dir,p2Dir};
        const unsigned buttons[2]={p1Buttons,p2Buttons};
        for(unsigned side=0u;side<2u;++side){
            const unsigned base=0x4000u+side*0x400u;
            const unsigned fighter=rw(ram,base+0x52u);
            const unsigned reserve=rw(ram,0x4800u+side*0x400u+0x52u);
            const unsigned action=rb(ram,base+0x60u);
            const unsigned selector=rb(ram,base+0x61u);
            const unsigned script=rl(ram,base+0x34u);
            const bool injected=(side==0u && p1Injected);
            sample(side,now,directions[side],buttons[side],
                   rb(ram,base+0x4Bu)!=0u,injected);
            const bool actionChanged=lastFighter[side]!=fighter ||
                lastAction[side]!=(action<<8u|selector);
            const bool scriptChanged=lastScript[side]!=script;
            lastFighter[side]=fighter;
            lastAction[side]=action<<8u|selector;
            lastScript[side]=script;
            if(action!=0x2Eu || (fighter&1u) ||
               fighter>0x40u)continue;
            const XM87142::Lookup x=XM87142::resolve(
                rom,size,fighter,action,selector);
            const bool matches=x.valid && x.script==script;
            // 87.160 logged every animation-pointer advance as a new
            // ROM super, causing >1000 phantom "UNKNOWN_ROM_SCRIPT"
            // records in the user's capture. Record ONLY a native
            // action's exact ROM script ROOT on entry or re-entry.
            if(!matches || !(actionChanged || scriptChanged))continue;
            const XM87143Queue::Known* known=
                matches?label(fighter,action,selector,script):nullptr;
            const Input last=recentButton(side,now);
            const char* source=last.frame==0u?"NONE":
                last.injected?"DHC_TEST_INJECTED":"PHYSICAL_CANDIDATE";
            const char* hint=last.injected?"UNREAD":"UNKNOWN";
            if(last.frame && !last.injected)
                hint=motionHint(side,now,last.frame);
            char path[96];
            directionPath(side,last.frame,path,sizeof(path));
            ++observedCount;
            lastObservedFighter[side]=fighter;
            lastObservedAction[side]=action<<8u|selector;
            lastObservedScript[side]=script;
            if(observations){
                std::fprintf(observations,
                    "%u\t%u\t%02X\t%02X\t%02X\t%02X"
                    "\t%06X\t%06X\t%04X\t%u"
                    "\t%u\t%02X\t%s\t%s\t%s\t%s\t%s\t%lld\n",
                    now,side+1u,fighter,reserve,action,selector,
                    script,x.script,x.animation,matches?1u:0u,
                    last.frame,last.buttons,physicalChord(last.buttons),
                    hint,source,
                    known?"VERIFIED_RYU_KEN_SUPER":
                    matches?"ROM_2E_SCRIPT_OBSERVED_COMMAND_UNVERIFIED":
                            "UNKNOWN_ROM_SCRIPT",path,
                    movieFrame==~0u?-1LL:static_cast<long long>(movieFrame));
                std::fflush(observations);
            }
        }
    }
};
static inline State& state(){static State s;return s;}
} // namespace XM87160
#endif
