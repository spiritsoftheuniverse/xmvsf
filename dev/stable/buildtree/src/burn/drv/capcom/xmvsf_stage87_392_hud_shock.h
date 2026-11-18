#ifndef XMVSF_STAGE87_392_HUD_SHOCK_H
#define XMVSF_STAGE87_392_HUD_SHOCK_H
// TEST 87.399: source-verified HUD concealment from VS song request
// and the separate SDL stun bar. No ROM/RAM mutations or opaque screen masks.
// TEST 87.428: normal VS shock reveal no longer uses this 130f
// compatibility delay. Song stays phase 02+27; 04>06 +23 starts the
// unchanged 30-frame HUD reveal. HUD sprite classification is frozen.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace XM87392 {
static const unsigned DelayFrames=130u;
static const unsigned RevealFrames=30u;

// Native original xmvsfu is not CpsXmvSixEnabled. Guard by actual driver
// identity so normal Arcade/Training/Versus and the 3v3 derivative all work.
inline bool eligible(const char* driver,bool version4){
    return version4 && driver && std::strncmp(driver,"xmvsf",5)==0;
}

struct Timeline {
    unsigned start=0u;
    unsigned primedAt=0u;
    unsigned lastFrame=~0u;
    unsigned lastSongStart=0u;
    unsigned firstSourceFrame=0u;
    unsigned visualShockFrame=0u; // independent visual activation
    int lastPhase=-1;
    bool primed=false;
    bool armed=false;
    bool capturedAnchor=false;
    bool visualShockActive=false; // true only after phase 06+23
    bool sourceSeen=false;

    void reset(){*this=Timeline{};}

    // Called on the SAME M68K chooser->VS handoff that requests the song.
    // Hide actual HUD source objects immediately, before audio decode/start.
    void prepareForSong(unsigned frame){
        if(primed && !armed)return; // duplicate cue before audio starts
        primed=true;armed=false;capturedAnchor=false;
        visualShockActive=false;visualShockFrame=0u;primedAt=frame;
        sourceSeen=false;firstSourceFrame=0u;
        std::fprintf(stderr,"XM87.399 HUD_SHOCK_PRIMED frame=%u\n",frame);
    }

    // The fallback audio-clock delay uses the same 130-frame hold.
    void startWithSong(unsigned songSequence,unsigned frame){
        if(!songSequence || songSequence==lastSongStart)return;
        lastSongStart=songSequence;
        // Mixer scheduling is asynchronous. The measured screenshot
        // anchor owns both clocks; audible PCM must not restart the fade.
        if(capturedAnchor)return;
        if(!primed || armed){sourceSeen=false;firstSourceFrame=0u;}
        primed=true;armed=true;start=frame;
        std::fprintf(stderr,
            "XM87.399 HUD_SHOCK_SONG_START song=%u frame=%u hold=%u reveal=%u\n",
            songSequence,frame,DelayFrames,RevealFrames);
    }

    // Song anchor is separate from the shock reveal; no sprite source,
    // palette or dither rules are modified in TEST 87.428.
    void beginAtCapturedPosition(unsigned frame){
        if(capturedAnchor)return;
        if(!primed){
            primed=true;primedAt=frame;
            sourceSeen=false;firstSourceFrame=0u;
        }
        armed=true;capturedAnchor=true;start=frame;
        visualShockActive=false;visualShockFrame=0u;
        std::fprintf(stderr,
            "XM87.428 HUD_SONG_ANCHOR_ONLY frame=%u phase02_offset=27 shock_wait=1\n",
            frame);
    }

    // Called once by the observed 04->06 transition +23 frame gate,
    // NOT by the song mixer or any fixed song-to-shock timer.
    void activateShockAtVisualFrame(unsigned frame){
        if(!primed || !armed || !capturedAnchor || visualShockActive)return;
        visualShockActive=true;
        visualShockFrame=frame;
        std::fprintf(stderr,
            "XM87.428 HUD_SHOCK_VISUAL_START frame=%u fade_frames=%u song_start=%u\n",
            frame,RevealFrames,start);
    }

    // 02/04 overlap the VS transition and native forest. There is NEVER
    // any broad priority-7 suppression here; only source-confirmed HUD
    // tiles may be touched, by objectForPhase() below.
    static bool mayMaskNativeHud(int phase){
        return phase==2 || phase==4 || phase==6 || phase==8;
    }

