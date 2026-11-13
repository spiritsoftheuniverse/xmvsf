// Standalone 87.331 M68K endian-corrected opcode locator test; not a build gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_331_morphus_endian_locator.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace XM87331Endian;
int main(){
    unsigned base=0u,ext=0u;
    XM87330Locator::sites(&base);extra(&ext);
    assert(base==22u && ext==24u);
    const std::uint8_t bigEndian[]={0x4A,0x2E,0x02,0x17,0x67,0x04};
    const unsigned euro=0x009EE6u;
    std::vector<std::uint8_t> code(0x20000u,0u);
    for(unsigned i=0u;i<6u;++i)code[euro+(i^1u)]=bigEndian[i];
    assert(matchesAt(code.data(),unsigned(code.size()),euro,bigEndian,6u));
    assert(!XM87330Locator::atOriginal(code.data(),unsigned(code.size()),
                                     bigEndian,6u,euro));
    char formatted[32]={};normalizedSix(code.data(),unsigned(code.size()),euro,formatted);
    assert(std::strcmp(formatted,"4A2E02176704")==0);
    Hits found=scan(code.data(),unsigned(code.size()),bigEndian,6u);
    assert(found.total==1u && found.address[0]==euro);
    const unsigned moved=euro+0x200u;
    for(unsigned i=0;i<6u;++i)code[moved+(i^1u)]=bigEndian[i];
    found=scan(code.data(),unsigned(code.size()),bigEndian,6u);
    assert(found.total==2u);
    assert(!matchesAt(code.data(),unsigned(code.size()),euro+1u,bigEndian,6u));
    assert(!matchesAt(code.data(),unsigned(code.size()),unsigned(code.size())-4u,bigEndian,6u));
    assert(!matchesAt(code.data(),unsigned(code.size()),euro,bigEndian,5u));
    std::puts("XMVSF 87.331 endian-corrected Morphus opcode locator: PASS");
}
