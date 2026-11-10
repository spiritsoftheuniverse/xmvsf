#ifndef XMVSF_STAGE23_TRAINING_ENGINE_H
#define XMVSF_STAGE23_TRAINING_ENGINE_H
#include "xmvsf_stage23_ui.h"
#include "burnint.h"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>

namespace xm23 {
static inline unsigned r8(cpu_core_config* c,unsigned a) {return c->read(a)&0xffu;}
static inline unsigned r16(cpu_core_config* c,unsigned a) {return (r8(c,a)<<8)|r8(c,a+1);}
static inline int s16(cpu_core_config* c,unsigned a) {return (int16_t)r16(c,a);}
static inline unsigned r32(cpu_core_config* c,unsigned a) {return (r16(c,a)<<16)|r16(c,a+2);}
static inline void w16(cpu_core_config* c,unsigned a,unsigned v) {
    c->write(a,(v>>8)&255u);c->write(a+1,v&255u);
}
static inline bool addr(unsigned a) {return a>=0x100 && a<=0xFFFFF7;}
static inline void addBox(cpu_core_config* c,unsigned base,int projectile,int posx,int posy,int flip,
                          unsigned entry,int type,int idIsByte,int owner) {
    if(xm23State.count>=192)return;
    unsigned id=idIsByte?r8(c,base+entry):r16(c,base+entry);
    if(!id)return;
    if(type==1 && (r8(c,base+0x82)==0 || (id&0x8000)))return;
    if((type==0||type==4) && r8(c,base+0x83)==0)return;
    if(type==3 && (r8(c,base+0x120)!=0 || r8(c,base+0x83)==0))return;
    unsigned ptr=0;
    if(type==2) {
        unsigned slot=0x08B022 + 0x2E + id*2;
        if(!addr(slot))return;
        ptr=r32(c,slot)+(r16(c,base+0x52)<<2);
    } else {
        const unsigned table=r32(c,base+0x6c);
        if(!addr(table))return;
        ptr=table+((id&0x1fffu)<<3);
    }
    if(!addr(ptr) || !addr(ptr+7))return;
    const int rx=(int)r16(c,ptr+2),ry=(int)r16(c,ptr+6);
    if(rx<=0||ry<=0||rx>320||ry>240)return;
    const int x=posx+s16(c,ptr)*(flip&1?-1:1);
    const int y=posy+s16(c,ptr+4)*((flip&2)&&type!=2?-1:1);
    XM23Box& b=xm23State.box[xm23State.count++];
    b.x=x-rx;b.y=y-ry;b.w=rx*2;b.h=ry*2;b.kind=type;b.owner=owner;b.sourceBase=base;
    if(projectile) {if(type==0)b.kind=4;if(type==1)b.kind=5;}
}
static inline void object(cpu_core_config* c,unsigned base,bool projectile,int left,int top,int owner) {
    // Linked CPS2 projectile objects may have base[0]==0 while their
    // offensive collision descriptor (base+0x70) is live. The linked-list
    // entry, address bounds, and collision table remain authoritative.
    // Do not relax the fighter-object activity gate.
    if(!addr(base+0xB2) || (!projectile && !r8(c,base)))return;
    const int x=s16(c,base+0x0c)-left;
    const int y=s16(c,base+0x10)-top-15;
    const int flip=(int)r8(c,base+0x4b);
    const unsigned hurt[4]={0x74,0x76,0x78,0x7A};
    for(int i=0;i<4;i++)addBox(c,base,projectile,x,y,flip,hurt[i],0,0,owner);
    addBox(c,base,projectile,x,y,flip,0x70,1,0,owner);
    addBox(c,base,projectile,x,y,flip,0x72,1,0,owner);
    if(!projectile && base<=0xFF4400 && !r8(c,base+0x105)
       && r16(c,base+0x006)!=4)
        addBox(c,base,0,x,y,flip,0xA4,2,1,owner);
    // Throw vulnerability, when available. Special-case guards in Marvel viewer.
    if(!(r8(c,0xFF500F)&8) && !r8(c,0xFF5031) && !r8(c,base+0x120))
        addBox(c,base,projectile,x,y,flip,0x7c,3,0,owner);
}
static inline void gather(cpu_core_config* c) {
    xm23State.count=0;
    // Exact USA 961023 match/profile addresses, from marvel-hitboxes.lua.
    if(r8(c,0xFF5000)!=0x08 || r32(c,0xFFA014)!=0x00FF4000) return;
    const unsigned stage=r8(c,0xFF5113)&15u;
    const int offsets[16]={-0x2cc0,-0x2dc0,-0x2dc0,-0x2d40,-0x2dc0,-0x2dc0,-0x2cc0,-0x2dc0,
                           -0x2d40,-0x2d40,-0x2dc0,-0x2dc0,-0x2dc0,-0x2dc0,-0x2dc0,-0x2dc0};
    const unsigned screenbase=0xFF8000+offsets[stage];
    const int left=s16(c,screenbase+0x0c)+0x40;
    const int top=s16(c,screenbase+0x10);
    for(unsigned p=0;p<4;p++)object(c,0xFF4000+p*0x400,false,left,top,(int)(p&1u));
    for(unsigned player=0;player<2;player++) {
        for(unsigned i=1;i<=12;i++) {
            const unsigned a=0xFFE3D8+player*0x150-i*4;
            if(!addr(a))break;
            const unsigned base=r32(c,a);
            if(base<0xFF5000||base>0xFFFF00)break;
            object(c,base,true,left,top,(int)player);
        }
    }
}
}

