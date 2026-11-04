#ifndef XMVSF_STAGE25_5_UI_H
#define XMVSF_STAGE25_5_UI_H
// Native pixel presentation layer for the Stage25 Versus setup/results/scoreboard.
// Uses the existing Stage25.1 glyphs and SDL renderer; no game ROM or audio changes.
#include <SDL.h>
#include "xmvsf_stage86_55_options.h" /* universal menu grayscale */
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

struct XM255RGB { Uint8 r,g,b; };
static inline XM255RGB xm255Rgb(Uint8 r,Uint8 g,Uint8 b){return {r,g,b};}
static inline XM255RGB xm255ModeColor(int target){
    switch(target){
        case 3: return xm255Rgb(90,221,138);   // FT3 green
        case 5: return xm255Rgb(252,222,94);   // FT5 yellow
        case 10:return xm255Rgb(255,160,73);   // FT10 orange
        case 20:return xm255Rgb(255,91,103);   // FT20 red
        default:return xm255Rgb(90,176,255);  // FFA blue
    }
}
static inline void xm255Rect(SDL_Renderer* r,int x,int y,int w,int h,XM255RGB col, bool fill=true){
    if(!r || w<=0 || h<=0)return;
    xm8655RenderColor(r,col.r,col.g,col.b);
    SDL_Rect rc={x,y,w,h};
    if(fill)SDL_RenderFillRect(r,&rc);else SDL_RenderDrawRect(r,&rc);
}
static inline void xm255Text(SDL_Renderer* r,const char* s,int x,int y,int maxWidth,int scale,XM255RGB col){
    if(!r||!s||scale<1||maxWidth<6*scale)return;
    xm8655RenderColor(r,col.r,col.g,col.b);
    const int step=6*scale, count=maxWidth/step;
    for(int i=0;s[i] && i<count;++i){
        const unsigned char* rows=xm251Glyph(s[i]);
        if(!rows)continue;
        for(int yy=0;yy<7;++yy)for(int xx=0;xx<5;++xx){
            if(rows[yy]&(1u<<(4-xx))){SDL_Rect px={x+i*step+xx*scale,y+yy*scale,scale,scale};SDL_RenderFillRect(r,&px);}
        }
    }
}
static inline int xm255MaxPage(){return xm25State.entries.empty()?0:((int)xm25State.entries.size()-1)/7;}
static inline void xm255BoundPage(){xm25State.page=std::max(0,std::min(xm255MaxPage(),xm25State.page));}
static inline const char* xm255Status(const XM25LedgerRow& r){
    if(r.status=="COMPLETE")return "COMPLETE";
    if(r.status=="FFA_FINAL")return "FINAL";
    if(r.status=="ABANDONED")return "LEFT EARLY";
    return "HISTORY";
}
static inline void xm255Date(const XM25LedgerRow& row,char* dst,size_t cap){
    const std::time_t t=(std::time_t)row.epoch;
    const std::tm* d=std::localtime(&t);
    if(d)std::snprintf(dst,cap,"%02d/%02d/%04d",d->tm_mon+1,d->tm_mday,d->tm_year+1900);
    else std::snprintf(dst,cap,"--/--/----");
}
static inline void xm255BootVersus(SDL_Renderer* r){
    const XM255RGB fg=xm255Rgb(225,231,254), muted=xm255Rgb(145,167,202),edge=xm255Rgb(72,116,195);
    xm255Rect(r,66,22,828,503,xm255Rgb(8,13,31));
    xm255Rect(r,66,22,828,503,edge,false);
    xm255Rect(r,82,38,5,30,xm255Rgb(88,175,255));
    xm255Text(r,"VERSUS",103,42,350,3,fg);
    xm255Text(r,"CHOOSE A MATCH TYPE",550,50,300,2,muted);
    xm255Rect(r,94,81,772,1,edge);
    const char* modes[5]={"FFA","FT3","FT5","FT10","FT20"};
    for(int i=0;i<5;++i){
        const int y=100+i*47;const bool current=xm25State.setupRow==i;
        xm255Rect(r,220,y-7,520,35,current?xm255Rgb(29,51,91):xm255Rgb(15,24,46));
        if(current)xm255Rect(r,220,y-7,5,35,xm255ModeColor(xm25TargetAt(i)));
        xm255Text(r,current?">":" ",238,y,25,2,fg);
        xm255Text(r,modes[i],282,y-1,142,3,xm255ModeColor(xm25TargetAt(i)));
        char desc[65];
        std::snprintf(desc,sizeof(desc),"%s",i==0?"FREE FOR ALL":i==1?"FIRST TO THREE":i==2?"FIRST TO FIVE":i==3?"FIRST TO TEN":"FIRST TO TWENTY");
        xm255Text(r,desc,440,y+5,277,2,muted);
    }
    for(int i=5;i<=6;++i){
        const int y=i==5?351:399; const bool current=xm25State.setupRow==i;
        xm255Rect(r,220,y-6,520,34,current?xm255Rgb(29,51,91):xm255Rgb(15,24,46));
        if(current)xm255Rect(r,220,y-6,5,34,xm255Rgb(111,192,255));
        xm255Text(r,current?">":" ",238,y,24,2,fg);
        xm255Text(r,i==5?"SCOREBOARD":"MAIN MENU",276,y,380,2,current?fg:muted);
    }
    for(int p=0;p<2;++p){
        int left=p==0?85:510;
        xm255Rect(r,left,459,290,38,xm255Rgb(18,28,53));
        xm255Rect(r,left,459,290,38,xm25State.focus==p?xm255Rgb(255,211,109):edge,false);
        xm255Text(r,p==0?"P1":"P2",left+10,465,35,2,xm255Rgb(108,190,255));
        xm255Text(r,xm25State.names[p],left+51,469,225,2,fg);
        xm255Rect(r,left+295,459,40,38,xm255Rgb(37,30,55));
        xm255Rect(r,left+295,459,40,38,xm255Rgb(191,95,115),false);
        xm255Text(r,"X",left+307,470,20,2,xm255Rgb(255,140,150));
    }
    xm255Text(r,"UP/DOWN MOVE  A SELECT  B BACK",115,507,740,1,muted);
}
static inline void xm255BootScoreboard(SDL_Renderer* r){
    const XM255RGB fg=xm255Rgb(227,235,254),muted=xm255Rgb(149,174,207),edge=xm255Rgb(75,116,190);
    xm255Rect(r,66,22,828,503,xm255Rgb(8,13,31));
    xm255Rect(r,66,22,828,503,edge,false);
    xm255Rect(r,82,36,5,30,xm255Rgb(100,190,255));
    xm255Text(r,"SCOREBOARD",102,38,650,3,fg);
    char count[50];std::snprintf(count,sizeof(count),"%d SESSIONS",(int)xm25State.entries.size());
    xm255Text(r,count,666,51,183,2,muted);
    xm255Rect(r,94,79,772,1,edge);
    xm255BoundPage();
    for(int j=0;j<7;++j){
        int ix=(int)xm25State.entries.size()-1-(xm25State.page*7+j);
        if(ix<0)break;
        const XM25LedgerRow& row=xm25State.entries[(size_t)ix];
        const int top=86+j*51;
        xm255Rect(r,86,top,788,44,j%2?xm255Rgb(17,27,50):xm255Rgb(25,38,66));
        XM255RGB tint=xm255ModeColor(row.target);
        xm255Rect(r,86,top,5,44,tint);
        xm255Text(r,xm25ModeName(row.target),100,top+8,100,2,tint);
        char date[32];xm255Date(row,date,sizeof(date));
        char id[50];std::snprintf(id,sizeof(id),"SET %lld",row.id);
        xm255Text(r,id,206,top+3,160,2,muted);
        xm255Text(r,date,378,top+3,177,2,muted);
        xm255Text(r,xm255Status(row),701,top+4,152,1,tint);
        xm255Text(r,row.name1.c_str(),204,top+24,206,2,fg);
        char score[40];std::snprintf(score,sizeof(score),"%d - %d",row.p1,row.p2);
        xm255Text(r,score,444,top+24,120,2,xm255Rgb(255,218,123));
        xm255Text(r,row.name2.c_str(),575,top+24,260,2,fg);
    }
    if(xm25State.entries.empty())xm255Text(r,"NO RECORDED SESSIONS YET",239,256,600,2,muted);
    char page[52];std::snprintf(page,sizeof(page),"PAGE %d / %d",xm25State.page+1,xm255MaxPage()+1);
    xm255Text(r,"< PREV",100,466,160,2,muted);
    xm255Text(r,page,368,466,246,2,fg);
    xm255Text(r,"NEXT >",698,466,150,2,muted);
    xm255Text(r,"LEFT/RIGHT PAGE  B BACK   P1/P2 SCORES",115,504,740,1,muted);
}
static inline void xm255InGameScoreboard(SDL_Renderer* r,int x,int y,int w){
    const XM255RGB fg=xm255Rgb(226,238,255),muted=xm255Rgb(153,174,205);
    xm255Text(r,"SCOREBOARD",x,y,w,2,fg);
    xm255BoundPage();
    for(int i=0;i<7;++i){
        const int idx=(int)xm25State.entries.size()-1-(xm25State.page*7+i);
        if(idx<0)break;
        const XM25LedgerRow& v=xm25State.entries[(size_t)idx];
        const int top=y+20+i*22;
        xm255Rect(r,x,top,w,21,i%2?xm255Rgb(21,35,61):xm255Rgb(30,46,80));
        XM255RGB tint=xm255ModeColor(v.target);
        xm255Rect(r,x,top,3,21,tint);
        xm255Text(r,xm25ModeName(v.target),x+6,top+1,32,1,tint);
        char head[68],date[24];xm255Date(v,date,sizeof(date));
        std::snprintf(head,sizeof(head),"%s  SET %lld  %s",date,v.id,xm255Status(v));
        xm255Text(r,head,x+46,top+1,w-51,1,muted);
        char score[110];std::snprintf(score,sizeof(score),"%.12s  %d - %d  %.12s",v.name1.c_str(),v.p1,v.p2,v.name2.c_str());
        xm255Text(r,score,x+8,top+11,w-12,1,fg);
    }
    if(xm25State.entries.empty())xm255Text(r,"NO RECORDED SESSIONS",x+12,y+64,w-20,1,muted);
    char footer[80];std::snprintf(footer,sizeof(footer),"< PREV   PAGE %d/%d   NEXT >",xm25State.page+1,xm255MaxPage()+1);
    xm255Text(r,footer,x,y+179,w,1,fg);
    xm255Text(r,"UP/DOWN PAGE  B BACK",x,y+191,w,1,muted);
}
// Kept only as a compatibility API for older menu call sites.
// Stage 26 never launches a child process or terminates the running program.
extern bool xm26ReturnToBoot;
static inline bool xm255RestartAtBoot(){
    xm26ReturnToBoot=true;
    return true;
}
#endif

/* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
