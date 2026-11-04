
#ifndef XMVSF_STAGE87_76_SELECT_NUM5_H
#define XMVSF_STAGE87_76_SELECT_NUM5_H
namespace XM8776Select {
static inline bool Bounds(int x,int y,unsigned wide,unsigned tall){
 return !(x>=384 || y>=224 || x+int(wide*16u)<=0 || y+int(tall*16u)<=0);
}
static inline bool CharacterDisplay(unsigned phase,unsigned priority,unsigned palette,
                           unsigned tile,int x,int y,unsigned wide,unsigned tall,
                           bool hud,bool scenery){
 if(tile>=0xBA00u && tile<=0xBA7Fu) return false; // grid portrait family
 if(phase>1u || priority!=7u || hud || scenery || !Bounds(x,y,wide,tall)) return false;
 if(palette==27u) return false;
 const bool sideRegion = (x < 176) || (x >= 208);
 const bool fighterSized = (wide >= 2u && tall >= 4u) || (wide >= 3u && tall >= 3u);
 const bool nativePal = palette >= 8u && palette <= 26u;
 return sideRegion && fighterSized && nativePal && y >= 8 && y < 216;
}
static inline bool Portrait(unsigned phase,unsigned priority,unsigned palette,
                           unsigned tile,int x,int y,unsigned wide,unsigned tall,
                           bool hud,bool scenery){
 if(phase>1u || priority!=7u || hud || scenery || !Bounds(x,y,wide,tall)) return false;
 return tile>=0xBA00u && tile<=0xBA7Fu && palette>=4u && palette<=26u &&
        wide>=2u && tall>=2u && x>=80 && x<=352 && y>=64 && y<224;
}
static inline bool Subject(unsigned phase,unsigned priority,unsigned palette,
                           unsigned tile,int x,int y,unsigned wide,unsigned tall,
                           bool hud,bool scenery){
 return CharacterDisplay(phase,priority,palette,tile,x,y,wide,tall,hud,scenery);
}
}
#endif
