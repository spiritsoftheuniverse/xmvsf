#ifndef XMVSF_STAGE87_194_NATIVE_PASSIVE_JUMP_ENTRY_H
#define XMVSF_STAGE87_194_NATIVE_PASSIVE_JUMP_ENTRY_H
// 87.194: An incoming actor must TRAVEL to its target during the
// original native airborne entry, rather than have its Y teleported to
// the floor and its native velocity zeroed. The original ROM supplies
// vertical motion, facing, and real ground contact; the host only steers
// horizontal placement to the existing opponent-relative DHC target.
//
// 87.193(1) Sabretooth reference: native point swap 0A/12, then
// action 0000 / mode 0000 / D6=00 with original ROM idle script
// 0BD212 -> 0BD222 -> 0BD232; no synthesized Ken 2A08/instant idle.
// This is a landing/standing reference, not evidence for a fabricated
// Sabretooth 2A selector or an inferred Sabretooth horizontal velocity.
//
// This module is pure arithmetic / observed state, never creates a
// synthetic sprite, upward velocity, action timer, or native ROM script.
#include <cstdint>
namespace XM87194Jump {
struct Arrival {
    bool active=false,descended=false,landed=false;
    unsigned side=0u,fighter=0u,startFrame=0u,landingFrame=0u;
    int startX=0,startY=0,floorY=0,targetX=0;
    int maxY=0,lastY=0,positionX=0;
    unsigned nativeDownwardSamples=0u;
    bool begin(unsigned owner,unsigned id,unsigned frame,int x,int y,
               int floor,int foeX,bool outgoingWasLeft){
        if(active || owner>=2u || floor<=y || floor>768 ||
           floor<128 || y<0 || x<0)return false;
        side=owner;fighter=id;startFrame=frame;landingFrame=0u;
        startX=x;startY=y;floorY=floor;lastY=y;maxY=y;
        targetX=foeX+(outgoingWasLeft?-48:48);
        if(targetX<192)targetX=192;
        if(targetX>768)targetX=768;
        positionX=x;nativeDownwardSamples=0u;
        active=true;descended=false;landed=false;
        return true;
    }
    // ROM supplies the actual Y trajectory. X only follows the existing
    // opponent-relative target as a function of observed native descent;
    // no 10/15/20-frame home-baked leap or artificial acceleration.
    // If the ROM never descends, do NOT claim the fighter arrived.
    int position(unsigned frame,int nativeY){
        if(!active||landed||frame<startFrame)return positionX;
        if(nativeY>lastY && nativeY<floorY){
            descended=true;
            ++nativeDownwardSamples;
        }
        lastY=nativeY;
        if(nativeY>maxY)maxY=nativeY;
        if(!descended){
            // A single host/ROM jump straight to floor is not a
            // physically observed native jump. Keep the native spawn
            // X instead of silently recreating the old teleport.
            positionX=startX;
            return positionX;
        }
        int progress=nativeY-startY;
        const int span=floorY-startY;
        if(progress<0)progress=0;
        if(progress>span)progress=span;
        const int delta=targetX-startX;
        positionX=startX+(delta*progress + (delta>=0?span/2:-span/2))/span;
        // A natural native ground contact is authoritative, not elapsed
        // frames. Require multiple actual airborne Y samples to prevent
        // a one-frame teleported floor from counting as a jump.
        if(nativeY>=floorY && nativeDownwardSamples>=2u &&
           frame>startFrame){
            positionX=targetX;
            landed=true;landingFrame=frame;
        }
        return positionX;
    }
    bool ready(unsigned id,int nativeY)const {
        return active&&landed&&id==fighter&&nativeY>=floorY;
    }
    void reset(){*this=Arrival();}
};
} // namespace XM87194Jump
#endif
