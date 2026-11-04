#ifndef XMVSF_STAGE87_98_LATENCY_TRACE_H
#define XMVSF_STAGE87_98_LATENCY_TRACE_H
// Stage 87.98: passive timestamps + native action-change candidates + DF draw cost.
// No game-RAM, ROM, keyboard, or controller writes. SDL counters are host time,
// never physical switch / photon latency. Action changes are candidates only.
#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8798Latency {
static const char* const READY="XM87.98 INPUT_ROM_DF_TRACE_READY";
static inline double ms(Uint64 a, Uint64 b){const Uint64 f=SDL_GetPerformanceFrequency();return (f && b>=a)?1000.0*double(b-a)/double(f):0.0;}
static inline unsigned b8(const std::uint8_t* p,unsigned addr){return p[(addr&65535u)^1u];}
static inline unsigned b16(const std::uint8_t* p,unsigned addr){return (b8(p,addr)<<8)|b8(p,addr+1);}
static inline bool df(){const char* s=std::getenv("XM8649_WILY_RENDER");return s&&s[0]=='1'&&s[1]==0;}
struct Pending{
 bool active=false;
 unsigned player=0,sub=0,frame=0,baseAction=0,baseSubstate=0;
 Uint64 pressQpc=0,seq=0;
};
struct State{
 std::FILE *edgeLog=nullptr,*phaseLog=nullptr;
 bool opened=false,prevButton[2][8]={{false}},lastActionValid[2]={false,false};
 unsigned lastAction[2]={0,0},lastSubstate[2]={0,0};
 unsigned frame=0,nativeFrames=0,observedFrame=0;
 Uint64 epoch=0,frameQpc=0,drawStart=0,objectStart=0;
 Uint64 lastEdgeQpc=0,lastResponsePressQpc=0,lastResponseQpc=0,lastPresentationQpc=0;
 double lastDrawMs=0,lastObjectMs=0,lastSampleToActionMs=0,lastSampleToSubmitMs=0;
 Uint64 edgeCount=0,candidateCount=0,timeoutCount=0,logEvents=0;
 unsigned drawPasses=0;
 unsigned objectPasses=0,lastObjectPasses=0;
 unsigned lastResponsePlayer=0,lastResponseSub=0,lastResponseAction=0,lastResponseFrames=0;
 bool lastResponseWasAttack=false,lastResponseCandidate=false;
 Pending pending[2][16];
 ~State(){if(edgeLog)std::fclose(edgeLog);if(phaseLog)std::fclose(phaseLog);}
 void open(){
  if(opened)return;
  opened=true;
  const char* a=std::getenv("XM8798_INPUT_TRACE_FILE");
  const char* b=std::getenv("XM8798_PIPELINE_TRACE_FILE");
  if(a&&*a){edgeLog=std::fopen(a,"wb");if(edgeLog){std::setvbuf(edgeLog,nullptr,_IOFBF,128*1024);
   std::fputs("event\tseq\tframe\tromframe\tdf\tplayer\tsub\tqpc\tqpc_freq\thost_ms\tbase_action\tnew_action\tbase_sub\tnew_sub\tframes_from_edge\tms_from_sample\tdetail\n",edgeLog);}}
  if(b&&*b){phaseLog=std::fopen(b,"wb");if(phaseLog){std::setvbuf(phaseLog,nullptr,_IOFBF,64*1024);
   std::fputs("frame\tdf\tcps_draw_ms\tcps_draw_passes\tobj_draw_ms\tobj_passes\tsample_to_latest_candidate_ms\tcandidate_count\tedges\tinfo\n",phaseLog);}}
  epoch=SDL_GetPerformanceCounter();
  std::fprintf(stderr,"%s initialized (mode=%s)\n",READY,df()?"DOUBLE_FEATURE":"NORMAL");
 }
 void report(const char* name,Uint64 seq,unsigned player,unsigned sub,Uint64 stamp,
             unsigned base,unsigned next,unsigned preSub,unsigned postSub,unsigned elapsed,
             double delay,const char* extra){
  if(!edgeLog)return;
  std::fprintf(edgeLog,"%s\t%llu\t%u\t%u\t%u\t%u\t%02X\t%llu\t%llu\t%.4f\t%04X\t%04X\t%04X\t%04X\t%u\t%.4f\t%s\n",
   name,(unsigned long long)seq,frame,observedFrame,df()?1u:0u,player,sub,
   (unsigned long long)stamp,(unsigned long long)SDL_GetPerformanceFrequency(),ms(epoch,stamp),
   base,next,preSub,postSub,elapsed,delay,extra);
  // Buffered logging: never force a disk flush on the input sampling path.
  if((++logEvents%128u)==0u)std::fflush(edgeLog);
 }
 void frameStart(unsigned emu){open();frame=emu+1u;frameQpc=SDL_GetPerformanceCounter();objectPasses=0;drawPasses=0;lastDrawMs=0.0;lastObjectMs=0.0;}
 void mapped(unsigned player,unsigned sub,int value){
  if(player>=2||sub<0x80u||sub>0x87u)return;
  bool& prev=prevButton[player][sub-0x80u];const bool down=value!=0;
  if(down&&!prev){
   const Uint64 now=SDL_GetPerformanceCounter();++edgeCount;lastEdgeQpc=now;
   // Keep FIFO sequence per fighter: never overwrite pending inputs silently.
   Pending* slot=nullptr;
   for(unsigned k=0;k<16;++k)if(!pending[player][k].active){slot=&pending[player][k];break;}
   if(!slot){report("QUEUE_FULL",edgeCount,player+1,sub,now,lastAction[player],lastAction[player],lastSubstate[player],lastSubstate[player],0,0.0,"no_candidate");}
   else{
    slot->active=true;slot->player=player;slot->sub=sub;slot->frame=frame;
    slot->baseAction=lastAction[player];slot->baseSubstate=lastSubstate[player];
    slot->pressQpc=now;slot->seq=edgeCount;
    report("MAPPED_DOWN",slot->seq,player+1,sub,now,slot->baseAction,slot->baseAction,
      slot->baseSubstate,slot->baseSubstate,0,0.0,"sampled_not_physical");
   }
  } else if(!down&&prev){
   report("MAPPED_UP",0,player+1,sub,SDL_GetPerformanceCounter(),lastAction[player],lastAction[player],lastSubstate[player],lastSubstate[player],0,0.0,"released");
  }
  prev=down;
 }
 void beforeDraw(){drawStart=SDL_GetPerformanceCounter();++drawPasses;}
 void afterDraw(){if(drawStart){lastDrawMs+=ms(drawStart,SDL_GetPerformanceCounter());drawStart=0;}}
 void beforeObjects(){objectStart=SDL_GetPerformanceCounter();++objectPasses;}
 void afterObjects(){if(objectStart){lastObjectMs+=ms(objectStart,SDL_GetPerformanceCounter());objectStart=0;}}
 void nativeFrame(const std::uint8_t* ram,unsigned romFrame){
  if(!ram)return;
  const unsigned phase=b8(ram,0x5000u);
  const Uint64 now=SDL_GetPerformanceCounter();observedFrame=romFrame;
  for(unsigned p=0;p<2;++p){
   const unsigned base=p?0x4400u:0x4000u;
   const unsigned action=b16(ram,base+0x60u),substate=b16(ram,base+6u);
   if(phase==8u && lastActionValid[p] && action!=lastAction[p]){
    Pending* first=nullptr;
    for(unsigned i=0;i<16;++i){Pending& q=pending[p][i];
     if(q.active&&frame>=q.frame&&frame-q.frame<=12u && (!first || q.seq<first->seq))first=&q;
    }
    if(first){
     const unsigned elapsed=frame-first->frame;
     const double delay=ms(first->pressQpc,now);
     const bool attack=(action>=0x1800u&&action<=0x1affu);
     report("ROM_ACTION_CANDIDATE",first->seq,p+1,first->sub,now,
       first->baseAction,action,first->baseSubstate,substate,elapsed,delay,
       attack?"attack_range_candidate_not_proven":"action_change_not_proven");
     lastResponseQpc=now;lastResponsePressQpc=first->pressQpc;lastSampleToActionMs=delay;lastResponsePlayer=p+1;
     lastResponseSub=first->sub;lastResponseAction=action;lastResponseFrames=elapsed;
     lastResponseWasAttack=attack;lastResponseCandidate=true;
     ++candidateCount;first->active=false;
    }else report("ROM_ACTION_UNPAIRED",0,p+1,0,now,lastAction[p],action,
          lastSubstate[p],substate,0,0.0,"no_pending_button");
   }
   lastAction[p]=action;lastSubstate[p]=substate;lastActionValid[p]=phase==8u;
   for(unsigned i=0;i<16;++i){Pending& q=pending[p][i];
    if(q.active&&frame>=q.frame&&frame-q.frame>12u){
      report("NO_ACTION_WITHIN_12_FRAMES",q.seq,p+1,q.sub,now,q.baseAction,action,
        q.baseSubstate,substate,frame-q.frame,ms(q.pressQpc,now),"may_be_buffered_blocked_or_non_attack");
      q.active=false;++timeoutCount;
    }
   }
  }
  ++nativeFrames;
  if(phase==8u && (nativeFrames%60u)==0u){
   if(phaseLog){std::fprintf(phaseLog,"%u\t%u\t%.4f\t%u\t%.4f\t%u\t%.4f\t%llu\t%llu\t60_frame_sample\n",
      frame,df()?1u:0u,lastDrawMs,drawPasses,lastObjectMs,objectPasses,lastSampleToActionMs,
      (unsigned long long)candidateCount,(unsigned long long)edgeCount);if((nativeFrames%600u)==0u)std::fflush(phaseLog);}
  }
 }
 void presented(){
  const Uint64 now=SDL_GetPerformanceCounter();lastPresentationQpc=now;
  if(lastResponseCandidate && lastResponseQpc && now>=lastResponseQpc){
   lastSampleToSubmitMs=ms(lastResponsePressQpc,now);
   report("FIRST_PRESENT_AFTER_ACTION",0,lastResponsePlayer,lastResponseSub,now,0,lastResponseAction,
      0,0,lastResponseFrames,lastSampleToSubmitMs,"SDL_present_call_return_not_display_photon");
   lastResponseCandidate=false;
  }
 }
};
inline State& state(){static State x;return x;}
} // namespace XM8798Latency
#endif
