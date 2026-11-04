#ifndef XMVSF_STAGE87_162_REPLAY_SYNC_H
#define XMVSF_STAGE87_162_REPLAY_SYNC_H
// Native 87.162: READ-ONLY bridge between the original XM87.61 F9
// movie recorder and the ROM action observer in cps_run.cpp.
//
// Both the SDL host and CPS2 engine include this pure C++11 header.
// One external-linkage inline accessor supplies the same static storage
// across translation units. Never use this to inject emulator inputs,
// mutate RAM, or alter DHC timing/point ownership.
namespace XM87162ReplaySync {
inline unsigned& slot(){
    static unsigned frame=~0u;
    return frame;
}
inline void publish(unsigned frame){slot()=frame;}
inline unsigned movieFrame(){return slot();}
} // namespace XM87162ReplaySync
#endif
