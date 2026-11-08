// ROM-free TEST 87.220: actor-owned dash startup countdown, never motion.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_220_zero_dash_startup.h"
using namespace XM87220Dash;
static std::uint8_t memory[65536] = {};
static void prepare(unsigned base,unsigned fighter,unsigned mode,unsigned timer){
    setWord(memory,base,0x0101u);
    setWord(memory,base+0x52u,fighter);
    setWord(memory,base+0x06u,0u);
    setWord(memory,base+0xa0u,mode);
    setWord(memory,base+0x3eu,timer);
}
static void setLong(unsigned base,unsigned offset,std::uint32_t value){
    setWord(memory,base+offset,value>>16u);
    setWord(memory,base+offset+2u,value);
}
int main(){
    Controller c;memory[0x5001u]=8u; // guest 0x5000, word-swapped
    prepare(0x4000u,0x00u,0x1400u,13u); // Wolverine forward
    prepare(0x4400u,0x22u,0x1600u,9u);  // Cammy reverse
    c.frame(1u,memory,true);
    assert(word(memory,0x403eu)==1u);
    assert(word(memory,0x443eu)==1u);
    setWord(memory,0x403eu,7u); // another wind-up animation frame
    c.frame(2u,memory,true);
    assert(word(memory,0x403eu)==1u);
    setLong(0x4000u,0x0cu,0x00110000u); // actual native X begins
    setWord(memory,0x403eu,19u); // active dash duration preserved
    c.frame(3u,memory,true);
    assert(word(memory,0x403eu)==19u);
    setWord(memory,0x403eu,15u); // action still dash, do not shorten
    c.frame(4u,memory,true);
    assert(word(memory,0x403eu)==15u);
    prepare(0x4000u,0x02u,0x3c00u,21u); // Cyclops air dash
    c.frame(5u,memory,true);assert(word(memory,0x403eu)==1u);
    setWord(memory,0x403eu,8u); // air movement unaffected after entry
    c.frame(6u,memory,true);assert(word(memory,0x403eu)==8u);
    prepare(0x4000u,0x12u,0x1400u,0x7fffu); // ROM held sentinel
    c.frame(7u,memory,true);assert(word(memory,0x403eu)==0x7fffu);
    prepare(0x4000u,0x12u,0x1400u,0u); // 0 is not a safe countdown
    c.frame(8u,memory,true);assert(word(memory,0x403eu)==0u);
    prepare(0x4000u,0x12u,0x1400u,12u);
    setWord(memory,0x4006u,0x000cu); // hitstun: never touch
    c.frame(9u,memory,true);assert(word(memory,0x403eu)==12u);
    setWord(memory,0x4006u,0u);
    prepare(0x4000u,0x12u,0x2800u,12u); // attack
    c.frame(10u,memory,true);assert(word(memory,0x403eu)==12u);
    prepare(0x4000u,0x14u,0x1400u,12u);
    c.frame(11u,memory,false);assert(word(memory,0x403eu)==12u); // other title
    c.frame(12u,memory,true);assert(word(memory,0x403eu)==1u);
    setWord(memory,0x403eu,9u);memory[0x5001u]=0u;
    c.frame(13u,memory,true);assert(word(memory,0x403eu)==9u); // menu
    memory[0x5001u]=8u;
    prepare(0x4000u,0x14u,0x1400u,9u);
    c.frame(14u,memory,true);assert(word(memory,0x403eu)==1u);
    // A player-entered dash cancel produces a new state and is never
    // overridden if no longer a native dash.
    prepare(0x4000u,0x14u,0x0000u,11u);
    c.frame(15u,memory,true);assert(word(memory,0x403eu)==11u);
    // No fighter lookup: all 19 native even-numbered roster IDs behave alike.
    for(unsigned id=0;id<=0x24u;id+=2u){
        prepare(0x4000u,id,0x1400u,10u+id);
        c.frame(20u+id,memory,true);
        assert(word(memory,0x403eu)==1u);
    }
    std::puts("XMVSF 87.220 zero-delay dash: ROM-free source regression passed");
}