    void observe(int phase,unsigned frame,bool allowed){
        if(!allowed){
            primed=false;armed=false;capturedAnchor=false;
            visualShockActive=false;visualShockFrame=0u;
            lastFrame=frame;lastPhase=-1;return;
        }
        if(lastFrame!=~0u && frame<lastFrame){
            primed=false;armed=false;capturedAnchor=false;
            visualShockActive=false;visualShockFrame=0u; // rewind
        }
        if(phase==0x0a || phase==0x0c){
            primed=false;armed=false;capturedAnchor=false;
            visualShockActive=false;visualShockFrame=0u;
            sourceSeen=false;firstSourceFrame=0u;
        }
        lastFrame=frame;lastPhase=phase;
    }

    // Called when an actual ROM-verified HUD source first reaches the
    // painter. The VS song can finish its delay BEFORE these sprites exist.
    // If so, prevent a fully-visible first frame, then reveal over 30 frames.
    void noteHudSource(unsigned frame){
        if(!primed || sourceSeen)return;
        sourceSeen=true;firstSourceFrame=frame;
        const unsigned fadeDue=capturedAnchor?
            (visualShockActive?visualShockFrame:0u):
            (armed?start+DelayFrames:0u);
        std::fprintf(stderr,
            "XM87.428 HUD_SHOCK_FIRST_SOURCE frame=%u fade_due=%u "
            "waiting_for_visual=%u late=%u\n",
            frame,fadeDue,(unsigned)(capturedAnchor && !visualShockActive),
            (unsigned)(fadeDue && frame>=fadeDue));
    }

    unsigned step(unsigned frame)const {
        // Before the verified song request: never hide selection art.
        if(!primed)return RevealFrames;
        // Hide from the handoff while the PCM decoder prepares.
        if(!armed)return 0u;
        if(frame<start)return 0u;
        // Normal captured VS matches wait for the verified phase-06+23
        // visual trigger; they DO NOT use the song-to-shock delay.
        if(capturedAnchor && !visualShockActive)return 0u;
        // Compatibility path retains the former 130f mixer-based hold.
        // Source-late guard still prevents a sudden fully-visible first
        // HUD frame when sprite creation occurs after the cue.
        unsigned due=capturedAnchor?visualShockFrame:start+DelayFrames;
        if(sourceSeen && firstSourceFrame>=due)due=firstSourceFrame+1u;
        if(frame<due)return 0u;
        if(frame>=due+RevealFrames)return RevealFrames;
        return frame-due+1u;
    }

    // TEST 87.431: renderer-owned dither, not a best-effort "primed" cue.
    // Native precombat HUD is already on screen during phases 02/04/06.
    // Force the ALREADY-APPROVED step-zero invisibility immediately,
    // including when the chooser observer or audio setup is late/missed.
    // After the fixed 04->06 + 23 shock trigger, the original step()
    // runs its unchanged 30-frame reveal. Selection/results remain native.
    unsigned paintStep(int phase,unsigned frame)const {
        if((phase==2 || phase==4 || phase==6) && !visualShockActive)
            return 0u;
        return step(frame);
    }
};
inline Timeline& timeline(){static Timeline s;return s;}

