// XMVSF Stage18 -- Training-only side placement, no input buffering or ROM modification.
// Side shortcut is applied once at the end of a real frame, never during runahead.
#ifndef XMVSF_STAGE18_POSITION_H
#define XMVSF_STAGE18_POSITION_H
#include "xmvsf_stage14_mode.h"
#include "burnint.h"
#include <stdio.h>

struct XM18Position {
    unsigned char held[2];  // 0 none, 1 left+start, 2 right+start
    unsigned char requested[2];
    unsigned char combat;
    FILE* log;
};
extern XM18Position xm18Position;

// Called from the already mapped SDL input path. Left/right are read from the
// SAME player's physical controller, so no new polling latency or macros.
static inline int xm18ProcessInput(int player,int sub,int val,int left,int right) {
    if (!xm14TrainingMode || player<0 || player>1 || sub!=0x87 ||
        !xm18Position.combat) return val;
    const unsigned char chord=(val && (left != right)) ? (left?1:2) : 0;
    if (!bBurnRunAheadFrame) {
        if (!val) xm18Position.held[player]=0;
        else if (chord && !xm18Position.held[player]) {
            xm18Position.requested[player]=chord;
            xm18Position.held[player]=1;
        }
    }
    // Start+direction is a training command, not an in-game Start press.
    return chord ? 0 : val;
}

static inline unsigned int xm18Read16(cpu_core_config* c,unsigned int addr) {
    return ((c->read(addr)&255u)<<8)|(c->read(addr+1)&255u);
}
static inline unsigned int xm18Read32(cpu_core_config* c,unsigned int addr) {
    return (xm18Read16(c,addr)<<16)|xm18Read16(c,addr+2);
}
static inline void xm18Write16(cpu_core_config* c,unsigned int addr,unsigned int v) {
    c->write(addr,(v>>8)&255u);c->write(addr+1,v&255u);
}
static inline void xm18Write32(cpu_core_config* c,unsigned int addr,unsigned int v) {
    xm18Write16(c,addr,(v>>16)&65535u);xm18Write16(c,addr+2,v&65535u);
}

static inline void xm18ApplyPosition(int ready) {
    if (!xm14TrainingMode || !ready || bBurnRunAheadFrame) {
        if (!xm14TrainingMode) {
            xm18Position.combat=0;
            xm18Position.held[0]=xm18Position.held[1]=0;
            xm18Position.requested[0]=xm18Position.requested[1]=0;
        }
        return;
    }
    cpu_core_config* cpu=GetCpuCoreConfig(0);
    if (!cpu || !cpu->open || !cpu->close || !cpu->read || !cpu->write) return;
    cpu->open(0);
    // Verified by the XMVSF hitbox research: match status $FF5000, live P1
    // pointer $FFA014 == $00FF4000, active point fighters $FF4000/$FF4400.
    const bool live = cpu->read(0xFF5000)==0x08 && xm18Read32(cpu,0xFFA014)==0x00FF4000;
    xm18Position.combat=live?1:0;
    if (!live) {
        xm18Position.held[0]=xm18Position.held[1]=0;
        xm18Position.requested[0]=xm18Position.requested[1]=0;
        cpu->close();return;
    }
    // On a shared frame, favor P1's last request; never write both orders.
    int player=-1;unsigned int desired=0;
    if (xm18Position.requested[0]) {player=0;desired=xm18Position.requested[0];}
    else if (xm18Position.requested[1]) {player=1;desired=xm18Position.requested[1];}
    xm18Position.requested[0]=xm18Position.requested[1]=0;
    if (player<0) {cpu->close();return;}
    const unsigned int p1=0xFF4000,p2=0xFF4400;
    const unsigned int x1=xm18Read16(cpu,p1+0x0C),x2=xm18Read16(cpu,p2+0x0C);
    const int sep=(int)x1-(int)x2;
    // Fail closed if the actors are absent, positions are identical, or RAM
    // fields don't look like actual fight positions (avoid writing during tags).
    if (cpu->read(p1)==0 || cpu->read(p2)==0 || sep==0 || sep < -2048 || sep>2048) {
        cpu->close();return;
    }
    const bool playerWantsLeft = desired==1;
    const bool p1WantsLeft = player==0 ? playerWantsLeft : !playerWantsLeft;
    const bool p1IsLeft = x1<x2;
    if (p1WantsLeft != p1IsLeft) {
        // Preserve the original fighter separation and fractional X values.
        const unsigned int full1=xm18Read32(cpu,p1+0x0C),full2=xm18Read32(cpu,p2+0x0C);
        xm18Write32(cpu,p1+0x0C,full2);
        xm18Write32(cpu,p2+0x0C,full1);
        // Bit 0 of +$4B is the horizontal sprite facing flag. Preserve other
        // bits, and let the original game maintain its facing thereafter.
        const unsigned int dir1=cpu->read(p1+0x4B)&255u;
        const unsigned int dir2=cpu->read(p2+0x4B)&255u;
        cpu->write(p1+0x4B,(dir1 & ~1u) | (p1WantsLeft?0u:1u));
        cpu->write(p2+0x4B,(dir2 & ~1u) | (p1WantsLeft?1u:0u));
    }
    if (!xm18Position.log) xm18Position.log=fopen("xmvsf_stage18_position.log","w");
    if (xm18Position.log) {
        fprintf(xm18Position.log,"player=%d desired=%s p1x=%u p2x=%u changed=%d\n",
                player+1,desired==1?"LEFT":"RIGHT",x1,x2,(int)(p1WantsLeft!=p1IsLeft));
        fflush(xm18Position.log);
    }
    cpu->close();
}
#endif
