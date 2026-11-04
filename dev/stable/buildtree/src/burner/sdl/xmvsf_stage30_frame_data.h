#ifndef XMVSF_STAGE30_FRAME_DATA_H
#define XMVSF_STAGE30_FRAME_DATA_H
// Frame-accurate to the emulated real-frame clock, independent of GPU presents.
// Start is measured from a NEW mapped attack-input edge to the first observed
// live attacking hitbox. It is not a disassembled character animation startup.
// Motion detection is an INPUT HEURISTIC, NOT A game move-ID lookup.
// A tag indicates an observed direction sequence followed by a live attack box.
enum XM32Motion { XM32_NORMAL=0, XM32_QCF, XM32_QCB, XM32_DP,
                  XM32_HALF, XM32_DOUBLE, XM32_PROJECTILE };
static inline const char* xm32MotionTag(int motion) {
    switch(motion){
    case XM32_QCF:return "QC";
    case XM32_QCB:return "QB";
    case XM32_DP:return "DP";
    case XM32_HALF:return "HC";
    case XM32_DOUBLE:return "DQ";
    case XM32_PROJECTILE:return "PR";
    default:return "NM";
    }
}
struct XM32DirStep {int dir=5; unsigned long long frame=0;};
struct XM32MotionHistory {
    static const int CAP=32;
    XM32DirStep step[CAP];int count=0;
    void direction(int dir,unsigned long long frame){
        if(dir<1||dir>9)return;
        if(count && step[count-1].dir==dir)return;
        if(count==CAP){for(int i=1;i<CAP;++i)step[i-1]=step[i];--count;}
        step[count].dir=dir;step[count].frame=frame;++count;
    }
    bool matches(const int* seq,int n,unsigned long long now,int maxAge=30) const {
        int k=n-1; unsigned long long newest=0;
        for(int i=count-1;i>=0&&k>=0;--i){
            const XM32DirStep& s=step[i];
            if(now<s.frame||now-s.frame>(unsigned)maxAge)break;
            if(s.dir==5)continue;
            if(k==n-1 && s.dir==seq[k] && now-s.frame>8) return false;
            if(s.dir==seq[k]) {if(k==n-1)newest=s.frame;--k;}
        }
        return k<0 && now-newest<=8;
    }
    int classify(unsigned long long now) const {
        const int qf1[]={2,3,6},qf2[]={2,1,4};
        
        const int dp1[]={6,2,3},dp2[]={4,2,1};
        const int hc1[]={4,1,2,3,6},hc2[]={6,3,2,1,4};
        const int dq1[]={2,3,6,2,3,6},dq2[]={2,1,4,2,1,4};
        if(matches(dq1,6,now,45)||matches(dq2,6,now,45))return XM32_DOUBLE;
        if(matches(hc1,5,now)||matches(hc2,5,now))return XM32_HALF;
        if(matches(dp1,3,now)||matches(dp2,3,now))return XM32_DP;
        if(matches(qf1,3,now)||matches(qf2,3,now))return XM32_QCF;

        return XM32_NORMAL;
    }
};
// Stage 87.154: observe ROM command ownership, not a player's ground state.
// Neither direction inputs nor character height are used for attack termination.
struct XM41ActorSample {
    bool valid=false;
    unsigned action=0;          // legacy +A0 high byte (diagnostic only)
    unsigned nativeAction=0;    // full native fighter action +60
    unsigned mode=0;            // full native action controller +A0
};
// Native action-family classification from recorded ROM traces:
// A0=28xx: normal punches/kicks; A0=2Axx + action=2Cxx: specials;
// A0=2Axx + action=2Exx: supers (including their internal 2E substeps).
// Movement, jumping, landing, blocking and idle are NOT attack families.
static inline unsigned xm30NativeAttackFamily(const XM41ActorSample& a){
    if(!a.valid)return 0u;
    const unsigned mode=(a.mode>>8u)&255u;
    if(mode==0x28u)return 0x28u;
    if(mode==0x2Au){
        const unsigned family=(a.nativeAction>>8u)&255u;
        if(family==0x2Cu || family==0x2Eu)return family;
    }
    return 0u;
}
static inline bool xm30SameNativeAttack(const XM41ActorSample& a,
                                       unsigned family,unsigned firstAction){
    if(xm30NativeAttackFamily(a)!=family)return false;
    // Normal +60 action changes identify a new punch/kick in a sequence.
    // A super's 2E00->2E02->2E10 transitions belong to ONE super.
    return family!=0x28u || a.nativeAction==firstAction;
}
struct XM30Event {
    int player=0, startup=0, active=0, code=0, x=0, y=0, motion=XM32_NORMAL;
    unsigned long long inputFrame=0,spawnFrame=0,firstFrame=0,endFrame=0;
    int source=0; // 0=fighter hitbox; 1=projectile hitbox
    bool closed=false;
    bool noBox=false;  // Real input edge, but no supported hitbox detected before timeout.
    // Synchronized projectile-travel and separate fighter-recovery tracks.
    unsigned projectileBase=0;
    unsigned long long firstMotionFrame=0,lastMotionFrame=0,despawnFrame=0;
    int moveFrames=0, lastProjectileX=0, lastProjectileY=0;
    unsigned long long motionBits[16]={}; // exact movement-edge frames, up to 1024f
    bool motionOverflow=false;
    bool positionKnown=false;
    // Action duration is separate from the hitbox and projectile clocks.
    // Start at the first observed real ROM attack action, end on the first
    // different action/controller family, independent of input or height.
    bool attackObserved=false,attackEnded=false;
    unsigned attackFamily=0,attackAction=0;
    unsigned long long attackStartFrame=0,attackLastFrame=0,attackEndFrame=0;
};
struct XM30Float {
    bool visible=false;
    int player=0, startup=0, x=0, y=0, age=0;
};
struct XM30Hit { bool active=false; int x=0,y=0; unsigned sourceBase=0; };
struct XM30Sample {
    XM30Hit melee, projectile;
    int projectileCount=0;
    unsigned projectileBases[12]={};
    int projectileX[12]={},projectileY[12]={};
    XM30Hit projectileHits[12]; // attack-box signal keyed to linked projectile identity
};
struct XM30Track {
    long long pending=-1;
    int code=0, motion=XM32_NORMAL;
    // Index -1 if no active row; separate lifetimes prevent an old projectile
    // suppressing the rising edge of a new airborne/ground melee hitbox.
    int eventIndex[2]={-1,-1};
    bool previous[2]={false,false};
    // Projectile identity is observed in the existing CPS2 linked object list.
    // Match by actual object base; never invent a spawn time from animation.
    unsigned pendingSpawnBase=0;
    unsigned long long pendingSpawnFrame=0;
    unsigned long long pendingFirstMotion=0,pendingLastMotion=0;
    int pendingMoveFrames=0,pendingX=0,pendingY=0;
    unsigned long long pendingMotionBits[16]={};
    bool pendingMotionOverflow=false;
    bool pendingPositionKnown=false;
    // A not-yet-matched input keeps its own first observed attack action.
    // Existing events retain independent ROM-state tracking after new presses.
    unsigned pendingAttackFamily=0,pendingAttackAction=0;
    unsigned long long pendingAttackStart=0,pendingAttackLast=0,pendingAttackEnd=0;
    int previousProjectileCount=0;
    unsigned previousProjectileBases[12]={};
    int previousActiveCount=0;
    unsigned previousActiveBases[12]={};
};
struct XM30FrameData {
    static const int HISTORY=24, FLOATS=8, MAX_WAIT=120, FLOAT_LIFE=120;
    bool enabled=false;
    bool inCombat=false;
    unsigned long long frame=0;
    int count=0, floatNext=0;
    XM30Event history[HISTORY];
    XM30Float floats[FLOATS];
    XM30Track track[2];
    XM32MotionHistory directions[2];
    void clear(bool preserveToggle=true) {
        const bool setting=enabled;
        frame=0; count=0; floatNext=0; inCombat=false;
        for(int i=0;i<HISTORY;++i)history[i]=XM30Event{};
        for(int i=0;i<FLOATS;++i)floats[i]=XM30Float{};
        for(int i=0;i<2;++i){track[i]=XM30Track{};directions[i]=XM32MotionHistory{};}
        enabled=preserveToggle?setting:false;
    }
    void setEnabled(bool on){clear(false);enabled=on;}
    void beginCombat(){if(!inCombat){clear(true);inCombat=true;}}
    void direction(int player,bool up,bool down,bool left,bool right){
        if(!enabled||!inCombat||player<0||player>1)return;
        const int x=(right&&!left)?1:((left&&!right)?-1:0);
        const int y=(up&&!down)?1:((down&&!up)?-1:0);
        directions[player].direction(5+x+3*y,frame);
    }
    void press(int player,int code){
        if(!enabled||!inCombat||player<0||player>1 || code<0x80||code>0x85)return;
        XM30Track& t=track[player];
        // A+B pressed on the same frame is one candidate, not two moves.
        if(t.pending>=0 && (long long)frame-t.pending<=2)return;
        // Do not reject new attack inputs just because an older projectile is
        // still on screen. A new row still requires a NEW hitbox edge.
        t.pending=(long long)frame;
        t.pendingSpawnBase=0;
        t.pendingSpawnFrame=0;
        // A new button edge only starts the next candidate. It NEVER ends an
        // earlier attack: each existing card stops on native ROM action exit.
        t.pendingAttackFamily=t.pendingAttackAction=0u;
        t.pendingAttackStart=t.pendingAttackLast=t.pendingAttackEnd=0u;
        t.pendingFirstMotion=0;t.pendingLastMotion=0;t.pendingMoveFrames=0;
        t.pendingPositionKnown=false;t.pendingMotionOverflow=false;
        for(int word=0;word<16;++word)t.pendingMotionBits[word]=0;
        t.code=code;
        t.motion=directions[player].classify(frame);
        if(t.motion!=XM32_NORMAL)directions[player].count=0;
    }
    int appendEvent(){
        if(count==HISTORY){
            for(int k=1;k<HISTORY;++k)history[k-1]=history[k];
            --count;
            for(int p=0;p<2;++p)for(int source=0;source<2;++source){
                int& idx=track[p].eventIndex[source];
                if(idx==0)idx=-1;
                else if(idx>0)--idx;
            }
         }
        return count++;
    }
    void onFrame(const XM30Sample& p1,const XM30Sample& p2,
                 const XM41ActorSample& a1=XM41ActorSample{},
                 const XM41ActorSample& a2=XM41ActorSample{}){
        if(!enabled)return;
        beginCombat();
        ++frame;
        const XM30Sample samples[2]={p1,p2};
        const XM41ActorSample actors[2]={a1,a2};
        for(int i=0;i<FLOATS;++i)if(floats[i].visible){
            if(++floats[i].age>=FLOAT_LIFE)floats[i].visible=false;
        }
        for(int p=0;p<2;++p){
            XM30Track& t=track[p];
            const XM30Hit hits[2]={samples[p].melee,samples[p].projectile};
            // Record native action tenure without consulting current direction,
            // held buttons, Y-position, jumping, or the later idle stance.
            const unsigned family=xm30NativeAttackFamily(actors[p]);
            for(int i=0;i<count;++i){
                XM30Event& e=history[i];
                if(e.player!=p || !e.attackObserved || e.attackEnded)continue;
                if(xm30SameNativeAttack(actors[p],e.attackFamily,e.attackAction))
                    e.attackLastFrame=frame;
                else {
                    // First nonmatching ROM frame is already the NEXT action.
                    // Last matching frame is the exact inclusive end frame.
                    e.attackEndFrame=e.attackLastFrame;
                    e.attackEnded=true;
                }
            }
            // Capture the action associated with the pending input before its
            // hitbox may appear. If it already exited, save its end; never
            // rearm from unrelated walking/jumping/another special later.
            if(t.pending>=0 && frame>=(unsigned long long)t.pending &&
               frame-(unsigned long long)t.pending<=MAX_WAIT){
                if(!t.pendingAttackFamily && family){
                    t.pendingAttackFamily=family;
                    t.pendingAttackAction=actors[p].nativeAction;
                    t.pendingAttackStart=t.pendingAttackLast=frame;
                }else if(t.pendingAttackFamily && !t.pendingAttackEnd){
                    if(xm30SameNativeAttack(actors[p],t.pendingAttackFamily,
                                            t.pendingAttackAction))
                        t.pendingAttackLast=frame;
                    else
                        t.pendingAttackEnd=t.pendingAttackLast;
                }
            }
            // Every projectile record is advanced by ITS OWN linked object.
            // An unrelated live projectile, or a held direction/button, cannot
            // extend its active count. Disappearance of this exact object ends
            // its chart and closes its attack-box lifetime.
            for(int i=0;i<count;++i){
                XM30Event& e=history[i];
                if(e.player!=p || e.source!=1 || !e.projectileBase || e.despawnFrame)continue;
                int slot=-1;
                for(int j=0;j<samples[p].projectileCount && j<12;++j)
                    if(samples[p].projectileBases[j]==e.projectileBase){slot=j;break;}
                if(slot<0){
                    e.despawnFrame=frame; // first frame the linked object is absent
                    if(!e.closed){e.closed=true;e.endFrame=frame-1;}
                }else{
                    const int x=samples[p].projectileX[slot],y=samples[p].projectileY[slot];
                    const XM30Hit& own=samples[p].projectileHits[slot];
                    const bool ownAttack=own.active && own.sourceBase==e.projectileBase;
                    if(!e.closed && frame>e.firstFrame){
                        if(ownAttack)++e.active;
                        else {e.closed=true;e.endFrame=frame-1;}
                    }
                    if(e.positionKnown && (x!=e.lastProjectileX || y!=e.lastProjectileY)){
                        if(!e.firstMotionFrame)e.firstMotionFrame=frame;
                        e.lastMotionFrame=frame;++e.moveFrames;
                        const unsigned long long delta=frame-e.inputFrame;
                        if(delta<1024)e.motionBits[delta/64] |= (1ull<<(delta%64));
                        else e.motionOverflow=true;
                    }
                    e.lastProjectileX=x;e.lastProjectileY=y;e.positionKnown=true;
                }
            }
            // Record only a genuinely new projectile-object base while a move
            // input is pending. A known old projectile cannot create a false
            // spawn marker for the next move.
            for(int n=0;n<samples[p].projectileCount && n<12;++n){
                const unsigned base=samples[p].projectileBases[n];
                bool wasPresent=false;
                for(int k=0;k<t.previousProjectileCount;++k)
                    if(t.previousProjectileBases[k]==base)wasPresent=true;
                if(!wasPresent && t.pending>=0 && !t.pendingSpawnBase){
                    t.pendingSpawnBase=base;
                    t.pendingSpawnFrame=frame;
                    t.pendingPositionKnown=true;
                    t.pendingX=samples[p].projectileX[n];
                    t.pendingY=samples[p].projectileY[n];
                }
            }
            if(t.pendingSpawnBase){
                for(int n=0;n<samples[p].projectileCount && n<12;++n){
                    if(samples[p].projectileBases[n]!=t.pendingSpawnBase)continue;
                    const int x=samples[p].projectileX[n],y=samples[p].projectileY[n];
                    if(t.pendingPositionKnown && (x!=t.pendingX || y!=t.pendingY)){
                        if(!t.pendingFirstMotion)t.pendingFirstMotion=frame;
                        t.pendingLastMotion=frame;++t.pendingMoveFrames;
                        const unsigned long long delta=frame-(unsigned long long)t.pending;
                        if(delta<1024)t.pendingMotionBits[delta/64] |= (1ull<<(delta%64));
                        else t.pendingMotionOverflow=true;
                    }
                    t.pendingX=x;t.pendingY=y;t.pendingPositionKnown=true;
                    break;
                }
            }
            // Fighter-attached hitboxes retain their own rising-edge counter.
            // Projectile active frames are handled above PER LINKED OBJECT.
            int& meleeIndex=t.eventIndex[0];
            if(meleeIndex>=0){
                if(!hits[0].active){
                    if(meleeIndex<count){history[meleeIndex].closed=true;history[meleeIndex].endFrame=frame-1;}
                    meleeIndex=-1;
                }else if(meleeIndex<count)++history[meleeIndex].active;
                else meleeIndex=-1;
            }
            const long long age=t.pending>=0?(long long)frame-t.pending:-1;
            if(t.pending>=0 && age<=MAX_WAIT){
                const bool meleeRise=hits[0].active && !t.previous[0];
                XM30Hit newProjectile{};
                for(int j=0;j<samples[p].projectileCount && j<12;++j){
                    const XM30Hit& h=samples[p].projectileHits[j];
                    if(!h.active || !h.sourceBase)continue;
                    if(t.pendingSpawnBase && h.sourceBase!=t.pendingSpawnBase)continue;
                    bool already=false;
                    for(int k=0;k<t.previousActiveCount;++k)
                        if(t.previousActiveBases[k]==h.sourceBase)already=true;
                    if(!already){newProjectile=h;break;}
                }
                // Compatibility with readers that only supply a primary box.
                if(!newProjectile.active && hits[1].active && !t.previous[1] &&
                   (!t.pendingSpawnBase || hits[1].sourceBase==t.pendingSpawnBase))
                    newProjectile=hits[1];
                const bool projectileRise=newProjectile.active;
                if(meleeRise || projectileRise){
                    const int source=(projectileRise && (!meleeRise || t.motion!=XM32_NORMAL))?1:0;
                    const XM30Hit& chosen=source==1?newProjectile:hits[0];
                    const int idx=appendEvent();
                    XM30Event& e=history[idx];e=XM30Event{};
                    e.player=p;e.startup=(int)age;e.active=1;
                    e.attackObserved=t.pendingAttackFamily!=0u;
                    e.attackFamily=t.pendingAttackFamily;
                    e.attackAction=t.pendingAttackAction;
                    e.attackStartFrame=t.pendingAttackStart;
                    e.attackLastFrame=t.pendingAttackLast;
                    e.attackEndFrame=t.pendingAttackEnd;
                    e.attackEnded=t.pendingAttackEnd!=0u;
                    e.inputFrame=(unsigned long long)t.pending;
                    e.source=source;
                    if(source==1 && t.pendingSpawnBase &&
                       chosen.sourceBase==t.pendingSpawnBase)
                        e.spawnFrame=t.pendingSpawnFrame;
                    e.code=t.code;e.x=chosen.x;e.y=chosen.y;e.firstFrame=frame;
                    e.motion=t.motion!=XM32_NORMAL?t.motion:
                        (source==1?XM32_PROJECTILE:XM32_NORMAL);
                    if(source==1){
                        e.projectileBase=chosen.sourceBase;
                        if(e.spawnFrame){
                            e.firstMotionFrame=t.pendingFirstMotion;
                            e.lastMotionFrame=t.pendingLastMotion;
                            e.moveFrames=t.pendingMoveFrames;
                            for(int word=0;word<16;++word)
                                e.motionBits[word]=t.pendingMotionBits[word];
                            e.motionOverflow=t.pendingMotionOverflow;
                            e.lastProjectileX=t.pendingX;e.lastProjectileY=t.pendingY;
                            e.positionKnown=t.pendingPositionKnown;
                        }else{
                            // Spawn timestamp is unknown, but the linked
                            // object's identity and position are observable.
                            for(int j=0;j<samples[p].projectileCount && j<12;++j)
                                if(samples[p].projectileBases[j]==e.projectileBase){
                                    e.lastProjectileX=samples[p].projectileX[j];
                                    e.lastProjectileY=samples[p].projectileY[j];
                                    e.positionKnown=true;break;
                                }
                        }
                    }
                    if(source==0)t.eventIndex[0]=idx;
                    XM30Float& popup=floats[floatNext++%FLOATS];popup=XM30Float{};
                    popup.visible=true;popup.player=p;popup.startup=e.startup;
                    popup.x=e.x;popup.y=e.y;
                    t.pending=-1;
                }
            }
            if(t.pending>=0 && age>MAX_WAIT){
                // Report missing signal as missing, never fabricate an S/A value.
                const int idx=appendEvent();
                XM30Event& e=history[idx];e=XM30Event{};
                e.player=p;e.code=t.code;e.motion=t.motion;e.noBox=true;
                e.attackObserved=t.pendingAttackFamily!=0u;
                e.attackFamily=t.pendingAttackFamily;
                e.attackAction=t.pendingAttackAction;
                e.attackStartFrame=t.pendingAttackStart;
                e.attackLastFrame=t.pendingAttackLast;
                e.attackEndFrame=t.pendingAttackEnd;
                e.attackEnded=t.pendingAttackEnd!=0u;
                e.inputFrame=(unsigned long long)t.pending;
                e.endFrame=frame;e.closed=true;
                t.pending=-1;
            }
            t.previous[0]=hits[0].active;
            t.previous[1]=hits[1].active;
            t.previousActiveCount=0;
            for(int j=0;j<samples[p].projectileCount && j<12;++j)
                if(samples[p].projectileHits[j].active &&
                   t.previousActiveCount<12)
                    t.previousActiveBases[t.previousActiveCount++]=samples[p].projectileHits[j].sourceBase;
            t.previousProjectileCount=(samples[p].projectileCount<12?samples[p].projectileCount:12);
            for(int k=0;k<t.previousProjectileCount;++k)
                t.previousProjectileBases[k]=samples[p].projectileBases[k];
        }
    }
};
#endif