// TEST 87.414: 02/04 MUST be source-scoped. The 87.413 fallback
// "any small priority-7 object" hid original forest/tree imagery again.
// Preserve the native 120+30 frame shock clock, but mask only native
// HP/time, candidate name glyph sheets in the original documented name
// footprints and ROM portrait sprites during pre-fight transitions.
inline bool timeIndicator(unsigned priority,unsigned palette,unsigned tile,
                          int x,int y,unsigned wide,unsigned tall,bool transition){
    (void)transition;
    if(priority!=0u || palette!=1u || !wide || !tall ||
       wide>2u || tall>2u || x<128 || x>=256 ||
       y< -16 || y>=56)return false;
    return (tile>=87808u && tile<=87831u) ||
           (x>=144 && x<224 && y>=0 && y<40);
}
inline bool verifiedHpStrip(unsigned priority,unsigned palette,unsigned tile,
                            int x,int y,unsigned wide,unsigned tall){
    return priority==7u && (palette==2u || palette==4u) &&
           x>=-4 && x<=370 && (y==20 || y==32) && wide && tall &&
           ((tile>=88417u && tile<=88579u) ||
             tile==89417u || tile==89418u);
}
// Coordinates match the native Stage87.35 left/right name ink footprints,
// including long multi-cell names such as Wolverine. Geometry is deliberately
// not the generic 87.413 <=4x4 test. Candidate ROM bank is upper HUD range,
// NOT Stage05 109xxx-110xxx scenery.
inline bool nameFootprint(unsigned priority,int x,int y,
                          unsigned wide,unsigned tall){
    if(priority!=7u || !wide || !tall ||
       wide>16u || tall>3u || y>=43 ||
       y+int(tall*16u)<=16)return false;
    const int right=x+int(wide*16u);
    // Require the sprite ORIGIN to be on its HUD side. A long object
    // spanning from the center into the right name area is not name text.
    return (x>=0 && x<113 && right>20) ||
           (x>=220 && x<366 && right>271);
}
inline bool nameGlyph(unsigned priority,unsigned palette,unsigned tile,
                      int x,int y,unsigned wide,unsigned tall){
    return palette<25u && tile>=80000u && tile<100000u &&
           nameFootprint(priority,x,y,wide,tall);
}
inline bool sidePortrait(unsigned priority,unsigned palette,unsigned tile,
                         int x,int y,unsigned wide,unsigned tall,bool portrait){
    if(priority!=7u || tile<80000u || tile>=100000u ||
       !wide || !tall || wide>9u || tall>5u ||
       y< -48 || y>=56 || palette>=25u)return false;
    return (portrait || palette==12u || palette==15u) &&
           (x<96 || x>=288);
}
// Source-verified portrait frame trio from XM87.41. These frame boxes
// sit ABOVE name-glyph ink, so the 87.414 nameFootprint rule missed them.
inline bool portraitBackdrop(unsigned priority,unsigned palette,unsigned tile,
                             int x,int y){
    return priority==7u && palette==2u &&
           tile>=89113u && tile<=89115u &&
           (y==-2 || y==8) &&
           ((x>=-20 && x<=32) || (x>=348 && x<=388));
}
// CPS2 ROM upper-HUD graphics sheets, bounded by atlas AND location.
// Avoid 87.413's arbitrary "small priority 7" policy: that admitted
// forest/tree objects, which belong to different (109xxx) families.
inline bool hudSheetObject(unsigned priority,unsigned palette,unsigned tile,
                           int x,int y,unsigned wide,unsigned tall){
    if(priority!=7u || palette>=25u || tile<87808u || tile>=90000u ||
       !wide || !tall || wide>16u || tall>5u ||
       y< -48 || y>=56 || x>=384 ||
       x+int(wide*16u)<=0)return false;
    const bool side=(x>=-20 && x<144) || (x>=224 && x<388);
    const bool clock=(x>=136 && x<248 && y>=-16 && y<=40 &&
                      wide<=3u && tall<=2u &&
                      (palette==1u || palette==2u || palette==4u));
    return (y>=-16 && y<=40) && (side || clock);
}
inline bool topHudObject(unsigned priority,unsigned palette,unsigned tile,
                         int x,int y,unsigned wide,unsigned tall,bool portrait,
                         bool scenery,bool transition){
    if(scenery || !wide || !tall || x>=384 ||
       x+int(wide*16u)<=0 || y>=56 ||
       y+int(tall*16u)<=0)return false;
    if(priority==0u)
        return timeIndicator(priority,palette,tile,x,y,wide,tall,transition);
    if(priority!=7u)return false;
    if(verifiedHpStrip(priority,palette,tile,x,y,wide,tall))return true;
    if(y< -48 || palette>=25u ||
       (tile>=109000u && tile<=110500u))return false;
    if(nameGlyph(priority,palette,tile,x,y,wide,tall) ||
       sidePortrait(priority,palette,tile,x,y,wide,tall,portrait) ||
       portraitBackdrop(priority,palette,tile,x,y) ||
       hudSheetObject(priority,palette,tile,x,y,wide,tall))return true;
    // NO broad 02/04 fallback: that is how the forest/tree glitch returned.
    if(transition)return false;
    // In actual combat 06/08 retain older native HUD ornament grouping.
    // Long name sprites are handled above regardless of this size limit.
    return wide<=9u && tall<=5u;
}
inline bool object(unsigned priority,unsigned palette,unsigned tile,
                   int x,int y,unsigned wide,unsigned tall,bool portrait=false,
                   bool scenery=false){
    return topHudObject(priority,palette,tile,x,y,wide,tall,
                        portrait,scenery,false);
}
inline bool verifiedTransitionHud(unsigned priority,unsigned palette,
                                  unsigned tile,int x,int y,
                                  unsigned wide,unsigned tall){
    return topHudObject(priority,palette,tile,x,y,wide,tall,
                        false,false,true);
}
// TEST 87.432: only the PHASE-02/04 source gap is changed. The accepted
// 04->06+23 activation and all 30 dither frames are untouched.
// Phase-02/04 uses narrow upper-HUD SOURCE footprints, never all P7.
inline bool verifiedPrecombatUpperHud(unsigned priority,unsigned palette,
                                      unsigned tile,int x,int y,
                                      unsigned wide,unsigned tall,
                                      bool portrait){
    if(!wide || !tall || x>=384 || x+int(wide*16u)<=0 ||
       y>=56 || y+int(tall*16u)<=0)return false;
    // Time includes lettering/face that isn't always the 87808..87831
    // 2x2 numeral; scope by native palette and upper center clock bounds.
    if(priority==0u)
        return palette==1u && x>=128 && x<256 && y>=-16 &&
               wide<=4u && tall<=4u;
    if(priority!=7u)return false;
    if(verifiedHpStrip(priority,palette,tile,x,y,wide,tall) ||
       portraitBackdrop(priority,palette,tile,x,y))return true;
    if(palette>=25u || tile<80000u || tile>=100000u ||
       y< -48 || (tile>=109000u && tile<=110500u))return false;
    // Portrait artwork has distinct palette-12/15 ink at the physical
    // outermost character positions; don't suppress central VS wall art.
    if((portrait || palette==12u || palette==15u) &&
       (x<=8 || x>=352) && y<=32 && wide<=9u && tall<=5u)
        return true;
    // Native NEW HERO headers and TIME lettering reside at the very top.
    // Deliberately reject large/low-position intro/logo blocks. In
    // particular 3-row tiles at y=0 and 2-row tiles at y=16/32 stay
    // untouched, preserving the 87.417 transition-wall regression guards.
    if(y>=-16 && y<=12 && tall<=2u && wide<=9u){
        const bool left=x>=40 && x<144;
        const bool right=x>=240 && x<360;
        const bool clock=x>=144 && x<240 && wide<=4u;
        return left || right || clock;
    }
    return false;
}

