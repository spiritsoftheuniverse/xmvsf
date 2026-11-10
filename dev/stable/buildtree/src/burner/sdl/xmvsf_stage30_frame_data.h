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
    unsigned attackPhase=0;     // ROM +64: 0x80 attacking, 0x40 recovery
    unsigned nativeStep=0;      // ROM +08: super 2E script stage, not a frame timer
    unsigned hitStop=0;         // ROM +85: explicit per-fighter stop steps
    unsigned animPtr=0;         // ROM +34 native animation script pointer
    unsigned animTimer=0;       // ROM +3E native animation frame countdown
    unsigned health=0;          // HUD-resolved active fighter HP; real hit confirmation
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
    // A native 2E super may keep its attacking phase after the first
    // offensive collision box vanishes. The ROM phase, not a guessed move
    // name or timeout, continues its active span through such substeps.
    bool nativeSuperPhase=false;
    // Separate hit confirmations from the lifetime of the attack. A hit
    // cannot end a ROM-owned super (or a normal attack), and multi-hit
    // attacks may record multiple distinct health-loss frames.
    static const int CONTACTS=32;
    unsigned long long contactFrames[CONTACTS]={};
    int contactCount=0;
    unsigned long long motionBits[16]={}; // exact movement-edge frames, up to 1024f
    bool motionOverflow=false;
    bool positionKnown=false;
    // Action duration is separate from the hitbox and projectile clocks.
    // Start at the first observed real ROM attack action, end on the first
    // different action/controller family, independent of input or height.
    bool attackObserved=false,attackEnded=false;
    unsigned attackFamily=0,attackAction=0;
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
    // At the INPUT EDGE the previous native action may still be active.
    // Combo cancels frequently inherit a continuously active box70/72.
    // The old code only watched !previousHitbox and missed those attacks.
    unsigned lastNativeFamily=0,lastNativeAction=0;
    bool lastNativeValid=false;
    unsigned pendingOriginFamily=0,pendingOriginAction=0;
    bool pendingOriginValid=false,pendingComboTransition=false;
    // A rejected input inside a live attack must not steal the next
    // repeating projectile box or create a timeout NO BOX card.
    bool pendingOriginLeft=false;
    int previousProjectileCount=0;
    unsigned previousProjectileBases[12]={};
    int previousActiveCount=0;
    unsigned previousActiveBases[12]={};
};
struct XM30FrameData {
    // Preserve the 120-native-frame ascent to the 40px apex, then
    // squeeze width across SIX native frames while rising one glyph
    // height IN THE 384x224 SOURCE IMAGE (never SDL window coordinates).
    // This modifies presentation timing, never attack measurements.
    static const int HISTORY=24, FLOATS=8, MAX_WAIT=120;
    static const int FLOAT_APEX=120, FLOAT_EXIT=6;
    static const int FLOAT_LIFE=FLOAT_APEX+FLOAT_EXIT;
    bool enabled=false;
    bool inCombat=false;
    // A shared host-frame axis cannot time two fighters independently:
    // the attacker continues 2E during defender-only hitstop. Event
    // timestamps are on the owning fighter's unfrozen native step axis.
    unsigned long long frame=0; // display/observation tick when either player advances
    unsigned long long playerFrame[2]={0,0};
    // Native cinematic holds may keep +85=0 and phase=80, notably 2C12.
    // Preserve the previous sampled animation, not the wall-clock time.
    bool animationSeeded=false;
    unsigned previousAnimPtr[2]={0,0};
    unsigned previousAnimTimer[2]={0,0};
    unsigned previousAnimMode[2]={0,0};
    unsigned previousAnimAction[2]={0,0};
    unsigned previousAnimStep[2]={0,0};
    unsigned long long eventsCreated=0; // monotonic, unlike bounded history.count
    unsigned long long superFreezeFrames=0; // callbacks with neither player advancing
    // Unlike the FF5009 round clock, ROM hit-stop counters count down to
    // zero. They cannot permanently stall the meter when the round timer is
    // deliberately held at 99 in Training.
    bool hitStopSeeded=false;
    unsigned previousHitStop[2]={0,0};
    bool healthSeeded[2]={false,false};
    unsigned previousHealth[2]={0,0};
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
        animationSeeded=false;
        for(int p=0;p<2;++p){
            previousAnimPtr[p]=previousAnimTimer[p]=0;
            previousAnimMode[p]=previousAnimAction[p]=previousAnimStep[p]=0;
        }
        superFreezeFrames=0; count=0; floatNext=0; inCombat=false;
        hitStopSeeded=false;previousHitStop[0]=previousHitStop[1]=0;
        healthSeeded[0]=healthSeeded[1]=false;
        previousHealth[0]=previousHealth[1]=0;
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
        // A+B pressed on the same frame is one candidate, not two moves.
        if(t.pending>=0 && (long long)playerFrame[player]-t.pending<=2)return;
        // Do not reject new attack inputs just because an older projectile is
        // still on screen. A new row still requires a NEW hitbox edge.
        t.pending=(long long)playerFrame[player];
        t.pendingSpawnBase=0;
        t.pendingSpawnFrame=0;
        // A new button edge only starts the next candidate. It NEVER ends an
        // earlier attack: each existing card stops on native ROM action exit.
        t.pendingAttackFamily=t.pendingAttackAction=0u;
        t.pendingAttackStart=t.pendingAttackLast=t.pendingAttackEnd=0u;
        t.pendingOriginFamily=t.lastNativeFamily;
        t.pendingOriginAction=t.lastNativeAction;
        t.pendingOriginValid=t.lastNativeValid;
        t.pendingComboTransition=false;t.pendingOriginLeft=false;
        t.pendingFirstMotion=0;t.pendingLastMotion=0;t.pendingMoveFrames=0;
        t.pendingPositionKnown=false;t.pendingMotionOverflow=false;
        for(int word=0;word<16;++word)t.pendingMotionBits[word]=0;
        t.code=code;
        t.motion=directions[player].classify(playerFrame[player]);
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
                        const bool advanced[2]){
        for(int victim=0;victim<2;++victim){
            if(!actors[victim].valid){
                healthSeeded[victim]=false;pendingContact[1-victim]=false;
                continue;
            }
            const unsigned health=actors[victim].health;
            const bool damaged=healthSeeded[victim] &&
                               health<previousHealth[victim];
            previousHealth[victim]=health;
            healthSeeded[victim]=true;
            if(!damaged)continue;
            const int attacker=1-victim;
            // An impact while frozen has no new gameplay frame. Its red
            // marker belongs to this tick, but the native-action handoff
            // must remember the strike until the next counted frame.
            if(!advanced[attacker])pendingContact[attacker]=true;
            const unsigned long long at=playerFrame[attacker];
            for(int i=count-1;i>=0;--i){
                XM30Event& e=history[i];
                if(e.player!=attacker || e.noBox || e.firstFrame>at)continue;
                // Collision may disappear on contact; keep the most recent
                // move if the damage is synchronous with its exit.
                if(e.closed && at>e.endFrame+1u &&
                   (e.attackEnded || at>e.attackLastFrame+1u))continue;
                if(e.contactCount>0 &&
                   e.contactFrames[e.contactCount-1]==at)break;
                if(e.contactCount==XM30Event::CONTACTS){
                    for(int k=1;k<XM30Event::CONTACTS;++k)
                        e.contactFrames[k-1]=e.contactFrames[k];
                    --e.contactCount;
                }
                e.contactFrames[e.contactCount++]=at;
                break;
            }
        }
    }
    void onFrame(const XM30Sample& p1,const XM30Sample& p2,
                 const XM41ActorSample& a1=XM41ActorSample{},
                 const XM41ActorSample& a2=XM41ActorSample{},
                 unsigned clockValue=~0u){
        if(!enabled)return;
        beginCombat();
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
        const bool nativeSetup=
            ((xm30NativeAttackFamily(a1)==0x2Cu ||
              xm30NativeAttackFamily(a1)==0x2Eu) &&
             a1.nativeStep==2u &&
             !p1.melee.active && !p1.projectile.active) ||
            ((xm30NativeAttackFamily(a2)==0x2Cu ||
              xm30NativeAttackFamily(a2)==0x2Eu) &&
             a2.nativeStep==2u &&
             !p2.melee.active && !p2.projectile.active);
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
        const bool cinematicStop=nativeSetup && bothAnimationsStatic;
        bool advancePlayer[2]={false,false};
        for(int p=0;p<2;++p){
            if(!actors[p].valid || cinematicStop)continue;
            // A newly accepted first impact can show +85 on the very
            // same native sample as its first offensive box. Count THAT
            // owner's step once; do not invent the rest of its stop ticks.
            bool firstImpact=false;
            if(stops[p]>0u && (!hitStopSeeded || previousHitStop[p]==0u)){
                const XM30Track& t=track[p];
                if(t.pending>=0 && !t.pendingAttackEnd){
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
            // Even while both fighters are frozen, record HP changes at
            // the LAST real attacker tick, never at an invented one.
            recordContacts(actors,advancePlayer);
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
            // ROM attack flag is authoritative; 2E's post-startup script
            // stage is a fallback once the move has a proven hitbox.
            // 0x40 recovery explicitly cancels both active indicators.
            const bool superPhaseActive=family==0x2Eu &&
                (actors[p].attackPhase&0x80u)!=0u;
            const bool superStageActive=family==0x2Eu &&
                actors[p].nativeStep>=4u &&
                (actors[p].nativeStep&1u)==0u &&
                (actors[p].attackPhase&0x40u)==0u;
            for(int i=0;i<count;++i){
                XM30Event& e=history[i];
                if(e.player!=p || !e.attackObserved || e.attackEnded)continue;
                if(xm30SameNativeAttack(actors[p],e.attackFamily,e.attackAction) ||
                   (e.attackFamily==0x2Eu && (actors[p].attackPhase&0x80u)!=0u)){
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
                     actors[p].nativeAction!=t.pendingOriginAction)))
                    t.pendingOriginLeft=true;
                const bool originStillActive=t.pendingOriginValid &&
                    t.pendingOriginFamily!=0u &&
                    !t.pendingOriginLeft &&
                    family==t.pendingOriginFamily &&
                    (family!=0x28u ||
                     actors[p].nativeAction==t.pendingOriginAction);
                // A second normal press during the first normal's recovery
                // is NOT automatically a new move. Wait for a changed ROM
                // action selector (or a different native attack family).
                // 2C/2E can change internal substeps, so their same-family
                // transitions remain owned by the existing move.
                if(!t.pendingAttackFamily && family && !originStillActive){
                    t.pendingAttackFamily=family;
                    t.pendingAttackAction=actors[p].nativeAction;
                    t.pendingAttackStart=t.pendingAttackLast=frame;
                    t.pendingComboTransition=t.pendingOriginValid &&
                        t.pendingOriginFamily!=0u;
                }else if(t.pendingAttackFamily && !t.pendingAttackEnd){
                    if(xm30SameNativeAttack(actors[p],t.pendingAttackFamily,
                                            t.pendingAttackAction) ||
                       (t.pendingAttackFamily==0x2Eu &&
                        (actors[p].attackPhase&0x80u)!=0u) ||
                       landed[p] || contactExitGrace[p]>0)
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
                if(e.player!=p || e.source!=1 || !e.projectileBase ||
                   (e.despawnFrame && e.closed))continue;
                int slot=-1;
                for(int j=0;j<samples[p].projectileCount && j<12;++j)
                    if(samples[p].projectileBases[j]==e.projectileBase){slot=j;break;}
                if(slot<0){
                    if(!e.despawnFrame)e.despawnFrame=frame; // actual object removal
                    if(!e.closed){
                        if(e.attackFamily==0x2Eu &&
                           (superPhaseActive || (e.nativeSuperPhase && superStageActive))){
                            e.nativeSuperPhase=true;++e.active;
                        }else if(e.attackFamily==0x2Eu &&
                                 (landed[p] || contactExitGrace[p]>0)){
                            // Do not close a super projectile on the hitstop
                            // handoff; the ROM will resume or actually exit.
                        }else{e.closed=true;e.endFrame=frame-1;}
                    }
                }else{
                    const int x=samples[p].projectileX[slot],y=samples[p].projectileY[slot];
                    const XM30Hit& own=samples[p].projectileHits[slot];
                    const bool ownAttack=own.active && own.sourceBase==e.projectileBase;
                    if(!e.closed && frame>e.firstFrame){
                        // Native super attack phase can cover successive beam /
                        // cinematic substeps with temporarily absent collision
                        // boxes. Its explicit ROM recovery phase stops the run.
                        if(ownAttack ||
                           (e.attackFamily==0x2Eu &&
                            (superPhaseActive || (e.nativeSuperPhase && superStageActive)))){
                            if(superPhaseActive || superStageActive)e.nativeSuperPhase=true;
                            ++e.active;
                        }else if(e.attackFamily==0x2Eu &&
                                 (landed[p] || contactExitGrace[p]>0)){
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
            if(meleeIndex>=0){
                if(meleeIndex>=count){meleeIndex=-1;}
                else {
                    XM30Event& e=history[meleeIndex];
                    const bool nativeSuperOffense=e.attackFamily==0x2Eu &&
                        (superPhaseActive || (e.nativeSuperPhase && superStageActive));
                    // A native combo handoff closes the OLD hitbox lane
                    // at its last frame even when RAM box70/72 did not fall.
                    // The new move's first active frame is THIS frame.
                    if(nativeComboEdge &&
                       (e.attackFamily!=t.pendingAttackFamily ||
                        e.attackAction!=t.pendingAttackAction)){
                        e.closed=true;e.endFrame=frame-1;
                        meleeIndex=-1;
                    }else if(hits[0].active || nativeSuperOffense){
                        if(nativeSuperOffense)e.nativeSuperPhase=true;
                        ++e.active;
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
                if((meleeRise || projectileRise) && nativeCandidateAccepted){
                    // Multi-hit super collision boxes may disappear during
                    // hit-stop and rise again. The original 2E action owns
                    // ONE duration card; a second hit is not a new move.
                    bool continuingSuper=false;
                    if(t.pendingAttackFamily==0x2Eu)for(int k=count-1;k>=0;--k){
                        const XM30Event& old=history[k];
                        if(old.player!=p || !old.attackObserved ||
                           old.attackEnded || old.attackFamily!=0x2Eu)continue;
                        if(old.attackAction==t.pendingAttackAction){
                            continuingSuper=true;break;
                        }
                    }
                    if(continuingSuper){
                        t.pending=-1;
                        t.previous[0]=hits[0].active;
                        t.previous[1]=hits[1].active;
                    }else{
                    const int source=(projectileRise && (!meleeRise || t.motion!=XM32_NORMAL))?1:0;
                    const XM30Hit& chosen=source==1?newProjectile:hits[0];
                    const int idx=appendEvent();
                    XM30Event& e=history[idx];e=XM30Event{};
                    e.player=p;e.startup=(int)age;e.active=1;
                    e.nativeSuperPhase=t.pendingAttackFamily==0x2Eu &&
                        (superPhaseActive || superStageActive);
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
            }
            t.previous[0]=hits[0].active;
            t.previous[1]=hits[1].active;
            t.lastNativeFamily=family;
            t.lastNativeAction=actors[p].nativeAction;
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
        // After rising-edge creation: first active frame and first actual
        // damage sample may be identical. Assign the marker to the new row.
        recordContacts(actors,advancePlayer);
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