// Stage 36: output next to the running EXE, independent of the MSYS shell's
// working directory. Give a visible success or failure instead of silently
// suppressing future attempts when fopen() fails.
static inline FILE* xm36OpenTrace(const char* filename, const char* header) {
    char* base = SDL_GetBasePath();
    char path[2048];
    std::snprintf(path,sizeof(path),"%s%s",base?base:"",filename);
    if(base) SDL_free(base);
    FILE* out=std::fopen(path,"w");
    if(!out){
        std::fprintf(stderr,"XM36: TRACE_OPEN_FAILED %s (%s)\n",path,std::strerror(errno));
        std::fflush(stderr);
        return nullptr;
    }
    std::fputs(header,out);
    std::fflush(out);
    std::printf("XM36: TRACE_READY %s\n",path);
    std::fflush(stdout);
    return out;
}

// Stage 34: read-only hitbox probe. Data is sampled while an input-to-box
// candidate is pending. No RAM writes, no render changes, no timing changes.
// The trace is intentionally bounded and contains only game RAM descriptors.
static inline void xm34ProbeFrame(cpu_core_config* c, const XM30Sample (&samples)[2]) {
    if(!xm23State.frameData.enabled) return;
    static FILE* logFile=nullptr;
    static bool opened=false;
    static unsigned sampledFrames=0;
    if(!opened){
        opened=true;
        logFile=xm36OpenTrace("xmvsfu_hitbox_probe.tsv",
            "frame\trequested_player\tobject_slot\towner\tbase\tobject_alive\tcombat_flag\tprofile_ptr\tid70\tid72\tgate82\tgate83\tanim_state\tbox_table\tattack70_table_entry\tattack72_table_entry\thit70_rx\thit70_ry\thit72_rx\thit72_ry\tmelee0\tprojectile0\tmelee1\tprojectile1\taccepted_total\n");
    }
    if(!logFile || sampledFrames>=3600) return;
    const unsigned long long frame=xm23State.frameData.frame+1;
    const bool pending=xm23State.frameData.track[0].pending>=0 || xm23State.frameData.track[1].pending>=0;
    if(!pending && frame>6 && frame%30!=0) return; // Baseline idle snapshot every 30f.
    ++sampledFrames;
    const unsigned profile=xm23::r32(c,0xFFA014);
    const unsigned combat=xm23::r8(c,0xFF5000);
    for(int slot=0;slot<4;++slot){
        const unsigned base=0xFF4000u+(unsigned)slot*0x400u;
        const unsigned id70=xm23::r16(c,base+0x70);
        const unsigned id72=xm23::r16(c,base+0x72);
        const unsigned table=xm23::r32(c,base+0x6c);
        const unsigned ptr70=xm23::addr(table) && id70 ? table+((id70&0x1fffu)<<3):0u;
        const unsigned ptr72=xm23::addr(table) && id72 ? table+((id72&0x1fffu)<<3):0u;
        const bool valid70=xm23::addr(ptr70) && xm23::addr(ptr70+7);
        const bool valid72=xm23::addr(ptr72) && xm23::addr(ptr72+7);
        const int owner=slot&1;
        const int requested=xm23State.frameData.track[0].pending>=0?0:1;
        std::fprintf(logFile,"%llu\t%d\t%d\t%d\t%06X\t%u\t%02X\t%08X\t%04X\t%04X\t%u\t%u\t%04X\t%08X\t%08X\t%08X\t%u\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\n",
                     frame,requested,slot,owner,base,xm23::r8(c,base),combat,profile,id70,id72,
                     xm23::r8(c,base+0x82),xm23::r8(c,base+0x83),xm23::r16(c,base+0x06),table,
                     ptr70,ptr72,valid70?xm23::r16(c,ptr70+2):0u,valid70?xm23::r16(c,ptr70+6):0u,
                     valid72?xm23::r16(c,ptr72+2):0u,valid72?xm23::r16(c,ptr72+6):0u,
                     (int)samples[0].melee.active,(int)samples[0].projectile.active,
                     (int)samples[1].melee.active,(int)samples[1].projectile.active,xm23State.count);
    }
    // Always flush, including when the user exits to boot instead of terminating.
    std::fflush(logFile);
}


