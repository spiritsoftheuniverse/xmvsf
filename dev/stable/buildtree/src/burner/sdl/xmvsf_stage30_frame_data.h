#ifndef XMVSF_STAGE30_FRAME_DATA_H
#define XMVSF_STAGE30_FRAME_DATA_H
#include <cstdio> // report rare stalled frame cards only when detected
// Frame-accurate to the emulated real-frame clock, independent of GPU presents.
// Start is measured from a NEW mapped attack-input edge to the first observed
// live attacking hitbox. It is not a disassembled character animation startup.
// Motion detection is an INPUT HEURISTIC, NOT A game move-ID lookup.
// A tag indicates an observed direction sequence followed by a live attack box.
enum XM32Motion { XM32_NORMAL=0, XM32_QCF, XM32_QCB, XM32_DP,
                  XM32_HALF, XM32_DOUBLE, XM32_PROJECTILE, XM32_QCD };
static inline const char* xm32MotionTag(int motion) {
    switch(motion){
    case XM32_QCF:return "QC";
    case XM32_QCB:return "QB";
    case XM32_QCD:return "QD";
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
        // 632/412: quarter-circle DOWN, including the aimed Cyclops
        // hyper command. Motion detection is generic, never tied to a
        // particular character, move name or action ID.
        const int qd1[]={6,3,2},qd2[]={4,1,2};
        const int hc1[]={4,1,2,3,6},hc2[]={6,3,2,1,4};
        const int dq1[]={2,3,6,2,3,6},dq2[]={2,1,4,2,1,4};
        if(matches(dq1,6,now,45)||matches(dq2,6,now,45))return XM32_DOUBLE;
        if(matches(hc1,5,now)||matches(hc2,5,now))return XM32_HALF;
        if(matches(dp1,3,now)||matches(dp2,3,now))return XM32_DP;
        if(matches(qd1,3,now)||matches(qd2,3,now))return XM32_QCD;
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
    unsigned attackPhase=0;     // ROM +64: 0x80 attacking, 0x40 recovery
    unsigned nativeStep=0;      // ROM +08: super 2E script stage, not a frame timer
    unsigned hitStop=0;         // ROM +85: explicit per-fighter stop steps
    unsigned animPtr=0;         // ROM +34 native animation script pointer
    unsigned animTimer=0;       // ROM +3E native animation frame countdown
    unsigned health=0;          // HUD-resolved active fighter HP
    // Optional native HUD combo signal (per-player, read-only). This
    // can continue increasing after Training has clamped HP to 1.
    unsigned combo=0;
    bool comboKnown=false;
    unsigned sourceBase=0; // active primary or reserve ROM object
};
// Native action-family classification from recorded ROM traces:
// A0=28xx: normal punches/kicks; A0=2Axx + action=2Cxx: specials;
// A0=2Axx + action=2Exx: supers (including their internal 2E substeps).
// Movement, jumping, landing, blocking and idle are NOT attack families.
static inline unsigned xm30ActiveFighterBase(int player,bool reserve){
    return 0xFF4000u+(unsigned)player*0x400u+(reserve?0x800u:0u);
}
// Controller 32 is the native throw/grab sequence. Keep it observable
// rather than treating command grabs, counters and throws as neutral.
static const unsigned XM30_THROW_FAMILY=0x32u;
static inline unsigned xm30NativeAttackFamily(const XM41ActorSample& a){
    if(!a.valid)return 0u;
    const unsigned mode=(a.mode>>8u)&255u;
    const unsigned actionFamily=(a.nativeAction>>8u)&255u;
    if(mode==0x28u)return 0x28u;
    if(mode==0x2Au || (a.attackPhase&0x80u)!=0u){
        // An actual special or super action takes precedence over any
        // auxiliary controller. Preserve Cammy's proven 2C transitions.
        if(actionFamily==0x2Cu || actionFamily==0x2Eu)
            return actionFamily;
    }
    // A whiffed native command grab can run its attempt under 20/30
    // before any successful catch switches to throw controller 32.
    // The family alone is not enough to CREATE a card: the 87.259
    // input-backed freshThrow gate remains mandatory for a new root.
    if(mode==0x20u || mode==0x30u || mode==0x32u)
        return XM30_THROW_FAMILY;
    return 0u;
}
static inline bool xm30SameNativeAttack(const XM41ActorSample& a,
                                       unsigned family,unsigned firstAction,
                                       unsigned firstMode=0u){
    const unsigned current=xm30NativeAttackFamily(a);
    // A 2C special or 2E grab super can enter ROM throw controller 32
    // during the catch/hold/release without becoming another move.
    if(current==XM30_THROW_FAMILY &&
       (family==0x2Cu || family==0x2Eu))return true;
    if(current!=family)return false;
    // Normal +60 action changes identify new punches/kicks.
    // 2C/2E/32 have internal substages owned by the original action.
    return family!=0x28u ||
           (a.nativeAction==firstAction &&
            (!firstMode || a.mode==firstMode));
}
struct XM30Event {
    int player=0, startup=0, active=0, code=0, x=0, y=0, motion=XM32_NORMAL;
    // Capture only actual mapped input: a bounded direction-edge sequence
    // plus the complete pressed button chord. ROM-only events stay unknown.
    static const int INPUT_DIRS=6;
    unsigned inputButtons=0;
    unsigned char inputDirections[INPUT_DIRS]={};
    unsigned inputDirectionCount=0;
    unsigned long long inputFrame=0,spawnFrame=0,firstFrame=0,endFrame=0;
    // Visual host-frame axis advances during native hitstop; native-frame
    // measurements above remain unchanged by this presentation axis.
    unsigned long long inputWallFrame=0,lastWallFrame=0;
    int source=0; // 0=fighter hitbox; 1=projectile hitbox
    bool closed=false;
    bool noBox=false; // No supported offensive collision box observed
    bool contactOnly=false; // Native action plus HP loss, box unavailable
    unsigned long long hitboxLastFrame=0; // real last active-box/native-super sample
    // Synchronized projectile-travel and separate fighter-recovery tracks.
    unsigned projectileBase=0;
    // One native 2E super can own multiple successive linked objects.
    static const int OWNED_PROJECTILES=32;
    unsigned ownedProjectileBases[OWNED_PROJECTILES]={};
    int ownedProjectileCount=0;
    unsigned long long firstMotionFrame=0,lastMotionFrame=0,despawnFrame=0;
    int moveFrames=0, lastProjectileX=0, lastProjectileY=0;
    unsigned long long lastProjectileFrame=0; // last actual object observation
    // A native 2E super may keep its attacking phase after the first
    // offensive collision box vanishes. The ROM phase, not a guessed move
    // name or timeout, continues its active span through such substeps.
    bool nativeSuperPhase=false;
    // Separate hit confirmations from the lifetime of the attack. A hit
    // cannot end a ROM-owned super (or a normal attack), and multi-hit
    // attacks may record multiple distinct health-loss frames.
    static const int CONTACTS=32;
    unsigned long long contactFrames[CONTACTS]={};
    unsigned long long contactWallFrames[CONTACTS]={};
    int contactCount=0;
    unsigned long long motionBits[16]={}; // exact movement-edge frames, up to 1024f
    bool motionOverflow=false;
    bool positionKnown=false;
    // Action duration is separate from the hitbox and projectile clocks.
    // Start at the first observed real ROM attack action, end on the first
    // different action/controller family, independent of input or height.
    bool attackObserved=false,attackEnded=false;
    bool inputObserved=true; // false: native ROM detected attack but mapped edge not seen
    unsigned attackFamily=0,attackAction=0,attackMode=0;
    unsigned actorBase=0; // native owner of this card
    unsigned long long attackStartFrame=0,attackLastFrame=0,attackEndFrame=0;
    unsigned attackExitPending=0; // unresolved native controller during impact

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
static inline bool xm30OwnsProjectile(const XM30Event& e,unsigned base){
    if(!base)return false;
    if(e.projectileBase==base)return true;
    for(int i=0;i<e.ownedProjectileCount;++i)
        if(e.ownedProjectileBases[i]==base)return true;
    return false;
}
static inline void xm30ClaimProjectile(XM30Event& e,unsigned base){
    if(!base || xm30OwnsProjectile(e,base))return;
    if(e.ownedProjectileCount<XM30Event::OWNED_PROJECTILES)
        e.ownedProjectileBases[e.ownedProjectileCount++]=base;
}
static inline bool xm30OwnedProjectileLive(const XM30Event& e,
                                            const XM30Sample& s){
    for(int j=0;j<s.projectileCount && j<12;++j)
        if(xm30OwnsProjectile(e,s.projectileBases[j]))return true;
    return false;
}
static inline bool xm30OwnedProjectileAttack(const XM30Event& e,
                                              const XM30Sample& s){
    for(int j=0;j<s.projectileCount && j<12;++j)
        if(s.projectileHits[j].active &&
           xm30OwnsProjectile(e,s.projectileBases[j]))return true;
    return false;
}
static inline void xm30AddContact(XM30Event& e,unsigned long long at,
                                     unsigned long long wall=0){
    // Distinct impacts during hitstop can share the SAME native frame.
    // The host observation tick deduplicates signals from one impact.
    if(e.contactCount && e.contactFrames[e.contactCount-1]==at &&
       (!wall || e.contactWallFrames[e.contactCount-1]==wall))return;
    if(e.contactCount==XM30Event::CONTACTS){
        for(int j=1;j<XM30Event::CONTACTS;++j){
            e.contactFrames[j-1]=e.contactFrames[j];
            e.contactWallFrames[j-1]=e.contactWallFrames[j];
        }
        --e.contactCount;
    }
    e.contactFrames[e.contactCount]=at;
    e.contactWallFrames[e.contactCount++]=wall;
}
struct XM30Track {
    long long pending=-1;
    unsigned long long pendingWallFrame=0;
    bool pendingNativeOnly=false;
    int code=0, motion=XM32_NORMAL;
    unsigned pendingButtons=0;
    unsigned char pendingDirections[XM30Event::INPUT_DIRS]={};
    unsigned pendingDirectionCount=0;
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
    unsigned pendingAttackFamily=0,pendingAttackAction=0,pendingAttackMode=0;
    unsigned long long pendingAttackStart=0,pendingAttackLast=0,pendingAttackEnd=0;
    // At the INPUT EDGE the previous native action may still be active.
    // Combo cancels frequently inherit a continuously active box70/72.
    // The old code only watched !previousHitbox and missed those attacks.
    unsigned lastNativeFamily=0,lastNativeAction=0,lastNativeMode=0,lastNativeStep=0;
    unsigned lastNativeBase=0;
    bool lastNativeValid=false;
    // Observe accepted super roots on the ROM HOST callback, not on
    // SDL button edges or the delayed first offensive collision box.
    // Separate latch prevents duplicate portraits during native hitstop.
    bool superRootObserved=false;
    unsigned pendingOriginFamily=0,pendingOriginAction=0,pendingOriginMode=0,pendingOriginStep=0;
    bool pendingOriginValid=false,pendingComboTransition=false;
    // A rejected input inside a live attack must not steal the next
    // repeating projectile box or create a timeout NO BOX card.
    bool pendingOriginLeft=false;
    int previousProjectileCount=0;
    unsigned previousProjectileBases[12]={};
    int previousActiveCount=0;
    unsigned previousActiveBases[12]={};
};
// Stable, uncompressed timeline geometry. One observed host tick is
// exactly one horizontal pixel; overflow moves to the next pair of slim
// HIT / ACTION lanes. Previous rows never change scale as time accumulates.
// Render metadata is sampled at the original physical button edge.
// This does not change motion classification or the native attack clock.
static inline void xm30CaptureInputDirections(
        XM30Track& t,const XM32MotionHistory& h,unsigned long long now){
    t.pendingDirectionCount=0;
    const unsigned long long age=t.motion==XM32_DOUBLE?45u:
        (t.motion==XM32_NORMAL?8u:30u);
    for(int i=h.count-1;i>=0 &&
        t.pendingDirectionCount<XM30Event::INPUT_DIRS;--i){
        const XM32DirStep& step=h.step[i];
        if(now<step.frame || now-step.frame>age)break;
        if(step.dir==5)continue; // neutral is a release, not a direction key
        t.pendingDirections[t.pendingDirectionCount++]=(unsigned char)step.dir;
    }
    // A held direction is still pressed even if its last edge is older
    // than the motion-detection window.
    if(!t.pendingDirectionCount && h.count &&
       h.step[h.count-1].dir!=5)
        t.pendingDirections[t.pendingDirectionCount++]=
            (unsigned char)h.step[h.count-1].dir;
    for(unsigned i=0;i<t.pendingDirectionCount/2;++i){
        const unsigned j=t.pendingDirectionCount-1u-i;
        const unsigned char tmp=t.pendingDirections[i];
        t.pendingDirections[i]=t.pendingDirections[j];
        t.pendingDirections[j]=tmp;
    }
}
static inline void xm30AssignInput(XM30Event& e,const XM30Track& t){
    e.inputButtons=t.pendingButtons;
    e.inputDirectionCount=t.pendingDirectionCount;
    for(unsigned i=0;i<t.pendingDirectionCount;++i)
        e.inputDirections[i]=t.pendingDirections[i];
}
struct XM30TimelineWrap {
    unsigned long long start=0,finish=0;
    unsigned ticksPerRow=1;
    unsigned long long span() const {
        return finish>=start ? finish-start+1u : 1u;
    }
    unsigned rows() const {
        return (unsigned)((span()+ticksPerRow-1u)/ticksPerRow);
    }
    unsigned row(unsigned long long at) const {
        return (unsigned)((at<=start?0u:at-start)/ticksPerRow);
    }
    unsigned column(unsigned long long at) const {
        return (unsigned)((at<=start?0u:at-start)%ticksPerRow);
    }
};
struct XM30WallTick {
    unsigned long long wall=0,native=0;
    bool stopped=false;
    bool opponentHitStop=false; // visual evidence, never alters native clock
};
// Mark an already-observed 2E card with dark gray while the opposing
// fighter's native +85 stop is live. The attacker can still run its own
// native clock during a beam/lightning hit: changing that clock would
// corrupt measured action duration. Do not gray ordinary attacks,
// unrelated cards, or time after a super/projectile has ended.
static inline bool xm30VisualHeld(const XM30WallTick& tick,
                                   const XM30Event& e,
                                   unsigned long long wall,
                                   unsigned long long native){
    if(!tick.wall)return false;
    if(tick.stopped)return true;
    if(!tick.opponentHitStop || e.attackFamily!=0x2Eu ||
       !e.attackObserved || wall<e.inputWallFrame ||
       wall>e.lastWallFrame || native<e.attackStartFrame)
        return false;
    if(!e.attackEnded || native<=e.attackEndFrame)return true;
    return e.source==1 && e.projectileBase &&
           (!e.despawnFrame || native<=e.despawnFrame);
}
struct XM30FrameData {
    // Preserve the 120-native-frame ascent to the 40px apex, then
    // squeeze width across SIX native frames while rising one glyph
    // height IN THE 384x224 SOURCE IMAGE (never SDL window coordinates).
    // This modifies presentation timing, never attack measurements.
    static const int HISTORY=24, FLOATS=8, MAX_WAIT=120;
    // This is NOT an attack-duration ceiling. Only an exact unchanged
    // native animation/controller with no offense for 360 host ticks
    // is considered abandoned; real supers may continue indefinitely.
    static const unsigned STALE_NO_PROGRESS_WALL=360u;
    unsigned staleNoProgress[2]={0u,0u};
    static const int FLOAT_APEX=120, FLOAT_EXIT=6;
    static const int FLOAT_LIFE=FLOAT_APEX+FLOAT_EXIT;
    bool enabled=false;
    bool inCombat=false;
    // A shared host-frame axis cannot time two fighters independently:
    // the attacker continues 2E during defender-only hitstop. Event
    // timestamps are on the owning fighter's unfrozen native step axis.
    unsigned long long frame=0; // display/observation tick when either player advances
    unsigned long long playerFrame[2]={0,0};
    // One screen-frame tick is retained per owner, even when its native
    // emulation clock is stopped. This is a READ-ONLY display axis.
    static const int WALL_HISTORY=4096;
    unsigned long long wallFrame[2]={0,0};
    XM30WallTick wallTicks[2][WALL_HISTORY];
    XM30WallTick wallTick(int p,unsigned long long at) const {
        if(p<0 || p>1 || at==0)return XM30WallTick{};
        const XM30WallTick& t=wallTicks[p][at%WALL_HISTORY];
        return t.wall==at?t:XM30WallTick{};
    }
    unsigned long long wallForNative(int p,unsigned long long native,
                                     unsigned long long earliest) const {
        if(p<0 || p>1)return earliest;
        unsigned long long start=earliest;
        if(wallFrame[p]>WALL_HISTORY && start<wallFrame[p]-WALL_HISTORY+1u)
            start=wallFrame[p]-WALL_HISTORY+1u;
        for(unsigned long long w=start;w<=wallFrame[p];++w){
            const XM30WallTick t=wallTick(p,w);
            if(t.wall && t.native>=native)return w;
        }
        // If this old event's timestamps fell out of the bounded map,
        // never extend a closed card to the current host frame.
        return earliest;
    }
    void recordWallTicks(const bool advanced[2]){
        for(int p=0;p<2;++p){
            XM30WallTick& t=wallTicks[p][wallFrame[p]%WALL_HISTORY];
            t.wall=wallFrame[p];t.native=playerFrame[p];
            t.stopped=!advanced[p];
            // previousHitStop[] has the CURRENT sampled +85 values.
            // Keep the opponent's stop for the card renderer only.
            t.opponentHitStop=previousHitStop[1-p]!=0u;
        }
        for(int i=0;i<count;++i){
            XM30Event& e=history[i];
            const int p=e.player;
            if((e.attackObserved && !e.attackEnded) ||
               (e.source==1 && e.projectileBase && !e.closed &&
                !e.despawnFrame))
                e.lastWallFrame=wallFrame[p];
        }
    }
    // Native cinematic holds may keep +85=0 and phase=80, notably 2C12.
    // Preserve the previous sampled animation, not the wall-clock time.
    bool animationSeeded=false;
    unsigned previousAnimPtr[2]={0,0};
    unsigned previousAnimTimer[2]={0,0};
    unsigned previousAnimMode[2]={0,0};
    unsigned previousAnimAction[2]={0,0};
    unsigned previousAnimStep[2]={0,0};
    unsigned long long eventsCreated=0; // monotonic, unlike bounded history.count
    // Preserve pre-box super damage sampled during native hitstop.
    unsigned long long deferredSuperContacts[2][XM30Event::CONTACTS]={};
    unsigned long long deferredSuperContactWalls[2][XM30Event::CONTACTS]={};
    unsigned deferredSuperContactCount[2]={0,0};
    unsigned deferredSuperAction[2]={0,0};
    unsigned long long superFreezeFrames=0; // callbacks with neither player advancing
    // Unlike the FF5009 round clock, ROM hit-stop counters count down to
    // zero. They cannot permanently stall the meter when the round timer is
    // deliberately held at 99 in Training.
    bool hitStopSeeded=false;
    unsigned previousHitStop[2]={0,0};
    bool healthSeeded[2]={false,false};
    unsigned previousHealth[2]={0,0};
    // Independent of the +85 clock gate: capture *reload edges* even
    // when the owner is frozen, and deduplicate across evidence types.
    bool impactStopSeeded[2]={false,false};
    unsigned previousImpactStop[2]={0,0};
    bool comboSeeded[2]={false,false};
    unsigned previousCombo[2]={0,0};
    unsigned long long hpContactProofs[2]={0,0};
    unsigned long long comboContactProofs[2]={0,0};
    unsigned long long reloadContactProofs[2]={0,0};
    unsigned long long unassignedContactProofs[2]={0,0};
    // Damage can be first observed on a frozen host tick. Carry it into
    // the NEXT real combat sample rather than consuming it at the return.
    bool pendingContact[2]={false,false};
    unsigned contactExitGrace[2]={0,0};
    int count=0, floatNext=0;
    XM30Event history[HISTORY];
    XM30Float floats[FLOATS];
    XM30Track track[2];
    XM32MotionHistory directions[2];
    void clear(bool preserveToggle=true) {
        const bool setting=enabled;
        frame=0; playerFrame[0]=playerFrame[1]=0; eventsCreated=0;
        wallFrame[0]=wallFrame[1]=0;
        for(int p=0;p<2;++p)for(int i=0;i<WALL_HISTORY;++i)
            wallTicks[p][i]=XM30WallTick{};
        animationSeeded=false;
        staleNoProgress[0]=staleNoProgress[1]=0u;
        for(int p=0;p<2;++p){
            previousAnimPtr[p]=previousAnimTimer[p]=0;
            previousAnimMode[p]=previousAnimAction[p]=previousAnimStep[p]=0;
        }
        superFreezeFrames=0; count=0; floatNext=0; inCombat=false;
        deferredSuperContactCount[0]=deferredSuperContactCount[1]=0;
        deferredSuperAction[0]=deferredSuperAction[1]=0;
        hitStopSeeded=false;previousHitStop[0]=previousHitStop[1]=0;
        healthSeeded[0]=healthSeeded[1]=false;
        previousHealth[0]=previousHealth[1]=0;
        impactStopSeeded[0]=impactStopSeeded[1]=false;
        previousImpactStop[0]=previousImpactStop[1]=0;
        comboSeeded[0]=comboSeeded[1]=false;
        previousCombo[0]=previousCombo[1]=0;
        hpContactProofs[0]=hpContactProofs[1]=0;
        comboContactProofs[0]=comboContactProofs[1]=0;
        reloadContactProofs[0]=reloadContactProofs[1]=0;
        unassignedContactProofs[0]=unassignedContactProofs[1]=0;
        pendingContact[0]=pendingContact[1]=false;
        contactExitGrace[0]=contactExitGrace[1]=0;
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
        directions[player].direction(5+x+3*y,playerFrame[player]);
    }
    void press(int player,int code){
        if(!enabled||!inCombat||player<0||player>1 || code<0x80||code>0x85)return;
        XM30Track& t=track[player];
        // Further button edges while the same confirmed 2E super is
        // running do not restart its visual input clock. The next super
        // must be accepted independently by the native ROM controller.
        for(int i=count-1;i>=0;--i){
            const XM30Event& e=history[i];
            if(e.player==player && e.attackFamily==0x2Eu &&
               !e.attackEnded)return;
        }
        // Chord edges belong to one action, but ALL of their pressed
        // buttons must appear in the input title (LP+MP, all punches etc.).
        const unsigned buttonBit=1u<<(unsigned)(code-0x80);
        if(t.pending>=0 && (long long)playerFrame[player]-t.pending<=2){
            t.pendingButtons|=buttonBit;
            return;
        }
        // 87.251: Cyclops 2E00 begins in stage02, long before its beam's
        // linked projectile first becomes damaging. More button edges during
        // that very same accepted super must NOT erase the original
        // input, ROM action root and projectile-spawn candidate. This guard
        // applies only to a STILL-PENDING 2E; once a card is emitted or the
        // native family exits, the next genuine attack remains possible.
        if(t.pending>=0 && t.pendingAttackFamily==0x2Eu &&
           !t.pendingAttackEnd && t.lastNativeFamily==0x2Eu)return;
        // Do not reject new attack inputs just because an older projectile is
        // still on screen. A new row still requires a NEW hitbox edge.
        t.pending=(long long)playerFrame[player];
        t.pendingWallFrame=wallFrame[player];
        t.pendingNativeOnly=false;
        t.pendingButtons=buttonBit;
        t.pendingSpawnBase=0;
        t.pendingSpawnFrame=0;
        // A new button edge only starts the next candidate. It NEVER ends an
        // earlier attack: each existing card stops on native ROM action exit.
        t.pendingAttackFamily=t.pendingAttackAction=t.pendingAttackMode=0u;
        t.pendingAttackStart=t.pendingAttackLast=t.pendingAttackEnd=0u;
        t.pendingOriginFamily=t.lastNativeFamily;
        t.pendingOriginAction=t.lastNativeAction;
        t.pendingOriginMode=t.lastNativeMode;
        t.pendingOriginStep=t.lastNativeStep;
        t.pendingOriginValid=t.lastNativeValid;
        t.pendingComboTransition=false;t.pendingOriginLeft=false;
        t.pendingFirstMotion=0;t.pendingLastMotion=0;t.pendingMoveFrames=0;
        t.pendingPositionKnown=false;t.pendingMotionOverflow=false;
        for(int word=0;word<16;++word)t.pendingMotionBits[word]=0;
        t.code=code;
        t.motion=directions[player].classify(playerFrame[player]);
        xm30CaptureInputDirections(t,directions[player],playerFrame[player]);
        if(t.motion!=XM32_NORMAL)directions[player].count=0;
    }
    int appendEvent(){
        ++eventsCreated; // bounded 24-card buffer still identifies every row created
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
    // Train on native ROM samples, not on displayed video frames.
    // FF5009 is deliberately ignored: a constant clock is not evidence of
    // super freeze, and it previously locked BOTH meters indefinitely.
    // One hit-confirm marker per defender HP drop. Record it on the
    // attacker's newest live frame-data row, not against any unrelated old
    // projectile. A contact does NOT close or shorten the move's action.
    void recordContacts(const XM41ActorSample actors[2],
                        const XM30Sample samples[2],
                        const bool advanced[2]){
        bool comboRise[2]={false,false},stopReload[2]={false,false};
        // Update both signals on EVERY emulation callback, including
        // host samples where neither fighter's native clock advances.
        for(int p=0;p<2;++p){
            if(actors[p].valid){
                comboRise[p]=actors[p].comboKnown && comboSeeded[p] &&
                    actors[p].combo>previousCombo[p];
                comboSeeded[p]=actors[p].comboKnown;
                previousCombo[p]=actors[p].combo;
                stopReload[p]=impactStopSeeded[p] &&
                    actors[p].hitStop>previousImpactStop[p] &&
                    actors[p].hitStop>=2u;
                previousImpactStop[p]=actors[p].hitStop;
                impactStopSeeded[p]=true;
            }else{
                comboSeeded[p]=false;impactStopSeeded[p]=false;
            }
        }
        for(int victim=0;victim<2;++victim){
            if(!actors[victim].valid){
                healthSeeded[victim]=false;pendingContact[1-victim]=false;
                continue;
            }
            const unsigned health=actors[victim].health;
            const bool damaged=healthSeeded[victim] &&
                               health<previousHealth[victim];
            previousHealth[victim]=health;healthSeeded[victim]=true;
            const int attacker=1-victim;
            const unsigned family=xm30NativeAttackFamily(actors[attacker]);
            const XM30Track& t=track[attacker];
            const unsigned long long at=playerFrame[attacker];
            // The local HUD combo may be encoded on the attacker or
            // defender's counter depending on the game/UI mode. Accept a
            // rising counter only if this side owns a live offensive
            // source; simultaneous opposing offense is ambiguous.
            bool observedBox=samples[attacker].melee.active ||
                             samples[attacker].projectile.active;
            for(int j=0;j<samples[attacker].projectileCount && j<12;++j)
                if(samples[attacker].projectileHits[j].active)
                    observedBox=true;
            bool liveOwned=false,liveNative=false;
            for(int i=count-1;i>=0;--i){
                const XM30Event& e=history[i];
                if(e.player!=attacker ||
                   (e.actorBase && actors[attacker].sourceBase &&
                    e.actorBase!=actors[attacker].sourceBase))continue;
                if(xm30OwnedProjectileLive(e,samples[attacker]))
                    liveOwned=true;
                if(!e.attackEnded && e.attackFamily &&
                   xm30SameNativeAttack(actors[attacker],
                       e.attackFamily,e.attackAction,e.attackMode))
                    liveNative=true;
            }
            const bool pendingSuper=family==0x2Eu &&
                                   t.pendingAttackFamily==0x2Eu;
            // Native action tenure supports contact-proof attribution
            // when a collision box is missing; only HP/combo changes
            // actually produce contacts (never speculative damage).
            const bool offense=(observedBox || liveOwned ||
                               liveNative || pendingSuper);
            const bool victimOffense=
                xm30NativeAttackFamily(actors[victim])!=0u &&
                (samples[victim].melee.active ||
                 samples[victim].projectile.active);
            const bool comboProof=offense &&
                (comboRise[attacker] ||
                 (comboRise[victim] && !victimOffense));
            // Native +85 can reload repeatedly during one damaging attack.
            // Reload alone is only a hit proof at the HP floor, where
            // further real hits cannot appear as additional HP drops.
            const bool reloadProof=offense && stopReload[victim] &&
                health<=1u && (observedBox || liveOwned);
            // HP changes still have priority. A combo increment or a
            // defender's actual +85 reload proves an impact at HP=1.
            // Rising +85 during a boxless cinematic is NOT a hit.
            if(!damaged && !comboProof && !reloadProof)continue;
            if(damaged)++hpContactProofs[attacker];
            else if(comboProof)++comboContactProofs[attacker];
            else ++reloadContactProofs[attacker];
            if(!advanced[attacker])pendingContact[attacker]=true;
            int selected=-1,rank=-1;
            for(int i=count-1;i>=0;--i){
                const XM30Event& e=history[i];
                if(e.player!=attacker || e.firstFrame>at ||
                   (e.actorBase && actors[attacker].sourceBase &&
                    e.actorBase!=actors[attacker].sourceBase) ||
                   (e.noBox && !e.contactOnly && !e.attackObserved))
                    continue;
                const bool live=xm30OwnedProjectileLive(e,samples[attacker]);
                const bool box=xm30OwnedProjectileAttack(e,samples[attacker]);
                const bool native=!e.attackEnded && family!=0u &&
                    xm30SameNativeAttack(actors[attacker],
                        e.attackFamily,e.attackAction,e.attackMode);
                // A previous move's still-live projectile must not lend
                // its impact to the freshly accepted portrait-only super.
                // Native ownership alone is not damage attribution proof
                // while ONLY unmatched projectile objects exist.
                if(native && e.noBox &&
                   samples[attacker].projectileCount>0 && !live &&
                   !samples[attacker].melee.active)
                    continue;
                if(e.closed && at>e.endFrame+1u &&
                   (e.attackEnded || at>e.attackLastFrame+1u) &&
                   !live && !native)continue;
                if(family==0x2Eu && t.pendingAttackFamily==0x2Eu &&
                   !native && !live)continue;
                int score=1;
                if(family==0x2Eu && native)score=6;
                else if(box)score=5;
                else if(live && !samples[attacker].melee.active)score=4;
                else if(native)score=3;
                else if(live)score=2;
                if(score>rank){rank=score;selected=i;}
            }
            if(selected>=0){
                XM30Event& e=history[selected];
                if(e.noBox)e.contactOnly=true;
                xm30AddContact(e,at,wallFrame[attacker]);
            }else if(family==0x2Eu && t.pendingAttackFamily==0x2Eu){
                unsigned& n=deferredSuperContactCount[attacker];
                bool freshLinked=false;
                for(int j=0;j<samples[attacker].projectileCount && j<12;++j)
                    if(t.pendingSpawnBase &&
                       samples[attacker].projectileBases[j]==t.pendingSpawnBase)
                        freshLinked=true;
                if(samples[attacker].projectileCount>0 && !freshLinked)
                    continue;
                const unsigned root=t.pendingAttackAction;
                if(n && deferredSuperAction[attacker]!=root)n=0;
                deferredSuperAction[attacker]=root;
                if((!n || deferredSuperContactWalls[attacker][n-1]!=
                        wallFrame[attacker]) && n<XM30Event::CONTACTS){
                    deferredSuperContacts[attacker][n]=at;
                    deferredSuperContactWalls[attacker][n]=wallFrame[attacker];
                    ++n;
                }
            }else{
                ++unassignedContactProofs[attacker];
            }
        }
    }
    void onFrame(const XM30Sample& p1,const XM30Sample& p2,
                 const XM41ActorSample& a1=XM41ActorSample{},
                 const XM41ActorSample& a2=XM41ActorSample{},
                 unsigned clockValue=~0u){
        if(!enabled)return;
        beginCombat();
        ++wallFrame[0];++wallFrame[1]; // even when both native clocks stall
        (void)clockValue; // retained for older callers/tests, never a freeze gate
        const XM30Sample samples[2]={p1,p2};
        const XM41ActorSample actors[2]={a1,a2};
        const unsigned stops[2]={a1.hitStop,a2.hitStop};
        // The uploaded 87.245 ROM trace proves two different native
        // cinematics with +85=0:
        // - 2E/step02/phase81: 45 held callbacks (already handled)
        // - 2C36/step02/phase81 and 2C12/step02/phase80: 43-44
        //   consecutive callbacks with BOTH native animation pointers and
        //   countdowns unchanged. The prior gate counted them as played
        //   move frames, adding ~44F to the final frame-data card.
        // Detect stasis from actual ROM animation state instead of assuming
        // that all 2C/2E phase81 samples are frozen; the first 2-3 samples
        // in those scripts DO animate and must remain counted.
        // Stage02 is not the only native 2C/2E animation hold. In later
        // special phases the same all-fighter stall can occur immediately
        // after a combo, with +85=0. Do not count it if both native
        // animation scripts are static AND no attacking object/box exists.
        // A live projectile forbids this gate: fighter poses can remain
        // static while the game/projectile advances normally.
        const bool nativeSetup=
            ((xm30NativeAttackFamily(a1)==0x2Cu ||
              xm30NativeAttackFamily(a1)==0x2Eu) &&
             a1.nativeStep>=2u) ||
            ((xm30NativeAttackFamily(a2)==0x2Cu ||
              xm30NativeAttackFamily(a2)==0x2Eu) &&
             a2.nativeStep>=2u) ||
            (xm30NativeAttackFamily(a1)==XM30_THROW_FAMILY &&
             a1.nativeStep==2u && a1.attackPhase==0x81u) ||
            (xm30NativeAttackFamily(a2)==XM30_THROW_FAMILY &&
             a2.nativeStep==2u && a2.attackPhase==0x81u);
        const bool offensiveObjectPresent=
            p1.melee.active || p1.projectile.active ||
            p2.melee.active || p2.projectile.active ||
            p1.projectileCount>0 || p2.projectileCount>0;
        // Snapshot before the existing animation references are updated.
        // Hitstop counter changes count as native progress.
        bool unchangedOwner[2]={false,false};
        for(int p=0;p<2;++p){
            const XM41ActorSample& a=actors[p];
            unchangedOwner[p]=animationSeeded && hitStopSeeded &&
                a.valid && a.animPtr!=0u &&
                previousAnimPtr[p]==a.animPtr &&
                previousAnimTimer[p]==a.animTimer &&
                previousAnimMode[p]==a.mode &&
                previousAnimAction[p]==a.nativeAction &&
                previousAnimStep[p]==a.nativeStep &&
                previousHitStop[p]==stops[p];
        }
        bool bothAnimationsStatic=animationSeeded;
        for(int p=0;p<2;++p){
            const XM41ActorSample& a=actors[p];
            if(!a.valid || a.animPtr==0u ||
               previousAnimPtr[p]!=a.animPtr ||
               previousAnimTimer[p]!=a.animTimer ||
               previousAnimMode[p]!=a.mode ||
               previousAnimAction[p]!=a.nativeAction ||
               previousAnimStep[p]!=a.nativeStep)
                bothAnimationsStatic=false;
            previousAnimPtr[p]=a.animPtr;
            previousAnimTimer[p]=a.animTimer;
            previousAnimMode[p]=a.mode;
            previousAnimAction[p]=a.nativeAction;
            previousAnimStep[p]=a.nativeStep;
        }
        animationSeeded=true;
        // Actual HP loss is native evidence that a collision happened on
        // this callback even when the one-frame hitbox was not observable.
        // Never discard that sample as a global cinematic freeze: doing so
        // loses the exact contact-only combo case this pass must recover.
        const bool healthDroppedNow=
            (actors[0].valid && healthSeeded[0] &&
             actors[0].health<previousHealth[0]) ||
            (actors[1].valid && healthSeeded[1] &&
             actors[1].health<previousHealth[1]);
        // ROM portrait phase 81 is not guaranteed to remain at stage 02
        // across fighters. Also recognize frozen stage-02 startup when
        // phase 80 is used (even if an old projectile still exists).
        // Never mistake a confirmed damage frame for a global hold.
        const bool superPortrait=(
            (xm30NativeAttackFamily(a1)==0x2Eu &&
             (a1.attackPhase==0x81u ||
              (a1.nativeStep==2u && bothAnimationsStatic &&
               !healthDroppedNow))) ||
            (xm30NativeAttackFamily(a2)==0x2Eu &&
             (a2.attackPhase==0x81u ||
              (a2.nativeStep==2u && bothAnimationsStatic &&
               !healthDroppedNow))));
        const bool cinematicStop=superPortrait ||
            (nativeSetup && bothAnimationsStatic &&
             !offensiveObjectPresent && !healthDroppedNow);
        // An orphaned native controller or frozen animation can otherwise
        // leave a frame-data card open forever. Do NOT time out real supers:
        // a changing script, timer, native hitstop counter, live hitbox,
        // projectile or opponent damage resets the watchdog completely.
        for(int p=0;p<2;++p){
            const bool noOffense=!samples[p].melee.active &&
                !samples[p].projectile.active &&
                samples[p].projectileCount==0 &&
                !(actors[1-p].valid && healthSeeded[1-p] &&
                  actors[1-p].health<previousHealth[1-p]);
            if(unchangedOwner[p] && noOffense){
                if(staleNoProgress[p]<=STALE_NO_PROGRESS_WALL)
                    ++staleNoProgress[p];
            }else staleNoProgress[p]=0u;
            // The equality fires once. Do not consume an extra native
            // frame or invent active/contact frames at the timeout.
            if(staleNoProgress[p]!=STALE_NO_PROGRESS_WALL)continue;
            bool retired=false;
            for(int i=0;i<count;++i){
                XM30Event& e=history[i];
                if(e.player!=p || !e.attackObserved || e.attackEnded ||
                   (e.actorBase && actors[p].sourceBase &&
                    e.actorBase!=actors[p].sourceBase))continue;
                e.attackEnded=true;
                e.attackEndFrame=e.attackLastFrame;
                if(!e.closed){
                    e.closed=true;
                    e.endFrame=e.source==1 && e.lastProjectileFrame?
                        e.lastProjectileFrame:
                        (e.hitboxLastFrame?e.hitboxLastFrame:e.attackLastFrame);
                    if(e.source==1 && !e.despawnFrame)
                        e.despawnFrame=e.endFrame;
                }
                retired=true;
            }
            if(retired){
                track[p].eventIndex[0]=-1;
                track[p].pending=-1;
                std::fprintf(stderr,
                    "XM87.265 FRAME_CARD_STALLED_CLOSED player=P%d wall=%llu native=%llu no_progress=%u\n",
                    p+1,(unsigned long long)wallFrame[p],
                    (unsigned long long)playerFrame[p],
                    staleNoProgress[p]);
            }
        }
        // Before hit-stop gating, recognize an accepted ROM action so the
        // FIRST contact can create a card even if the SDL button edge was
        // missed and the ROM already set +85 on that exact host callback.
        // The old placement after advancePlayer[] silently discarded those
        // one-frame combo attacks during frozen impact.
        for(int p=0;p<2;++p){
            XM30Track& t=track[p];
            const unsigned family=xm30NativeAttackFamily(actors[p]);
            // Direct 2E->2E tag: the next fighter owns a NEW super card.
            const bool actorChanged=t.lastNativeBase && actors[p].sourceBase &&
                t.lastNativeBase!=actors[p].sourceBase;
            if(actorChanged){
                for(int i=0;i<count;++i){
                    XM30Event& e=history[i];
                    if(e.player!=p || !e.attackObserved || e.attackEnded ||
                       (e.actorBase && e.actorBase==actors[p].sourceBase))continue;
                    e.attackEndFrame=e.attackLastFrame;
                    e.attackEnded=true;
                    if(e.source==0 && !e.closed){
                        e.closed=true;
                        e.endFrame=e.hitboxLastFrame?
                            e.hitboxLastFrame:e.attackLastFrame;
                    }
                }
                t.eventIndex[0]=-1;t.previous[0]=false;
                t.lastNativeFamily=t.lastNativeAction=t.lastNativeMode=0;
                t.superRootObserved=false;
                comboSeeded[p]=false;healthSeeded[p]=false;
                impactStopSeeded[p]=false;
                pendingContact[p]=false;contactExitGrace[p]=0;
                if(t.pending>=0){
                    t.pendingOriginValid=false;
                    t.pendingOriginFamily=0;
                    t.pendingOriginLeft=true;
                }
            }
            // Remember actor ownership on EVERY native callback, including
            // portrait holds that never advance the per-fighter frame clock.
            // Otherwise the next frozen callback would falsely detect the
            // same tag again and allocate another 2E card.
            if(actors[p].sourceBase)t.lastNativeBase=actors[p].sourceBase;
            // Capture genuine accepted native attacks even if the mapped
            // button edge occurred inside hit-stop or was not observable.
            // 28: both +60 action and +A0 controller identify a new normal.
            // 2C: only family entry or stage>=04 -> new stage02 root is a
            //     new special; internal 2C50/4E/4C/54 cycling is NOT.
            // 2E: only family entry starts a new super; 2E02/10 substeps
            //     always belong to the original super.
            // 2E02/2E10 internal stages are NOT fresh supers; only
            // observed entry into the native 2E family arms a new card.
            // Do not use a stage-02 restart as an unproven DHC signal.
            // Some grab/counter supers show controller 32 BEFORE
            // the original 2C/2E opcode. That later ROM opcode upgrades
            // the SAME input-owned throw row; no second gray bar.
            // A fresh button candidate is instead a genuine new move.
            if((family==0x2Cu || family==0x2Eu) &&
               t.lastNativeFamily==XM30_THROW_FAMILY && t.pending<0)
                for(int i=count-1;i>=0;--i){
                    XM30Event& e=history[i];
                    if(e.player!=p || e.attackEnded ||
                       e.attackFamily!=XM30_THROW_FAMILY ||
                       (e.actorBase && actors[p].sourceBase &&
                        e.actorBase!=actors[p].sourceBase))continue;
                    e.attackFamily=family;
                    e.attackAction=actors[p].nativeAction;
                    e.attackMode=actors[p].mode;
                    e.nativeSuperPhase=family==0x2Eu;
                    if(family==0x2Eu)t.superRootObserved=true;
                    break;
                }
            // Keep the super root across its 32 throw/catch controller:
            // returning from a Zangief grab is NOT a second super.
            bool liveSuper=false,liveSpecial=false;
            for(int i=count-1;i>=0;--i){
                const XM30Event& e=history[i];
                if(e.player!=p || e.attackEnded ||
                   (e.actorBase && actors[p].sourceBase &&
                    e.actorBase!=actors[p].sourceBase))continue;
                if(e.attackFamily==0x2Eu)liveSuper=true;
                if(e.attackFamily==0x2Cu)liveSpecial=true;
            }
            const bool acceptedSuper=actors[p].valid &&
                family==0x2Eu && !t.superRootObserved;
            t.superRootObserved=actors[p].valid &&
                (family==0x2Eu ||
                 (family==XM30_THROW_FAMILY && liveSuper));
            // Controller 32 stages are one catch/hold/release sequence.
            // A real follow-up needs its own observed button AND a changed
            // ROM action, not just a throw animation substep.
            const bool chainedThrow=t.pending>=0 &&
                t.lastNativeFamily==XM30_THROW_FAMILY &&
                actors[p].nativeAction!=t.lastNativeAction;
            // The victim of a throw can also enter controller 32.
            // Never mistake that passive state for the victim attacking.
            // An independent command grab requires a recent real button
            // candidate; continuity from a live 2C/2E needs no new row.
            const bool freshThrow=family==XM30_THROW_FAMILY &&
                (((t.lastNativeFamily!=XM30_THROW_FAMILY &&
                   !liveSpecial && !liveSuper &&
                   t.pending>=0 &&
                   playerFrame[p]>=(unsigned long long)t.pending &&
                   playerFrame[p]-(unsigned long long)t.pending<=MAX_WAIT))
                 || chainedThrow);
            const bool resumedSpecial=family==0x2Cu &&
                t.lastNativeFamily==XM30_THROW_FAMILY && liveSpecial &&
                t.pending<0;
            const bool nativeNewAttack=actors[p].valid &&
                (acceptedSuper ||
                 (family==0x28u && (actors[p].attackPhase&0x80u)!=0u &&
                  (t.lastNativeFamily!=0x28u ||
                   actors[p].nativeAction!=t.lastNativeAction ||
                   actors[p].mode!=t.lastNativeMode)) ||
                 (family==0x2Cu && !resumedSpecial &&
                  (t.lastNativeFamily!=0x2Cu ||
                   (t.lastNativeStep>=4u && actors[p].nativeStep==2u &&
                    actors[p].nativeAction!=t.lastNativeAction))) ||
                 freshThrow);
            if(t.pending<0 && nativeNewAttack){
                // This card has no known mapped button edge. Show that
                // limitation explicitly rather than fabricating an S value.
                t.pending=(long long)playerFrame[p];
                t.pendingWallFrame=wallFrame[p];
                t.pendingNativeOnly=true;
                t.pendingButtons=0;t.pendingDirectionCount=0;
                t.code=0;t.motion=XM32_NORMAL;
                t.pendingSpawnBase=0;t.pendingSpawnFrame=0;
                t.pendingFirstMotion=t.pendingLastMotion=0;
                t.pendingMoveFrames=0;t.pendingPositionKnown=false;
                t.pendingMotionOverflow=false;
                for(int word=0;word<16;++word)t.pendingMotionBits[word]=0;
                t.pendingAttackFamily=t.pendingAttackAction=t.pendingAttackMode=0;
                t.pendingAttackStart=t.pendingAttackLast=t.pendingAttackEnd=0;
                t.pendingOriginFamily=t.lastNativeFamily;
                t.pendingOriginAction=t.lastNativeAction;
                t.pendingOriginMode=t.lastNativeMode;
                t.pendingOriginStep=t.lastNativeStep;
                t.pendingOriginValid=t.lastNativeValid;
                t.pendingOriginLeft=false;t.pendingComboTransition=false;
            }
            // ROM acceptance must produce a row for ANY fighter even if
            // the observer cannot see its collision rectangles. Later
            // real boxes promote this same row; they never create another.
            // Families 28/2C/2E are ROM-owned, not a fighter whitelist.
            const bool nativeCard=nativeNewAttack && family!=0u;
            if(nativeCard){
                // Original ROM acceptance; no fabricated active frames.
                for(int i=count-1;i>=0;--i){
                    XM30Event& prior=history[i];
                    if(prior.player!=p || !prior.attackObserved ||
                       prior.attackEnded)continue;
                    // Super entry is a confirmed native CANCEL of any
                    // previous fighter action, not merely a box rise.
                    prior.attackEnded=true;
                    prior.attackEndFrame=prior.attackLastFrame;
                    // A prior projectile may keep flying after its owner
                    // exits. Only fighter-attached boxes are shut down.
                    if(prior.source==0 && !prior.closed){
                        prior.closed=true;
                        prior.endFrame=prior.hitboxLastFrame?
                            prior.hitboxLastFrame:prior.attackLastFrame;
                    }
                }
                if(t.pending<0){
                    t.pending=(long long)playerFrame[p];
                    t.pendingWallFrame=wallFrame[p];
                    t.pendingNativeOnly=true;
                    t.pendingButtons=0;t.pendingDirectionCount=0;
                    t.code=0;t.motion=XM32_NORMAL;
                }
                const int idx=appendEvent();
                XM30Event& e=history[idx];e=XM30Event{};
                e.player=p;
                e.actorBase=actors[p].sourceBase;
                e.inputObserved=!t.pendingNativeOnly;
                e.inputFrame=(unsigned long long)t.pending;
                e.inputWallFrame=t.pendingWallFrame;
                e.lastWallFrame=wallFrame[p];
                e.code=t.code;e.motion=t.motion;
                xm30AssignInput(e,t);
                e.noBox=true;e.contactOnly=false;e.source=0;
                e.startup=0;e.active=0;
                e.firstFrame=playerFrame[p]; // no collision assertion
                e.attackObserved=true;e.attackFamily=family;
                e.nativeSuperPhase=acceptedSuper;
                e.attackAction=actors[p].nativeAction;
                e.attackMode=actors[p].mode;
                // The next counted native tick belongs to this new ROM
                // action. Frozen host observations have their own clock.
                e.attackStartFrame=playerFrame[p]+1u;
                e.attackLastFrame=e.attackStartFrame;
                if(deferredSuperContactCount[p] &&
                   deferredSuperAction[p]==actors[p].nativeAction){
                    for(unsigned j=0;j<deferredSuperContactCount[p];++j)
                        xm30AddContact(e,deferredSuperContacts[p][j],
                                         deferredSuperContactWalls[p][j]);
                    if(e.contactCount)e.contactOnly=true;
                }
                deferredSuperContactCount[p]=0;
                // Latch even on a frozen host tick so a stopped 28/2C
                // action cannot allocate the same card repeatedly.
                t.lastNativeFamily=family;
                t.lastNativeAction=actors[p].nativeAction;
                t.lastNativeMode=actors[p].mode;
                t.lastNativeStep=actors[p].nativeStep;
                t.lastNativeValid=actors[p].valid;
                t.pending=-1;
                t.pendingAttackFamily=0;
                t.eventIndex[0]=-1;
                t.eventIndex[1]=-1;
            }
        }
        bool advancePlayer[2]={false,false};
        for(int p=0;p<2;++p){
            if(!actors[p].valid || cinematicStop)continue;
            // A newly accepted first impact can show +85 on the very
            // same native sample as its first offensive box. Count THAT
            // owner's step once; do not invent the rest of its stop ticks.
            bool firstImpact=false;
            if(stops[p]>0u && (!hitStopSeeded || previousHitStop[p]==0u)){
                const XM30Track& t=track[p];
                // Native acceptance consumes pending input before the first
                // box; permit the one true impact tick despite rising +85.
                bool unboxedNative=false;
                for(int i=count-1;i>=0;--i){
                    const XM30Event& e=history[i];
                    if(e.player!=p || !e.noBox || !e.attackObserved ||
                       e.attackEnded ||
                       e.attackFamily!=xm30NativeAttackFamily(actors[p]) ||
                       (e.actorBase && actors[p].sourceBase &&
                        e.actorBase!=actors[p].sourceBase))continue;
                    unboxedNative=true;break;
                }
                if((t.pending>=0 && !t.pendingAttackEnd) || unboxedNative){
                    bool priorOffense=false;
                    for(int i=count-1;i>=0;--i){
                        const XM30Event& e=history[i];
                        if(e.player==p && !e.noBox && !e.attackEnded &&
                           e.attackObserved){priorOffense=true;break;}
                    }
                    const unsigned accepted=xm30NativeAttackFamily(actors[p]);
                    const bool newCombo=t.pendingOriginValid &&
                        t.pendingOriginFamily!=0u && accepted!=0u &&
                        (accepted!=t.pendingOriginFamily ||
                         (accepted==0x28u &&
                          (actors[p].nativeAction!=t.pendingOriginAction ||
                           actors[p].mode!=t.pendingOriginMode)) ||
                         (accepted==0x2Cu &&
                          t.pendingOriginFamily==0x2Cu &&
                          t.pendingOriginStep>=4u &&
                          actors[p].nativeStep==2u &&
                          actors[p].nativeAction!=t.pendingOriginAction));
                    if(!priorOffense || newCombo){
                        bool witnessed=samples[p].melee.active ||
                                       samples[p].projectile.active;
                        for(int j=0;j<samples[p].projectileCount && j<12;++j)
                            if(samples[p].projectileHits[j].active)witnessed=true;
                        if(actors[1-p].valid && healthSeeded[1-p] &&
                           actors[1-p].health<previousHealth[1-p])
                            witnessed=true;
                        firstImpact=witnessed;
                    }
                }
            }
            advancePlayer[p]=stops[p]==0u || firstImpact;
        }
        previousHitStop[0]=stops[0];previousHitStop[1]=stops[1];
        hitStopSeeded=true;
        if(!advancePlayer[0] && !advancePlayer[1]){
            ++superFreezeFrames;
            recordWallTicks(advancePlayer);
            // Native clocks stay frozen; the chart's visible host ticks
            // advance in dark gray on BOTH hitbox and move-duration lanes.
            recordContacts(actors,samples,advancePlayer);
            return;
        }
        // A strike can change the attack controller on the exact impact
        // sample. Protect that one contact frame from being misread as a
        // completed attack, then resume normal ROM-owned exit detection.
        const bool landed[2]={
            pendingContact[0] || (actors[1].valid && healthSeeded[1] &&
                                  actors[1].health<previousHealth[1]),
            pendingContact[1] || (actors[0].valid && healthSeeded[0] &&
                                  actors[0].health<previousHealth[0])
        };
        // The first real post-stop sample gets the carried contact signal.
        // At most two subsequent action-family transition samples may be
        // inconclusive; a genuine exit still uses its last matching frame.
        for(int p=0;p<2;++p){
            if(!advancePlayer[p])continue; // preserve latch until owner resumes
            if(landed[p])contactExitGrace[p]=2;
            pendingContact[p]=false;
        }
        ++frame;
        for(int i=0;i<FLOATS;++i)if(floats[i].visible){
            if(++floats[i].age>=FLOAT_LIFE)floats[i].visible=false;
        }
        for(int p=0;p<2;++p){
            if(!advancePlayer[p])continue;
            const unsigned long long frame=++playerFrame[p];
            XM30Track& t=track[p];
            const XM30Hit hits[2]={samples[p].melee,samples[p].projectile};
            // Record native action tenure without consulting current direction,
            // held buttons, Y-position, jumping, or the later idle stance.
            const unsigned family=xm30NativeAttackFamily(actors[p]);
            if(family!=0x2Eu)deferredSuperContactCount[p]=0;
            // ROM attack flag is authoritative; 2E's post-startup script
            // stage is a fallback once the move has a proven hitbox.
            // 0x40 recovery explicitly cancels both active indicators.
            const bool superPhaseActive=family==0x2Eu &&
                (actors[p].attackPhase&0x80u)!=0u;
            const bool superStageActive=family==0x2Eu &&
                actors[p].nativeStep>=4u &&
                (actors[p].nativeStep&1u)==0u &&
                (actors[p].attackPhase&0x40u)==0u;
            // A second 2C special can be cancelled directly from stage04
            // into the NEXT stage02 root without ever passing idle. Its
            // old projectile may persist, but its native action duration
            // must stop at the new action acceptance, not absorb 44F again.
            const bool newSpecialRoot=t.pending>=0 &&
                t.pendingOriginValid && t.pendingOriginFamily==0x2Cu &&
                t.pendingOriginStep>=4u && family==0x2Cu &&
                actors[p].nativeStep==2u &&
                actors[p].nativeAction!=t.pendingOriginAction;
            for(int i=0;i<count;++i){
                XM30Event& e=history[i];
                if(e.player!=p || !e.attackObserved || e.attackEnded ||
                   (e.actorBase && actors[p].sourceBase &&
                    e.actorBase!=actors[p].sourceBase))continue;
                if(newSpecialRoot && e.attackFamily==0x2Cu){
                    e.attackEndFrame=e.attackLastFrame;
                    e.attackEnded=true;
                    continue;
                }
                if(xm30SameNativeAttack(actors[p],e.attackFamily,e.attackAction,e.attackMode) ||
                   (e.attackFamily==0x2Eu && family==0x2Eu &&
                     (actors[p].attackPhase&0x80u)!=0u)){
                    // Any match proves the action survived impact.
                    e.attackLastFrame=frame;
                    e.attackExitPending=0;
                }else if(landed[p] || contactExitGrace[p]>0){
                    // Hitstop can temporarily expose a different action
                    // controller. Retain the last true ROM-owned tick and
                    // defer the exit decision, rather than reset the meter.
                    e.attackExitPending++;
                }else{
                    e.attackEndFrame=e.attackLastFrame;
                    e.attackEnded=true;
                }
            }
            // Capture the action associated with the pending input before its
            // hitbox may appear. If it already exited, save its end; never
            // rearm from unrelated walking/jumping/another special later.
            if(t.pending>=0 && frame>=(unsigned long long)t.pending &&
               frame-(unsigned long long)t.pending<=MAX_WAIT){
                if(t.pendingOriginValid && t.pendingOriginFamily!=0u &&
                   (family!=t.pendingOriginFamily ||
                    (family==0x28u &&
                     (actors[p].nativeAction!=t.pendingOriginAction ||
                      actors[p].mode!=t.pendingOriginMode)) ||
                    // A 2C special may cancel directly into another 2C:
                    // internal stage04 and stage08 actions are ONE special,
                    // but a return from stage>=04 to stage02 and a new 2C
                    // root action after an input is ROM acceptance evidence.
                    (family==0x2Cu && t.pendingOriginFamily==0x2Cu &&
                     t.pendingOriginStep>=4u &&
                     actors[p].nativeStep==2u &&
                     actors[p].nativeAction!=t.pendingOriginAction)))
                    t.pendingOriginLeft=true;
                const bool originStillActive=t.pendingOriginValid &&
                    t.pendingOriginFamily!=0u &&
                    !t.pendingOriginLeft &&
                    family==t.pendingOriginFamily &&
                    (family!=0x28u ||
                     (actors[p].nativeAction==t.pendingOriginAction &&
                      actors[p].mode==t.pendingOriginMode));
                // A second normal press during the first normal's recovery
                // is NOT automatically a new move. Wait for a changed ROM
                // action selector (or a different native attack family).
                // 2C/2E can change internal substeps, so their same-family
                // transitions remain owned by the existing move.
                if(!t.pendingAttackFamily && family && !originStillActive){
                    t.pendingAttackFamily=family;
                    t.pendingAttackAction=actors[p].nativeAction;
                    t.pendingAttackMode=actors[p].mode;
                    t.pendingAttackStart=t.pendingAttackLast=frame;
                    t.pendingComboTransition=t.pendingOriginValid &&
                        t.pendingOriginFamily!=0u;
                }else if(t.pendingAttackFamily && !t.pendingAttackEnd){
                    if(xm30SameNativeAttack(actors[p],t.pendingAttackFamily,
                                            t.pendingAttackAction,
                                            t.pendingAttackMode) ||
                       (t.pendingAttackFamily==0x2Eu && family==0x2Eu &&
                        (actors[p].attackPhase&0x80u)!=0u) ||
                       landed[p] || contactExitGrace[p]>0)
                        t.pendingAttackLast=frame;
                    else
                        t.pendingAttackEnd=t.pendingAttackLast;
                }
            }
            // Link new ROM projectile objects without taking ownership
            // of projectiles from the previous move. 2E multi-object
            // continuity is unchanged; other families need a no-box row.
            if(family!=0u)for(int i=count-1;i>=0;--i){
                XM30Event& e=history[i];
                if(e.player!=p || e.attackFamily!=family ||
                   e.attackEnded ||
                   (e.actorBase && actors[p].sourceBase &&
                    e.actorBase!=actors[p].sourceBase) ||
                   (family!=0x2Eu && !e.noBox))continue;
                for(int j=0;j<samples[p].projectileCount && j<12;++j){
                    const unsigned base=samples[p].projectileBases[j];
                    bool old=false;
                    for(int k=0;k<t.previousProjectileCount;++k)
                        if(t.previousProjectileBases[k]==base)old=true;
                    if(!old){
                        xm30ClaimProjectile(e,base);
                        if(e.noBox && e.projectileBase==0u){
                            // Promotion is based on an actual newly linked
                            // ROM object, not a guessed QCD/QCF button.
                            e.source=1;e.projectileBase=base;
                            e.spawnFrame=frame;
                            e.lastProjectileFrame=frame;
                            e.positionKnown=true;
                            e.lastProjectileX=samples[p].projectileX[j];
                            e.lastProjectileY=samples[p].projectileY[j];
                            e.x=e.lastProjectileX;e.y=e.lastProjectileY;
                            if(e.motion==XM32_NORMAL)
                                e.motion=XM32_PROJECTILE;
                        }
                    }
                }
                break;
            }
            // Every projectile record is advanced by ITS OWN linked object.
            // An unrelated live projectile, or a held direction/button, cannot
            // extend its active count. Disappearance of this exact object ends
            // its chart and closes its attack-box lifetime.
            for(int i=0;i<count;++i){
                XM30Event& e=history[i];
                if(e.player!=p || e.source!=1 || !e.projectileBase ||
                   (e.despawnFrame && e.closed))continue;
                int slot=-1;
                for(int j=0;j<samples[p].projectileCount && j<12;++j)
                    if(samples[p].projectileBases[j]==e.projectileBase){slot=j;break;}
                const bool ownedLive=xm30OwnedProjectileLive(e,samples[p]);
                const bool ownedAttack=xm30OwnedProjectileAttack(e,samples[p]);
                if(ownedLive)e.lastProjectileFrame=frame;
                if(slot<0){
                    if(!ownedLive && !e.despawnFrame)e.despawnFrame=frame;
                    if(!e.closed){
                        if(ownedAttack){
                            if(e.noBox){
                                e.noBox=false;e.contactOnly=false;
                                e.firstFrame=frame;
                                e.startup=(int)(frame-e.inputFrame);e.active=1;
                            }else ++e.active;
                            e.hitboxLastFrame=frame;
                        }else if(e.attackFamily==0x2Eu && !e.noBox &&
                                 (superPhaseActive ||
                                  (e.nativeSuperPhase && superStageActive))){
                            e.nativeSuperPhase=true;++e.active;
                            e.hitboxLastFrame=frame;
                        }else if(ownedLive || (e.attackFamily==0x2Eu &&
                                 (landed[p] || contactExitGrace[p]>0))){
                            // Do not close a super projectile on the hitstop
                            // handoff; the ROM will resume or actually exit.
                        }else{e.closed=true;e.endFrame=frame-1;}
                    }
                }else{
                    const int x=samples[p].projectileX[slot],y=samples[p].projectileY[slot];
                    e.lastProjectileFrame=frame;
                    const XM30Hit& own=samples[p].projectileHits[slot];
                    const bool ownAttack=own.active && own.sourceBase==e.projectileBase;
                    if(!e.closed && (frame>e.firstFrame || e.noBox)){
                        if(ownAttack || ownedAttack){
                            if(e.noBox){
                                e.noBox=false;e.contactOnly=false;
                                e.firstFrame=frame;
                                e.startup=(int)(frame-e.inputFrame);e.active=1;
                            }else ++e.active;
                            e.hitboxLastFrame=frame;
                            if(superPhaseActive || superStageActive)
                                e.nativeSuperPhase=true;
                        }else if(e.attackFamily==0x2Eu && !e.noBox &&
                                 (superPhaseActive ||
                                  (e.nativeSuperPhase && superStageActive))){
                            e.nativeSuperPhase=true;++e.active;
                            e.hitboxLastFrame=frame;
                        }else if(ownedLive || (e.attackFamily==0x2Eu &&
                                 (landed[p] || contactExitGrace[p]>0))){
                            // Missing damage box during impact is not the end
                            // of the move. Preserve this card across stop.
                        }else{e.closed=true;e.endFrame=frame-1;}
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
            // A new native action can be accepted DURING a continuing old
            // box (light->medium, normal->special, normal->super). Its new
            // attack must claim the box instead of extending the old row.
            const bool nativeComboEdge=t.pending>=0 &&
                t.pendingComboTransition &&
                t.pendingAttackFamily!=0u &&
                hits[0].active &&
                (actors[p].attackPhase&0x80u)!=0u;
            // Projectile active frames are handled above PER LINKED OBJECT.
            int& meleeIndex=t.eventIndex[0];
            if(meleeIndex<0 && hits[0].active && family!=0u)
                for(int i=count-1;i>=0;--i){
                    XM30Event& e=history[i];
                    if(e.player!=p || e.attackFamily!=family ||
                       e.attackEnded || e.source!=0 || !e.noBox ||
                       (e.actorBase && actors[p].sourceBase &&
                        e.actorBase!=actors[p].sourceBase) ||
                       !xm30SameNativeAttack(actors[p],
                           e.attackFamily,e.attackAction,e.attackMode))continue;
                    e.noBox=false;e.contactOnly=false;
                    e.firstFrame=frame;e.startup=(int)(frame-e.inputFrame);
                    e.closed=false;e.hitboxLastFrame=0;
                    meleeIndex=i;break;
                }
            if(meleeIndex>=0){
                if(meleeIndex>=count){meleeIndex=-1;}
                else {
                    XM30Event& e=history[meleeIndex];
                    const bool nativeSuperOffense=e.attackFamily==0x2Eu &&
                        (superPhaseActive || (e.nativeSuperPhase && superStageActive));
                    // A native combo handoff closes the OLD hitbox lane
                    // at its last frame even when RAM box70/72 did not fall.
                    // The new move's first active frame is THIS frame.
                    if(e.attackEnded && !landed[p] && contactExitGrace[p]==0u){
                        // A stale RAM melee box can remain asserted after
                        // the native action ended. It cannot extend that
                        // move's ACTIVE count or paint green after exit.
                        e.closed=true;e.endFrame=e.attackLastFrame;
                        meleeIndex=-1;
                    }else if(nativeComboEdge &&
                       (e.attackFamily!=t.pendingAttackFamily ||
                        e.attackAction!=t.pendingAttackAction ||
                        (e.attackFamily==0x28u &&
                         e.attackMode!=t.pendingAttackMode))){
                        e.closed=true;e.endFrame=frame-1;
                        meleeIndex=-1;
                    }else if(hits[0].active || nativeSuperOffense){
                        if(nativeSuperOffense)e.nativeSuperPhase=true;
                        ++e.active;
                        e.hitboxLastFrame=frame;
                    }else if(e.attackFamily==0x2Eu &&
                             (landed[p] || contactExitGrace[p]>0)){
                        // No attack frames are invented for a blank box, but
                        // the hit freeze must not retire the original row.
                    }else{
                        e.closed=true;e.endFrame=frame-1;
                        meleeIndex=-1;
                    }
                }
            }
            const long long age=t.pending>=0?(long long)frame-t.pending:-1;
            if(t.pending>=0 && age<=MAX_WAIT){
                const bool meleeRise=hits[0].active &&
                    (!t.previous[0] || nativeComboEdge);
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
                // 87.245 native trace: repeated button edges at host 1385
                // and 1398 occur while 2E00/stage04 owns ONE super. Its
                // projectile box cycles at 1397/1428, but the pending input
                // has NO NEW ROM ACTION (pendingAttackFamily==0). The old
                // check made a false card from each new projectile object.
                // Only an accepted new native action may claim that box
                // while the prior action remains in progress.
                const bool nativeCandidateAccepted=
                    t.pendingAttackFamily!=0u ||
                    !(t.pendingOriginValid && t.pendingOriginFamily!=0u);
                // An accepted new native attack + opponent HP loss is
                // a real hit even when the one-frame box is not sampled.
                // Make a contact-only card, with UNKNOWN hitbox timings,
                // rather than silently dropping this combo hit.
                const bool hpFellNow=actors[1-p].valid && healthSeeded[1-p] &&
                    actors[1-p].health<previousHealth[1-p];
                // A linked beam object can damage without an observable
                // kind-5 box at the exact HP-loss sample. Its genuinely new
                // linked identity is acceptable evidence, whereas an old
                // projectile from a prior move is NOT allowed to claim this
                // newly accepted native super.
                bool linkedProjectileContact=false;
                if(t.pendingSpawnBase)for(int j=0;
                    j<samples[p].projectileCount && j<12;++j)
                    if(samples[p].projectileBases[j]==t.pendingSpawnBase)
                        linkedProjectileContact=true;
                const bool deferredHit=t.pendingAttackFamily==0x2Eu &&
                    deferredSuperContactCount[p]>0u &&
                    deferredSuperAction[p]==t.pendingAttackAction;
                const bool contactWithoutBox=(hpFellNow || deferredHit) &&
                    t.pendingAttackFamily!=0u &&
                    !meleeRise && !projectileRise &&
                    (samples[p].projectileCount==0 || linkedProjectileContact);
                // Any accepted native super with a NEW linked object earns
                // a provisional card even before kind-5 boxes are visible.
                const bool objectOnlySuper=t.pendingAttackFamily==0x2Eu &&
                    t.pendingSpawnBase!=0u &&
                    t.pendingSpawnFrame>=t.pendingAttackStart &&
                    linkedProjectileContact && !meleeRise && !projectileRise;
                const bool projectileContactOnly=contactWithoutBox &&
                    t.pendingSpawnBase!=0u && linkedProjectileContact;
                if((meleeRise || projectileRise || contactWithoutBox ||
                    objectOnlySuper) &&
                   nativeCandidateAccepted){
                    // Multi-hit super collision boxes may disappear during
                    // hit-stop and rise again. The original 2E action owns
                    // ONE duration card; a second hit is not a new move.
                    bool continuingSuper=false;
                    if(t.pendingAttackFamily==0x2Eu)for(int k=count-1;k>=0;--k){
                        const XM30Event& old=history[k];
                        if(old.player!=p || !old.attackObserved ||
                           old.attackEnded || old.attackFamily!=0x2Eu ||
                           (old.actorBase && actors[p].sourceBase &&
                            old.actorBase!=actors[p].sourceBase))continue;
                        if(old.attackAction==t.pendingAttackAction){
                            continuingSuper=true;break;
                        }
                    }
                    if(continuingSuper){
                        t.pending=-1;
                        t.previous[0]=hits[0].active;
                        t.previous[1]=hits[1].active;
                    }else{
                    const int source=(projectileRise && (!meleeRise || t.motion!=XM32_NORMAL)) ||
                                     projectileContactOnly || objectOnlySuper ? 1:0;
                    XM30Hit chosen=source==1?newProjectile:hits[0];
                    if(projectileContactOnly || objectOnlySuper){
                        chosen.sourceBase=t.pendingSpawnBase;
                        chosen.x=t.pendingX;chosen.y=t.pendingY;
                    }
                    const int idx=appendEvent();
                    XM30Event& e=history[idx];e=XM30Event{};
                    e.player=p;e.actorBase=actors[p].sourceBase;e.startup=(int)age;
                    const bool boxMissing=contactWithoutBox || objectOnlySuper;
                    e.active=boxMissing?0:1;
                    e.noBox=boxMissing;
                    e.contactOnly=contactWithoutBox;
                    e.hitboxLastFrame=boxMissing?0u:frame;
                    e.inputObserved=!t.pendingNativeOnly;
                    xm30AssignInput(e,t);
                    e.nativeSuperPhase=t.pendingAttackFamily==0x2Eu &&
                        (superPhaseActive || superStageActive);
                    e.attackObserved=t.pendingAttackFamily!=0u;
                    e.attackFamily=t.pendingAttackFamily;
                    e.attackAction=t.pendingAttackAction;
                    e.attackMode=t.pendingAttackMode;
                    e.attackStartFrame=t.pendingAttackStart;
                    e.attackLastFrame=t.pendingAttackLast;
                    e.attackEndFrame=t.pendingAttackEnd;
                    e.attackEnded=t.pendingAttackEnd!=0u;
                    e.inputFrame=(unsigned long long)t.pending;
                    e.inputWallFrame=t.pendingWallFrame;
                    e.lastWallFrame=wallFrame[p];
                    e.source=source;
                    if(source==1 && t.pendingSpawnBase &&
                       chosen.sourceBase==t.pendingSpawnBase)
                        e.spawnFrame=t.pendingSpawnFrame;
                    e.code=t.code;e.x=chosen.x;e.y=chosen.y;e.firstFrame=frame;
                    if(contactWithoutBox && t.pendingAttackFamily!=0x2Eu){
                        e.closed=true;e.endFrame=frame;
                    }
                    e.motion=t.motion!=XM32_NORMAL?t.motion:
                        (source==1?XM32_PROJECTILE:XM32_NORMAL);
                    if(source==1){
                        e.projectileBase=chosen.sourceBase;
                        xm30ClaimProjectile(e,e.projectileBase);
                        // Unknown collision timing must never be upgraded to
                        // a fictional active-box observation.
                        e.lastProjectileFrame=
                            (!projectileContactOnly || linkedProjectileContact)?frame:0u;
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
                    if(source==0 && !boxMissing)t.eventIndex[0]=idx;
                    if(e.attackFamily==0x2Eu &&
                       deferredSuperContactCount[p] &&
                       deferredSuperAction[p]==e.attackAction){
                        for(unsigned j=0;j<deferredSuperContactCount[p];++j)
                            xm30AddContact(e,deferredSuperContacts[p][j],
                                               deferredSuperContactWalls[p][j]);
                        deferredSuperContactCount[p]=0;
                        if(e.noBox && e.contactCount)e.contactOnly=true;
                    }
                    XM30Float& popup=floats[floatNext++%FLOATS];popup=XM30Float{};
                    // A native-only or contact-only row has no observed
                    // button-to-box startup; never flash a fictional 0F.
                    popup.visible=e.inputObserved && !e.noBox;
                    popup.player=p;popup.startup=e.startup;
                    popup.x=e.x;popup.y=e.y;
                    t.pending=-1;
                    }
                }
            }
            if(t.pending>=0 && age>MAX_WAIT){
                // A button pressed inside an UNCHANGED native move was
                // rejected/absorbed by that move. It is not a new attack
                // and must not become an invented NO BOX card at timeout.
                if(t.pendingOriginValid && t.pendingOriginFamily!=0u &&
                   t.pendingAttackFamily==0u){
                    t.pending=-1;
                }else{
                // Report missing signal as missing, never fabricate an S/A value.
                const int idx=appendEvent();
                XM30Event& e=history[idx];e=XM30Event{};
                e.player=p;e.actorBase=actors[p].sourceBase;
                e.code=t.code;e.motion=t.motion;e.noBox=true;
                e.inputObserved=!t.pendingNativeOnly;
                xm30AssignInput(e,t);
                e.attackObserved=t.pendingAttackFamily!=0u;
                e.attackFamily=t.pendingAttackFamily;
                e.attackAction=t.pendingAttackAction;
                e.attackMode=t.pendingAttackMode;
                e.attackStartFrame=t.pendingAttackStart;
                e.attackLastFrame=t.pendingAttackLast;
                e.attackEndFrame=t.pendingAttackEnd;
                e.attackEnded=t.pendingAttackEnd!=0u;
                e.inputFrame=(unsigned long long)t.pending;
                e.inputWallFrame=t.pendingWallFrame;
                e.lastWallFrame=wallFrame[p];
                e.endFrame=frame;e.closed=true;
                t.pending=-1;
                }
            }
            t.previous[0]=hits[0].active;
            t.previous[1]=hits[1].active;
            t.lastNativeFamily=family;
            t.lastNativeBase=actors[p].sourceBase;
            t.lastNativeAction=actors[p].nativeAction;
            t.lastNativeMode=actors[p].mode;
            t.lastNativeStep=actors[p].nativeStep;
            t.lastNativeValid=actors[p].valid;
            t.previousActiveCount=0;
            for(int j=0;j<samples[p].projectileCount && j<12;++j)
                if(samples[p].projectileHits[j].active &&
                   t.previousActiveCount<12)
                    t.previousActiveBases[t.previousActiveCount++]=samples[p].projectileHits[j].sourceBase;
            t.previousProjectileCount=(samples[p].projectileCount<12?samples[p].projectileCount:12);
            for(int k=0;k<t.previousProjectileCount;++k)
                t.previousProjectileBases[k]=samples[p].projectileBases[k];
            if(contactExitGrace[p]>0)--contactExitGrace[p];
        }
        // Store both advancing and frozen host ticks before red contacts.
        recordWallTicks(advancePlayer);
        // After rising-edge creation: first active frame and first actual
        // damage sample may be identical. Assign the marker to the new row.
        recordContacts(actors,samples,advancePlayer);
    }
};
// Stage 87.217: native pixel-space geometry, deterministic frame-data
// clock. Rise 40 SOURCE pixels to the apex, then in SIX real game
// frames squeeze only width and travel one font height (8 source px).
// No SDL logical/window scaling participates in these coordinates.
struct XM30FloatPose {
    int rise=0;
    int alpha=255;
    double horizontal=1.0;
};
static inline XM30FloatPose xm30FloatPose(int age,int fontHeight){
    XM30FloatPose pose;
    if(age<0)age=0;
    if(fontHeight<1)fontHeight=1;
    if(age<XM30FrameData::FLOAT_APEX){
        pose.rise=(age*40)/XM30FrameData::FLOAT_APEX;
        return pose;
    }
    const int elapsed=age-XM30FrameData::FLOAT_APEX+1;
    const int progress=elapsed<0?0:(elapsed>XM30FrameData::FLOAT_EXIT?
                               XM30FrameData::FLOAT_EXIT:elapsed);
    pose.rise=40+(progress*fontHeight)/XM30FrameData::FLOAT_EXIT;
    pose.horizontal=(double)(XM30FrameData::FLOAT_EXIT-progress)/
                    (double)XM30FrameData::FLOAT_EXIT;
    pose.alpha=255*(XM30FrameData::FLOAT_EXIT-progress)/
               XM30FrameData::FLOAT_EXIT;
    return pose;
}
#endif
