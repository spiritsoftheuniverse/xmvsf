// 87.332 standalone policy test, not a native build gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_332_morphus_dhc_context.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace XM87332Context;
int main(){
    const Site* input=siteFor("input");
    const Site* chain=siteFor("dhc_chain");
    const Site* commit=siteFor("incoming_hyper_commit");
    assert(input && chain && commit);
    assert(input->euroAddress==0x010410u);
    assert(chain->euroAddress==0x013AA8u);
    assert(commit->euroAddress==0x011DB0u);
    unsigned total=0; focus(&total);assert(total>=12u);
    std::vector<std::uint8_t> code(0x20000u,0u);
    std::uint8_t bytes[32]={};
    const unsigned n=normalizeLength(commit,bytes);
    assert(n==6u);
    const unsigned relocated=commit->euroAddress+14u;
    for(unsigned i=0;i<n;++i)code[relocated+(i^1u)]=bytes[i];
    assert(!plausible(code.data(),unsigned(code.size()),commit,0));
    assert(plausible(code.data(),unsigned(code.size()),commit,14));
    unsigned first=0u;
    assert(localHits(code.data(),unsigned(code.size()),commit,128,&first)==1u);
    assert(first==relocated);
    char snippet[161]={};
    context80(code.data(),unsigned(code.size()),relocated,snippet);
    assert(std::strlen(snippet)==160u);
    assert(std::strncmp(snippet+32u,"302E0052D040",12u)==0);
    assert(!plausible(code.data(),unsigned(code.size()),commit,12));
    code[relocated+2u]^=0xffu;
    assert(!plausible(code.data(),unsigned(code.size()),commit,14));
    assert(localHits(code.data(),unsigned(code.size()),commit,128,&first)==0u);
    std::puts("XMVSF 87.332 native DHC opcode-context probe PASS");
}
