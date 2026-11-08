// TEST 87.232: source-backed 68k FS automatic offscreen-entry repair.
// Evidence: native PPC 0109B4 corrupts X, PPC 01284A sets 1400;
// frame 6570 was a genuine moving 37f native dash wrongly watchdog-cut.
// RAM-only verification; actual Windows 68000 gameplay still needs a run.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_220_zero_dash_startup.h"
#include "xmvsf_stage87_232_native_pc_words.h"
using namespace XM87220Dash;
static std::uint8_t ram[65536]={};
static constexpr unsigned P=0x4000u,Q=0x4400u;
static void w(unsigned p,unsigned v){setWord(ram,p,v);}
static void l(unsigned p,std::uint32_t v){setLong(ram,p,v);}
static unsigned rw(unsigned p){return word(ram,p);}
static std::uint32_t rl(unsigned p){return longword(ram,p);}
static void base(bool opponent=true){
    std::memset(ram,0,sizeof(ram));
    ram[0x5001u]=8u;
    w(P,0x0101u);w(P+0x52u,0x0000u);
    w(P+0x06u,0u);w(P+0x08u,0u);
    w(P+0xa0u,0u);w(P+0xaau,0u);
    w(P+0x254u,472u);
    w(P+0x3eu,6u);w(P+0x42u,0x0032u);
    w(P+0x60u,0u);
    l(P+0x0cu,0x02b90000u);l(P+0x14u,0u);
    l(P+0x34u,0x000ac000u);l(P+0x38u,0x000ac010u);
    if(opponent){
        w(Q,0x0101u);w(Q+0x52u,0x0008u);
        l(Q+0x0cu,0x03200000u); // native world X=800
        w(Q+0x254u,472u);
    }
}
static void dash(std::uint32_t x=0x02b90000u,unsigned timer=6u,
                 std::uint32_t vx=0u){
    w(P+0xa0u,0x1400u);
    w(P+0x3eu,timer);l(P+0x0cu,x);
    l(P+0x14u,vx);
    l(P+0x34u,0x000a0502u);l(P+0x38u,0x000a0512u);
}
int main(){
    // Native 37+ frame dash can be travelling with nonzero velocity.
    // The old age-only watchdog killed this at frame 6570.
    base();
    Controller moving;
    moving.frame(1u,ram,true);
    for(unsigned frame=2u;frame<74u;++frame){
        dash(0x02b90000u+(frame-2u)*0x00010000u,
             frame&1u?5u:6u,0x00010000u);
        moving.frame(frame,ram,true);
        assert(rw(P+0xa0u)==0x1400u);
        assert(!moving.actor[0].recoveryActive);
    }
    // The zero-wait modification still applies to voluntary native dashes.
    base();Controller voluntary;voluntary.frame(1u,ram,true);
    dash(0x02b90000u,9u,0u);
    voluntary.frame(2u,ram,true);
    assert(rw(P+0xa0u)==0x1400u && rw(P+0x3eu)==1u);
    // Native source corruption: X jumps to B27C0700 before 1400 mode.
    base();Controller entry;
    w(P+0x210u,0x038fu);
    entry.frame(1u,ram,true);
    assert(entry.actor[0].safeValid &&
           entry.actor[0].neutralSnapshotValid);
    dash(0xb27c0700u,6u,0u);
    w(P+0x10u,100u);w(P+0xaa,0x0102u);
    l(P+0x1cu,0x00140000u); // falling out of the sky
    entry.frame(2u,ram,true);
    assert(entry.actor[0].autoReturn);
    assert(entry.actor[0].returnAnchorValid);
    // Opponent at 800px; entry is 448px left, fully outside CPS2 view.
    assert(rl(P+0x0cu)==0x01600000u);
    assert(rw(P+0x10u)==472u);
    assert(rw(P+0xaau)==0x0002u); // clear only air byte
    assert(rl(P+0x1cu)==0u);
    assert(rw(P+0xa0u)==0x1400u); // original native dash
    assert(rl(P+0x34u)==0x000a0502u); // native script intact
    assert(rw(P+0x3eu)==6u); // original entry delay preserved
    assert(rw(P+0x210u)==0x038fu);

    // On the next ORIGINAL native update, entry still owns its staging
    // timer; no voluntary zero-startup injection applies.
    l(P+0x0cu,0x01600000u);
    w(P+0x3eu,7u);
    entry.frame(3u,ram,true);
    assert(entry.actor[0].autoReturn);
    assert(rw(P+0x3eu)==7u);
    // Original 68000 may reassert its broken X. Bound retries: never
    // silently generate a fake perpetual host-controlled dash.
    for(unsigned frame=4u;frame<=7u;++frame){
        dash(0xb27c0700u,6u,0u);
        w(P+0x10u,100u);w(P+0xaau,0x0102u);
        entry.frame(frame,ram,true);
        assert(rl(P+0x0cu)==0x01600000u);
        assert(rw(P+0x10u)==472u);
        assert((rw(P+0xaau)>>8u)==0u);
        assert(rw(P+0x210u)==0x038fu);
    }
    dash(0xb27c0700u,6u,0u);
    entry.frame(8u,ram,true);
    assert(!entry.actor[0].autoReturn);
    assert(entry.actor[0].recoveryActive);
    assert(rl(P+0x0cu)==0x01600000u);
    assert(rw(P+0x10u)==472u);
    // No established opponent means no fabricated stage anchor. Original
    // ROM-native neutral snapshot remains the conservative fallback.
    base(false);Controller isolated;
    isolated.frame(1u,ram,true);
    dash(0xb27c0700u,6u,0u);
    w(P+0x10u,100u);
    isolated.frame(2u,ram,true);
    assert(!isolated.actor[0].autoReturn);
    assert(isolated.actor[0].recoveryActive);
    assert(rl(P+0x0cu)==0x02b90000u);
    assert(rw(P+0x10u)==472u);

    // Read 68k opcodes using actual decrypted host-word-swapped layout.
    const std::uint8_t fakeCode[8]={0x34,0x12,0x78,0x56,
                                     0xbcu,0x9au,0xf0u,0xdeu};
    assert(XM87232NativePC::opcodeWord(fakeCode,8u,0u)==0x1234u);
    assert(XM87232NativePC::opcodeWord(fakeCode,8u,2u)==0x5678u);
    assert(XM87232NativePC::opcodeWord(fakeCode,8u,4u)==0x9abcu);
    assert(XM87232NativePC::opcodeWord(fakeCode,8u,7u)==0u);
    std::puts("87.232 native 68k FS ground-entry / moving dash / opcode trace: PASS");
}