// TEST 87.416: shock-only coverage, derived from the earlier complete
// priority-7 native HUD stream. This is deliberately NOT used by key 9
// (XM87412::nativeHud continues using topHudObject unchanged).
// The game's own palette split identifies priority 7 as upper HUD;
// the ROM stage-05 scenery veto was added subsequently. Preserve both.
inline bool shockHudSource(int phase,unsigned priority,unsigned palette,
                           unsigned tile,int x,int y,unsigned wide,unsigned tall,
                           bool portrait,bool scenery){
    if(scenery || !Timeline::mayMaskNativeHud(phase) ||
       !wide || !tall || x>=384 || x+int(wide*16u)<=0 ||
       y>=56 || y+int(tall*16u)<=0)return false;
    // 87.417's strict HP-only phase-02/04 routing omitted the portrait
    // artwork, NEW HERO labels and clock art even when dither STEP=0.
    // Keep the approved scenery veto and the geometric intro-wall guards.
    if(phase==2 || phase==4)
        return verifiedPrecombatUpperHud(priority,palette,tile,
                                         x,y,wide,tall,portrait);
    if(priority==0u){
        // Time/clock uses priority 0, palette 1. Earlier source whitelists
        // caught the digits but left its adjacent clock graphics visible.
        // The center clock band is not the stage-05 OBJ scenery family.
        return palette==1u && x>=128 && x<256 && y>=-16 &&
               wide<=4u && tall<=4u;
    }
    if(priority!=7u || palette>=25u || y< -48 ||
       (tile>=109000u && tile<=110500u))return false;
    if(topHudObject(priority,palette,tile,x,y,wide,tall,
                    portrait,false,phase==2 || phase==4))return true;
    // Entire native priority-7 HUD on combat phases 06/08. This must
    // never be evaluated for the 02/04 tiled VS transition above.
    if(wide<=9u && tall<=5u)return true;
    // Long multi-cell names including Wolverine; no stage-sized panels.
    return wide<=16u && tall<=3u &&
           nameFootprint(priority,x,y,wide,tall);
}
inline bool objectForPhase(int phase,unsigned priority,unsigned palette,
                           unsigned tile,int x,int y,unsigned wide,unsigned tall,
                           bool portrait=false,bool scenery=false){
    return shockHudSource(phase,priority,palette,tile,x,y,wide,tall,
                          portrait,scenery);
}
static const unsigned char Bayer8[64]={
     0,48,12,60, 3,51,15,63,
    32,16,44,28,35,19,47,31,
     8,56, 4,52,11,59, 7,55,
    40,24,36,20,43,27,39,23,
     2,50,14,62, 1,49,13,61,
    34,18,46,30,33,17,45,29,
    10,58, 6,54, 9,57, 5,53,
    42,26,38,22,41,25,37,21
};
inline bool visible(int x,int y,unsigned step){
    if(step>=RevealFrames)return true;
    if(!step)return false;
    return (step*64u/RevealFrames)>Bayer8[((unsigned)y&7u)*8u+((unsigned)x&7u)];
}
inline unsigned byteBlend(unsigned a,unsigned b,unsigned amount,unsigned total){
    return (a*(total-amount)+b*amount+total/2u)/total;
}
// White blast -> electric blue -> exactly the authored source colors.
// Input/output is 0x00RRGGBB from BurnHighCol (high byte preserved).
inline std::uint32_t shockRGB(std::uint32_t source,unsigned step) {
    if(step>=RevealFrames)return source;
    const unsigned r=(source>>16)&255u,g=(source>>8)&255u,b=source&255u;
    const unsigned lum=(r*77u+g*150u+b*29u+128u)>>8;
    const unsigned blueR=25u+lum/5u;
    const unsigned blueG=92u+lum/3u;
    const unsigned blueB=182u+lum/4u;
    unsigned rr,gg,bb;
    if(step<=12u){
        rr=byteBlend(255u,blueR,step,12u);
        gg=byteBlend(255u,blueG,step,12u);
        bb=byteBlend(255u,blueB,step,12u);
    }else{
        const unsigned amount=step-12u;
        rr=byteBlend(blueR,r,amount,RevealFrames-12u);
        gg=byteBlend(blueG,g,amount,RevealFrames-12u);
        bb=byteBlend(blueB,b,amount,RevealFrames-12u);
    }
    return (source&0xff000000u)|(rr<<16)|(gg<<8)|bb;
}

