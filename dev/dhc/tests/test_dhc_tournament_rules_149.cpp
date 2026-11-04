// Tournament display flag is NOT sufficient: all native hooks and the
// latest Tournament-only flight switch must be enabled and installed.
// This is a read-only audit; classic/failed signatures never get bypassed.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_149_tournament_rules_audit.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
int main(){
    using namespace XM87149Rules;
    Status classic;
    assert(classic.ready() && classic.mask()==0u);
    Status enabled;
    enabled.tournament=true;
    enabled.launchFlight=true;
    enabled.hit95=true;
    enabled.hand96=true;
    enabled.ground99=true;
    assert(!enabled.ready()); // menu ON but ROM patch not installed
    enabled.installed95=true;
    enabled.installed96=true;
    assert(!enabled.ready()); // grounded defense still unavailable
    enabled.installed99=true;
    assert(enabled.ready());
    enabled.launchFlight=false;
    assert(!enabled.ready()); // Tournament flying-screen rules lost
    enabled.launchFlight=true;
    assert(enabled.ready());
    enabled.tournament=false;
    enabled.installed95=false;enabled.installed96=false;
    enabled.installed99=false;
    assert(enabled.ready()); // Classic must remain native
    assert(setenv("XM8786_STORM_BUTTON_TEST","1",1)==0);
    assert(setenv("XM8795_NATIVE_HIT_TEST","1",1)==0);
    assert(setenv("XM8796_DIZZY_HANDOFF_TEST","1",1)==0);
    assert(setenv("XM8799_GROUND_DEFENSE_TEST","1",1)==0);
    assert(flag("XM8786_STORM_BUTTON_TEST"));
    assert(flag("XM8795_NATIVE_HIT_TEST"));
    assert(flag("XM8796_DIZZY_HANDOFF_TEST"));
    assert(flag("XM8799_GROUND_DEFENSE_TEST"));
    assert(unsetenv("XM8799_GROUND_DEFENSE_TEST")==0);
    assert(!flag("XM8799_GROUND_DEFENSE_TEST"));
    Audit a;
    a.frame(100,"othergame",true,true,true,true);
    assert(!a.haveLast); // no claim about other drivers
    a.frame(101,"xmvsfu",false,false,false,false);
    assert(a.haveLast && a.changes==1u);
    a.frame(102,"xmvsfu",false,false,false,false);
    assert(a.changes==1u); // no duplicate log each frame
    a.frame(103,"xmvsfu",true,true,true,false);
    assert(a.changes==2u); // incomplete Tournament status logged
    return 0;
}
