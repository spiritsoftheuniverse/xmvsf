#ifndef XMVSF_STAGE87_140_CHAT_OVERLAY_H
#define XMVSF_STAGE87_140_CHAT_OVERLAY_H
// Warcraft-style combat chat, presentation only. Does NOT intercept native
// pixels, change CPS2 palettes, or depend on DF's sprite ownership filter.
// New lines fade in place and expire after 180 native frames (~3s at 60fps).
// Shift+G expands/collapses persistent bounded message history.
#include <SDL.h>
#include <algorithm>
#include <cstring>
#include "../../../burner/sdl/xmvsf_stage87_140_super_chat.h"
static inline void xm87140ChatOverlay(SDL_Renderer* render){
    if(!render)return;
    const XM87140Chat::State& st=XM87140Chat::state();
    const unsigned now=st.lastFrame;
    const unsigned available=st.visibleLines(now);
    if(!available)return;
    SDL_Texture* font=get_inline_font();
    if(!font)return;
    int width=0,height=0;
    SDL_RenderGetLogicalSize(render,&width,&height);
    if(width<=0||height<=0)SDL_GetRendererOutputSize(render,&width,&height);
    if(width<250||height<160)return;
    const unsigned shown=std::min<unsigned>(available,st.open?
        unsigned(std::min(12,(height-70)/16)):5u);
    if(!shown)return;
    const int left=15;
    const int boxWidth=std::min(610,width-30);
    const int lineHeight=16;
    const int boxHeight=int(shown)*lineHeight+26;
    const int top=height-boxHeight-21;
    if(top<10)return;
    SDL_BlendMode oldBlend=SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(render,&oldBlend);
    Uint8 oldR=0,oldG=0,oldB=0,oldA=0;
    SDL_GetRenderDrawColor(render,&oldR,&oldG,&oldB,&oldA);
    Uint8 fr=255,fg=255,fb=255,fa=255;
    SDL_GetTextureColorMod(font,&fr,&fg,&fb);
    SDL_GetTextureAlphaMod(font,&fa);
    SDL_SetRenderDrawBlendMode(render,SDL_BLENDMODE_BLEND);
    SDL_Rect box={left,top,boxWidth,boxHeight};
    SDL_SetRenderDrawColor(render,8,13,23,st.open?214:142);
    SDL_RenderFillRect(render,&box);
    SDL_SetRenderDrawColor(render,83,131,190,st.open?194:132);
    SDL_RenderDrawRect(render,&box);
    SDL_Rect topStrip={left+1,top+1,boxWidth-2,2};
    SDL_SetRenderDrawColor(render,79,144,205,st.open?230:160);
    SDL_RenderFillRect(render,&topStrip);
    SDL_SetTextureColorMod(font,210,176,90);
    SDL_SetTextureAlphaMod(font,230);
    inprint_shadowed(render,st.open?"DHC COMMAND LOG [SHIFT+G CLOSE]":
                      "DHC COMMANDS  [SHIFT+G HISTORY]",left+8,top+7);
    // Newest at the bottom. 'open' shows the entire last twelve messages
    // regardless of age; normal chat shows up to five recent messages.
    const unsigned capacity=XM87140Chat::CHAT_CAPACITY;
    const int maxChars=(boxWidth-16)/8;
    for(unsigned j=0;j<shown;++j){
        const unsigned ago=shown-1u-j;
        const XM87140Chat::Line& line=st.lines[(st.write+capacity-1u-ago)%capacity];
        const unsigned age=(now>=line.frame)?now-line.frame:180u;
        if(!st.open && age>=180u)continue;
        Uint8 alpha=245;
        if(!st.open&&age>120u)
            alpha=Uint8((180u-age)*245u/60u);
        Uint8 cr=220,cg=225,cb=226;
        if(line.kind==XM87140Chat::ALERT){cr=255;cg=196;cb=92;}
        else if(line.kind==XM87140Chat::INPUT){cr=168;cg=204;cb=255;}
        else if(line.kind==XM87140Chat::MATCH){cr=113;cg=239;cb=149;}
        SDL_SetTextureColorMod(font,cr,cg,cb);
        SDL_SetTextureAlphaMod(font,alpha);
        char shortLine[XM87140Chat::CHAT_LENGTH];
        std::snprintf(shortLine,sizeof(shortLine),"%s",line.text);
        if(maxChars>=1 && maxChars<int(sizeof(shortLine)))
            shortLine[maxChars]=0;
        inprint_shadowed(render,shortLine,left+9,top+23+int(j)*lineHeight);
    }
    SDL_SetTextureColorMod(font,fr,fg,fb);
    SDL_SetTextureAlphaMod(font,fa);
    SDL_SetRenderDrawBlendMode(render,oldBlend);
    SDL_SetRenderDrawColor(render,oldR,oldG,oldB,oldA);
}
#endif
