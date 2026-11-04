// Portable isolated test: F10 diagnostic never changes native RAM or OAM.
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage87_139_probe.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
static void sb(std::vector<std::uint8_t>& r,unsigned a,unsigned v){r[a^1u]=std::uint8_t(v);}
static void sw(std::vector<std::uint8_t>& r,unsigned a,unsigned v){sb(r,a,v>>8);sb(r,a+1u,v);}
static void fighter(std::vector<std::uint8_t>& r,unsigned a,unsigned id,unsigned x){
 sw(r,a,0x0101u);sw(r,a+0x52u,id);sw(r,a+0x0Cu,x);sw(r,a+0x10u,420u);
}
int main(){
 char scratch[]="/tmp/xm87139-ci-XXXXXX";
 assert(mkdtemp(scratch)!=nullptr);
 assert(setenv("XM87139_PROBE_DIR",scratch,1)==0);
 XM87139Probe::State p;
 assert(p.toggle(true) && p.active);
 std::vector<std::uint8_t> ram(65536,0);
 sb(ram,0x5000u,8u);sb(ram,0x5113u,5u);
 fighter(ram,0x4000u,0x20u,350u);
 fighter(ram,0x4400u,0x24u,500u);
 fighter(ram,0x4800u,0x22u,340u);
 fighter(ram,0x4C00u,0x26u,600u);
 const std::vector<std::uint8_t> original=ram;
 p.frame(ram.data(),"xmvsfu",100u,false,3u,24u,0x7Cu,0xFFu,0xA0u,
    100u,2u,3u,1u,100u,1u,12u,4u);
 assert(p.frames==1u && p.tagEvents==0u);
 assert(ram==original);
 // Real point/reserve identity swaps; this recorder MUST never reposition.
 sw(ram,0x4000u+0x52u,0x22u);
 sw(ram,0x4800u+0x52u,0x20u);
 const auto nativeSwap=ram;
 p.frame(ram.data(),"xmvsfu",101u,false,3u,24u,0x7Cu,0xFFu,0xA0u,
    101u,2u,3u,1u,101u,1u,12u,4u);
 assert(p.frames==2u && p.tagEvents==1u);
 assert(ram==nativeSwap);
 // OAM: raw first word C000 priority 6, palette 8; trace must read only.
 std::uint16_t raw[4]={0x00C0u,0x0002u,0x0300u,0x0811u};
 const std::uint16_t snapshot[4]={0x00C0u,0x0002u,0x0300u,0x0811u};
 p.oamFrame(102u,raw,1u,6u,6u,false);
 assert(p.oamCount==1u && std::memcmp(raw,snapshot,sizeof(raw))==0);
 const std::string dataFile=p.tracePath,oamFile=p.oamPath;
 assert(p.toggle(true) && !p.active);
 std::ifstream native(dataFile.c_str()),tile(oamFile.c_str());
 assert(native.good() && tile.good());
 const std::string nativeText((std::istreambuf_iterator<char>(native)),
                               std::istreambuf_iterator<char>());
 const std::string oamText((std::istreambuf_iterator<char>(tile)),
                             std::istreambuf_iterator<char>());
 assert(nativeText.find("mapP1dir")!=std::string::npos);
 assert(nativeText.find("IDENTITY")!=std::string::npos);
 assert(nativeText.find("script34")!=std::string::npos);
 assert(oamText.find("palette")!=std::string::npos);
 assert(oamText.find("\t6\t8\t")!=std::string::npos);
 assert(nativeText.find("# F10_START armed")!=std::string::npos);
 assert(nativeText.find("# F10_STOP native_frames=2")!=std::string::npos);
 assert(oamText.find("# F10_START armed")!=std::string::npos);
 // The user must get a tangible file even if F10 is used outside combat.
 XM87139Probe::State idle;
 assert(idle.toggle(true)&&idle.active);
 const std::string emptyPath=idle.tracePath;
 assert(idle.toggle(true)&&!idle.active);
 std::ifstream blank(emptyPath.c_str());
 assert(blank.good());
 const std::string blankText((std::istreambuf_iterator<char>(blank)),
                              std::istreambuf_iterator<char>());
 assert(blankText.find("# F10_START armed")!=std::string::npos);
 assert(blankText.find("NO_COMBAT_FRAMES_SEEN")!=std::string::npos);
 return 0;
}
