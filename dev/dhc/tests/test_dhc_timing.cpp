// Standalone: g++ -std=c++11 -Wall -Wextra -pedantic tests/test_dhc_timing.cpp -o /tmp/dhc_test && /tmp/dhc_test
#include "../xmvsf_stage87_137_dhc_timing.h"
#include <cassert>
using namespace XM87137Timing;
static Command good() {
 Command c;c.combat=true;c.outgoingSuperVerified=true;
 c.reserveReady=true;c.commandIsVerifiedReserveSuper=true;
 c.meterForOneMoreSuper=true;c.reserveId=0x21;c.superId=2;
 return c;
}
static Signal frozen() {
 Signal s;s.combat=true;s.worldHeld=true;s.darkScenePresented=true;
 return s;
}
int main() {
 Coordinator x;
 assert(!x.request(good()));
 assert(x.observeFirstSuper(true,true,false));
 Command c=good();c.meterForOneMoreSuper=false;
 assert(!x.request(c));assert(x.phase==FIRST_SUPER);
 c=good();assert(x.request(c));
 assert(!x.request(c)); // duplicate input cannot request twice
 assert(!x.nativeCommit(true,true,false)); // missing debit aborts
 assert(x.phase==FAILED);assert(x.tick(frozen()).rollbackRequired);
 x.rollbackComplete();assert(x.phase==IDLE);
 assert(x.observeFirstSuper(true,true,false));
 assert(x.request(good()));
 assert(x.nativeCommit(true,true,true));
 assert(x.didDHC);
 Signal s=frozen();
 Result r=x.tick(s);
 assert(x.phase==TAG_ENTER && r.holdWorld && r.dimStage && r.showIncomingFighter && r.suppressIncomingPortrait);
 s.nativeTagComplete=true;
 r=x.tick(s);
 assert(x.phase==TAG_ENTER && !r.tickIncomingOnly); // not landed yet
 s.incomingGrounded=true;s.incomingAtTakeoverPosition=true;
 r=x.tick(s);
 assert(x.phase==STARTUP && r.tickIncomingOnly && r.showIncomingFighter);
 // Screen going dark is NOT enough to release.
 s.incomingSuperStarted=true;s.nativePortraitIntroBypassed=true;s.incomingStartupTick=true;
 for(unsigned i=0;i<50;++i){
    r=x.tick(s);assert(!r.releaseBeforeNextNativeFrame);
    assert(r.holdWorld&&r.dimStage&&r.showIncomingFighter&&r.suppressIncomingPortrait);
 }
 assert(x.startupTicks>=50);
 s.incomingStartupTick=false;s.firstActiveOnNextTick=true;
 r=x.tick(s);
 assert(r.releaseBeforeNextNativeFrame && x.phase==INCOMING);
 // Even though incoming super is now underway, another DHC cannot queue.
 assert(!x.request(good()));
 Signal active;active.combat=true;active.firstActiveNow=true;
 r=x.tick(active);assert(!r.rollbackRequired && x.phase==INCOMING);
 active.firstActiveNow=false;
 r=x.tick(active);assert(x.phase==INCOMING);
 active.incomingSuperEnded=true;
 x.tick(active);assert(x.phase==LOCKED);
 assert(!x.request(good()));
 x.newChainAfterNeutral(false);assert(x.phase==LOCKED);
 x.newChainAfterNeutral(true);assert(x.phase==IDLE);
 assert(x.observeFirstSuper(true,true,false));

 // Any opponent/world advancement under black requires transactional rollback.
 Coordinator y;y.observeFirstSuper(true,true,false);
 assert(y.request(good()) && y.nativeCommit(true,true,true));
 Signal leak=frozen();leak.foreignWorldAdvanced=true;
 r=y.tick(leak);assert(y.phase==FAILED && r.rollbackRequired);
 y.rollbackComplete();assert(y.phase==IDLE);

 // Failure to reach the active attack on the first visible frame is fatal.
 Coordinator z;z.observeFirstSuper(true,true,false);z.request(good());
 z.nativeCommit(true,true,true);
 s=frozen();s.nativeTagComplete=true;s.incomingGrounded=true;
 s.incomingAtTakeoverPosition=true;s.incomingSuperStarted=true;
 s.nativePortraitIntroBypassed=true;s.incomingStartupTick=true;z.tick(s);
 s.incomingStartupTick=false;s.firstActiveOnNextTick=true;
 assert(z.tick(s).releaseBeforeNextNativeFrame);
 Signal miss;miss.combat=true;miss.firstActiveNow=false;
 assert(z.tick(miss).rollbackRequired);
 assert(z.phase==FAILED);

 // A cinematic exit without one hidden startup tick cannot arm release.
 Coordinator w;w.observeFirstSuper(true,true,false);w.request(good());
 w.nativeCommit(true,true,true);
 s=frozen();s.nativeTagComplete=true;s.incomingGrounded=true;
 s.incomingAtTakeoverPosition=true;s.incomingSuperStarted=true;
 s.nativePortraitIntroBypassed=true;s.firstActiveOnNextTick=true;
 assert(w.tick(s).rollbackRequired);

 // A DHC portrait is a rendering defect: block the transition.
 Coordinator p;p.observeFirstSuper(true,true,false);
 assert(p.request(good()) && p.nativeCommit(true,true,true));
 Signal bad=frozen();bad.incomingPortraitVisible=true;
 r=p.tick(bad);
 assert(r.rollbackRequired && p.phase==FAILED);
 // Suppression of sprite art ALONE is insufficient; skip native portrait
 // animation/timer before super startup advances.
 Coordinator intro;intro.observeFirstSuper(true,true,false);
 assert(intro.request(good()) && intro.nativeCommit(true,true,true));
 Signal wrong=frozen();
 wrong.nativeTagComplete=true;wrong.incomingGrounded=true;
 wrong.incomingAtTakeoverPosition=true;wrong.incomingSuperStarted=true;
 assert(intro.tick(wrong).rollbackRequired);
 assert(intro.phase==FAILED);
 // Ordinary supers are unaffected by this DHC-only presentation gate.
 Coordinator ordinary;ordinary.observeFirstSuper(true,true,false);
 r=ordinary.tick(frozen());
 assert(!r.suppressIncomingPortrait && !r.dimStage);
 return 0;
}
