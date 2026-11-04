#ifndef XMVSF_STAGE87_137_DHC_LANDING_H
#define XMVSF_STAGE87_137_DHC_LANDING_H
// Stage 87.137 DHC jump destination. Pure geometry: no character teleport,
// no native state writes. The host uses this as the TARGET for the normal
// tag-in jump, preserving the native landing animation and collision.
// Landing beside the opponent replaces the old outgoing-fighter takeover X.
namespace XM87137Landing {
struct Target {
    int x=0;
    bool faceRight=true;
    bool valid=false;
};
// Native world-coordinate inputs (not SDL screen coordinates). The incoming
// fighter lands on the same side of the opponent the outgoing fighter occupies.
// separation is a FIRST-PLAYABLE-PASS tuning value supplied by the caller;
// the host should ultimately derive it from the game's actual pushbox width.
// Clamp only to caller-provided confirmed stage bounds; never move opponent.
inline Target choose(int outgoingX,int opponentX,int separation,
                     int worldLeft,int worldRight){
    Target t;
    if(separation<=0 || worldLeft>=worldRight ||
       opponentX<worldLeft || opponentX>worldRight)return t;
    const bool incomingLeft=outgoingX<=opponentX;
    const long long wanted=(long long)opponentX+
                           (incomingLeft?-(long long)separation:(long long)separation);
    if(wanted<worldLeft || wanted>worldRight)return t;
    t.x=(int)wanted;
    t.faceRight=incomingLeft;
    t.valid=true;
    return t;
}
} // namespace XM87137Landing
#endif
