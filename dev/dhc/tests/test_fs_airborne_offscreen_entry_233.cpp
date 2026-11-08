// TEST 87.233: Wolverine/any fighter airborne while horizontally offscreen
// must be grounded BEFORE original 68000 decides the native ground dash.
// Uses 87.232's proven floor writer and ROM-owned 1400/1600 dash controller.
// ROM-free host-swapped CPS2 RAM fixtures, no invented action script.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_220_zero_dash_startup.h"
using namespace XM87220Dash;
using namespace XM87233AirReturn;

static std::uint8_t r[65536]={};
static const unsigned P=0x4000u,Q=0x4400u;
static void w(unsigned p,unsigned v){setWord(r,p,v);}
static void l(unsigned p,std::uint32_t v){setLong(r,p,v);}
static unsigned rw(unsigned p){return word(r,p);}
static std::uint32_t rl(unsigned p){return longword(r,p);}

static void resetNative(unsigned fighter=0x0008u,bool opponent=true){
    std::memset(r,0,sizeof(r));
    r[0x5001u]=8u; // byte-swapped FF5000 match state
    w(P,0x0101u);w(P+0x52u,fighter);
    w(P+0x06u,0u);w(P+0x08u,0u);
    w(P+0x60u,0u);w(P+0xa0u,0u);w(P+0xaau,0u);
    w(P+0x10u,472u);w(P+0x254u,472u);
    w(P+0x3eu,6u);w(P+0x42u,0x0032u);
    l(P+0x0cu,0x02b90000u);l(P+0x14u,0u);l(P+0x1cu,0u);
    l(P+0x34u,0x000ac000u);l(P+0x38u,0x000ac010u);
    w(P+0x210u,0x038fu); // HP must never rewind
    if(opponent){
        w(Q,0x0101u);w(Q+0x52u,0x0012u);
        w(Q+0x10u,472u);w(Q+0x254u,472u);
        l(Q+0x0cu,0x03200000u); // other x=800
        w(Q+0xaau,0u);
    }
}
static void flyOffscreen(unsigned action=0x0800u,unsigned mode=0x1c00u,
                         std::uint32_t x=0x01600000u){
    w(P+0x60u,action);w(P+0xa0u,mode);
    w(P+0x10u,144u);w(P+0xaau,0x0102u);
    w(P+0x3eu,11u);
    l(P+0x1cu,0x000b0000u);
    l(P+0x0cu,x);
    l(P+0x34u,0x000a7000u);l(P+0x38u,0x000a7010u);
}
static void nativeGroundDash(unsigned mode=0x1400u,
                            std::uint32_t x=0x01600000u){
    w(P+0x60u,0x0400u);w(P+0xa0u,mode);
    w(P+0x3eu,7u);l(P+0x0cu,x);
    l(P+0x34u,0x000a6200u);l(P+0x38u,0x000a6210u);
}
static void checkGround(){
    assert(rw(P+0x10u)==472u);
    assert(rw(P+0x12u)==0u);
    assert(rw(P+0xaau)==0x0002u); // low native status unchanged
    assert(rl(P+0x1cu)==0u);
    assert(rw(P+0x210u)==0x038fu);
}
int main(){
    // Pure classifier first: one offscreen frame alone is insufficient.
    Native n;
    n.combat=true;n.active=true;n.opponentActive=true;
    n.fighter=0x0008u;n.mode=0x1c00u;n.action=0x0800u;
    n.y=144u;n.floor=472u;n.air=0x0102u;
    n.x=0x01600000u;n.opponentX=0x03200000u;
    Tracker t;
    assert(t.observe(n)==OUTSIDE_AIR_CANDIDATE);
    assert(t.observe(n)==GROUND_BEFORE_NATIVE_DASH);
    n.y=472u;n.air=0x0002u;
    assert(t.observe(n)==WAIT_FOR_NATIVE_DASH);
    n.mode=0x1400u;n.action=0x0400u;
    // Native X advanced eight pixels into the fight; do not cancel
    // merely because distance drops from 448 to 440 px.
    n.x=0x01680000u;
    assert(t.observe(n)==NATIVE_GROUND_DASH_READY);

    // Test all fighter identities through one generic path. No special
    // hardcoded Wolverine/Cyclops/Charlie dispatcher or guessed ROM ID.
    const unsigned ids[]={0x0000u,0x0002u,0x0008u,0x0020u};
    for(unsigned id:ids){
        resetNative(id);
        Controller c;c.frame(1u,r,true);
        assert(c.actor[0].neutralSnapshotValid);
        flyOffscreen();
        c.frame(2u,r,true);
        assert(rw(P+0x10u)==144u); // do not ground on transient crossing
        assert((rw(P+0xaau)>>8u)==1u);
        c.frame(3u,r,true);
        checkGround();
        assert(c.actor[0].airReturn.pending);
        assert(rw(P+0xa0u)==0x1c00u);  // do NOT invent ground mode
        assert(rw(P+0x60u)==0x0800u); // original action survives
        assert(rl(P+0x0cu)==0x01600000u); // original offscreen X
        assert(rl(P+0x34u)==0x000a7000u); // original script intact
        assert(rw(P+0x3eu)==11u); // original air script timer unchanged
        nativeGroundDash();
        c.frame(4u,r,true);
        assert(c.actor[0].autoReturn);
        assert(c.actor[0].returnAnchorValid);
        assert(!c.actor[0].airReturn.pending);
        checkGround();
        assert(rw(P+0xa0u)==0x1400u);
        assert(rl(P+0x34u)==0x000a6200u);
        assert(rw(P+0x3eu)==7u); // 87.232 original return delay
        // Real native dash velocity and script still owned by the ROM.
        l(P+0x0cu,0x01680000u);l(P+0x14u,0x00080000u);
        c.frame(5u,r,true);
        assert(c.actor[0].autoReturn);
        assert(rw(P+0x3eu)==7u);
        // Native return ends: new voluntary dash remains instant.
        w(P+0xa0u,0u);w(P+0x60u,0u);
        l(P+0x14u,0u);
        c.frame(6u,r,true);
        assert(!c.actor[0].autoReturn);
        nativeGroundDash(0x1402u,0x01680000u);
        w(P+0x3eu,9u);
        c.frame(7u,r,true);
        assert(rw(P+0x3eu)==1u); // 87.220 instant startup unchanged
    }

    // The original 68k may write an impossible position on transition.
    // Use the previously observed OFFSCREEN air anchor, not visible
    // center or another fighter's neutral snapshot.
    resetNative();Controller broken;broken.frame(1u,r,true);
    flyOffscreen();broken.frame(2u,r,true);broken.frame(3u,r,true);
    nativeGroundDash(0x1400u,0xb27c0700u);
    broken.frame(4u,r,true);
    assert(broken.actor[0].autoReturn);
    assert(rl(P+0x0cu)==0x01600000u);
    checkGround();
    assert(rw(P+0x3eu)==7u);

    // Another 68k failure jumps directly onto the visible stage.
    resetNative();Controller warped;warped.frame(1u,r,true);
    flyOffscreen();warped.frame(2u,r,true);warped.frame(3u,r,true);
    nativeGroundDash(0x1400u,0x028a0000u); // 298 px world teleport
    warped.frame(4u,r,true);
    assert(warped.actor[0].autoReturn);
    assert(rl(P+0x0cu)==0x01600000u);
    checkGround();

    // Airborne state reasserted by native CPU is grounded again ONLY
    // while the original offscreen-return transaction is still pending.
    resetNative();Controller reasserted;reasserted.frame(1u,r,true);
    flyOffscreen();reasserted.frame(2u,r,true);
    reasserted.frame(3u,r,true);
    flyOffscreen();reasserted.frame(4u,r,true);
    checkGround();
    assert(reasserted.actor[0].airReturn.pending);
    for(unsigned f=5u;f<=8u;++f){
        flyOffscreen();
        reasserted.frame(f,r,true);
    }
    assert(reasserted.actor[0].airReturn.blockedUntilBackInView);
    assert(!reasserted.actor[0].airReturn.pending);

    // No native 1400/1600 dispatch: after 24 frames, stop retrying.
    resetNative();Controller missed;missed.frame(1u,r,true);
    flyOffscreen();missed.frame(2u,r,true);missed.frame(3u,r,true);
    for(unsigned frame=4u;frame<=29u;++frame){
        w(P+0x10u,472u);w(P+0xaau,0x0002u);
        missed.frame(frame,r,true);
    }
    assert(!missed.actor[0].airReturn.pending);
    assert(missed.actor[0].airReturn.blockedUntilBackInView);
    flyOffscreen();missed.frame(30u,r,true);
    assert(rw(P+0x10u)==144u); // cooldown blocks endless ground snap

    // Never interfere with hitstun, regular airborne attacks, missing
    // opponent, or a superjump still within the native combat view.
    for(unsigned k=0u;k<4u;++k){
        resetNative(k==2u?0x0020u:0x0008u,k!=2u);
        Controller protectedMove;protectedMove.frame(1u,r,true);
        flyOffscreen(k==1u?0x1800u:0x0800u,
                     k==3u?0x1c00u:0x3c00u,
                     k==3u?0x02a00000u:0x01600000u);
        if(k==0u)w(P+0x06u,0x000cu); // native hit reaction
        protectedMove.frame(2u,r,true);
        protectedMove.frame(3u,r,true);
        assert(rw(P+0x10u)==144u);
        assert(!protectedMove.actor[0].airReturn.pending);
        assert(!protectedMove.actor[0].autoReturn);
    }
    std::puts("87.233 grounded-before-68k-entry / no pop / no attack leaks: PASS");
}