// Stage 35: bounded read-only raw fighter-object trace. The existing profile
// permits box-descriptor lookup, but does not establish attack activity. Keep
// the high-bit rejection in the production hitbox renderer until confirmed.
// The raw record includes the entire 0x000-0x17F fighter object region, so
// attacks can be compared with idle frames without guessing new offsets.
static inline void xm35RawHitboxProbe(cpu_core_config* c) {
    if (!xm23State.frameData.enabled) return;
    const XM30Track& p1=xm23State.frameData.track[0];
    const XM30Track& p2=xm23State.frameData.track[1];
    static FILE* out=nullptr;
    static bool attempted=false;
    static unsigned loggedFrames=0;
    if (!attempted) {
        attempted=true;
        out=xm36OpenTrace("xmvsfu_hitbox_deep.tsv",
            "frame\tobject_slot\towner\tbase\tobject_alive\tp1_pending_age\tp2_pending_age\tp1_code\tp2_code\tid70\tid72\tgate82\tgate83\traw_0000_017F\n");
    }
    if (!out || loggedFrames>=3600) return;
    const unsigned long long frame=xm23State.frameData.frame+1;
    const bool pending=p1.pending>=0 || p2.pending>=0;
    if(!pending && frame>6 && frame%30!=0) return; // Preserve idle vs attack contrast.

    const long long age1=p1.pending<0 ? -1 : (long long)frame-p1.pending;
    const long long age2=p2.pending<0 ? -1 : (long long)frame-p2.pending;
    static const char hex[]="0123456789ABCDEF";
    for (unsigned slot=0;slot<4;++slot) {
        const unsigned base=0xFF4000u+slot*0x400u;
        char raw[0x180*2+1];
        for (unsigned i=0;i<0x180;++i) {
            const unsigned b=xm23::r8(c,base+i);
            raw[2*i]=hex[(b>>4)&15u];raw[2*i+1]=hex[b&15u];
        }
        raw[sizeof(raw)-1]=0;
        std::fprintf(out,"%llu\t%u\t%u\t%06X\t%u\t%lld\t%lld\t%d\t%d\t%04X\t%04X\t%u\t%u\t%s\n",
                     frame,slot,slot&1u,base,xm23::r8(c,base),age1,age2,p1.code,p2.code,
                     xm23::r16(c,base+0x70),xm23::r16(c,base+0x72),
                     xm23::r8(c,base+0x82),xm23::r8(c,base+0x83),raw);
    }
    ++loggedFrames;
    std::fflush(out);
}

