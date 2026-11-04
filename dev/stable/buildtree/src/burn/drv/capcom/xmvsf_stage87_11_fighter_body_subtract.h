#ifndef XMVSF_STAGE87_11_FIGHTER_BODY_SUBTRACT_H
#define XMVSF_STAGE87_11_FIGHTER_BODY_SUBTRACT_H
// XM87.11: RGB-first gameplay OBJ rendering. The only reason to apply the
// existing fighter ink palette is an actor-anchored sprite group, not an old
// palette/FX classifier. This is a geometric ownership HYPOTHESIS (not a ROM
// ownership ID). Individual overlapping FX may still need refinement.
// The actual fighter positions come from XM8707::Snapshot() (game RAM).
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
namespace XM8711 {
static inline bool Enabled(){
    static const bool on=[](){
        const char* v=std::getenv("XM8711_BODY_ONLY_GRAY");
        return !(v && v[0]=='0' && !v[1]);
    }();
    return on;
}
static inline bool BridgeEnabled(){
    static const bool enabled=[](){
        const char* v=std::getenv("XM8712_LIGHTING_BRIDGE");
        return !(v && v[0]=='0' && !v[1]);
    }();
    return enabled;
}
static inline unsigned Word(const UINT16* p,unsigned at){
    return (unsigned)BURN_ENDIAN_SWAP_INT16(p[at]);
}
// Same full CPS2 tile number as the existing cps_obj.cpp draw path.
// Priority+palette are attributes, not a reliable fighter identity.
static inline unsigned Tile(const UINT16* obj,unsigned i){
    return Word(obj,i*4u+2u) | ((Word(obj,i*4u+1u)&0x6000u)<<3);
}
static inline int S10(unsigned x){
    x &= 1023u;
    return (x&512u)?int(x)-1024:int(x);
}
struct Group{ unsigned start=0,end=0,priority=0,palette=0;
    int x0=32767,y0=32767,x1=-32768,y1=-32768;
    bool actorBody=false;
};
struct Cache {
    const UINT16* ptr=nullptr;
    unsigned frame=~0u,count=0;
    int sx=0,sy=0;
    bool actorValid=false;
    std::vector<unsigned char> mask;
    struct BodyEvidence {unsigned lastFrame=0,distinctFrames=0;};
    std::unordered_map<unsigned,BodyEvidence> bodyTiles;
    unsigned long long bridged=0;
    unsigned long long articulated=0;
    unsigned previousFrame=~0u;
    // XM87.14: the actor's body palette is learned from repeated compact
    // actor-following groups at match entry. These are NOT FX whitelists.
    // A different palette overlapping the fighter is not body ownership.
    struct PaletteOwner {
        int primary=-1;
        int candidate=-1;
        unsigned candidateRun=0;
        unsigned lastCandidateFrame=~0u;
        unsigned lastPrimaryFrame=~0u;
        unsigned substituteRun=0;
    } owner[2];
    unsigned long long foreignGroups=0;
    unsigned lastLearnFrame=~0u;
    static bool ProvenanceEnabled(){
        static const bool on=[](){
            const char* v=std::getenv("XM8714_BODY_PALETTE_PROVENANCE");
            return !(v && v[0]=='0' && !v[1]);
        }();
        return on;
    }
    struct Candidate {int palette=-1,score=-999999;};
    void LearnOwners(const UINT16* obj,unsigned n,unsigned f,int shiftX,int shiftY,
                     const XM8707::Frame& state){
        if(!ProvenanceEnabled())return;
        if(lastLearnFrame==f)return;
        lastLearnFrame=f;
        Candidate best[2];
        for(unsigned start=0;start<n;){
            const unsigned p=Word(obj,start*4u)>>13;
            const int pal=int(Word(obj,start*4u+3u)&31u);
            unsigned end=start+1u;
            while(end<n && (Word(obj,end*4u)>>13)==p &&
                int(Word(obj,end*4u+3u)&31u)==pal)++end;
            const unsigned count=end-start;
            if(p==6u && count>=3u && count<=62u){
                int x0=32767,y0=32767,x1=-32768,y1=-32768;
                for(unsigned i=start;i<end;++i){
                    const unsigned a=Word(obj,i*4u+3u);
                    const int x=S10(Word(obj,i*4u))+shiftX;
                    const int y=S10(Word(obj,i*4u+1u))+shiftY;
                    const int right=x+int((((a>>8)&15u)+1u)*16u);
                    const int bottom=y+int((((a>>12)&15u)+1u)*16u);
                    if(x<x0)x0=x;
                    if(y<y0)y0=y;
                    if(right>x1)x1=right;
                    if(bottom>y1)y1=bottom;
                }
                const int w=x1-x0,h=y1-y0;
                // A strict, compact baseline. No wide super/force-field
                // groups are allowed to establish an actor's identity.
                if(w>=32 && w<=176 && h>=64 && h<=208){
                    for(unsigned a=0;a<2u;++a){
                        if(!state.actor[a].active)continue;
                        const int ax=state.actor[a].screenX;
                        const int ay=state.actor[a].screenY;
                        const int dx=ax<x0?x0-ax:(ax>x1?ax-x1:0);
                        const int dy=y1>ay?y1-ay:ay-y1;
                        const int cx=(x0+x1)/2;
                        const int dc=cx>ax?cx-ax:ax-cx;
                        if(dx>12 || dy>24 || dc>64)continue;
                        // Prefer persistent dense body assemblies to single
                        // special-effect pieces even when both are nearby.
                        const int score=int(count)*24-dc-dy;
                        if(score>best[a].score){best[a].score=score;best[a].palette=pal;}
                    }
                }
            }
            start=end;
        }
        for(unsigned a=0;a<2u;++a){
            PaletteOwner& x=owner[a];
            if(!state.actor[a].active){x=PaletteOwner();continue;}
            const int proposed=best[a].palette;
            if(proposed<0)continue;
            if(proposed==x.candidate && x.lastCandidateFrame!=~0u &&
               f>=x.lastCandidateFrame && f-x.lastCandidateFrame<=2u)
                ++x.candidateRun;
            else {x.candidate=proposed;x.candidateRun=1u;}
            x.lastCandidateFrame=f;
            if(x.primary==proposed){x.lastPrimaryFrame=f;x.substituteRun=0u;}
            if(x.primary<0 && x.candidateRun>=3u){
                x.primary=proposed;x.lastPrimaryFrame=f;
                std::fprintf(stderr,"XM87.14 BODY_PALETTE_LOCK frame=%u actor=%u palette=%d\n",f,a,x.primary);
            }
            // A tag may load a different fighter into the same actor slot.
            // Never relearn during brief effects: require the old main body
            // palette to be absent for a long time and the replacement to
            // follow the RAM actor for many consecutive frames.
            if(x.primary>=0 && proposed!=x.primary &&
               x.lastPrimaryFrame!=~0u && f>=x.lastPrimaryFrame &&
               f-x.lastPrimaryFrame>90u && x.candidateRun>=12u &&
               state.actor[a].anim!=12u && state.actor[a].screenY>=180){
                x.primary=proposed;x.lastPrimaryFrame=f;
                bodyTiles.clear();
                std::fprintf(stderr,"XM87.14 BODY_PALETTE_TAG_RELOCK frame=%u actor=%u palette=%d\n",f,a,x.primary);
            }
        }
    }
    // XM87.13: a real fighter pose can be wider than the old 176-pixel
    // palette-group limit, especially during jumps and air attacks. Accept
    // *only* a tall, multi-entry group whose center and lower edge follow
    // the actor in RAM. This never uses palette or previously guessed grades
    // as an ownership label; thin projectiles and giant supers remain RGB.
    static bool ArticulatedBody(unsigned n,int w,int h,int x0,int x1,int y1,int ax,int ay){
        if(n<6u || n>62u || w<32 || w>288 || h<96 || h>208)return false;
        const int center=(x0+x1)/2;
        const int deltaX=center>ax?center-ax:ax-center;
        if(deltaX>72)return false;
        const int dx=ax<x0?x0-ax:(ax>x1?ax-x1:0);
        const int deltaY=y1>ay?y1-ay:ay-y1;
        return dx<=16 && deltaY<=40;
    }
    // XM87.15: actor-aligned AIRBODY evidence is separate from an FX allowlist.
    // A jumping fighter can use a DIFFERENT low palette bank than its normal
    // standing costume. Never promote an arbitrary high-bank effect palette.
    static bool AirAlternateEnabled(){
        static const bool on=[](){
            const char* p=std::getenv("XM8715_AIR_ALTERNATE_BODY");
            return !(p && p[0]=='0' && !p[1]);
        }();
        return on;
    }
    static bool TinyFragmentEnabled(){
        static const bool on=[](){
            const char* p=std::getenv("XM8715_TINY_BODY_FRAGMENTS");
            return !(p && p[0]=='0' && !p[1]);
        }();
        return on;
    }
    unsigned long long recoveredAir=0, recoveredTiny=0;
    unsigned long long auditedGroups=0;
    // Only a diagnostic; no body decision depends on its output.
    void Audit(const UINT16* obj,unsigned n,unsigned f,int shiftX,int shiftY,
               const XM8707::Frame& state){
        static FILE* fp=[](){
            const char* path=std::getenv("XM8715_GROUP_AUDIT_FILE");
            if(!path || !*path)return (FILE*)nullptr;
            FILE* out=std::fopen(path,"wb");
            if(out)std::fprintf(out,"frame\tpriority\tpalette\tobjects\tfirst_tile\tx0\ty0\tx1\ty1\tbody_objects\tactor0_x\tactor0_y\tactor1_x\tactor1_y\n");
            return out;
        }();
        if(!fp || f%4u || auditedGroups>=75000ull)return;
        for(unsigned start=0;start<n;){
            unsigned p=Word(obj,start*4u)>>13;
            unsigned pal=Word(obj,start*4u+3u)&31u;
            unsigned end=start+1u;
            while(end<n && (Word(obj,end*4u)>>13)==p &&
                 (Word(obj,end*4u+3u)&31u)==pal)++end;
            if(p>=5u && p<=7u){
                int x0=32767,y0=32767,x1=-32768,y1=-32768,masked=0;
                for(unsigned i=start;i<end;++i){
                    unsigned attr=Word(obj,i*4u+3u);
                    int x=S10(Word(obj,i*4u))+shiftX;
                    int y=S10(Word(obj,i*4u+1u))+shiftY;
                    int r=x+int((((attr>>8)&15u)+1u)*16u);
                    int b=y+int((((attr>>12)&15u)+1u)*16u);
                    if(x<x0)x0=x;
                    if(y<y0)y0=y;
                    if(r>x1)x1=r;
                    if(b>y1)y1=b;
                    masked+=mask[i]?1:0;
                }
                bool near=(p!=7u);
                // Priority 7 is predominantly HUD: keep only actor-adjacent
                // samples to catch alternative force-field rendering layers.
                if(p==7u){
                    for(unsigned a=0;a<2u;++a){
                        if(!state.actor[a].active)continue;
                        const int ax=state.actor[a].screenX,ay=state.actor[a].screenY;
                        if(x1>=ax-160 && x0<=ax+160 && y1>=ay-224 && y0<=ay+96)
                            near=true;
                    }
                }
                if(near){
                    std::fprintf(fp,"%u\t%u\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                        f,p,pal,end-start,Tile(obj,start),x0,y0,x1,y1,masked,
                        state.actor[0].screenX,state.actor[0].screenY,
                        state.actor[1].screenX,state.actor[1].screenY);
                    ++auditedGroups;
                    if(auditedGroups%1024ull==0ull)std::fflush(fp);
                }
            }
            start=end;
        }
    }
    static bool AirGeometryEnabled(){
        static const bool on=[](){
            const char* v=std::getenv("XM8713_AIR_BODY_GEOMETRY");
            return !(v && v[0]=='0' && !v[1]);
        }();
        return on;
    }
    void Rebuild(const UINT16* obj,unsigned n,unsigned f,int shiftX,int shiftY){
        // Combat can exit and restart without resetting the global frame
        // counter. A long gap invalidates the prior fighter palette identity.
        if(previousFrame!=~0u && (f<previousFrame || f-previousFrame>90u)){
            bodyTiles.clear();owner[0]=PaletteOwner();owner[1]=PaletteOwner();lastLearnFrame=~0u;
        }
        previousFrame=f;
        ptr=obj;count=n;frame=f;sx=shiftX;sy=shiftY;
        mask.assign(n,0);
        // Fresh actor coordinates are read once per CPS-2 OBJ frame.
        const auto state=XM8707::Snapshot();
        actorValid=state.valid;
        if(!actorValid)return;
        LearnOwners(obj,n,f,shiftX,shiftY,state);
        for(unsigned start=0;start<n;){
            const unsigned p=Word(obj,start*4)>>13;
            const unsigned pal=Word(obj,start*4+3)&31u;
            unsigned end=start+1;
            while(end<n && (Word(obj,end*4)>>13)==p &&
                (Word(obj,end*4+3)&31u)==pal)++end;
            if(p==6u && end-start>=3u && end-start<=62u){
                Group g;g.start=start;g.end=end;g.priority=p;g.palette=pal;
                for(unsigned i=start;i<end;++i){
                    const int x=S10(Word(obj,i*4))+shiftX;
                    const int y=S10(Word(obj,i*4+1))+shiftY;
                    const unsigned a=Word(obj,i*4+3);
                    const int right=x+int((((a>>8)&15u)+1u)*16u);
                    const int bottom=y+int((((a>>12)&15u)+1u)*16u);
                    if(x<g.x0)g.x0=x;
                    if(y<g.y0)g.y0=y;
                    if(right>g.x1)g.x1=right;
                    if(bottom>g.y1)g.y1=bottom;
                }
                const int w=g.x1-g.x0,h=g.y1-g.y0;
                // Real fighters form compact groups whose lower edge follows
                // the actor's RAM Y coordinate (including jumping). A super
                // extending hundreds of pixels or a projectile flying away
                // does not satisfy that relation.
                if((w>=32 && w<=176 && h>=32 && h<=210) ||
                    (AirGeometryEnabled() && w>=32 && w<=288 && h>=96 && h<=208 && end-start>=6u)){
                    for(unsigned a=0;a<2;++a){
                        if(!state.actor[a].active)continue;
                        // A close effect is NOT a fighter just because its
                        // group is shaped like a large jump animation. Only
                        // the established per-actor body palette may qualify
                        // for geometric body ownership. Alternate palette
                        // flashes are handled solely by proven body tile IDs.
                        const bool primary=(owner[a].primary==int(pal));
                        const bool airAlternate=ProvenanceEnabled() && AirAlternateEnabled() &&
                            owner[a].primary>=0 && !primary && pal<16u &&
                            int(pal)!=owner[1u-a].primary &&
                            state.actor[a].anim==12u && end-start>=6u &&
                            w>=32 && w<=176 && h>=64 && h<=208;
                        if(ProvenanceEnabled() && !primary && !airAlternate)continue;
                        const int ax=state.actor[a].screenX;
                        const int ay=state.actor[a].screenY;
                        const int dx=ax<g.x0?g.x0-ax:(ax>g.x1?ax-g.x1:0);
                        const int dy=g.y1>ay?g.y1-ay:ay-g.y1;
                        if(w>=32 && w<=176 && h>=32 && h<=210 &&
                           dx<=16 && dy<=(airAlternate?40:28)){
                            const int cx=(g.x0+g.x1)/2;
                            const int dc=cx>ax?cx-ax:ax-cx;
                            if(!airAlternate || dc<=64){
                                g.actorBody=true;
                                if(airAlternate){
                                    ++recoveredAir;
                                    if(recoveredAir<=12ull || recoveredAir%4096ull==0ull)
                                        std::fprintf(stderr,"XM87.15 AIR_ALT_BODY frame=%u actor=%u pal=%u n=%u count=%llu\n",
                                            f,a,pal,end-start,recoveredAir);
                                }
                                break;
                            }
                        }
                        // Broader *body-shaped* pose; only activate if the
                        // same RAM actor is at its central lower edge.
                        if(AirGeometryEnabled() &&
                           ArticulatedBody(end-start,w,h,g.x0,g.x1,g.y1,ax,ay)){
                            g.actorBody=true;
                            ++articulated;
                            if(articulated<=16ull || articulated%4096ull==0ull)
                                std::fprintf(stderr,
                                    "XM87.13 AIR_BODY_GEOMETRY frame=%u actor=%u n=%u bbox=%dx%d dy=%d count=%llu\n",
                                    f,a,end-start,w,h,dy,articulated);
                            break;
                        }
                    }
                }
                if(!g.actorBody && ProvenanceEnabled() &&
                   (w>=32 && w<=288 && h>=96 && h<=208) &&
                   end-start>=6u && (owner[0].primary>=0 || owner[1].primary>=0) &&
                   foreignGroups++<12ull)
                    std::fprintf(stderr,"XM87.14 FOREIGN_GROUP_RGB frame=%u pal=%u n=%u bbox=%dx%d\n",
                        f,pal,end-start,w,h);
                if(g.actorBody){
                    for(unsigned i=start;i<end;++i){
                        mask[i]=1u;
                        // Only primary-palette, actor-anchored groups may
                        // teach enduring body identity; old `fighter` labels
                        // and alternate jump palette groups NEVER do.
                        if(bodyTiles.size()>8192u)bodyTiles.clear();
                        bool acceptedPrimary=false;
                        for(unsigned a=0;a<2u;++a)
                            if(owner[a].primary==int(pal))acceptedPrimary=true;
                        if(ProvenanceEnabled() && !acceptedPrimary)continue;
                        BodyEvidence& evidence=bodyTiles[Tile(obj,i)];
                        if(evidence.lastFrame!=f){
                            if(evidence.distinctFrames<65535u)++evidence.distinctFrames;
                            evidence.lastFrame=f;
                        }
                    }
                }
            }
            start=end;
        }
        // XM87.12 BODY_LIGHTING_BRIDGE: a fighter changes palette during
        // hit flashes / lighting. The contiguous same-palette group can split
        // into 1-2 sprite fragments, so the old group-only test flashes RGB.
        // Follow *previously established body tiles*, regardless of palette,
        // but only while they remain in an active fighter's screen envelope.
        // This never learns from an effect, an unknown tile, or old grade labels.
        if(BridgeEnabled()){
            for(unsigned i=0;i<n;++i){
                if(mask[i] || (Word(obj,i*4u)>>13)!=6u)continue;
                const auto it=bodyTiles.find(Tile(obj,i));
                if(it==bodyTiles.end() || it->second.distinctFrames<2u)continue;
                if(f<it->second.lastFrame || f-it->second.lastFrame>180u)continue;
                const unsigned attr=Word(obj,i*4u+3u);
                const int w=int((((attr>>8)&15u)+1u)*16u);
                const int h=int((((attr>>12)&15u)+1u)*16u);
                if(w>128 || h>160)continue; // reject super-sized artwork
                const int x=S10(Word(obj,i*4u))+shiftX;
                const int y=S10(Word(obj,i*4u+1u))+shiftY;
                bool anchored=false;
                for(unsigned a=0;a<2u;++a){
                    if(!state.actor[a].active)continue;
                    const int ax=state.actor[a].screenX;
                    const int ay=state.actor[a].screenY;
                    // Bounding box is contained in the fighter silhouette
                    // envelope, including high jump poses and feet/shadows.
                    if(x+w<ax-96 || x>ax+96)continue;
                    if(y+h<ay-232 || y>ay+44)continue;
                    anchored=true;break;
                }
                if(anchored){
                    mask[i]=1u;
                    ++bridged;
                    if(bridged<=12ull || bridged%8192ull==0ull)
                        std::fprintf(stderr,
                            "XM87.12 BODY_LIGHTING_BRIDGE frame=%u tile=%u pal=%u bridged=%llu\n",
                            f,Tile(obj,i),attr&31u,bridged);
                }
            }
        }
        // XM87.15: short 1..4 sprite body fragments on dash / initial
        // superjump frames can fail EVERY whole-group geometry test. Their
        // established primary actor palette plus tight core positioning is
        // stronger evidence than general proximity alone. No foreign palette
        // or large force-field group qualifies here.
        if(TinyFragmentEnabled() && ProvenanceEnabled()){
            for(unsigned start=0;start<n;){
                unsigned p=Word(obj,start*4u)>>13;
                unsigned pal=Word(obj,start*4u+3u)&31u;
                unsigned end=start+1u;
                while(end<n && (Word(obj,end*4u)>>13)==p &&
                     (Word(obj,end*4u+3u)&31u)==pal)++end;
                if(p==6u && end-start<=4u){
                    for(unsigned a=0;a<2u;++a){
                        if(!state.actor[a].active || owner[a].primary!=int(pal))continue;
                        const int ax=state.actor[a].screenX;
                        const int ay=state.actor[a].screenY;
                        bool compact=true;
                        for(unsigned i=start;i<end;++i){
                            unsigned attr=Word(obj,i*4u+3u);
                            int w=int((((attr>>8)&15u)+1u)*16u);
                            int h=int((((attr>>12)&15u)+1u)*16u);
                            int x=S10(Word(obj,i*4u))+shiftX;
                            int y=S10(Word(obj,i*4u+1u))+shiftY;
                            const int center=x+w/2;
                            if(w>64 || h>80 || center<ax-48 || center>ax+48 ||
                               y+h<ay-164 || y>ay+32){compact=false;break;}
                        }
                        if(compact){
                            for(unsigned i=start;i<end;++i){
                                if(!mask[i]){mask[i]=1u;++recoveredTiny;}
                            }
                            if(recoveredTiny<=12ull || recoveredTiny%4096ull==0ull)
                                std::fprintf(stderr,"XM87.15 TINY_BODY frame=%u actor=%u pal=%u n=%u count=%llu\n",
                                    f,a,pal,end-start,recoveredTiny);
                            break;
                        }
                    }
                }
                start=end;
            }
        }
        Audit(obj,n,f,shiftX,shiftY,state);
    }
    bool IsBody(const UINT16* obj,int total,unsigned f,int shiftX,int shiftY,unsigned slot){
        if(!obj || total<=0 || total>4096 || slot>=(unsigned)total)return false;
        if(ptr!=obj||frame!=f||count!=(unsigned)total||sx!=shiftX||sy!=shiftY)
            Rebuild(obj,(unsigned)total,f,shiftX,shiftY);
        return actorValid && mask[slot]!=0;
    }
};
static inline bool IsBody(const UINT16* obj,int total,unsigned frame,int sx,int sy,unsigned slot){
    static Cache cache;
    return cache.IsBody(obj,total,frame,sx,sy,slot);
}
} // namespace XM8711
#endif
