#ifndef XMVSF_STAGE87_118_METER_AUDIO_H
#define XMVSF_STAGE87_118_METER_AUDIO_H
// XM87.118: use the established ROM-work-RAM stock bytes. Read-only observer.
// Sound plays when a player GAINS a completed super stock (0..3), not per gauge pixel.
// The first live frame only seeds the counters; menu, runahead and training
// infinite-meter mode never trigger a charge sound.
static inline void xm87118OnRealFrame(bool ready){
    static bool seeded=false;
    static unsigned previous[2]={0u,0u};
    if(!ready || bBurnRunAheadFrame ||
       (xm14TrainingMode && xm23State.infiniteMeter)){
        seeded=false;return;
    }
    const char* game=BurnDrvGetTextA(DRV_NAME);
    if(!game || std::strncmp(game,"xmvsf",5)!=0){seeded=false;return;}
    cpu_core_config* cpu=GetCpuCoreConfig(0);
    if(!cpu||!cpu->open||!cpu->close||!cpu->read){seeded=false;return;}
    cpu->open(0);
    const bool combat=xm23::r8(cpu,0xff5000)==8u &&
                      xm23::r32(cpu,0xffa014)==0x00ff4000u;
    const unsigned stock[2]={xm23::r8(cpu,0xff4214),xm23::r8(cpu,0xff4614)};
    cpu->close();
    if(!combat || stock[0]>3u || stock[1]>3u){seeded=false;return;}
    if(seeded){
        for(unsigned side=0;side<2;++side){
            if(stock[side]>previous[side])xm20MeterCharged(int(side));
        }
    }
    previous[0]=stock[0];previous[1]=stock[1];seeded=true;
}
#endif