// Stage 37: read-only hit-confirmation correlation.
// 'NO BOX' cannot establish hitbox activation. Capture health/combo changes
// against the exact fighter-object state surrounding a candidate attack.
// In particular, an animation-state change is not a proven active hitbox.
static inline void xm37HitConfirmProbe(cpu_core_config* c, const XM30Sample (&samples)[2]) {
    if(!xm23State.frameData.enabled)return;
    static FILE* out=nullptr;
    static bool attempted=false;
    static unsigned loggedFrames=0;
    static int lastHealth[2]={-1,-1};
    static int lastCombo[2]={-1,-1};
    static int afterChange=0;
    if(!attempted){
        attempted=true;
        out=xm36OpenTrace("xmvsfu_hit_confirm.tsv",
            "frame\towner\tbase\tattack_pending_age\tattack_code\tp1_health\tp2_health\tp1_health_delta\tp2_health_delta\tp1_combo\tp2_combo\tp1_combo_delta\tp2_combo_delta\tobject_alive\tstate_A0\tstate_A1\tstate_A4\tanim_3A\tid70\tid72\tgate82\tgate83\tmelee_detected\tprojectile_detected\traw_0000_017F\n");
    }
    if(!out || loggedFrames>=3600)return;
    const unsigned long long frame=xm23State.frameData.frame+1;
    const int hp[2]={xm23State.health[0],xm23State.health[1]};
    const int combo[2]={xm23State.combo[0],xm23State.combo[1]};
    const int dHp[2]={lastHealth[0]<0?0:hp[0]-lastHealth[0],lastHealth[1]<0?0:hp[1]-lastHealth[1]};
    const int dCombo[2]={lastCombo[0]<0?0:combo[0]-lastCombo[0],lastCombo[1]<0?0:combo[1]-lastCombo[1]};
    lastHealth[0]=hp[0];lastHealth[1]=hp[1];
    lastCombo[0]=combo[0];lastCombo[1]=combo[1];
    const bool changed=dHp[0]||dHp[1]||dCombo[0]||dCombo[1];
    if(changed)afterChange=30;
    const bool pending=xm23State.frameData.track[0].pending>=0 || xm23State.frameData.track[1].pending>=0;
    const bool record=pending||afterChange>0||frame<=6||frame%30==0;
    if(afterChange>0)--afterChange;
    if(!record)return;
    static const char hex[]="0123456789ABCDEF";
    for(unsigned owner=0;owner<2;++owner){
        const unsigned base=0xFF4000u+owner*0x400u;
        const XM30Track& track=xm23State.frameData.track[owner];
        const long long pendingAge=track.pending<0?-1:(long long)frame-track.pending;
        char raw[0x180*2+1];
        for(unsigned i=0;i<0x180;++i){
            const unsigned b=xm23::r8(c,base+i);
            raw[i*2]=hex[(b>>4)&15u];raw[i*2+1]=hex[b&15u];
        }
        raw[sizeof(raw)-1]=0;
        std::fprintf(out,"%llu\t%u\t%06X\t%lld\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%u\t%02X\t%02X\t%02X\t%02X\t%04X\t%04X\t%u\t%u\t%d\t%d\t%s\n",
            frame,owner,base,pendingAge,track.code,hp[0],hp[1],dHp[0],dHp[1],
            combo[0],combo[1],dCombo[0],dCombo[1],xm23::r8(c,base),
            xm23::r8(c,base+0xA0),xm23::r8(c,base+0xA1),xm23::r8(c,base+0xA4),
            xm23::r8(c,base+0x3A),xm23::r16(c,base+0x70),xm23::r16(c,base+0x72),
            xm23::r8(c,base+0x82),xm23::r8(c,base+0x83),
            (int)samples[owner].melee.active,(int)samples[owner].projectile.active,raw);
    }
    ++loggedFrames;
    std::fflush(out);
}

