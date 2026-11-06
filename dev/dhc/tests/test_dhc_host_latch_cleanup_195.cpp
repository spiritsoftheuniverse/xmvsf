// 87.193(2) frame 5703: wrong native 2C26 left host-owned 259=08.
// Clean that latch without modifying any original ROM-selected action.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_195_tag_latch_guard.h"
int main(){
    using XM87195Latch::shouldReleaseWrongAction;
    // Exact observed failure from 87.193(2).zip: point Ryu,
    // reserve Ken, original Ryu super 2E02, next action 2C26.
    assert(shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x2C26u,0x2E02u,0u,8u,0u));
    // Native 2E continuation 2E0C still qualifies if legacy
    // stale-latch recovery did not already clear the host 08.
    assert(shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x2E0Cu,0x2E02u,0u,8u,0u));
    // Do not interfere with genuine original 3200 tag, an actor
    // that never consumed controller8, or any valid new transaction.
    assert(!shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x3200u,0x2E02u,0u,8u,0u));
    assert(!shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x2E02u,0x2E02u,0u,8u,0u));
    assert(!shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x2C26u,0x2E02u,8u,8u,0u));
    assert(!shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x2C26u,0x2E02u,0u,0u,0u));
    assert(!shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x12u,0x14u,0x2C26u,0x2E02u,0u,8u,2u));
    assert(!shouldReleaseWrongAction(2u,0x14u,0x12u,
        0x12u,0x14u,0x2C26u,0x2E02u,0u,8u,0u));
    assert(!shouldReleaseWrongAction(2u,0x12u,0x14u,
        0x00u,0x14u,0x2C26u,0x2E02u,0u,8u,0u));
    assert(!shouldReleaseWrongAction(8u,0x12u,0x14u,
        0x12u,0x14u,0x2C26u,0x2E02u,0u,8u,0u));
    std::puts("PASS 87.195 clean host-owned 259=08 after native 2C26 wrong action, never touch real tag");
}
