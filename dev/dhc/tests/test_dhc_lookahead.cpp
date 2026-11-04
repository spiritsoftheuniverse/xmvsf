// g++ -std=c++11 -Wall -Wextra -pedantic tests/test_dhc_lookahead.cpp -o dhc_peek && ./dhc_peek
#include "../xmvsf_stage87_137_dhc_lookahead.h"
#include <cassert>
using namespace XM87137Lookahead;
struct Fake {
    unsigned tick=0,saved=0,moves=0,saves=0,restores=0;
    bool restoreWorks=true,previewStepWorks=true;
};
static bool sample(void* p,Attack* a){
    Fake& f=*static_cast<Fake*>(p);
    a->valid=true;a->correctFighter=true;a->ownSuper=true;
    a->superId=17;a->firstActive=f.tick>=7;
    return true;
}
static bool save(void* p){Fake& f=*static_cast<Fake*>(p);f.saved=f.tick;++f.saves;return true;}
static bool step(void* p){Fake& f=*static_cast<Fake*>(p);++f.tick;++f.moves;return f.previewStepWorks;}
static bool restore(void* p){Fake& f=*static_cast<Fake*>(p);++f.restores;if(f.restoreWorks)f.tick=f.saved;return f.restoreWorks;}
static bool verified(void* p){Fake& f=*static_cast<Fake*>(p);return f.tick==f.saved;}
int main(){
 Fake f;Hooks h;h.ctx=&f;h.save=save;h.advanceReleasedFrame=step;
 h.observe=sample;h.restore=restore;h.restorationVerified=verified;
 f.tick=5;Outcome a=preview(h,17);
 assert(a.decision==NOT_READY && a.restored && f.tick==5);
 assert(f.saves==1 && f.restores==1);
 f.tick=6;
 a=preview(h,17);
 assert(a.decision==READY_NEXT_FRAME && a.restored && f.tick==6);
 // The preview never consumes the attack's first frame: real frame 7 is active.
 assert(step(&f) && f.tick==7);
 // The next speculative probe is now too late, and must not arm DHC.
 a=preview(h,17);assert(a.decision==NO_PREVIEW && !a.simulated);
 // Wrong super identity cannot authorize release.
 f.tick=6;a=preview(h,99);assert(a.decision==NO_PREVIEW);
 // A failed emulation step MUST restore its original snapshot.
 f.previewStepWorks=false;a=preview(h,17);
 assert(a.decision==NO_PREVIEW && a.restored && f.tick==6);
 f.previewStepWorks=true;
 // A failed rollback is a hard error: NEVER release from corrupted state.
 f.restoreWorks=false;a=preview(h,17);
 assert(a.decision==ROLLBACK_FAILED && !a.restored);
 return 0;
}