// Stage 38: isolate the object responsible for a game-confirmed hit.
// Previous snapshots covered only the primary fighters; this captures both
// additional fighter slots and the original CPS2 linked projectile lists.
// Descriptor IDs are reported raw: their high bit is NOT assumed active.
static inline void xm38HitSourceProbe(cpu_core_config* c){
    if(!xm23State.frameData.enabled)return;
    static FILE* out=nullptr;
    static bool attempted=false;
    static unsigned sampledFrames=0;
    static int previousCombo=-1;
    static unsigned previousReaction=0;
    static int tail=0;
    if(!attempted){
        attempted=true;
        out=xm36OpenTrace("xmvsfu_hit_sources.tsv",
            "frame\tcombo_p1\treaction_p2\tattack_age\tsource\tslot\towner\tbase\tvalid_object\tstate_a0\tanimation_3a\tpos_x\tpos_y\tid70\tid72\tgate82\tgate83\ttable\tptr70\tptr72\trx70\try70\trx72\try72\n");
    }
    if(!out || sampledFrames>=3600)return;
    const unsigned long long frame=xm23State.frameData.frame+1;
    const int combo=xm23State.combo[0];
    const unsigned reaction=xm23::r8(c,0xFF4400+0xA0);
    if((previousCombo>=0 && combo>previousCombo) ||
       (previousReaction!=0x36 && reaction==0x36))tail=45;
    previousCombo=combo;previousReaction=reaction;
    const XM30Track& track=xm23State.frameData.track[0];
    const bool pending=track.pending>=0 || xm23State.frameData.track[1].pending>=0;
    const bool record=(pending || tail>0 || frame<=6 || frame%30==0);
    if(tail>0)--tail;
    if(!record)return;
    ++sampledFrames;
    const long long age=track.pending<0?-1:(long long)frame-track.pending;
    // Read each actor independently, without changing the production renderer.
    auto emit=[&](const char* category,int slot,int owner,unsigned base){
        const bool addressOkay=base>=0xFF4000u && base<=0xFFFF00u && xm23::addr(base+0xA1);
        const unsigned alive=addressOkay?xm23::r8(c,base):0u;
        const unsigned id70=addressOkay?xm23::r16(c,base+0x70):0u;
        const unsigned id72=addressOkay?xm23::r16(c,base+0x72):0u;
        const unsigned tbl=addressOkay?xm23::r32(c,base+0x6c):0u;
        const unsigned ptr70=xm23::addr(tbl)&&id70?tbl+((id70&0x1fffu)<<3):0u;
        const unsigned ptr72=xm23::addr(tbl)&&id72?tbl+((id72&0x1fffu)<<3):0u;
        const bool v70=xm23::addr(ptr70)&&xm23::addr(ptr70+7);
        const bool v72=xm23::addr(ptr72)&&xm23::addr(ptr72+7);
        std::fprintf(out,"%llu\t%d\t%u\t%lld\t%s\t%d\t%d\t%06X\t%u\t%02X\t%02X\t%d\t%d\t%04X\t%04X\t%u\t%u\t%08X\t%08X\t%08X\t%d\t%d\t%d\t%d\n",
            frame,combo,reaction,age,category,slot,owner,base,alive,
            addressOkay?xm23::r8(c,base+0xA0):0u,
            addressOkay?xm23::r8(c,base+0x3A):0u,
            addressOkay?xm23::s16(c,base+0x0c):0,
            addressOkay?xm23::s16(c,base+0x10):0,
            id70,id72,addressOkay?xm23::r8(c,base+0x82):0u,
            addressOkay?xm23::r8(c,base+0x83):0u,
            tbl,ptr70,ptr72,
            v70?(int)xm23::r16(c,ptr70+2):0,
            v70?(int)xm23::r16(c,ptr70+6):0,
            v72?(int)xm23::r16(c,ptr72+2):0,
            v72?(int)xm23::r16(c,ptr72+6):0);
    };
    for(int slot=0;slot<4;++slot)
        emit("fighter",slot,slot&1,0xFF4000u+(unsigned)slot*0x400u);
    for(int owner=0;owner<2;++owner){
        for(int slot=1;slot<=12;++slot){
            const unsigned link=0xFFE3D8u+(unsigned)owner*0x150u-(unsigned)slot*4u;
            const unsigned base=xm23::r32(c,link);
            if(base<0xFF5000u || base>0xFFFF00u){
                // Log the link termination itself; otherwise the absence of
                // projectile rows cannot distinguish no object from bad links.
                emit("list_end",slot,owner,base);
                break;
            }
            emit("projectile",slot,owner,base);
        }
    }
    std::fflush(out);
}

