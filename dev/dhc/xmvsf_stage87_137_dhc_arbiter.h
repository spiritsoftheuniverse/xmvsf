#ifndef XMVSF_STAGE87_137_DHC_ARBITER_H
#define XMVSF_STAGE87_137_DHC_ARBITER_H
// XM87.137 experimental pure DHC decision core.
// NO RAM edits, no synthetic controller presses, no SDL dependency.
// Native dispatcher (not implemented here) must validate/commit actual super.
namespace XM87137 {
enum Phase { IDLE=0, FIRST_SUPER=1, DHC_COMMITTED=2, INCOMING_SUPER=3, LOCKED=4 };
struct Request {
    bool combat=false, superActive=false, reserveReady=false;
    bool exactReserveSuperCommand=false, stockAvailable=false;
    unsigned reserveCharacter=0, move=0;
};
struct Arbiter {
    Phase phase=IDLE;
    unsigned reserveCharacter=0, incomingMove=0;
    void reset(){phase=IDLE;reserveCharacter=0;incomingMove=0;}
    void observe(bool combat,bool firstSuperActive,bool incomingActive,bool incomingEnded){
        if(!combat){reset();return;}
        if(phase==IDLE && firstSuperActive)phase=FIRST_SUPER;
        else if(phase==FIRST_SUPER && !firstSuperActive)reset();
        else if(phase==DHC_COMMITTED && incomingActive)phase=INCOMING_SUPER;
        else if(phase==INCOMING_SUPER && incomingEnded)phase=LOCKED;
        else if(phase==LOCKED && !incomingActive)reset();
    }
    // Query only: NO stocks consumed here. Host must atomically commit.
    bool canCommit(const Request& r)const{
        return phase==FIRST_SUPER && r.combat && r.superActive &&
            r.reserveReady && r.exactReserveSuperCommand &&
            r.stockAvailable && r.move!=0;
    }
    bool commit(const Request& r){
        if(!canCommit(r))return false;
        reserveCharacter=r.reserveCharacter;incomingMove=r.move;
        phase=DHC_COMMITTED;return true;
    }
};
} // namespace XM87137
#endif
