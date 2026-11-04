// g++ -std=c++11 -Wall -Wextra -pedantic dev/dhc/tests/test_dhc_native_capture.cpp -o dhc_capture_test
// No game ROM, emulator or external test file required.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_137_dhc_native_capture.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
static void setWord(std::vector<std::uint8_t>& mem,unsigned a,unsigned v) {
    mem[a^1u]=std::uint8_t(v>>8);
    mem[(a+1u)^1u]=std::uint8_t(v);
}
int main() {
    std::vector<std::uint8_t> ram(65536,0);
    ram[0x5000u^1u]=8u; // ROM phase == combat
    ram[0x4214u^1u]=2u;
    ram[0x4614u^1u]=1u;
    setWord(ram,0x4000u+0x52u,0x0021u); // P1 point
    setWord(ram,0x4800u+0x52u,0x0032u); // P1 reserve
    setWord(ram,0x4000u+0x60u,0x180Cu);
    setWord(ram,0x4000u+0x34u,0x1234u);
    setWord(ram,0x4000u+0x36u,0x5678u);
    const XM87137Capture::Actor before=XM87137Capture::read(ram.data(),0x4000u);
    assert(before.id==0x21u && before.action==0x180Cu);
    assert(before.script34==0x12345678u);
    XM87137Capture::Recorder capture;
    capture.initialized=true;capture.enabled=true;
    capture.tsv=std::tmpfile();
    capture.binary=std::tmpfile();
    assert(capture.tsv && capture.binary);
    std::fputs("test header\n",capture.tsv); // Recorder::init is bypassed by this fixture
    capture.frame(ram.data(),"xmvsfu",100u,0x11u,0x22u,0x33u,0x44u,0x55u);
    assert(capture.frames==1u);
    assert(capture.previous[0].id==0x21u);
    assert(capture.previous[2].id==0x32u);
    // Each native frame produces exactly four rows and one binary sample.
    setWord(ram,0x4000u+0x60u,0x1A12u);
    capture.frame(ram.data(),"xmvsfu",101u,0x11u,0x22u,0x33u,0x44u,0x55u);
    assert(capture.frames==2u);
    assert(capture.previous[0].action==0x1A12u);
    capture.frame(ram.data(),"xmvsfu",101u,0x11u,0x22u,0x33u,0x44u,0x55u);
    assert(capture.frames==2u); // no duplicate real-frame read
    std::fflush(capture.tsv);std::fflush(capture.binary);
    std::fseek(capture.binary,0,SEEK_END);
    assert(std::ftell(capture.binary)==2L*(6L*4L+0x1400L));
    std::rewind(capture.tsv);
    char line[2048]={};
    assert(std::fgets(line,sizeof(line),capture.tsv)); // header
    assert(std::fgets(line,sizeof(line),capture.tsv)); // P1 point
    assert(std::strstr(line,"\tFIRST\t")!=nullptr);
    for(unsigned i=0;i<3u;++i)assert(std::fgets(line,sizeof(line),capture.tsv));
    assert(std::fgets(line,sizeof(line),capture.tsv)); // second P1 point
    assert(std::strstr(line,"\tACTION\t")!=nullptr);
    ram[0x5000u^1u]=0u;
    capture.frame(ram.data(),"xmvsfu",102u,0,0,0,0,0);
    assert(capture.frames==2u && !capture.previousValid[0]);
    return 0;
}