// Optional, bounded read-only proof of ROM +85 and native combo moves
// versus the Training clock.
// Enable with XMVSF_FRAME_TRACE=1 before RUN_TEST.bat; the TSV is placed
// next to the TEST executable. No GPU capture, memory writes or timing hooks.
static inline void xm251FrameClockTrace(
        cpu_core_config* c,const XM30Sample (&samples)[2],
        const XM41ActorSample (&actors)[2],
        unsigned long long before,unsigned long long after,
        unsigned long long p1Before,unsigned long long p2Before){
    static int armed=-1;
    static FILE* file=nullptr;
    static unsigned long long sample=0;
    static unsigned logged=0,tail=0;
    if(armed<0){
        const char* flag=std::getenv("XMVSF_FRAME_TRACE");
        armed=flag && flag[0]=='1' && !flag[1] ? 1 : 0;
        if(armed){
            file=xm36OpenTrace("xmvsf_frame_clock_251.tsv",
                "host_sample\tlogical_before\tlogical_after\t"
                "p1_clock_before\tp1_clock_after\tp2_clock_before\tp2_clock_after\t"
                "p1_2e\tp1_hitstop\tp1_mode\tp1_action\tp1_phase\t"
                "p1_step\tp1_control\tp1_anim_timer\tp1_anim_ptr\t"
                "p1_hp\tp1_melee\tp1_proj\tp1_projectile_objects\tp1_projectile_attack_boxes\t"
                "p1_origin_family\tp1_origin_action\tp1_origin_mode\tp1_pending_family\t"
                "p1_pending_action\tp1_pending_mode\tp1_pending\tp1_native_only\tp1_pending_spawn_base\t"
                "p1_prev_box\tp1_last_contacts\tp1_contact_only\tp1_last_box_frame\t"
                "p2_2e\tp2_hitstop\tp2_mode\tp2_action\tp2_phase\t"
                "p2_step\tp2_control\tp2_anim_timer\tp2_anim_ptr\t"
                "p2_hp\tp2_melee\tp2_proj\tp2_projectile_objects\tp2_projectile_attack_boxes\t"
                "p2_origin_family\tp2_origin_action\tp2_origin_mode\tp2_pending_family\t"
                "p2_pending_action\tp2_pending_mode\tp2_pending\tp2_native_only\tp2_pending_spawn_base\t"
                "p2_prev_box\tp2_last_contacts\tp2_contact_only\tp2_last_box_frame\t"
                "events\tevents_created\tfreeze_steps\n");
            if(!file)armed=0;
        }
    }
    if(!armed || !file || logged>=2400u)return;
    ++sample;
    const bool attack=xm30NativeAttackFamily(actors[0])!=0u ||
                      xm30NativeAttackFamily(actors[1])!=0u;
    const bool pending=xm23State.frameData.track[0].pending>=0 ||
                       xm23State.frameData.track[1].pending>=0;
    // Also capture normal->normal and normal->special combo handoffs,
    // even when the fighter's +85 is zero and no super is on screen.
    if(attack || pending || actors[0].hitStop || actors[1].hitStop)
        tail=150u;
    if(!tail)return;
    --tail;
    std::fprintf(file,"%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu",
                 sample,before,after,p1Before,xm23State.frameData.playerFrame[0],
                 p2Before,xm23State.frameData.playerFrame[1]);
    for(int p=0;p<2;++p){
        const unsigned base=0xFF4000u+(unsigned)p*0x400u;
        unsigned observedProjectileAttackBoxes=0u;
        for(int j=0;j<samples[p].projectileCount && j<12;++j)
            if(samples[p].projectileHits[j].active)
                ++observedProjectileAttackBoxes;
        std::fprintf(file,"\t%u\t%u\t%04X\t%04X\t%02X\t"
                "%04X\t%04X\t%04X\t%08X\t%u\t%u\t%u\t%u\t%u",
            xm30NativeAttackFamily(actors[p])==0x2Eu?1u:0u,
            actors[p].hitStop,actors[p].mode,actors[p].nativeAction,
            actors[p].attackPhase,actors[p].nativeStep,
            xm23::r16(c,base+0x06u),xm23::r16(c,base+0x3Eu),
            xm23::r32(c,base+0x34u),actors[p].health,
            samples[p].melee.active?1u:0u,
            samples[p].projectile.active?1u:0u,
            (unsigned)samples[p].projectileCount,
            observedProjectileAttackBoxes);
        const XM30Track& t=xm23State.frameData.track[p];
        unsigned lastContacts=0u,lastBoxMissing=0u;
        unsigned long long lastBoxFrame=0u;
        for(int i=xm23State.frameData.count-1;i>=0;--i)
            if(xm23State.frameData.history[i].player==p){
                const XM30Event& e=xm23State.frameData.history[i];
                lastContacts=(unsigned)e.contactCount;
                lastBoxMissing=e.contactOnly?1u:0u;
                lastBoxFrame=e.hitboxLastFrame;
                break;
            }
        std::fprintf(file,
            "\t%02X\t%04X\t%04X\t%02X\t%04X\t%04X\t%d\t%d\t%08X\t%u\t%u\t%u\t%llu",
            t.pendingOriginFamily,t.pendingOriginAction,t.pendingOriginMode,
            t.pendingAttackFamily,t.pendingAttackAction,t.pendingAttackMode,
            t.pending>=0?1:0,t.pendingNativeOnly?1:0,t.pendingSpawnBase,
            t.previous[0]?1u:0u,lastContacts,lastBoxMissing,lastBoxFrame);
    }
    // HISTORY stops at 24; use the cumulative counter to expose every
    // unexpected card created AFTER the ring buffer fills.
    std::fprintf(file,"\t%d\t%llu\t%llu\n",
                 xm23State.frameData.count,
                 xm23State.frameData.eventsCreated,
                 xm23State.frameData.superFreezeFrames);
    if(++logged%60u==0u)std::fflush(file);
}

