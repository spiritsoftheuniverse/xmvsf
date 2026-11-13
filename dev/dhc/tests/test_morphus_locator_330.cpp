// Independent C++11 read-only opcode locator unit test. NO native build gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_330_morphus_locator.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace XM87330Locator;
int main(){
  unsigned nsites=0u;const Site* s=sites(&nsites);
  assert(nsites>=20u);
  unsigned dc=0u,tc=0u;
  for(unsigned i=0;i<nsites;++i) {
    if(std::strcmp(s[i].symbol,"dhc_chain")==0)dc=i;
    if(std::strcmp(s[i].symbol,"incoming_hyper_commit")==0)tc=i;
  }
  assert(s[dc].euroAddress==0x13AA8u);
  assert(s[tc].euroAddress==0x11DB0u);
  std::uint8_t pat[32]={};
  const unsigned n=decode(s[dc].hex,pat,sizeof(pat));
  assert(n==6u);
  std::vector<std::uint8_t> data(0x15000u,0u);
  const unsigned moved=0x13AA8u+0x100u;
  for(unsigned i=0;i<n;++i)data[moved+i]=pat[i];
  Hits h=scan(data.data(),unsigned(data.size()),pat,n);
  assert(h.total==1u && h.address[0]==moved);
  assert(!atOriginal(data.data(),unsigned(data.size()),pat,n,0x13AA8u));
  assert(atOriginal(data.data(),unsigned(data.size()),pat,n,moved));
  for(unsigned i=0;i<n;++i)data[moved+0x20u+i]=pat[i];
  h=scan(data.data(),unsigned(data.size()),pat,n);
  assert(h.total==2u); // duplicate => not safe for relocation
  assert(decode("0c2e000400a",pat,sizeof(pat))==0u);
  assert(decode("0c2e00GG00a4",pat,sizeof(pat))==0u);
  assert(scan(nullptr,0u,pat,n).total==0u);
  std::puts("XMVSF 87.330 Morphus read-only locator PASS");
}