// Separate specular shock front, not just the visibility dither. Two diagonal
// white-hot sweeps travel from the portrait edges toward the central timer.
// A broad blue corona follows each narrow white core, then all light ends on
// frame 30, restoring the exact unmodified source pixel.
inline int shineDistance(int x,int y,unsigned step){
    // Start ON the actual portrait areas at step 1, not outside the
    // 384px canvas. The first visible materialization frame must already
    // show a white primer before the Bayer pattern becomes dense.
    const int travel=int(step*174u/RevealFrames);
    const int slant=(y-28)/3;
    const int left=18+travel+slant;
    const int right=366-travel-slant;
    const int a=x>=left?x-left:left-x;
    const int b=x>=right?x-right:right-x;
    return a<b?a:b;
}
// The leading white filament is a separate reveal mask: it briefly cuts
// through the Bayer gaps while remaining restricted to actual HUD ink.
inline bool shineCore(int x,int y,unsigned step){
    return step>0u && step<RevealFrames && shineDistance(x,y,step)<=3;
}
inline unsigned clamp255(unsigned value){return value>255u?255u:value;}
inline std::uint32_t shineRGB(std::uint32_t source,int x,int y,unsigned step){
    if(step==0u || step>=RevealFrames)return source;
    const std::uint32_t base=shockRGB(source,step);
    const int distance=shineDistance(x,y,step);
    if(distance>30)return base;
    const unsigned r=(base>>16)&255u,g=(base>>8)&255u,b=base&255u;
    unsigned rr=r,gg=g,bb=b;
    if(distance<=3){
        // Hard, glossy white filament at the moving leading edge.
        const unsigned w=distance==0?255u:distance==1?245u:
                         distance==2?225u:200u;
        rr=byteBlend(r,255u,w,255u);
        gg=byteBlend(g,255u,w,255u);
        bb=byteBlend(b,255u,w,255u);
    }else if(distance<=12){
        // Icy electric-blue bloom immediately behind the white crest.
        const unsigned power=(unsigned)(13-distance)*18u;
        rr=clamp255(r+power/4u);
        gg=clamp255(g+power*3u/4u);
        bb=clamp255(b+power);
    }else{
        // The wider soft corona is source-pixel scoped and cannot tint
        // the arena underneath a suppressed dither pixel.
        const unsigned power=(unsigned)(31-distance)*4u;
        rr=clamp255(r+power/6u);
        gg=clamp255(g+power/2u);
        bb=clamp255(b+power);
    }
    return (source&0xff000000u)|(rr<<16)|(gg<<8)|bb;
}

