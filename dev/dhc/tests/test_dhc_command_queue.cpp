// g++ -std=c++11 -Wall -Wextra -pedantic dev/dhc/tests/test_dhc_command_queue.cpp -o dhc_command_test
#include "../xmvsf_stage87_137_dhc_command_queue.h"
#include <cassert>
using namespace XM87137Input;
using namespace XM87137Command;
static void qcfPP(History& h,unsigned first,bool rightFacing){
    const unsigned forward=rightFacing?1u:0u;
    h.mapped(3,true,first);             // down
    h.mapped(forward,true,first+1u);   // down-forward
    h.mapped(3,false,first+2u);        // forward
    h.mapped(0x80u,true,first+2u);    // LP
    h.mapped(0x81u,true,first+2u);    // MP
}
static Move verified(unsigned fighter,unsigned id){
 Move m;m.characterId=fighter;m.moveId=id;m.motion=QCF;
 m.mode=TWO_PUNCHES;m.nativeVerified=true;return m;
}
int main(){
 Registry reg;
 History h;Queue queue;
 assert(reg.count==0u);
 qcfPP(h,10u,true);
 assert(reg.resolve(h,0x20u,true)==0u); // empty registry never guesses supers
 Move unproved=verified(0x20u,1u);unproved.nativeVerified=false;
 assert(!reg.add(unproved));
 assert(reg.add(verified(0x20u,1u)));
 assert(reg.resolve(h,0x20u,true)==1u);
 assert(reg.resolve(h,0x22u,true)==0u); // wrong reserve fighter
 assert(reg.resolve(h,0x20u,false)==0u); // wrong facing
 assert(queue.poll(0u,true,0x20u,true,h,reg)==1u);
 assert(queue.poll(0u,true,0x20u,true,h,reg)==0u); // no repeated DHC
 assert(queue.poll(0u,false,0x20u,true,h,reg)==0u);
 queue.reset(0u);
 assert(queue.poll(0u,true,0x20u,true,h,reg)==1u);
 // Holding the chord during a later frame does NOT count as a new press.
 History held;
 qcfPP(held,100u,true);
 held.mapped(1,true,103u);held.mapped(0x80u,true,103u);
 held.mapped(0x81u,true,103u);
 assert(reg.resolve(held,0x20u,true)==0u);
 // Same QCF+PP command recognized relative to a left-facing reserve.
 History left;
 qcfPP(left,20u,false);
 assert(reg.resolve(left,0x20u,false)==1u);
 // Conflicting move mapping cannot choose a random super.
 Registry ambiguous;
 assert(ambiguous.add(verified(0x20u,1u)));
 assert(ambiguous.add(verified(0x20u,2u)));
 assert(ambiguous.resolve(h,0x20u,true)==0u);
 return 0;
}
