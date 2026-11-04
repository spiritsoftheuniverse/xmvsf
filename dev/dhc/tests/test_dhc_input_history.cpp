// g++ -std=c++11 -Wall -Wextra -pedantic dev/dhc/tests/test_dhc_input_history.cpp -o dhc_input_test
#include "../../stable/buildtree/src/intf/input/sdl/xmvsf_stage87_137_dhc_input_history.h"
#include <cassert>
using namespace XM87137Input;
int main() {
 State s;Sample q;
 assert(!s.sample(0,0,q));
 s.mapped(0,0,1,100);  // LEFT held
 s.mapped(0,0x80,1,100); // LP pressed
 s.mapped(0,0x80,1,100); // multiple polls: no duplicate new press
 assert(s.sample(0,0,q) && q.frame==100u && q.direction==1u);
 assert(q.buttons==1u && q.pressed==1u);
 assert(!s.sample(0,1,q));
 s.mapped(0,0,1,101);
 s.mapped(0,0x80,1,101);
 assert(s.sample(0,0,q) && q.frame==101u && q.pressed==0u);
 assert(q.buttons==1u && q.direction==1u);
 s.mapped(0,0x80,0,102);
 assert(s.sample(0,0,q) && q.buttons==0u && q.pressed==0u);
 s.mapped(0,0,0,103);
 s.mapped(0,1,1,103); // RIGHT held
 s.mapped(0,0x85,1,103); // HK pressed
 assert(s.sample(0,0,q) && q.direction==2u);
 assert(q.buttons==(1u<<5) && q.pressed==(1u<<5));
 // Per-side isolation; background players cannot consume P1's history.
 s.mapped(1,2,1,103);
 assert(s.sample(1,0,q) && q.direction==4u);
 assert(s.sample(0,0,q) && q.direction==2u);
 // Coin/Start must never participate in reserve super motion grammar.
 s.mapped(0,0x86,1,104);
 s.mapped(0,0x87,1,104);
 assert(s.sample(0,0,q) && q.frame==103u);
 // Time rewind (including savestate reload) resets old command queues.
 s.mapped(0,0,1,90);
 assert(s.sample(0,0,q) && q.frame==90u);
 assert(!s.sample(0,1,q));
 // Bounded ring has exactly 48 samples.
 for(unsigned f=91;f<151;++f)s.mapped(0,0,0,f);
 assert(s.player[0].count==CAPACITY);
 assert(s.sample(0,0,q) && q.frame==150u);
 assert(s.sample(0,CAPACITY-1,q) && q.frame==103u);
 assert(!s.sample(0,CAPACITY,q));
 return 0;
}
