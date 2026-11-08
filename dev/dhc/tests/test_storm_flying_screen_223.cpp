// TEST 87.223: ROM-free Storm-native flying-screen mechanics, severe-index
// substitution, one airborne launch, and all-mode Storm voice suppression.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_223_storm_native_flight.h"
#include "xmvsf_stage87_86_storm_button_guard.h"
#include "xmvsf_stage87_127_storm_voice_mute.h"
static std::uint8_t r[65536]={};
static const unsigned V=0x4000u, A=0x4400u;
static void set(unsigned a,unsigned v){XM8786Flight::setWord(r,a,v);}
static unsigned get(unsigned a){return XM8786Flight::wordAt(r,a);}
static void put32(unsigned a,std::uint32_t v){XM8786Flight::setFixed(r,a,v);}
static std::uint32_t get32(unsigned a){return XM8786Flight::rawFixed(r,a);}
int main(){
    using namespace XM87223Flight;
    assert(Tracker::motionFromStormHP(0x1c18u,0x0230u,0x000cu,2u,0x0010aa00u));
    assert(!Tracker::motionFromStormHP(0x1c16u,0x0220u,0x000cu,2u,0x0010aa00u));
    assert(!Tracker::motionFromStormHP(0x1c18u,0x0230u,0x000cu,2u,0x00050000u));
    Tracker model;
    assert(model.action()==0x1c18u && model.index()==0x0230u);
    assert(model.speedX()==0x0010aa00u && model.speedY()==0x0000b000u);
    model.observed.valid=true;
    model.observed.action=0x1c18u;model.observed.profile=0x0230u;
    model.observed.vx=0x00102000u;model.observed.vy=0x0000c000u;
    assert(model.action()==0x1c18u && model.index()==0x0230u);
    assert(model.speedX()==0x00102000u && model.speedY()==0x0000c000u);

    XM8786Flight::setByte(r,0x5000u,8u);
    set(A+0x52u,0x0004u);set(A+0x40u,0x0022u); // Storm attacker
    set(V+0x52u,0x0002u);
    set(A+0x10u,140u);set(V+0x10u,180u); // airborne
    set(V+0x254u,472u);set(V+0xaau,0x0100u);
    put32(A+0x0cu,0x01000000u);
    put32(V+0x0cu,0x02000000u);
    put32(V+0x14u,0x00070000u);
    put32(V+0x1cu,0u);
    set(V+0x06u,0x000cu);
    set(V+0x08u,2u);
    set(V+0x60u,0x1c14u);
    set(V+0xe0u,0x0210u);
    set(A+0x60u,0u);
    XM8786Flight::Actor actor;
    actor.evaluate(r,V,A,true,true,0u); // native up-launch evidence
    assert(actor.nativeLaunched);
    set(A+0x60u,0x1a00u); // first light connects
    set(V+0x08u,0u);set(V+0x60u,0x1c16u);set(V+0xe0u,0x0220u);
    const auto first=actor.evaluate(r,V,A,true,true,0u);
    assert(first.hitConfirmed && !first.heavyFollowup);
    assert(get(V+0x60u)==0x1c16u);
    set(V+0x08u,2u);
    actor.evaluate(r,V,A,true,true,0u);
    set(A+0x60u,0x1a04u); // new Storm HK/HP after confirmed light
    set(V+0x08u,0u);
    const auto next=actor.evaluate(r,V,A,true,true,4u); // HP physical edge
    assert(next.hitConfirmed && next.hadConnectedAirHit);
    assert(actor.pendingSevere);
    set(V+0x08u,2u); // native response has settled
    const auto converted=actor.evaluate(r,V,A,true,true,0u);
    assert(converted.launch && converted.velocityWritten && converted.stateWritten);
    assert(get(V+0x60u)==0x1c18u && get(V+0xe0u)==0x0230u);
    assert(get32(V+0x14u)==0x0010aa00u);
    assert(get32(V+0x1cu)==0x0000b000u);
    assert(actor.stormFlightUsedUntilLanding);
    // Anti-infinite latch must survive additional airborne reaction changes.
    set(A+0x60u,0x1a00u);
    set(V+0x08u,0u);
    actor.evaluate(r,V,A,true,true,0u);
    set(V+0x08u,2u);
    actor.evaluate(r,V,A,true,true,0u);
    assert(actor.stormFlightUsedUntilLanding);
    // Re-arm only after the original ROM reports ground contact.
    set(V+0x10u,472u);set(V+0xaau,0u);
    actor.evaluate(r,V,A,true,true,0u);
    assert(!actor.stormFlightUsedUntilLanding);

    // QSound channel-12 / sample 6508 is filtered without consulting
    // XM8755_TOURNAMENT_MODE. Other samples remain untouched.
    set(A+0x60u,0x1804u);
    set(A+0x10u,140u);
    XM87127Voice::Filter voice;
    voice.initialized=true;voice.enabled=true;
    voice.observe(10u,r,true);
    assert(voice.transform(10u,0x61u,0x6508u,r)==0x6508u);
    assert(voice.transform(10u,0x66u,0x1183u,r)==0u);
    assert(voice.transform(11u,0x61u,0x8000u,r)==0x8000u);
    assert(voice.transform(11u,0x66u,0x1183u,r)==0x1183u);
    // A delayed Z80 sample still matches the same Storm attack.
    set(A+0x60u,0u);
    assert(voice.transform(12u,0x61u,0x6508u,r)==0x6508u);
    assert(voice.transform(12u,0x66u,0x1183u,r)==0u);
    voice.reset();
    set(A+0x52u,0x0002u);
    assert(voice.transform(13u,0x61u,0x6508u,r)==0x6508u);
    assert(voice.transform(13u,0x66u,0x1183u,r)==0x1183u);

    std::puts("XMVSF 87.223 Storm-native FS / no synthetic OTG rule / voice: PASS");
    return 0;
}
