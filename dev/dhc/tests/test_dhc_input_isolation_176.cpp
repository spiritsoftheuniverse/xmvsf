// ROM-free C++11 regression: queued DHC physical chord must be isolated.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h"
using XM87176Input::neutralize;
int main(){
  // Captured 87.175: host queues at 1865, ROM still sees 2P at 1866.
  assert(neutralize(2,0x12,0x14,true,true,1866,0));
  assert(neutralize(3,0x12,0x14,true,true,1867,0));
  assert(neutralize(3,0x14,0x12,true,true,1880,0));
  assert(neutralize(4,0x14,0x12,true,true,1881,0));
  assert(neutralize(5,0x14,0x12,true,true,1882,0));
  assert(neutralize(8,0x14,0x12,true,true,1883,1884));
  // Original Ken QCF+PP stream becomes unshielded at inputBegin.
  assert(!neutralize(8,0x14,0x12,true,true,1884,1884));
  // No normal-tag, proof-mode, other-roster, or failed-chain mutation.
  for(unsigned p=0;p<=10;p++){
    assert(!neutralize(p,0x14,0x12,false,true,2000,2002));
    assert(!neutralize(p,0x14,0x12,true,false,2000,2002));
    assert(!neutralize(p,0x14,0x02,true,true,2000,2002));
    assert(!neutralize(p,0x12,0x14,true,true,2000,2002)||p==2||p==3);
  }
  assert(!neutralize(0,0x12,0x14,true,true,1865,0));
  assert(!neutralize(6,0x14,0x12,true,true,1889,1884));
  assert(!neutralize(9,0x12,0x14,true,true,1865,0));
  std::puts("PASS 87.176 DHC input isolation / native Ken super preserved");
  return 0;
}