// Snapshot only 16x16 pixels of the *selected HUD tile*. Restoring removed
// pixels and Z positions reveals the real scenery/fighters underneath instead
// of drawing an opaque rectangular overlay across the stage.
struct Tile {
    int x=0,y=0,w=0,h=0,pitch=0,screenWidth=0;
    std::uint32_t before[256];
    std::uint16_t zBefore[256];
    bool depthValid=false;
    void save(const std::uint8_t* pixels,int rowPitch,
              const std::uint16_t* depth,int sw,int sh,int tx,int ty){
        x=tx;y=ty;pitch=rowPitch;screenWidth=sw;
        w=h=16; // clip each pixel below; care tiles may cross screen edges
        depthValid=depth!=nullptr;
        if(!pixels || rowPitch<sw*4){w=h=0;return;}
        for(int yy=0;yy<16;++yy)for(int xx=0;xx<16;++xx){
            const int px=tx+xx,py=ty+yy;
            if(px<0 || py<0 || px>=sw || py>=sh)continue;
            const unsigned index=(unsigned)yy*16u+(unsigned)xx;
            std::memcpy(&before[index],pixels+(std::size_t)py*rowPitch+px*4,4);
            if(depth)zBefore[index]=depth[(std::size_t)py*sw+px];
        }
    }
    void finish(std::uint8_t* pixels,std::uint16_t* depth,
                int sw,int sh,unsigned step)const{
        if(!pixels || !w || !h || step>=RevealFrames)return;
        for(int yy=0;yy<16;++yy)for(int xx=0;xx<16;++xx){
            const int px=x+xx,py=y+yy;
            if(px<0 || py<0 || px>=sw || py>=sh)continue;
            const unsigned index=(unsigned)yy*16u+(unsigned)xx;
            std::uint8_t* dest=pixels+(std::size_t)py*pitch+px*4;
            std::uint32_t after=0u;
            std::memcpy(&after,dest,4);
            // The specular filament reveals only real HUD ink even on
            // Bayer-hidden coordinates; this is visibly distinct from the
            // ordered dither and never draws across transparent scenery.
            const bool ink=after!=before[index];
            const bool reveal=visible(px,py,step) || (ink && shineCore(px,py,step));
            if(!reveal){
                std::memcpy(dest,&before[index],4);
                if(depth && depthValid)depth[(std::size_t)py*sw+px]=zBefore[index];
            }else if(ink){
                const std::uint32_t tinted=shineRGB(after,px,py,step);
                std::memcpy(dest,&tinted,4);
            }
        }
    }
};

// Native SDL stun artwork is premultiplied neither by the emulator palette
// nor the CPS2 Z buffer. Apply the SAME Bayer positions and clock to its alpha.
inline void stunOverlay(std::uint8_t* rgba,int pitch,unsigned step){
    if(!rgba || pitch<384*4 || step>=RevealFrames)return;
    for(int y=43;y<=54;++y)for(int x=0;x<384;++x){
        std::uint8_t* p=rgba+(std::size_t)y*pitch+x*4;
        if(!p[3])continue;
        if(!visible(x,y,step) && !shineCore(x,y,step)){
            p[3]=0;continue;
        }
        const std::uint32_t native=(std::uint32_t(p[0])<<16)|
                                   (std::uint32_t(p[1])<<8)|p[2];
        const std::uint32_t tinted=shineRGB(native,x,y,step);
        p[0]=(unsigned char)((tinted>>16)&255u);
        p[1]=(unsigned char)((tinted>>8)&255u);
        p[2]=(unsigned char)(tinted&255u);
    }
}
}
#endif
