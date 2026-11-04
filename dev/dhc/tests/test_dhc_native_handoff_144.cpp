// 87.144: isolated test of gameplay-affecting DHC native-script attempt.
// This synthetic ROM recreates only six pointer-table cells; never embeds
// proprietary ROM bytes. Ordinary tag and non-DHC frames must be inert.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h"
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdlib>
using namespace XM87144;
static void romW(std::vector<std::uint8_t>& rom,unsigned at,unsigned v){
    rom[at]=std::uint8_t(v);rom[at+1]=std::uint8_t(v>>8);
}
static void romL(std::vector<std::uint8_t>& rom,unsigned at,unsigned v){
    romW(rom,at,v>>16);romW(rom,at+2,v);
}
static void entry(std::vector<std::uint8_t>& rom,unsigned fighter,
                  unsigned action,unsigned sub,unsigned script,unsigned duration=3u){
    const unsigned fighterTable=fighter==0x12u?0xCFAD4u:0xD378Eu;
    unsigned section=0;
    if(fighter==0x12u && action==0x2Eu)section=0xCFCA2u;
    if(fighter==0x12u && action==0x32u)section=0xCFCCAu;
    if(fighter==0x14u && action==0x2Au)section=0xD3910u;
    if(fighter==0x14u && action==0x2Cu)section=0xD3922u;
    if(fighter==0x14u && action==0x2Eu)section=0xD3966u;
    assert(section && section>fighterTable && script>section);
    romL(rom,0x8A946u+fighter*2u,fighterTable);
    romW(rom,fighterTable+action,section-fighterTable);
    romW(rom,section+sub,script-section);
    romW(rom,script,0u);
    romW(rom,script+2, duration);
    romL(rom,script+4,0x0026A5C2u);
    romL(rom,script+8,0x30008000u);
    romW(rom,script+12,0u);romW(rom,script+14,0u);
    romL(rom,0x8AA04u+fighter*2u,fighter==0x12u?0x9CA18u:0x9D0BAu);
}
int main(){
    std::vector<std::uint8_t> rom(0xE1000u,0),ram(65536u,0);
    entry(rom,0x12u,0x2Eu,0x08u,0xD3070u);
    entry(rom,0x12u,0x32u,0x00u,0xD1DACu,4u);
    entry(rom,0x14u,0x2Au,0x08u,0xD567Eu);
    entry(rom,0x14u,0x2Cu,0x00u,0xD5C46u);
    entry(rom,0x14u,0x2Cu,0x32u,0xD60AAu);
    entry(rom,0x14u,0x2Cu,0x04u,0xD5E2Eu);
    entry(rom,0x14u,0x2Eu,0x00u,0xD6722u);
    wb(ram.data(),0x5000u,8u);ww(ram.data(),0x4052u,0x12u);
    ww(ram.data(),0x4852u,0x14u);
    ww(ram.data(),0x4452u,0x20u);ww(ram.data(),0x4C52u,0x22u);
    ww(ram.data(),0x4000u+0x0Cu,400u);
    ww(ram.data(),0x4400u+0x0Cu,600u);
    ww(ram.data(),0x4000u+0x10u,472u);
    ww(ram.data(),0x4000u+0x60u,0x2E08u);
    ww(ram.data(),0x4000u+0xA0u,0x2A28u);
    ww(ram.data(),0x4214u,3u);
    const auto& q=XM87143Queue::state();
    (void)q;
    assert(XM87143Queue::state().prepare(ram.data(),rom.data(),
            unsigned(rom.size()),100u));
    assert(XM87143Queue::state().select(0u,0x14u,XM87143Queue::QCF_PP,101u));
    assert(setenv("XM87144_DHC_NATIVE_TEST","1",1)==0);
    State s;
    const auto original=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),100u,0u);
    assert(ram==original && s.phase[0]==IDLE);
    // 87.144 had the native mode backwards: super is 2A28, never 2A02.
    // A wrongly labeled +A0 state must NOT trigger gameplay writes.
    ww(ram.data(),0x40A0u,0x2A02u);
    const auto invalidMode=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),101u,1u);
    assert(s.phase[0]==IDLE && ram==invalidMode);
    ww(ram.data(),0x40A0u,0x2A28u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),102u,1u);
    assert(s.phase[0]==WAIT_TAG_CONTROLLER);
    assert(rw(ram.data(),0x4060u)==0x2E08u); // no scripted sprite hijack
    assert(rw(ram.data(),0x40A0u)==0x002Au); // real pre-tag: ROM 033762 will produce 2A00
    assert(rb(ram.data(),0x4256u)==0x08u);
    assert(rb(ram.data(),0x4259u)==0x08u);
    assert(rw(ram.data(),0x4214u)==3u); // no speculative meter debit
    assert(rw(ram.data(),0x4452u)==0x20u); // P2 untouched
    assert(XM87143Queue::state().selected[0]==nullptr);
    // Stage 87.146 captured exactly this ROM-owned controller
    // acceptance: +256 becomes 0, A0 becomes 2A00, but action stays
    // HADOKEN 2E08. The DHC-only tag bridge must convert the native
    // controller ACCEPTANCE into outgoing action 3200 and timer 001E.
    wb(ram.data(),0x4256u,0u);
    wb(ram.data(),0x4259u,8u);
    ww(ram.data(),0x40A0u,0x2A00u);
    assert(rw(ram.data(),0x4060u)==0x2E08u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),103u,1u);
    assert(s.phase[0]==WAIT_NATIVE_SWAP);
    assert(rw(ram.data(),0x4060u)==0x3200u);
    assert(rl(ram.data(),0x4034u)==0xD1DACu);
    assert(rw(ram.data(),0x40A0u)==0x2A00u);
    assert(rw(ram.data(),0x404Eu)==0x001Eu);
    assert(rb(ram.data(),0x4259u)==0u);
    // Verified from decrypted xmvsfu: Ryu +D6=14 dispatches via
    // 0337BA -> 0341F6 -> 011B00/011B1E native outgoing tag logic.
    // Without this the animation reaches D1DEC but stays forever.
    assert(rb(ram.data(),0x40D6u)==0x14u);
    assert(rw(ram.data(),0x4008u)==2u);
    assert(rw(ram.data(),0x4214u)==3u);
    // Real ROM must swap fighter IDs. Do not synthesize it in the engine.
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),104u,1u);
    assert(s.phase[0]==WAIT_NATIVE_SWAP);
    ww(ram.data(),0x4052u,0x14u);
    ww(ram.data(),0x4852u,0x12u);
    ww(ram.data(),0x4060u,0x2A0Cu);
    wb(ram.data(),0x40D6u,0x10u); // Ken ROM incoming-kick controller
    ww(ram.data(),0x4254u,472u);
    ww(ram.data(),0x4010u,430u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),115u,1u);
    assert(s.phase[0]==WAIT_LANDING);
    // 87.150: at y=430 (42 units ABOVE floor) the old 8-pixel
    // proximity trigger never fired. Immediately suppress Ken's
    // native incoming attack (D6=10/2A0C) after ROM fighter swap.
    ww(ram.data(),0x4060u,0x2A0Cu);
    ww(ram.data(),0x4010u,430u); // intentionally nowhere near landing
    ww(ram.data(),0x4070u,0x51u); // 2A0C native kick hitbox active
    wb(ram.data(),0x40A4u,4u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),116u,1u);
    assert(s.phase[0]==WAIT_KEN_EXIT);
    assert(rb(ram.data(),0x40D6u)==0x12u); // Ken native exit handler
    assert(rb(ram.data(),0x40A4u)==0u); // kick latch disabled
    assert(rw(ram.data(),0x4060u)==0x2A08u); // first-frame suppression
    assert(rl(ram.data(),0x4034u)==0xD567Eu); // ORIGINAL ROM exit script
    assert(rw(ram.data(),0x4070u)==0u); // no native attack box
    assert(rw(ram.data(),0x4008u)==2u); // native tag-exit animation step
    assert(rw(ram.data(),0x4010u)==472u);
    assert(rw(ram.data(),0x400Cu)==552u); // opponent X600-48
    assert(rb(ram.data(),0x404Bu)==1u);
    assert(rw(ram.data(),0x440Cu)==600u); // P2 never touched
    assert(rw(ram.data(),0x4214u)==3u); // no additional stock debit yet
    // The next real native ROM frame retains nonattacking 2A08.
    // No second tag kick may be treated as a valid exit.
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),117u,1u);
    assert(s.phase[0]==WAIT_KEN_EXIT);
    assert(s.kenExitObserved[0]);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),118u,1u);
    // DHC directly cancels the verified non-attacking exit into
    // the queued Ken native super; no 67-frame natural 2A08 wait.
    assert(s.phase[0]==SUPER_STARTED);
    assert(rb(ram.data(),0x40D6u)==0x0Au); // Ken 2C ROM handler
    assert(rw(ram.data(),0x40A0u)==0x2A28u);
    assert(rw(ram.data(),0x4008u)==0u);
    assert(rb(ram.data(),0x4259u)==0u);
    assert(rw(ram.data(),0x4214u)==3u);
    ww(ram.data(),0x4060u,0x2C32u); // ROM selects queued super
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),119u,1u);
    assert(s.phase[0]==SUPER_STARTED);
    assert(s.chainUsed[0] && s.kenSuperConfirmed[0]);
    assert(XM87143Queue::state().select(0u,0x12u,XM87143Queue::QCF_PP,120u));
    const auto after=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),120u,1u);
    assert(ram==after);
    // Different round resets only bookkeeping, not fighters or ROM memory.
    wb(ram.data(),0x5000u,0u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),300u,0u);
    assert(!s.chainUsed[0] && s.phase[0]==IDLE);
    // Failure must NOT lock the entire match. Requiring a fresh selected
    // command prevents accidental auto-retries from the same held chord.
    XM87143Queue::state().reset();
    std::vector<std::uint8_t> r2(65536u,0);
    wb(r2.data(),0x5000u,8u);
    ww(r2.data(),0x4052u,0x12u);ww(r2.data(),0x4852u,0x14u);
    ww(r2.data(),0x4452u,0x20u);ww(r2.data(),0x4C52u,0x22u);
    ww(r2.data(),0x4060u,0x2E08u);
    ww(r2.data(),0x40A0u,0x2A28u);
    assert(XM87143Queue::state().prepare(r2.data(),rom.data(),
        unsigned(rom.size()),400u));
    State trial;
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),400u,0u);
    assert(XM87143Queue::state().select(0u,0x14u,
        XM87143Queue::QCF_PP,401u));
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),401u,1u);
    assert(trial.phase[0]==WAIT_TAG_CONTROLLER);
    // ROM fails to accept: keep original fighter identity and native action.
    for(unsigned t=402u;t<=414u;++t)
        trial.frame(r2.data(),rom.data(),unsigned(rom.size()),t,1u);
    assert(trial.phase[0]==IDLE && !trial.chainUsed[0]);
    assert(rw(r2.data(),0x40A0u)==0x2A28u);
    assert(rb(r2.data(),0x4256u)==0u);
    assert(XM87143Queue::state().selected[0]==nullptr);
    // New input after cooldown (not the same pending command) can retry.
    assert(XM87143Queue::state().select(0u,0x14u,
        XM87143Queue::QCF_KK,425u));
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),425u,1u);
    assert(trial.phase[0]==WAIT_TAG_CONTROLLER);
    // 87.147 regression: ROM accepts controller 08, but fails to keep
    // tag action 3200. Do not lock this match after the reverted super.
    ww(r2.data(),0x40A0u,0x2A00u);
    wb(r2.data(),0x4256u,0u);
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),426u,1u);
    assert(trial.phase[0]==WAIT_NATIVE_SWAP);
    assert(rw(r2.data(),0x4060u)==0x3200u);
    assert(rw(r2.data(),0x404Eu)==0x001Eu);
    assert(rb(r2.data(),0x40D6u)==0x14u);
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),427u,1u);
    ww(r2.data(),0x4060u,0x2E08u); // 68000 reasserts Ryu super
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),429u,1u);
    assert(trial.phase[0]==IDLE && !trial.chainUsed[0]);
    ww(r2.data(),0x40A0u,0x2A28u);
    assert(XM87143Queue::state().select(0u,0x14u,
        XM87143Queue::QCF_PP,436u));
    trial.frame(r2.data(),rom.data(),unsigned(rom.size()),436u,1u);
    assert(trial.phase[0]==WAIT_TAG_CONTROLLER);
    return 0;
}