static inline void xm23OnRealFrame(int ready) {
    if(!xm14TrainingMode || !ready || bBurnRunAheadFrame) {
        if(!xm14TrainingMode) {xm23State.menuOpen=false;xm23State.combat=false;xm23State.count=0;xm23State.frameData.clear();}
        return;
    }
    cpu_core_config* c=GetCpuCoreConfig(0);
    if(!c || !c->open || !c->close || !c->read || !c->write)return;
    c->open(0);
    const bool live=xm23::r8(c,0xFF5000)==0x08 && xm23::r32(c,0xFFA014)==0x00FF4000;
    xm23State.combat=live;
    if(!live) {xm23State.menuOpen=false;xm23State.count=0;xm23State.frameData.clear();c->close();return;}
    if(xm23State.restoreSavedPlayerSelect){
        // XM17 must complete the actual P1/P2 team selection first. This is
        // the first live combat frame, so now restore the user's Training
        // PLAYER SELECT option without corrupting the selection handoff.
        xm14InputTarget=(int)xm23State.savedPlayerSelect;
        xm23State.restoreSavedPlayerSelect=false;
        if(xm14InputTarget!=0)xm25BeginInputHandoff();
        std::fprintf(stderr,
            "XM87.153 TRAINING_PLAYER_SELECT_RESTORED P%d after native select\n",
            xm14InputTarget+1);
    }
    xm23State.frameData.beginCombat();
    const bool p1Anchor=xm23::r8(c,0xff4220)!=0, p2Anchor=xm23::r8(c,0xff4620)!=0;
    xm23State.health[0]=xm23::r8(c,p1Anchor?0xff4a11:0xff4211);
    xm23State.health[1]=xm23::r8(c,p2Anchor?0xff4e11:0xff4611);
    xm23State.meter[0]=xm23::r8(c,0xff4214);xm23State.meter[1]=xm23::r8(c,0xff4614);
    xm23State.combo[0]=xm23::r8(c,0xff4110);xm23State.combo[1]=xm23::r8(c,0xff4510);
    if(xm23State.infiniteMeter) {
        c->write(0xff4214,3);xm23::w16(c,0xff4212,0x90);
        c->write(0xff4614,3);xm23::w16(c,0xff4612,0x90);
    }
    if(xm23State.hitboxes || xm23State.frameData.enabled)xm23::gather(c);
    else xm23State.count=0;
    if(xm23State.frameData.enabled){
        static bool xm36Announced=false;
        if(!xm36Announced) {
            std::puts("XM36: FRAME_DATA_ACTIVE; preparing traces in EXE directory");
            std::fflush(stdout);
            xm36Announced=true;
        }
        XM30Sample sample[2];
        // Track live projectile object identities separately from attack boxes.
        // The CPS2 object may spawn before its damaging box becomes active.
        for(int owner=0;owner<2;++owner){
            for(unsigned slot=1;slot<=12;++slot){
                const unsigned link=0xFFE3D8u+(unsigned)owner*0x150u-slot*4u;
                const unsigned base=xm23::r32(c,link);
                if(base<0xFF5000u || base>0xFFFF00u)break;
                // Linked projectile objects can have byte zero cleared even
                // while a genuine attack box exists. Record linked identities
                // for accurate input->spawn association.
                if(sample[owner].projectileCount>=12)break;
                const int n=sample[owner].projectileCount++;
                sample[owner].projectileBases[n]=base;
                sample[owner].projectileX[n]=xm23::s16(c,base+0x0c);
                sample[owner].projectileY[n]=xm23::s16(c,base+0x10);
            }
        }
        for(int i=0;i<xm23State.count;++i){
            const XM23Box& box=xm23State.box[i];
            if((box.kind!=1 && box.kind!=5) || box.owner<0 || box.owner>1)continue;
            XM30Hit& hit=box.kind==5?sample[box.owner].projectile:sample[box.owner].melee;
            if(!hit.active){
                hit.active=true;hit.x=box.x+box.w/2;hit.y=box.y+box.h/2;hit.sourceBase=box.sourceBase;
            }
            // Stage42: retain each linked projectile's hitbox independently.
            // The first aggregate hit alone cannot time overlapping objects.
            if(box.kind==5){
                XM30Sample& ownerSample=sample[box.owner];
                for(int slot=0;slot<ownerSample.projectileCount && slot<12;++slot){
                    if(ownerSample.projectileBases[slot]!=box.sourceBase)continue;
                    XM30Hit& own=ownerSample.projectileHits[slot];
                    if(!own.active){
                        own.active=true;own.sourceBase=box.sourceBase;
                        own.x=box.x+box.w/2;own.y=box.y+box.h/2;
                    }
                    break;
                }
            }
        }
        xm34ProbeFrame(c,sample);
        xm35RawHitboxProbe(c);
        xm37HitConfirmProbe(c,sample);
        xm38HitSourceProbe(c);
        // Stage41: read-only fighter state for a conservative return-to-neutral
        // estimate. This is NOT a decoded actionability / cancel permission bit.
        XM41ActorSample actor[2];
        for(int p=0;p<2;++p){
            const unsigned base=0xFF4000u+(unsigned)p*0x400u;
            actor[p].valid=xm23::r8(c,base)!=0;
            actor[p].action=xm23::r8(c,base+0xA0);
            actor[p].nativeAction=xm23::r16(c,base+0x60);
            actor[p].mode=xm23::r16(c,base+0xA0); // controller; independent of jump/directions
            actor[p].attackPhase=xm23::r8(c,base+0x64); // original ROM: 80=attack, 40=recovery
            actor[p].nativeStep=xm23::r16(c,base+0x08); // original ROM 2E substage
            actor[p].hitStop=xm23::r8(c,base+0x85); // original ROM per-player freeze counter
            actor[p].animPtr=xm23::r32(c,base+0x34u);
            actor[p].animTimer=xm23::r16(c,base+0x3Eu);
            // Use the SAME active-teammate HP selected for the Training HUD.
            // The old fixed +210 word can follow the wrong bench character
            // after a swap and includes an unrelated high byte.
            actor[p].health=(unsigned)xm23State.health[p];
        }
        // Do not use the FF5009 round subcounter for cinematic stop:
        // Training can hold it fixed while native fighter actions progress.
        const unsigned long long logicalBefore=xm23State.frameData.frame;
        const unsigned long long p1Before=xm23State.frameData.playerFrame[0];
        const unsigned long long p2Before=xm23State.frameData.playerFrame[1];
        xm23State.frameData.onFrame(sample[0],sample[1],actor[0],actor[1]);
        xm251FrameClockTrace(c,sample,actor,logicalBefore,
                             xm23State.frameData.frame,p1Before,p2Before);
    }
    c->close();
}
#endif
