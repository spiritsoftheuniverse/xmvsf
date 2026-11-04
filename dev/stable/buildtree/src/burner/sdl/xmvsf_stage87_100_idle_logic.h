#ifndef XMVSF_STAGE87_100_IDLE_LOGIC_H
#define XMVSF_STAGE87_100_IDLE_LOGIC_H
#include <cstdint>
namespace XM87100Idle {
static constexpr unsigned MAX_HP=0x90u;
static constexpr unsigned IDLE_FRAMES=120u;
struct Fighter {
 unsigned frames=0,previousHp=MAX_HP,previousX=0,previousY=0;
 unsigned previousAction=0,previousContact=0;
 bool initialized=false,refilled=false;
 void reset(){frames=0;previousHp=MAX_HP;previousX=previousY=previousAction=previousContact=0;initialized=false;refilled=false;}
 // Only native action 0000 means verified standing neutral; movement, blocking,
 // attacks, aerial actions and recovery all interrupt the timer. Displacement
 // checks catch slide/drift while the action temporarily still says neutral.
 bool tick(bool ready,unsigned action,unsigned hp,unsigned x,unsigned y,bool contact,unsigned targetHp=MAX_HP){
  if(!ready||hp<1u||hp>MAX_HP){reset();return false;}
  bool moved=initialized&&(x!=previousX||y!=previousY);
  bool damaged=initialized&&hp<previousHp;
  const bool idle=action==0u&&!moved&&!contact&&!damaged;
  if(!idle){frames=0;refilled=false;}
  else if(!refilled&&frames<IDLE_FRAMES){++frames;}
  bool restore=idle&&!refilled&&frames==IDLE_FRAMES;
  if(restore){refilled=true;frames=0;}
  initialized=true;previousX=x;previousY=y;previousAction=action;
  previousHp=restore?targetHp:hp;
  return restore;
 }
};
struct State {
 Fighter fighters[2];
 unsigned lastPhase=0xffffffffu;
 bool enabled=false;
 void reset(){fighters[0].reset();fighters[1].reset();lastPhase=0xffffffffu;enabled=false;}
};
inline State& state(){static State s;return s;}
} // namespace XM87100Idle
#endif
