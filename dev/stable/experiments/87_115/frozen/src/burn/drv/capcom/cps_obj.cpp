#include "xmvsf_stage86_54_object_filter.h"
#include "xmvsf_stage86_60_noir.h"
#include "xmvsf_stage86_65_classify.h" // XM86.65 verified scenery overrides and intro logo
#include "xmvsf_stage86_66_fx_mirror.h" // XM86.67 exact-tile match only
#include "xmvsf_stage86_67_fx_isolation.h" // XM86.67 mixed-sprite pen isolation
#include "xmvsf_stage86_69_fx_index.h" // XM86.69 P1_FX_INDEX: capture-derived exact tile+palette
#include "xmvsf_stage87_01_lighting_artwork_veto.h" // Reject recolored fighter artwork in lighting/FX bank
#include "xmvsf_stage87_02_fx_source_gate.h"
#include "xmvsf_stage87_06_fx_ramp.h" // Preserve actual pink/green FX palette gradient highlights
#include "xmvsf_stage87_05_pen_usage.h" // XR87.05 ACTUAL_PEN_USAGE (diagnostic only) // Seed P1 mirror only from post-veto P2 FX
#include "cps.h"
#include "../../../burner/sdl/xmvsf_stage87_98_latency_trace.h" // XM87.98 DF object draw passes
#include "xmvsf_stage87_17_provenance.h"
extern "C" bool xm8746DizzyHudAllowed(); // SDL game-wide + Training option
#include "xmvsf_stage87_18_source_color.h"
#include "xmvsf_stage87_20_secondary_ownership.h" // Writer-backed copies vs independent non-body FX
#include "xmvsf_stage87_21_exact_artwork.h" // Exact shadow copies + independent effects
#include "xmvsf_stage87_22_echo_source.h" // Identity of known secondary afterimage sources
#include "xmvsf_stage87_23_echo_mono.h" // Three-echo monochrome fallback, not a palette blanket
#include "xmvsf_stage87_32_stun_probe.h"
#include "xmvsf_stage87_45_hp_rgb_and_opening.h"
#include "../../../burner/sdl/xmvsf_stage87_108_lifecycle.h"
#include "xmvsf_stage87_47_movement_cues.h"
#include "xmvsf_stage87_55_super_probe.h" // Opt-in read-only ROM-action discovery
#include "xmvsf_stage87_59_global_super_audit.h" // global evidence, no gameplay changes
#include "xmvsf_stage87_60_native_object_audit.h" // opt-in native OAM, zero gameplay changes
#include "xmvsf_stage87_66_select_oam.h"
#include "xmvsf_stage87_74_native_selector.h" // XM87.74 aligned native tile and occlusion-aware overlay // XM87.69 diagnostic player-color selector markers // XM87.66 original selection sprite history, no masking
#include "xmvsf_stage87_46_hud_layers.h"
#include "xmvsf_stage87_76_select_num5.h" // XM87.77 selection sprites // Priority-aware HUD and selection Num5
#include "xmvsf_stage87_25_direct_outline.h" // Fallback direct four-way outline
#include "xmvsf_stage87_26_seamless_bloom.h" // Disabled Stage87.27 bloom legacy
#include "xmvsf_stage87_28_interior_mask.h" // Bright direct copies with body-union stencil
#include "xmvsf_stage87_08_group_fx.h" // XM87.08 large FX sprite-run RGB restoration
#include "xmvsf_stage87_07_owner_trace.h" // Opt-in game-RAM actors vs buffered OBJ list; no color change
#include "xmvsf_stage87_09_fragment_trace.h" // XM87.09 diagnostic: shorter OBJ groups during Tempest
#include "xmvsf_stage87_10_tempest_fragments.h"
#include "xmvsf_stage87_11_fighter_body_subtract.h" // original RGB except verified actor-anchored body groups // XM87.10 dense drifting Tempest fragment RGB
#include <unordered_map> // XM86.65 opt-in per-object classification trace
#include <unordered_set> // XM87.05 unique artwork/grade signatures
#include <cstring> // XM87.05 std::strcmp for renderer grade diagnostics
#include <cstdint> // XM86.65 trace key
#include <cstdio> // Stage 44.1: std::fprintf used by the sprite-list exporter
#include "xmvsf3v3.h"
extern "C" const unsigned char* xm8774GetNativeSelectorOverlay(int*w,int*h,int*pitch,
    const unsigned char*native,int nativePitch,int nativeBpp){
    return XM8773::State().ReadChecked(w,h,pitch,native,nativePitch,nativeBpp);
}

// CPS Objs (sprites)

INT32 nCpsObjectBank;
// 87.63: most recent ORIGINAL, unfiltered CPS2 sprite-list count,
// exported for the read-only cinematic-intro restoration probe.
unsigned xm8763NativeObjectCount=0;
unsigned xm8763NativeObjectFrame=~0u;
INT32 Sfa2ObjHack = 0;

UINT8 *CpsBootlegSpriteRam = NULL;

INT32 Cps1LockSpriteList910000 = 0;
INT32 Cps1DetectEndSpriteList8000 = 0;

Cps1ObjGetCallback Cps1ObjGetCallbackFunction = NULL;
Cps1ObjDrawCallback Cps1ObjDrawCallbackFunction = NULL;

// Our copy of the sprite table
static UINT8 *ObjMem = NULL;

static INT32 nMax = 0;
static INT32 nGetNext = 0;

static INT32 nMaxZValue;
static INT32 nMaxZMask;

static INT32 nZOffset;

// Object frames, so you can lag the Objs by nFrameCount-1 frames
struct ObjFrame {
	INT32 nShiftX, nShiftY;
	UINT8* Obj;
	INT32 nCount;
	XM8717::Source xm8717Sources[0x400]; // metadata parallel to compressed OBJ list
};

static INT32 nFrameCount = 0;
static struct ObjFrame of[3];

static UINT8 *blendtable;

static void CpsBlendInit()
{
	blendtable = NULL;

	TCHAR filename[MAX_PATH];

	_stprintf(filename, _T("%s%s.bld"), szAppBlendPath, BurnDrvGetText(DRV_NAME));

	FILE *fa = _tfopen(filename, _T("rt"));

	INT32 is_sfz3mix = strstr("sfz3mix", BurnDrvGetTextA(DRV_NAME)) != NULL;

	if (fa == NULL) {
		if (is_sfz3mix) {
			// for sfz3mix, don't use parent's blend table. sfz3mix uses different
			// sprite indexing
			return;
		}

		_stprintf(filename, _T("%s%s.bld"), szAppBlendPath, BurnDrvGetText(DRV_PARENT));

		fa = _tfopen(filename, _T("rt"));

		if (fa == NULL) {
			return;
		}
	}

	bprintf (PRINT_IMPORTANT, _T("Using sprite blending (.bld) table!\n"));

	blendtable = (UINT8*)BurnMalloc(0x40000); // maximum number of sprites
	memset (blendtable, 0, 0x40000); // no blend

	char szLine[64];

	INT32 table[4] = { 0, 0xff-0x3f, 0xff-0x7f, 0xff-0x7f }; // last one 7f?

	while (1)
	{
		if (fgets (szLine, 64, fa) == NULL) break;

		if (strncmp ("Game", szLine, 4) == 0) continue; 	// don't care
		if (strncmp ("Name", szLine, 4) == 0) continue; 	// don't care
		if (szLine[0] == ';') continue;				// comment (also don't care)

		INT32 type, single_entry = -1;
		UINT32 min,max,k;

		for (k = 0; k < strlen(szLine); k++) {
			if (szLine[k] == '-') { single_entry = k+1; break; }
		}

		if (single_entry < 0) {
			sscanf(szLine,"%x %d",&max,&type);
			min = max;
		} else {
			sscanf(szLine,"%x",&min);
			sscanf(szLine+single_entry,"%x %d",&max,&type);
		}

		for (k = min; k <= max; k++) {
			if (k < 0x40000)
				blendtable[k] = table[type&3];
		}
	}

	fclose (fa);
}

INT32 CpsObjInit()
{
	if (bBurnUseBlend) CpsBlendInit();

	nMax = 0x100;				// CPS1 has 256 sprites

	if (Cps == 2) {				// CPS2 has 1024 sprites
		nMax = 0x400;
	}

	nFrameCount = 2;			// CPS2 sprites lagged by 1 frame and double buffered
								// CPS1 sprites lagged by 1 frame

	ObjMem = (UINT8*)BurnMalloc((nMax << 3) * nFrameCount);
	if (ObjMem == NULL) {
		return 1;
	}

	// xmvsf3v3: every buffer its states hold starts the same way
	if (CpsXmvSixEnabled) memset(ObjMem, 0, (nMax << 3) * nFrameCount);

	// Set up the frame buffers
	for (INT32 i = 0; i < nFrameCount; i++) {
		of[i].Obj = ObjMem + (nMax << 3) * i;
		of[i].nCount = 0;
		if (CpsXmvSixEnabled) of[i].nShiftX = of[i].nShiftY = 0;
	}

	nGetNext=0;

	if (Cps == 2) {
		memset(ZBuf, 0, nCpsScreenWidth * nCpsScreenHeight * 2);
		nMaxZMask = nZOffset = 0;
		nMaxZValue = 1;
	}

	return 0;
}

INT32 CpsObjExit()
{
	if (blendtable)
		BurnFree(blendtable);

	for (INT32 i = 0; i < nFrameCount; i++) {
		of[i].Obj = NULL;
		of[i].nCount = 0;
	}

	BurnFree(ObjMem);

	nFrameCount = 0;
	nMax = 0;
	
	Cps1DetectEndSpriteList8000 = 0;
	
	Cps1ObjGetCallbackFunction = NULL;
	Cps1ObjDrawCallbackFunction = NULL;

	return 0;
}

// Get CPS sprites into Obj
INT32 CpsObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	if (Cps1ObjGetCallbackFunction) {
		return Cps1ObjGetCallbackFunction();
	}

	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	if (Cps == 2) {
		Get = CpsRam708 + ((nCpsObjectBank ^ 1) << 15);		// Select CPS2 sprite buffer
//		Get = CpsRam708 + ((GetCurrentFrame() & 1) << 15);	// Select CPS2 sprite buffer

		pof->nShiftX = (-CpsSaveFrg[0][0x9]) + nCpsGlobalXOffset;
//		bprintf(0, _T("shift-x  %d\n"), pof->nShiftX);
		pof->nShiftY = (-CpsSaveFrg[0][0xB]) + nCpsGlobalYOffset;
	} else {
		INT32 nOff = BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x00))) << 8;
		nOff &= 0xfff800;
		Get = CpsFindGfxRam(nOff, 0x800);		
		
		if (Cps1LockSpriteList910000) {
			Get = CpsFindGfxRam(0x910000, 0x800);
		}
		pof->nShiftX += nCpsGlobalXOffset;
		pof->nShiftY += nCpsGlobalYOffset;
	}
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;

		if (Cps == 2) {
			if (BURN_ENDIAN_SWAP_INT16(ps[1]) & 0x8000)	{														// end of sprite list
				break;
			}
			if (BURN_ENDIAN_SWAP_INT16(ps[3]) >= 0xff00) {														// end of sprite list (ringdest)
				break;
			}
		} else {
			if (BURN_ENDIAN_SWAP_INT16(ps[3]) >= 0xff00) {													// end of sprite list
				break;
			}
			if (Cps1DetectEndSpriteList8000) {
				if (BURN_ENDIAN_SWAP_INT16(ps[1]) & 0x8000) {
					break;
				}
			}
		}
		
		if ((BURN_ENDIAN_SWAP_INT16(ps[0]) | BURN_ENDIAN_SWAP_INT16(ps[3])) == 0) {													// sprite blank
			continue;
		}

		// Okay - this sprite is active:
		memcpy(po, pg, 8); // copy it over
		// XM87.17: Preserve the source bank + original slot through compression.
		// CpsObjGet() skips blanks, so output slot is NOT raw object-RAM slot.
		if (Cps==2 && XM8717::enabled) {
			const XM8717::Source& source=XM8717::slots[(nCpsObjectBank^1)&1][i];
			pof->xm8717Sources[pof->nCount]=XM8717::Recent(source,(unsigned)GetCurrentFrame())
				? source : XM8717::Source{};
		}

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}

	return 0;
}

void CpsObjDrawInit()
{
	nZOffset = nMaxZMask;

	if (nZOffset >= 0xFC00) {
		// The Z buffer might moverflow the next fram, so initialise it
		memset(ZBuf, 0, nCpsScreenWidth * nCpsScreenHeight * 2);
		nZOffset = 0;
	}

	nMaxZValue = nZOffset + 1;
	nMaxZMask = nZOffset;

	return;
}

INT32 Cps1ObjDraw(INT32 nLevelFrom,INT32 nLevelTo)
{
	INT32 i; UINT16 *ps; INT32 nPsAdd;
	struct ObjFrame *pof;
	(void)nLevelFrom; (void)nLevelTo;
	
	if (Cps1ObjDrawCallbackFunction) {
		return Cps1ObjDrawCallbackFunction(nLevelFrom, nLevelTo);
	}

	// Draw the earliest frame we have in history
	pof=of+nGetNext;

	// Point to Obj list
	ps=(UINT16 *)pof->Obj;

	if (!CpsDrawSpritesInReverse) {
		ps+=(pof->nCount-1)<<2; nPsAdd=-4; // CPS1 is reversed
	} else {
		nPsAdd=4;
	}

	// Go through all the Objs
	for (i=0; i<pof->nCount; i++,ps+=nPsAdd) {
		INT32 x,y,n,a,bx,by,dx,dy; INT32 nFlip;

		x = BURN_ENDIAN_SWAP_INT16(ps[0]); y = BURN_ENDIAN_SWAP_INT16(ps[1]); n = BURN_ENDIAN_SWAP_INT16(ps[2]); a = BURN_ENDIAN_SWAP_INT16(ps[3]);
			
		// Find out sprite size
		bx=((a>> 8)&15)+1;
		by=((a>>12)&15)+1;
		
		n = GfxRomBankMapper(GFXTYPE_SPRITES, n);
		if (n == -1) continue;
		
		n |= (y & 0x6000) << 3; // high bits of address
		
		// CPS1 coords are 9 bit signed?
		x&=0x01ff; if (x>=0x1c0) x-=0x200;
		y&=0x01ff; y^=0x100; y-=0x100;

		x+=pof->nShiftX;
		y+=pof->nShiftY;

		// Find the palette for the tiles on this sprite
		CpstPal = CpsPal + ((a & 0x1F) << 4);

		nFlip=(a>>5)&3;		

		// Take care with tiles if the sprite goes off the screen
		if (x<0 || y<0 || x+(bx<<4)>(nCpsScreenWidth-1) || y+(by<<4)>(nCpsScreenHeight-1)) {
			nCpstType=CTT_16X16 | CTT_CARE;
		} else {
			nCpstType=CTT_16X16;
		}

		nCpstFlip=nFlip;
		for (dy=0;dy<by;dy++) {
			for (dx=0;dx<bx;dx++) {
				INT32 ex,ey;
				if (nFlip&1) ex=(bx-dx-1);
				else ex=dx;
				if (nFlip&2) ey=(by-dy-1);
				else ey=dy;

				nCpstX=x+(ex<<4);
				nCpstY=y+(ey<<4);
				nCpstTile = (n & ~0x0F) + (dy << 4) + ((n + dx) & 0x0F);
				nCpsBlend = (blendtable) ? blendtable[nCpstTile] : 0;
				nCpstTile <<= 7;
				CpstOneObjDoX[0]();
				nCpsBlend = 0;
			}
		}

	}
	return 0;
}

// Stage 47: export the *decoded* CPS2 tile pixels actually referenced by
// buffered sprite objects. CPS2 stores a 16x16 4bpp tile as 128 packed bytes.
// Record: little-endian UINT32 original tile ID, then 128 decoded bytes.
// Pixel palette colors are exported per frame by the caller in cps_draw.cpp.
// These exports only read the ROM cache; no sprite draw state is mutated.
#include <vector>
static std::vector<UINT8> xm47CapturedTiles;
void XM47ResetCapturedTiles()
{
    xm47CapturedTiles.clear();
}
void XM47WriteSpriteTiles(FILE* stream)
{
    if (!stream || Cps != 2 || !ObjMem || !CpsGfx || nCpsGfxLen < 128 || nFrameCount <= 0) return;
    const struct ObjFrame* f = of + nGetNext;
    if (!f->Obj) return;
    const UINT16* ps = (const UINT16*)f->Obj;
    // The tile ID can exceed the cache's physical index, which wraps with
    // nCpsGfxMask; deduplicate by logical tile ID, not its wrapped address.
    if (xm47CapturedTiles.empty()) xm47CapturedTiles.resize(0x40000, 0);
    for (INT32 index=0; index<f->nCount; ++index) {
        const UINT16 yw = BURN_ENDIAN_SWAP_INT16(ps[index*4+1]);
        const UINT16 tw = BURN_ENDIAN_SWAP_INT16(ps[index*4+2]);
        const UINT16 attr = BURN_ENDIAN_SWAP_INT16(ps[index*4+3]);
        const UINT32 base = (UINT32)tw | (((UINT32)yw & 0x6000U) << 3);
        const INT32 columns = ((attr>>8)&15)+1;
        const INT32 rows = ((attr>>12)&15)+1;
        for (INT32 dy=0;dy<rows;++dy) {
            for (INT32 dx=0;dx<columns;++dx) {
                const UINT32 tile = (base & ~0x0FU) + (dy << 4) + ((base + dx) & 15);
                if (tile >= xm47CapturedTiles.size() || xm47CapturedTiles[tile]) continue;
                const UINT32 byteOffset = (tile << 7) & nCpsGfxMask;
                if (byteOffset > nCpsGfxLen - 128) continue;
                if (std::fwrite(&tile,sizeof(tile),1,stream) != 1 ||
                    std::fwrite(CpsGfx + byteOffset,1,128,stream) != 128) return;
                xm47CapturedTiles[tile] = 1;
            }
        }
    }
}

// Stage 44 - observe the exact double-buffered CPS2 sprite list used in rendering.
// Metadata only. No sprites are changed or reordered.
void XM44WriteBufferedSprites(FILE* f, int sample) {
    if (!f || Cps != 2 || !ObjMem || nFrameCount<=0) return;
    struct ObjFrame* pof=of+nGetNext;
    if (!pof->Obj) return;
    const UINT16* ps=(const UINT16*)pof->Obj;
    for (int i=0;i<pof->nCount;++i) {
        const UINT16 xw=BURN_ENDIAN_SWAP_INT16(ps[i*4+0]);
        const UINT16 yw=BURN_ENDIAN_SWAP_INT16(ps[i*4+1]);
        const UINT16 tw=BURN_ENDIAN_SWAP_INT16(ps[i*4+2]);
        const UINT16 attr=BURN_ENDIAN_SWAP_INT16(ps[i*4+3]);
        int x=xw;
        if (attr&0x80) x+=CpsSaveFrg[0][0x9]; // exact Cps2ObjDraw order
        x&=0x03ff; x^=0x200; x-=0x200;
        int y=yw&0x03ff; y^=0x200; y-=0x200;
        x+=pof->nShiftX; y+=pof->nShiftY;
        const int tile=tw|((yw&0x6000)<<3);
        std::fprintf(f,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",sample,i,x,y,tile,attr,xw>>13,attr&31,((attr>>8)&15)+1,((attr>>12)&15)+1);
    }
}

// XM86.54: this flag is scoped to the disposable Wily native OBJ overlay.
extern bool xm8651SpriteCaptureActive;
extern bool xm8651SpriteCaptureStats;
extern int xm8654ConsideredObjects;
extern int xm8654FilteredObjects;
extern int xm8654RetainedObjects;
extern int xm8654SceneryObjects;
extern int xm8654FilteredPriority[8];

// XM86.57: classification is per buffered object, BEFORE the object is drawn.
// No global saturation post-process can safely separate overdrawn pixels.
extern bool xm8657PaletteActive;
extern UINT32 xm8657MonoPal[512];
extern UINT32 xm8659InkPal[512];
extern UINT32 xm8657HudPal[512];
extern UINT32 xm8657FxPal[512];
extern UINT32 xm8658SceneryPal[512];
extern int xm8658GameStatus; // XM86.65 status-scoped intro wall classification
extern int xm14TrainingMode; // XM86.72: read-only training ownership context
extern int xm14InputTarget;  // XM86.72: 0=P1 controlled, 1=P2 controlled
// XM86.65: precise runtime identifiers for missing Magneto/Storm FX and
// misclassified scenery. Enable only with XM8665_SPRITE_AUDIT_FILE=<path>.
// Trace records the observed object, not a speculative color classification.
static inline void xm8665AuditObject(unsigned frame,int status,unsigned priority,
        unsigned palette,unsigned tile,unsigned w,unsigned h,int x,int y,
        const char* grade){
    struct Recorder {
        FILE* file=nullptr;
        unsigned long long written=0;
        Recorder(){
            const char* path=std::getenv("XM8665_SPRITE_AUDIT_FILE");
            if(!path || !*path)return;
            file=std::fopen(path,"wb");
            if(file){
                std::fprintf(file,"frame\tstatus\tpriority\tpalette\ttile\tfamily\twidth\theight\tx\ty\tgrade\tcontrolled_side\n");
                std::fprintf(stderr,"XM86.72 CONTROL_SIDE_LOG path=%s\n",path);
            } else std::fprintf(stderr,"XM86.65 SPRITE_AUDIT cannot_open=%s\n",path);
        }
        ~Recorder(){if(file)std::fclose(file);}
    };
    static Recorder rec;
    if(!rec.file)return;
    std::fprintf(rec.file,"%u\t%d\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%s\t%d\n",
        frame,status,priority,palette,tile,tile/256u,w,h,x,y,grade,
        xm14TrainingMode ? (xm14InputTarget ? 2 : 1) : 0);
    if((++rec.written%4096u)==0u)std::fflush(rec.file);
}
// XM87.02 RAW_PALETTE_PROBE: diagnostics for mixed glow/body artwork.
// Unlike grade labels, these are real RGB pens from the original CPS palette.
// A record is written once per changed palette revision, not per sprite.
// Logging is opt-in; no changes to sprites, controller routing or gameplay.
static inline void xm8702ProbePalette(unsigned frame,int status,unsigned priority,
         unsigned palette,unsigned tile,const char* grade){
    struct Probe {
        FILE* file=nullptr;
        std::uint64_t prev[32]={};
        bool known[32]={};
        unsigned long long records=0;
        Probe(){
            const char* path=std::getenv("XM8702_PALETTE_AUDIT_FILE");
            if(!path || !*path)return;
            file=std::fopen(path,"wb");
            if(!file){std::fprintf(stderr,"XM87.02 RAW_PALETTE_PROBE cannot_open=%s\n",path);return;}
            std::fprintf(file,"frame\tstatus\tcontrolled_side\tpalette\ttrigger_tile\ttrigger_grade");
            for(unsigned i=0;i<16u;++i)std::fprintf(file,"\tpen%02u_RGB",i);
            std::fprintf(file,"\n");
            std::fprintf(stderr,"XM87.02 RAW_PALETTE_PROBE path=%s\n",path);
        }
        ~Probe(){if(file)std::fclose(file);}
    };
    static Probe probe;
    if(!probe.file || status!=8 || priority!=6u || palette>=32u)return;
    const std::uint32_t* rgb=xm8657FxPal+(palette<<4);
    std::uint64_t hash=1469598103934665603ull;
    for(unsigned pen=0;pen<16u;++pen){hash^=rgb[pen];hash*=1099511628211ull;}
    if(probe.known[palette] && probe.prev[palette]==hash)return;
    probe.known[palette]=true;probe.prev[palette]=hash;
    std::fprintf(probe.file,"%u\t%d\t%d\t%u\t%u\t%s",frame,status,
        xm14TrainingMode?(xm14InputTarget?2:1):0,palette,tile,grade);
    for(unsigned pen=0;pen<16u;++pen){
        const unsigned c=rgb[pen];
        std::fprintf(probe.file,"\t%02X%02X%02X",c&255u,(c>>8)&255u,(c>>16)&255u);
    }
    std::fputc('\n',probe.file);
    if((++probe.records%256u)==0u)std::fflush(probe.file);
}
// XM87.05 ACTUAL_PEN_USAGE: tie color grading to palette indices actually
// present in decoded sprite artwork. Stage87.04 rendering is NOT changed.
// Records one row per exact (tile,palette,priority,size,grade) signature;
// captures P5/6/7, including HUD-classified objects during super animations.
// Grade is the *existing renderer's output*, NOT a fighter/FX ground truth.
static inline void xm8705AuditPens(unsigned frame,int status,unsigned pri,
        unsigned pal,unsigned base,unsigned width,unsigned height,int x,int y,
        const char* grade){
    struct Recorder {
        FILE* fp=nullptr;
        std::unordered_set<std::uint64_t> seen;
        unsigned long long rows=0;
        Recorder(){
            const char* path=std::getenv("XM8705_PEN_USAGE_FILE");
            if(!path || !*path)return;
            fp=std::fopen(path,"wb");
            if(!fp){std::fprintf(stderr,"XM87.05 ACTUAL_PEN_USAGE cannot_open=%s\n",path);return;}
            std::fprintf(fp,"frame\tcontrolled_side\tpriority\tpalette\ttile\twidth\theight\tx\ty\tgrade\tused_pen_mask_hex\tused_pen_count");
            for(unsigned i=0;i<16u;++i)std::fprintf(fp,"\tpen%02u_RGB",i);
            std::fprintf(fp,"\n");
            std::fprintf(stderr,"XM87.05 ACTUAL_PEN_USAGE path=%s\n",path);
        }
        ~Recorder(){if(fp)std::fclose(fp);}
    };
    static Recorder rec;
    if(!rec.fp || status!=8 || (pri!=5u && pri!=6u && pri!=7u) ||
       pal>=32u || width==0u || width>16u || height==0u || height>16u ||
       !CpsGfx || nCpsGfxLen<128u)return;
    unsigned gradeCode=0;
    if(!std::strcmp(grade,"fighter"))gradeCode=1;
    else if(!std::strcmp(grade,"fx"))gradeCode=2;
    else if(!std::strcmp(grade,"fx_masked"))gradeCode=3;
    else if(!std::strcmp(grade,"lighting_body"))gradeCode=4;
    else if(!std::strcmp(grade,"p1_fx_index"))gradeCode=5;
    else if(!std::strcmp(grade,"fx_p1_mirror"))gradeCode=6;
    else if(!std::strcmp(grade,"hud"))gradeCode=7;
    else if(!std::strcmp(grade,"scenery"))gradeCode=8;
    const auto key=XM8705::ObservationKey(base,pal,pri,width,height,gradeCode);
    if(!rec.seen.insert(key).second)return;
    std::uint16_t used=0;
    for(unsigned yy=0;yy<height;++yy){
        for(unsigned xx=0;xx<width;++xx){
            const unsigned subtile=XM8705::SubTile(base,xx,yy);
            const unsigned offset=(subtile<<7)&nCpsGfxMask;
            if(offset>nCpsGfxLen-128u)continue;
            used=std::uint16_t(used | XM8705::TilePenMask(CpsGfx+offset));
        }
    }
    unsigned usedCount=0;
    for(unsigned i=1u;i<16u;++i)usedCount+=unsigned((used>>i)&1u);
    std::fprintf(rec.fp,"%u\t%d\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%s\t%04X\t%u",
        frame,xm14TrainingMode?(xm14InputTarget?2:1):0,pri,pal,base,width,height,x,y,grade,
        unsigned(used),usedCount);
    const std::uint32_t* rgb=xm8657FxPal+(pal<<4);
    for(unsigned i=0;i<16u;++i){
        const auto c=rgb[i];
        std::fprintf(rec.fp,"\t%02X%02X%02X",c&255u,(c>>8)&255u,(c>>16)&255u);
    }
    std::fputc('\n',rec.fp);
    if((++rec.rows%512ull)==0ull)std::fflush(rec.fp);
}

// XM87.19: audit every combat priority-5/6 object, not pre-fight/HUD.
// Provenance + actual final color choice, diagnostic only, no rendering change.
static void XM8717DrawAudit(const ObjFrame* pof,unsigned compressedIndex,
                            unsigned frame,unsigned priority,unsigned palette,
                            unsigned tile,unsigned width,unsigned height,
                            unsigned rawAttr,int x,int y,const char* grade){
    // XM87.22: Force Field diagnosis must see ANY priority, including
    // priority-7 graphics that the older trace discarded as HUD.
    static const bool fullCapture=[](){const char* v=std::getenv("XM8722_CAPTURE_ALL_OBJECTS");
        return v && v[0]=='1' && v[1]=='\0';}();
    if(!XM8717::enabled || compressedIndex>=0x400u || xm8658GameStatus!=8 ||
       (!fullCapture && priority!=5u && priority!=6u))return;
    struct Recorder {
        FILE* fp=nullptr;
        unsigned long long rows=0;
        Recorder(){
            const char* path=std::getenv("XM8717_DRAW_PROVENANCE_FILE");
            if(path && *path){
                fp=std::fopen(path,"wb");
                if(fp){
                    std::setvbuf(fp,nullptr,_IOFBF,256*1024);
                    std::fprintf(fp,"frame\tcompressed_index\tpriority\tpalette\ttile\tx\ty\twriter_pc\tsource_a0\tsource_frame\towner\tcurrent_grade\twidth\theight\traw_attr\tsource_valid\tsource_age\n");
                    std::fprintf(stderr,"XM87.19 COMBAT_SOURCE_TRACE opened: %s\n",path);
                }
            }
        }
        ~Recorder(){if(fp)std::fclose(fp);}
    };
    static Recorder recorder;
    if(!recorder.fp || recorder.rows>=2000000ull)return;
    const auto& src=pof->xm8717Sources[compressedIndex];
    std::fprintf(recorder.fp,"%u\t%u\t%u\t%u\t%u\t%d\t%d\t%06X\t%06X\t%u\t%u\t%s\t%u\t%u\t%04X\t%u\t%d\n",
        frame,compressedIndex,priority,palette,tile,x,y,
        (unsigned)src.pc,(unsigned)src.a0,(unsigned)src.frame,(unsigned)src.owner,
        grade,width,height,rawAttr&0xffffu,(unsigned)src.valid,
        src.valid && frame>=src.frame?int(frame-src.frame):-1);
    if((++recorder.rows%4096ull)==0ull)std::fflush(recorder.fp);
}

// Delay sprite drawing by one frame
INT32 Cps2ObjDraw(INT32 nLevelFrom, INT32 nLevelTo)
{
    XM8798Latency::state().beforeObjects();
	const INT32 nPsAdd = 4;

	UINT16 *ps;
	struct ObjFrame *pof;
	CpstOneDoFn pCpstOne;
	INT32 nCount;

	bool bMask = 0;
	XM8732::Capture(CpsRamFF,(unsigned)GetCurrentFrame(),xm8658GameStatus);
    const int xm8745LivePhase=CpsRamFF?int(CpsRamFF[0x5001u]):xm8658GameStatus;
    // XM87.47 accepted ROM action cues. Read-only, once per emulated frame.
    XM8747::Observe(CpsRamFF,(unsigned)GetCurrentFrame(),xm8745LivePhase,
                    nLevelFrom<=6 && nLevelTo>=6 && !xm8651SpriteCaptureActive);
    XM8755::Observe(CpsRamFF,(unsigned)GetCurrentFrame(),xm8745LivePhase,
                    nLevelFrom<=6 && nLevelTo>=6 && !xm8651SpriteCaptureActive);
    XM8759::Observe(CpsRamFF,(unsigned)GetCurrentFrame(),xm8745LivePhase,
                    nLevelFrom<=6 && nLevelTo>=6 && !xm8651SpriteCaptureActive);
 // CPU FF5000 is swapped within each host word

	// Draw the earliest frame we have in history
	pof = of + nGetNext;
	// XM87.60: export the ORIGINAL buffered CPS2 object list, before any
    // custom Wily/Double Feature filtering. A stable OAM tile is not a
    // verified projectile or super: correlate with original ROM states.
    // Sample only the real CPS2 sprite pass, not Wily's extraction prepasses.
    if(nLevelFrom<=6 && nLevelTo>=6 && !xm8651SpriteCaptureActive){
        xm8763NativeObjectCount=(unsigned)pof->nCount;
        xm8763NativeObjectFrame=(unsigned)GetCurrentFrame();
    }
    XM8760::Observe(pof->Obj,(unsigned)pof->nCount,pof->xm8717Sources,
        XM8717::enabled,(unsigned)GetCurrentFrame(),(unsigned)xm8745LivePhase,
        nLevelFrom<=6 && nLevelTo>=6 && !xm8651SpriteCaptureActive);
    // XM87.73: begin native selector buffer once per real priority-7 pass.
    // FF5113 is zero throughout Training selection; stage assignment is nonzero.
    if(nLevelFrom<=7 && nLevelTo>=7 && !xm8651SpriteCaptureActive){
        const bool selection=xm14TrainingMode && CpsRamFF && xm8745LivePhase<8 &&
            CpsRamFF[0x5112u]==0u;
        XM8773::State().Begin((unsigned)GetCurrentFrame(),selection,
            CpsRamFF?(unsigned)CpsRamFF[0x5182u]:0u,
            (unsigned)nCpsScreenWidth,(unsigned)nCpsScreenHeight);
    }
    XM8766OAM::Observe(pof->Obj,(unsigned)pof->nCount,pof->xm8717Sources,
        XM8717::enabled,(unsigned)GetCurrentFrame(),(unsigned)xm8745LivePhase,
        pof->nShiftX,pof->nShiftY,
        nLevelFrom<=6 && nLevelTo>=6 && !xm8651SpriteCaptureActive);


    // Stage87.28: precompute opaque P1/P2 actor occupancy over the ENTIRE
    // buffered object list, rather than a single 16x16 tile's geometry.
    // This precedes offset copies, including later overlapping fighter pieces.
    XM8728::Mask& xm8728Mask=XM8728::State();
    if(XM8745::OpeningPhase(xm8745LivePhase) && xm8657PaletteActive && XM8725::Enabled() &&
       nLevelFrom<=6 && nLevelTo>=6){
        xm8728Mask.Begin((unsigned)GetCurrentFrame(),nCpsScreenWidth,
            nCpsScreenHeight,nBurnBpp);
        if(xm8728Mask.width && !xm8728Mask.ready && CpsGfx && nCpsGfxLen>=128u){
            const unsigned xm8728Count=(unsigned)pof->nCount<0x400u?
                (unsigned)pof->nCount:0x400u;
            for(unsigned ix=0;ix<xm8728Count;++ix){
                const UINT16* word=((const UINT16*)pof->Obj)+ix*4u;
                const unsigned wordX=BURN_ENDIAN_SWAP_INT16(word[0]);
                const unsigned wordY=BURN_ENDIAN_SWAP_INT16(word[1]);
                const unsigned rawTile=BURN_ENDIAN_SWAP_INT16(word[2]);
                const unsigned tile=rawTile|
                     ((wordY&(Cps2Turbo?0xe000u:0x6000u))<<3);
                const unsigned attr=BURN_ENDIAN_SWAP_INT16(word[3]);
                if((wordX>>13)!=6u || !(nSpriteEnable&(1<<6)))continue;
                if(!XM8717::enabled || !XM8717::Recent(
                    pof->xm8717Sources[ix],(unsigned)GetCurrentFrame()))continue;
                const unsigned owner=pof->xm8717Sources[ix].owner;
                if(owner!=1u && owner!=2u)continue;
                const unsigned wide=((attr>>8)&15u)+1u;
                const unsigned tall=((attr>>12)&15u)+1u;
                if(xm8651SpriteCaptureActive &&
                   !xm8654KeepNativeObject(6u,tile,attr&31u,wide,tall))continue;
                int ox=(int)wordX,oy=(int)wordY;
                if(attr&0x80u)ox+=CpsSaveFrg[0][0x9];
                ox=(ox&0x03ff)^0x200;ox-=0x200;
                oy=(oy&0x03ff)^0x200;oy-=0x200;
                ox+=pof->nShiftX;oy+=pof->nShiftY;
                const unsigned flip=(attr>>5)&3u;
                for(unsigned ty=0;ty<tall;++ty)for(unsigned tx=0;tx<wide;++tx){
                    const unsigned t=(tile&~15u)+(ty<<4)+((tile+tx)&15u);
                    const unsigned addr=(t<<7)&nCpsGfxMask;
                    if(addr>nCpsGfxLen-128u)continue;
                    const unsigned ex=(flip&1u)?wide-tx-1u:tx;
                    const unsigned ey=(flip&2u)?tall-ty-1u:ty;
                    xm8728Mask.Stamp(CpsGfx+addr,ox+int(ex<<4),oy+int(ey<<4),flip);
                }
            }
            xm8728Mask.ready=true;
        }
    }

    // XM87.46: native HUD underlay. Never render it into Wily's temporary
    // two-fill sprite-isolation passes; the Wily compositor inserts it there.
    if(XM8746::ShouldNativeMeter(nLevelFrom,nLevelTo,xm8745LivePhase,
          xm8651SpriteCaptureActive,xm8746DizzyHudAllowed(),
          nCpsScreenWidth,nCpsScreenHeight,nBurnBpp) && pBurnDraw){
        static unsigned lastFrame=~0u;
        const unsigned frame=(unsigned)GetCurrentFrame();
        if(lastFrame!=frame){
            lastFrame=frame;
            static unsigned char overlay[384*224*4];
            std::memset(overlay,0,sizeof overlay);
            const char* df=std::getenv("XM8649_WILY_RENDER");
            XM8733::Draw(CpsRamFF,overlay,384*4,4,frame,xm8745LivePhase,
                         df && df[0]=='1' && df[1]==0);
            const unsigned pixels=XM8746::BlitToNative(overlay,(unsigned char*)pBurnDraw,
                                                       nBurnPitch,BurnHighCol);
            static unsigned lastReport=0;
            if(pixels && (!lastReport || frame-lastReport>=300u)){
                std::fprintf(stderr,"XM87.46 HUD_UNDER_NATIVE frame=%u pixels=%u priority=5+\n",frame,pixels);
                lastReport=frame;
            }
        }
    }

    // XM87.55: all injected body-trail layers have been retired.

	// Point to Obj list
	ps = (UINT16*)pof->Obj + nPsAdd * (nMaxZValue - nZOffset - 1);
	nCount = nZOffset + pof->nCount;

	// Go through all the Objs
	for (ZValue = (UINT16)nMaxZValue; ZValue <= nCount; ZValue++, ps += nPsAdd) {
		INT32 x, y, n, a, bx, by, dx, dy;
		INT32 nFlip;
		INT32 v = BURN_ENDIAN_SWAP_INT16(ps[0]) >> 13;

		if ((nSpriteEnable & (1 << v)) == 0) {
			continue;
		}

		// Exclude foreign stage objects BEFORE Z/priority/mask bookkeeping.
		// XM86.54: ROM-inventory-backed stage-only scenery removal.
		// The ordinary native screen is never affected. Check EVERY buffered
		// object on every frame; no frozen address/index and no coarse masks.
		if (xm8651SpriteCaptureActive) {
			const UINT16 yw=BURN_ENDIAN_SWAP_INT16(ps[1]);
			const UINT16 attr=BURN_ENDIAN_SWAP_INT16(ps[3]);
			const UINT32 tile=(UINT32)BURN_ENDIAN_SWAP_INT16(ps[2]) |
			                   (((UINT32)yw & 0x6000U) << 3);
			const unsigned pal=attr & 0x1FU;
			const unsigned width=((attr>>8)&15U)+1U;
			const unsigned height=((attr>>12)&15U)+1U;
			const bool scenery=xm8654KnownNativeScenery((unsigned)v,tile,pal);
			const bool keep=xm8654KeepNativeObject((unsigned)v,tile,pal,width,height);
			if (xm8651SpriteCaptureStats && v >= nLevelFrom && v <= nLevelTo) {
				++xm8654ConsideredObjects;
				if (scenery) ++xm8654SceneryObjects;
				if (keep) ++xm8654RetainedObjects;
				else {
					++xm8654FilteredObjects;
					if (v>=0 && v<8) ++xm8654FilteredPriority[v];
				}
			}
			if (!keep) continue;
		}

		// Check if sprite is between these levels
		if (v > nLevelTo) {
			bMask = 1;
			continue;
		}
		if (v < nLevelFrom) {
			continue;
		}

		if (bMask) {
			nMaxZMask = ZValue;
		} else {
			nMaxZValue = ZValue;
		}


		// Select CpstOne function;
		if (bMask || nMaxZMask > nMaxZValue) {
			pCpstOne = CpstOneObjDoX[1];
		} else {
			pCpstOne = CpstOneObjDoX[0];
		}

		x = BURN_ENDIAN_SWAP_INT16(ps[0]);
		y = BURN_ENDIAN_SWAP_INT16(ps[1]);
		n = BURN_ENDIAN_SWAP_INT16(ps[2]);
		a = BURN_ENDIAN_SWAP_INT16(ps[3]);

		if (a & 0x80) {														// marvel vs capcom ending sprite off-set
			x += CpsSaveFrg[0][0x9];
		}
		
		// CPS2 coords are 10 bit signed (-512 to 511)
		x &= 0x03FF; x ^= 0x200; x -= 0x200;
		y &= 0x03FF; y ^= 0x200; y -= 0x200;

#if 0
		// This *MAY* be needed to get correct sprite positions when raster interrups are used.
		if (nRasterline[1]) {
			for (INT32 i = 1; i < MAX_RASTER; i++) {
				if ((y < nRasterline[i]) || (nRasterline[i] == 0)) {
					x -= CpsSaveFrg[i - 1][0x09];
					y -= CpsSaveFrg[i - 1][0x0B];
					break;
				}
			}
		} else {
			x -= CpsSaveFrg[0][0x9];
			y -= CpsSaveFrg[0][0xB];
		}
#else
		// Ignore sprite offsets when raster interrupts are used (seems to work for all games).
		x += pof->nShiftX;
		y += pof->nShiftY;

//		x -= CpsSaveFrg[0][0x9];
//		y -= CpsSaveFrg[0][0xB];

#endif
		if (Cps2Turbo) {
			if (BURN_ENDIAN_SWAP_INT16(ps[1]) & 0x1000) ps[1] |= BURN_ENDIAN_SWAP_INT16(0x8000);
			n |= (BURN_ENDIAN_SWAP_INT16(ps[1]) & 0xe000) << 3;	// high bits of address
		} else {
			n |= (BURN_ENDIAN_SWAP_INT16(ps[1]) & 0x6000) << 3;	// high bits of address
		}
		
        // XM87.73: genuine CPS2 selection outlines, not guessed tile IDs.
        // Complete composite OAM entry classified by its decoded pixel pens.
        // Never affect combat, non-Training, or Wily sprite-extraction passes.
        XM8773::Role xm8774Cursor=XM8773::None;
        if(XM8773::State().enabled && !xm8651SpriteCaptureActive &&
           (unsigned)v==7u && ((unsigned)a&31u)==27u){
            const unsigned wide=(((unsigned)a>>8)&15u)+1u;
            const unsigned tall=(((unsigned)a>>12)&15u)+1u;
            const XM8773::Role cursor=XM8773::ObjectRole((unsigned)v,
                (unsigned)a&31u,x,y,(unsigned)n,wide,tall,
                CpsGfx,(size_t)nCpsGfxLen,(size_t)nCpsGfxMask);
            if(cursor!=XM8773::None){
                // P2 remains invisible until the second ROM-accepted P1 pick.
                // Draw source pixels through exact 16x16 decoded pen shapes
                // after the DF pass instead of painting arbitrary UI rectangles.
                if(!XM8773::Allow(cursor,true,XM8773::State().picks))
                    continue; // only the still-inactive P2 artwork is hidden
                xm8774Cursor=cursor;
                // Delay the overlay stamp until the exact native tile draw.
                // This is essential when composite pieces are clipped, hidden
                // by later sprites or positioned outside the standard grid.
            }
        }

        // Find the palette for the tiles on this sprite
		CpstPal = CpsPal + ((a & 0x1F) << 4);
        // A mixed FX/body object can use different colors on its individual
        // 4-bit palette pens. Draw it with a local sixteen-pen palette so only
        // the luminous effect pens stay colored, not an entire arm/torso.
        UINT32 xm8667LocalFxPal[16];
        bool xm8667FxMasked=false;
        bool xm8669IndexedFull=false;
        bool xm8669IndexedMasked=false;
        bool xm8701LightingBody=false;
        bool xm8702P2SourceCandidate=false;
        bool xm8702P1Mirrored=false;
        bool xm8722EchoGray=false;
        bool xm8725FighterObject=false;
        if(xm8657PaletteActive){
            const unsigned pal=(unsigned)a&31u;
            const unsigned tile=(unsigned)n;
            const unsigned wide=(((unsigned)a>>8)&15u)+1u;
            const unsigned tall=(((unsigned)a>>12)&15u)+1u;
            const auto p1Index=XM8669::Classify((unsigned)v,pal,tile);
            UINT32* bank=xm8657MonoPal;
            // Stage86.65: background artwork carried as a native OBJ gets
            // the current 30%-peak, 70%-saturated treatment of native scroll layers.
            // Restrict to the already verified Stage84 tile/palette families;
            // DO NOT darken all priority-7 sprites (most are HUD).
            // XM86.65: stage scenery may be buffered as an OBJ, including
            // the large crushing X-Men logo during the pre-fight transition.
            // Evaluate scenery BEFORE the broad priority-7 HUD rule.
            const bool exactScenery=xm8654KnownNativeScenery((unsigned)v,tile,pal) ||
                  xm8665ExtraScenery((unsigned)v,pal,tile);
            const bool hudNumeral=((unsigned)v==0u && pal==1u && wide==2u && tall==2u &&
                  tile>=87808u && tile<=87826u && ((tile-87808u)%2u)==0u);
            const bool introWall=(!hudNumeral && xm8658GameStatus>=0 &&
                  xm8665IntroWall((unsigned)xm8658GameStatus,(unsigned)v,wide,tall));
            if(exactScenery || introWall)bank=xm8658SceneryPal;
            else if(xm8657Hud((unsigned)v,pal,tile,wide,tall))bank=xm8657HudPal;
            // Collision takes precedence over runtime learning and broad rules.
            // Body+FX sharing a tile AND palette remains grayscale until a
            // separate pixel/tile-region mask is verified from actual artwork.
            else if(p1Index==XM8669::BodyConflict)bank=xm8659InkPal;
            else if(p1Index==XM8669::ConfirmedFX || p1Index==XM8669::MixedFX){
                bank=xm8657FxPal;
                xm8669IndexedFull=(p1Index==XM8669::ConfirmedFX);
                xm8669IndexedMasked=(p1Index==XM8669::MixedFX);
                static unsigned xm8669LogCount=0;
                if(xm8669LogCount++<24u)std::fprintf(stderr,
                    "XM86.69 P1_FX_INDEX tile=%u palette=%u mode=%s\n",
                    tile,pal,xm8669IndexedFull?"full":"pen-mask");
            }
            else if(xm8657LikelyFx((unsigned)v,pal) || xm8659EffectFamily((unsigned)v,pal,tile)) {
                bank=xm8657FxPal;
                // XM87.02: defer cross-player learning until the Stage87.01
                // lighting/body veto has completed. Prior code seeded from
                // P2 palette membership alone, contaminating P1 with body FX.
                xm8702P2SourceCandidate=xm8657LikelyFx((unsigned)v,pal);
            }
            else if(XM8702::AllowMirror((unsigned)v,pal,xm8666FxMirror().mixed(tile)) &&
                    xm8666FxMirror().mirroredFx((unsigned)v,pal,tile,
                                                (unsigned)GetCurrentFrame())){
                bank=xm8657FxPal; // exact P2-only FX tile, with live body veto
                xm8702P1Mirrored=true;
            }

            else if((unsigned)v==5u || (unsigned)v==6u){
                bank=xm8659InkPal; // grayscale fighters and shadows
            }
            // XM87.01 LIGHTING_TILE_VETO: a palette change is not a new graphic.
            // Character artwork may be temporarily assigned the same high-color
            // banks as a projectile during hits, specials, and stage lighting.
            // Prefer witnessed fighter tile identity over a broad FX palette or
            // previous P1 mirroring. Only affects objects selected for RGB;
            // standalone projectile tiles retain their existing colorization.
            // Character-specific costume/attack rules are not introduced here.
            if(bank==xm8657FxPal && XM8701::IsLightingArtwork((unsigned)v,tile)){
                static const bool vetoEnabled=[](){
                    const char* p=std::getenv("XM8701_LIGHTING_VETO");
                    return !p || p[0]!='0' || p[1]!='\0';
                }();
                if(vetoEnabled){
                    bank=xm8659InkPal;
                    xm8701LightingBody=true;
                    static unsigned long long vetoCount=0;
                    ++vetoCount;
                    if(vetoCount<=16u || (vetoCount%8192u)==0u)
                        std::fprintf(stderr,
                            "XM87.01 LIGHTING_TILE_VETO tile=%u palette=%u frame=%u count=%llu\n",
                            tile,pal,(unsigned)GetCurrentFrame(),vetoCount);
                }
            }
            // XM87.02 FX_SOURCE_POST_VETO: a character lit with a P2 effect
            // palette must never teach P1 to color the same artwork. The
            // original source must survive the static veto and any live body
            // observation before it may seed an exact-tile P1 mirror.
            if(xm8702P2SourceCandidate && XM8702::AllowSource(
                    (unsigned)v,pal,bank==xm8657FxPal,
                    xm8701LightingBody || XM8701::IsLightingArtwork((unsigned)v,tile),
                    xm8667InventorySharedTile(tile),xm8666FxMirror().mixed(tile))){
                xm8666FxMirror().observe((unsigned)v,pal,tile,(unsigned)GetCurrentFrame());
                static unsigned xm8702SafeSeeds=0;
                if(xm8702SafeSeeds++<16u)std::fprintf(stderr,
                    "XM87.02 FX_SOURCE_POST_VETO tile=%u pal=%u frame=%u\n",
                    tile,pal,(unsigned)GetCurrentFrame());
            }
            // Observe ordinary grayscale fighter artwork on every priority-6
            // bank so a previously unseen same-artwork palette change is also
            // prevented from becoming a future opposite-side FX seed.
            if((unsigned)v==6u && pal<16u && bank==xm8659InkPal)
                xm8666FxMirror().observeFighter((unsigned)v,tile,(unsigned)GetCurrentFrame());
            // Apply a pen-level mask to ANY opposite-side mirrored object,
            // and to P2 FX artwork whose exact tile overlaps a known fighter
            // tile in the supplied inventory or this session. Standalone P2
            // effect sprites keep their established full-color treatment.
            // This does not disturb sprite geometry, alpha, priority, or Wily.
            // Confirmed standalone P1 effects use their actual ROM palette.
            // Prior Stage86.67 applied a restrictive chroma mask to *all* P1
            // effects, making many correctly indexed effects appear gray.
            if(bank==xm8657FxPal && !xm8669IndexedFull &&
                   ((pal<16u) || xm8669IndexedMasked ||
                    xm8667InventorySharedTile(tile) || xm8666FxMirror().mixed(tile))){
                xm8667FxMasked=true;
                xm8667FilteredPalette(xm8657FxPal+(pal<<4),
                                      xm8659InkPal+(pal<<4),xm8667LocalFxPal);
                // XM87.06 FX_GRADIENT_RESTORE: only sprites already accepted as
                // selectively-colored FX reach here; fighter lighting stays
                // completely grayscale. The previously discarded pastel end of
                // a real upper-bank pink/green ramp gets its ROM RGB back.
                if(XM8706::RampEnabled()){
                    const unsigned restored=XM8706::RestoreUpperRamp(
                        xm8657FxPal+(pal<<4),xm8659InkPal+(pal<<4),
                        xm8667LocalFxPal);
                    if(restored){
                        static unsigned notices=0;
                        if(notices++<12u)std::fprintf(stderr,
                            "XM87.06 FX_GRADIENT_RESTORE tile=%u pal=%u pens=%u\n",
                            tile,pal,restored);
                    }
                }
            }
            // XM87.08 GROUP_SUPER_FX: long, tall contiguous OAM runs are
            // special-effect artwork in the user's Stage87.07 recording, even
            // when an older palette-first classifier labeled them as fighter
            // or lighting_body. Override ONLY those large groups; individual
            // fighter tiles and normal attack classifications are unchanged.
            const unsigned xm8708Slot=(unsigned)((ps-(UINT16*)pof->Obj)/4);
            const bool xm8708LargeSuper=((unsigned)v==6u) &&
                XM8708::GroupSuper((const UINT16*)pof->Obj,pof->nCount,
                    (unsigned)GetCurrentFrame(),pof->nShiftY,xm8708Slot);
            if(xm8708LargeSuper && bank!=xm8658SceneryPal && bank!=xm8657HudPal){
                bank=xm8657FxPal;
                xm8667FxMasked=false;
                xm8701LightingBody=false;
                xm8669IndexedFull=false;
                xm8702P1Mirrored=false;
                static unsigned long long xm8708Total=0;
                if((++xm8708Total)<=8ull || xm8708Total%4096ull==0ull)
                    std::fprintf(stderr,"XM87.08 GROUP_SUPER_FX frame=%u pal=%u tile=%u count=%llu\n",
                        (unsigned)GetCurrentFrame(),pal,tile,xm8708Total);
            }
            // XM87.10 TEMPEST_FRAGMENT_COLOR: Stage87.09 identified a
            // coherent 24..26-unique-tile palette21 object group that was
            // briefly RGB but then vetoed to lighting_body during its flight.
            // Only this proven dense/non-repeating fragment structure is
            // restored; nearby 4..10-tile fighter-lit animation remains gray.
            const bool xm8710TempestFragment=((unsigned)v==6u) &&
                XM8710::Fragment((const UINT16*)pof->Obj,pof->nCount,
                    (unsigned)GetCurrentFrame(),pof->nShiftX,pof->nShiftY,
                    xm8708Slot);
            if(xm8710TempestFragment && bank!=xm8658SceneryPal &&
                                             bank!=xm8657HudPal){
                bank=xm8657FxPal;
                xm8667FxMasked=false;
                xm8701LightingBody=false;
                xm8669IndexedFull=false;
                xm8702P1Mirrored=false;
                static unsigned long long restored=0;
                if((++restored)<=8ull || restored%2048ull==0ull)
                    std::fprintf(stderr,
                        "XM87.10 TEMPEST_FRAGMENT_COLOR frame=%u pal=%u tile=%u count=%llu\n",
                        (unsigned)GetCurrentFrame(),pal,tile,restored);
            }
            // XM87.11 RGB_FIRST_SUBTRACT_FIGHTER: stop requiring FX to earn
            // color via palette banks, static whitelists, lighting vetoes,
            // or per-pen thresholds. In active gameplay, priority-5/6
            // objects use the ROM's original palette UNLESS their complete
            // OAM group follows an actual fighter RAM anchor. Maintain the
            // established independent scenery/HUD treatment. Existing
            // Stage87.06–87.10 logic remains available for A/B rollback.
            if(XM8711::Enabled() && xm8658GameStatus==8 &&
               ((unsigned)v==5u || (unsigned)v==6u) &&
               bank!=xm8658SceneryPal && bank!=xm8657HudPal){
                const bool body=((unsigned)v==6u) &&
                    XM8711::IsBody((const UINT16*)pof->Obj,pof->nCount,
                        (unsigned)GetCurrentFrame(),pof->nShiftX,pof->nShiftY,
                        xm8708Slot);
                bank=body?xm8659InkPal:xm8657FxPal;
                xm8667FxMasked=false;
                xm8701LightingBody=false;
                xm8669IndexedFull=false;
                xm8702P1Mirrored=false;
                static unsigned long long nBody=0,nRgb=0;
                if(body)++nBody;else ++nRgb;
                if((nBody+nRgb)<=16ull || ((nBody+nRgb)%32768ull)==0ull)
                    std::fprintf(stderr,
                        "XM87.11 RGB_FIRST_SUBTRACT_FIGHTER frame=%u pal=%u tile=%u body=%d totals=%llu/%llu\n",
                        (unsigned)GetCurrentFrame(),pal,tile,(int)body,nBody,nRgb);
            }
            // XM87.18 SOURCE_OWNER_GRAY: direct actor-object provenance wins
            // over provisional geometry when a confirmed fighter body was
            // briefly colored during jumps, dashes or palette transitions.
            // Unknown/non-actor sources are NOT inferred to be projectiles;
            // preserve their existing Stage87.15 RGB-first decision.
            if(XM8718::enabled && XM8717::enabled && xm8658GameStatus==8 &&
               (unsigned)v==6u && bank!=xm8658SceneryPal &&
               bank!=xm8657HudPal && xm8708Slot<0x400u &&
               (XM8718::State().Observe((const std::uint16_t*)pof->Obj,
                    (unsigned)pof->nCount,pof->xm8717Sources,
                    (unsigned)GetCurrentFrame()),
                XM8718::State().Classify(pof->xm8717Sources[xm8708Slot],
                    (unsigned)GetCurrentFrame(),(unsigned)v,pal,tile,true))==XM8718::FighterGray){
                if(bank!=xm8659InkPal){
                    static unsigned long long corrected=0;
                    if((++corrected)<=24ull || corrected%8192ull==0ull)
                        std::fprintf(stderr,"XM87.18 DIRECT_FIGHTER_GRAY frame=%u owner=%u a0=%06X pal=%u tile=%u corrected=%llu\n",
                            (unsigned)GetCurrentFrame(),(unsigned)pof->xm8717Sources[xm8708Slot].owner,
                            (unsigned)pof->xm8717Sources[xm8708Slot].a0,pal,tile,corrected);
                }
                bank=xm8659InkPal;
                xm8667FxMasked=false;
                xm8701LightingBody=false;
                xm8669IndexedFull=false;
                xm8702P1Mirrored=false;
            }
            // XM87.20 SECONDARY_OWNER: Source-backed final adjustment. This
            // repairs confirmed body-copy RGB flashes and one independently
            // sourced object accidentally grayed by proximity; no generalized
            // palette or distance rules and no override for HUD/scenery.
            if(XM8718::enabled && XM8717::enabled && xm8658GameStatus==8 &&
               (unsigned)v==6u && xm8708Slot<0x400u &&
               bank!=xm8658SceneryPal && bank!=xm8657HudPal){
                const auto decision=XM8720::Classify(
                    pof->xm8717Sources[xm8708Slot], XM8718::State(),
                    (unsigned)GetCurrentFrame(),(unsigned)v,pal,tile);
                if(decision!=XM8720::Unchanged){
                    bank=decision==XM8720::FighterEchoGray?xm8659InkPal:xm8657FxPal;
                    xm8667FxMasked=false;
                    xm8701LightingBody=false;
                    xm8669IndexedFull=false;
                    xm8702P1Mirrored=false;
                    static unsigned long long echoCount=0,fxCount=0;
                    if(decision==XM8720::FighterEchoGray)++echoCount;
                    else ++fxCount;
                    if(echoCount+fxCount<=20ull ||
                       ((echoCount+fxCount)%8192ull)==0ull)
                        std::fprintf(stderr,
                            "XM87.20 SECONDARY_OWNER frame=%u source=%06X pal=%u tile=%u mode=%s count=%llu/%llu\n",
                            (unsigned)GetCurrentFrame(),
                            (unsigned)pof->xm8717Sources[xm8708Slot].a0,
                            pal,tile,
                            decision==XM8720::FighterEchoGray?"body_echo":"independent_fx",
                            echoCount,fxCount);
                }
            }
            // XM87.21 EXACT_SOURCE_ARTWORK: last writer-supported correction.
            // The inherited echo mask learned body tiles too late for the first
            // piledriver frame. A source+tile signature fixes those copies.
            // Palette 6/7 effects have their own verified tile identities and
            // should not be swallowed by the old positional body mask.
            static const bool xm8722OldFxProbe=[](){
                const char* p=std::getenv("XM8722_OLD_FX_PROBE");
                return p && p[0]=='1' && p[1]=='\0';}();
            if(xm8722OldFxProbe && xm8658GameStatus==8 && xm8708Slot<0x400u &&
               bank!=xm8658SceneryPal && bank!=xm8657HudPal){
                const auto decision=XM8721::Classify(
                    pof->xm8717Sources[xm8708Slot],(unsigned)GetCurrentFrame(),
                    (unsigned)v,pal,tile,true);
                if(decision!=XM8721::Keep){
                    bank=decision==XM8721::GrayEcho?xm8659InkPal:xm8657FxPal;
                    xm8667FxMasked=false;xm8701LightingBody=false;
                    xm8669IndexedFull=false;xm8702P1Mirrored=false;
                    static unsigned long long echo=0,rgb=0;
                    if(decision==XM8721::GrayEcho)++echo;else ++rgb;
                    if(echo+rgb<=24ull || (echo+rgb)%8192ull==0ull){
                        if(decision==XM8721::GrayEcho)
                            std::fprintf(stderr,"XM87.21 SHADOW_COPY_GRAY frame=%u source=%06X pal=%u tile=%u count=%llu\n",
                                (unsigned)GetCurrentFrame(),
                                (unsigned)pof->xm8717Sources[xm8708Slot].a0,pal,tile,echo);
                        else
                            std::fprintf(stderr,"XM87.21 EFFECT_RGB frame=%u source=%06X pal=%u tile=%u count=%llu\n",
                                (unsigned)GetCurrentFrame(),
                                (unsigned)pof->xm8717Sources[xm8708Slot].a0,pal,tile,rgb);
                    }
                }
            }
            // XM87.22: the three separately sourced afterimage actors are
            // complete graphics producers, not lists of fixed tile IDs.
            // Never touch other sources or FX palettes. Stage87.21's
            // unverified effect-tile promotion is disabled below so this
            // recovery does not introduce another unproven Force Field rule.
            if(XM8717::enabled && xm8658GameStatus==8 &&
               xm8708Slot<0x400u && bank!=xm8658SceneryPal &&
               bank!=xm8657HudPal && XM8722::Echo(
                   pof->xm8717Sources[xm8708Slot],
                   (unsigned)GetCurrentFrame(),(unsigned)v,pal)){
                xm8722EchoGray=true;
                bank=xm8659InkPal;
                xm8667FxMasked=false;
                xm8701LightingBody=false;
                xm8669IndexedFull=false;
                xm8702P1Mirrored=false;
                static unsigned long long total=0;
                ++total;
                if(total<=16ull || total%4096ull==0ull)
                    std::fprintf(stderr,
                        "XM87.22 ECHO_SOURCE_GRAY frame=%u source=%06X pal=%u tile=%u total=%llu\n",
                        (unsigned)GetCurrentFrame(),
                        (unsigned)pof->xm8717Sources[xm8708Slot].a0,
                        pal,tile,total);
            }
            // XM87.23 ECHO_MONO: Zangief's blue first/air-grab frames are
            // not reliably source-tagged. His complete afterimage triplet
            // uses the simultaneous 21+22 dense-palette signature in every
            // captured frame. Apply a LAST monochrome decision only to that
            // grouped afterimage, never to isolated palette-21 projectiles.
            // Confirmed FF6xxx independent effect producers are exempt.
            if(XM8717::enabled && xm8658GameStatus==8 && xm8708Slot<0x400u &&
               bank!=xm8658SceneryPal && bank!=xm8657HudPal &&
               XM8723::GrayEcho((const std::uint16_t*)pof->Obj,
                   (unsigned)pof->nCount,(unsigned)GetCurrentFrame(),
                   (unsigned)v,pal,pof->xm8717Sources[xm8708Slot])){
                bank=xm8659InkPal;
                xm8722EchoGray=true;
                xm8667FxMasked=false;
                xm8701LightingBody=false;
                xm8669IndexedFull=false;
                xm8702P1Mirrored=false;
                static unsigned long long count=0;
                if((++count)<=16ull || count%8192ull==0ull)
                    std::fprintf(stderr,
                        "XM87.23 ECHO_MONO frame=%u pal=%u tile=%u source=%06X count=%llu\n",
                        (unsigned)GetCurrentFrame(),pal,tile,
                        (unsigned)pof->xm8717Sources[xm8708Slot].a0,count);
            }
            XM8709::Record((const UINT16*)pof->Obj,pof->nCount,
                (unsigned)GetCurrentFrame(),pof->nShiftX,pof->nShiftY);
            const char* xm8702Grade=xm8722EchoGray?"echo_source_gray":bank==xm8658SceneryPal?"scenery":bank==xm8657HudPal?"hud":
                xm8701LightingBody?"lighting_body":xm8702P1Mirrored?"fx_p1_mirror":xm8667FxMasked?"fx_masked":xm8669IndexedFull?"p1_fx_index":bank==xm8657FxPal?"fx":
                bank==xm8659InkPal?"fighter":"mono";
            xm8665AuditObject((unsigned)GetCurrentFrame(),xm8658GameStatus,(unsigned)v,
                pal,tile,wide,tall,x,y,xm8702Grade);
            xm8702ProbePalette((unsigned)GetCurrentFrame(),xm8658GameStatus,
                (unsigned)v,pal,tile,xm8702Grade);
            xm8705AuditPens((unsigned)GetCurrentFrame(),xm8658GameStatus,
                (unsigned)v,pal,tile,wide,tall,x,y,xm8702Grade);
            XM8707::TraceObject((unsigned)GetCurrentFrame(),
                (unsigned)((ps-(UINT16*)pof->Obj)/4), (unsigned)ZValue,
                nLevelFrom, nLevelTo, (unsigned)v,pal,tile,(unsigned)a,
                wide,tall,x,y,xm8702Grade);
            // One-frame lag and skipped blanks are resolved at CpsObjGet.
            // The old palette/geometry body decision is logged for comparison.
            XM8717DrawAudit(pof,xm8708Slot,(unsigned)GetCurrentFrame(),
                (unsigned)v,pal,tile,wide,tall,(unsigned)a,x,y,xm8702Grade);

            // XM87.25: only the same directly owned, already-grayscale P1/P2 body.
            // Zero ancestry guessing: no FX, source-echo, HUD or scenery promotion.
            if((unsigned)v==6u && XM8745::OpeningPhase(xm8745LivePhase) &&
               bank==xm8659InkPal && !xm8722EchoGray &&
               XM8717::enabled && xm8708Slot<0x400u){
                const auto& src=pof->xm8717Sources[xm8708Slot];
                xm8725FighterObject=XM8717::Recent(src,(unsigned)GetCurrentFrame()) &&
                    (src.owner==1u || src.owner==2u);
                if(xm8725FighterObject && xm8745LivePhase!=8){
                    static unsigned earlyCount=0;
                    if(++earlyCount<=12u || earlyCount%4096u==0u)
                        std::fprintf(stderr,
                            "XM87.45 OUTLINE_OPEN frame=%u phase=%02X owner=%u tile=%u count=%u\n",
                            (unsigned)GetCurrentFrame(),(unsigned)xm8745LivePhase,
                            (unsigned)src.owner,(unsigned)n,earlyCount);
                }
            }

// XM87.77: during character select, reuse the combat direct-outline
// pass on the native large fighter display sprites themselves. Do
// not require owner provenance; selection art is not a live combat
// body. Keep portraits, selectors, HUD and scenery excluded by the
// shared Stage87.77 classifier.
if(!xm8725FighterObject && XM8725::Enabled() &&
   XM8776Select::CharacterDisplay((unsigned)xm8745LivePhase,(unsigned)v,
        (unsigned)a&31u,(unsigned)n,x,y,wide,tall,
        xm8657Hud((unsigned)v,(unsigned)a&31u,(unsigned)n,wide,tall),
        xm8654KnownNativeScenery((unsigned)v,(unsigned)n,(unsigned)a&31u))){
    xm8725FighterObject=true;
    static unsigned selectOutlineObjects=0u;
    if(++selectOutlineObjects<=12u || (selectOutlineObjects%2048u)==0u)
        std::fprintf(stderr,
            "XM87.77 SELECT_CHARACTER_OUTLINE frame=%u tile=%u pal=%u size=%ux%u count=%u\n",
            (unsigned)GetCurrentFrame(),(unsigned)n,(unsigned)a&31u,
            (unsigned)wide,(unsigned)tall,selectOutlineObjects);
}
            if(XM8725::Enabled() && XM8745::OpeningPhase(xm8745LivePhase) && (unsigned)v==6u){
                const bool owned=XM8717::enabled && xm8708Slot<0x400u &&
                    XM8717::Recent(pof->xm8717Sources[xm8708Slot],(unsigned)GetCurrentFrame()) &&
                    (pof->xm8717Sources[xm8708Slot].owner==1u ||
                     pof->xm8717Sources[xm8708Slot].owner==2u);
                XM8725::Observe((unsigned)GetCurrentFrame(),bank==xm8659InkPal,
                    owned,xm8725FighterObject);
            }
            CpstPal=xm8667FxMasked?xm8667LocalFxPal:bank+(pal<<4);

        }

		// xmvsf3v3: the HUD's mini portraits in each fighter's colour, decoded from the program ROM
		UINT32* pStockPal = CpstPal;
		UINT32 nPortraitPal[16];
		UINT32 nPortraitColor = 0;
		if (CpsXmvSixEnabled && XmvPortraitObject(n, BURN_ENDIAN_SWAP_INT16(ps[0]), BURN_ENDIAN_SWAP_INT16(ps[1]), a, CpsXmvWide != 0, &nPortraitColor)) {
			UINT16 nColors[16];
			if (!XmvPortraitColors(CpsRom, nCpsRomLen, n, nPortraitColor, nColors)) {
				continue;	// the marker isn't a palette, so don't draw it with one
			}
			for (INT32 pen = 0; pen < 16; pen++) {
				UINT32 c = nColors[pen];
				nPortraitPal[pen] = BurnHighCol(((c >> 8) & 15) * 17, ((c >> 4) & 15) * 17, (c & 15) * 17, 0);
			}
			if(xm8657PaletteActive){
                const bool hud=xm8657Hud((unsigned)v,(unsigned)a&31u,(unsigned)n,
                                       (((unsigned)a>>8)&15u)+1u,(((unsigned)a>>12)&15u)+1u);
                for(unsigned pen=0;pen<16u;++pen)
                    nPortraitPal[pen]=hud?xm8657HalfSaturation(nPortraitPal[pen]):
                       (((unsigned)v==5u || (unsigned)v==6u)?xm8659FighterInk(nPortraitPal[pen]):xm8657Gray(nPortraitPal[pen]));
            }
			CpstPal = nPortraitPal;
		}

                // XM87.46: source-isolated selection character contrast. Do not
        // apply the 96 black point to the entire selection screen/backdrop.
        UINT32 xm8746SelectionPens[16];
        const unsigned xm8746Wide=((unsigned)a>>8 & 15u)+1u;
        const unsigned xm8746Tall=((unsigned)a>>12 & 15u)+1u;
        if(xm8657PaletteActive && CpstPal &&
           XM8746::SelectionArt(xm8745LivePhase,(unsigned)v,x,y,
               xm8746Wide,xm8746Tall,
               xm8657Hud((unsigned)v,(unsigned)a&31u,(unsigned)n,xm8746Wide,xm8746Tall),
               xm8654KnownNativeScenery((unsigned)v,(unsigned)n,(unsigned)a&31u))){
            for(unsigned ink=0;ink<16u;++ink)
                xm8746SelectionPens[ink]=XM8746::Num5Ink(CpstPal[ink]);
            CpstPal=xm8746SelectionPens;
            static unsigned selectObjects=0;
            if(++selectObjects<=16u || selectObjects%1024u==0u)
                std::fprintf(stderr,
                  "XM87.46 SELECT_NUM5 phase=%02X tile=%u pal=%u size=%ux%u objects=%u\n",
                  (unsigned)xm8745LivePhase,(unsigned)n,(unsigned)a&31u,
                  xm8746Wide,xm8746Tall,selectObjects);
        }
// XM87.77: selection sprites are PRIORITY SEVEN; the earlier priority-six
        // Num5 pass never covered their native palette banks. Scope the 96
        // black point to fighter displays and selectable portrait tiles only.
        UINT32 xm8776SelectPens[16];
        if(xm8657PaletteActive && CpstPal &&
           XM8776Select::Subject((unsigned)xm8745LivePhase,(unsigned)v,
                (unsigned)a&31u,(unsigned)n,x,y,xm8746Wide,xm8746Tall,
                xm8657Hud((unsigned)v,(unsigned)a&31u,(unsigned)n,xm8746Wide,xm8746Tall),
                xm8654KnownNativeScenery((unsigned)v,(unsigned)n,(unsigned)a&31u))){
           for(unsigned ink=0;ink<16u;++ink)
                xm8776SelectPens[ink]=XM8746::Num5Ink(CpstPal[ink]);
           CpstPal=xm8776SelectPens;
           static unsigned objects=0;
           if(++objects<=12u || objects%4096u==0u)
                std::fprintf(stderr,"XM87.77 SELECT_CHARACTER_NUM5 frame=%u tile=%u pal=%u count=%u\n",
                    (unsigned)GetCurrentFrame(),(unsigned)n,(unsigned)a&31u,objects);
        }
        // XM87.42: old temporary-palette hook removed. No portrait/name edits.
		nFlip = (a >> 5) & 3;
		// Find out sprite size
		bx = ((a >> 8) & 15) + 1;
		by = ((a >> 12) & 15) + 1;

		// Take care with tiles if the sprite goes off the screen
		if (x < 0 || y < 0 || x + (bx << 4) > (nCpsScreenWidth-1) || y + (by << 4) > (nCpsScreenHeight-1)) {
			nCpstType = CTT_16X16 | CTT_CARE;
		} else {
			nCpstType = CTT_16X16;
		}

//		if (v == 0) {
//			bprintf(PRINT_IMPORTANT, _T("  - %4i: 0x%04X 0x%04X 0x%04X 0x%04X\n"), ZValue - (UINT16)nMaxZValue, ps[0], ps[1], ps[2], ps[3]);
//		}

		nCpstFlip = nFlip;
        const bool xm8725DoCopies=xm8725FighterObject && XM8725::Enabled();
        const bool xm8726UseUnion=xm8725DoCopies && XM8726::State().armed;
        UINT32 xm8725Ink[4][16];
        if(xm8725DoCopies && !xm8726UseUnion)XM8725::Palettes(xm8725Ink,(unsigned)GetCurrentFrame());
        // XM87.42: this palette storage lives through the complete sprite draw.
        // Its owner is identified in the genuine priority-7 object list.
        UINT32 xm8742HpPens[16];
        if(XM87108::HudPhase(xm8658GameStatus) && CpstPal &&
           XM8745::HpObject((unsigned)v,(unsigned)a&31u,(unsigned)n,x,y)){
            UINT32* xm8742Original=CpstPal;
            XM8745::Stats xm8745Stats;
            const unsigned changed=XM8745::Make(xm8742Original,xm8742HpPens,(unsigned)a&31u,&xm8745Stats);
            if(changed)CpstPal=xm8742HpPens;
            if(changed && xm8658GameStatus>=0x0a){
                static int xm87108LastHpPhase=-1;
                if(xm87108LastHpPhase!=xm8658GameStatus){
                    xm87108LastHpPhase=xm8658GameStatus;
                    std::fprintf(stderr,"XM87.108 HUD_BLUE_LIFETIME status=%02X priority=%u pal=%u tile=%u\n",
                        (unsigned)xm8658GameStatus,(unsigned)v,(unsigned)a&31u,(unsigned)n);
                }
            }
            static unsigned xm8742Matches=0u,xm8742Modified=0u;
            ++xm8742Matches;
            if(changed)++xm8742Modified;
            if(xm8742Matches<=24u || (xm8742Matches%4096u)==0u)
                std::fprintf(stderr,
                    "XM87.45 HP_RGB frame=%u tile=%u pal=%u v=%u x=%d y=%d "
                    "changedPens=%u fillPens=%u grayPens=%u matched=%u modified=%u ink=%08X\n",
                    (unsigned)GetCurrentFrame(),(unsigned)n,(unsigned)a&31u,
                    (unsigned)v,x,y,changed,xm8745Stats.fill,xm8745Stats.frame,
                    xm8742Matches,xm8742Modified,
                    (unsigned)xm8742Original[1]);
        }
        UINT32* xm8725NormalPalette=CpstPal;
        const INT32 xm8725NormalType=nCpstType;
        const INT32 xm8725Passes=(xm8725DoCopies && !xm8726UseUnion)?5:1;
        for(INT32 xm8725Pass=0;xm8725Pass<xm8725Passes;++xm8725Pass){
            const bool xm8725OffsetPass=xm8725DoCopies && !xm8726UseUnion && xm8725Pass<4;
            const INT32 xm8725ShiftX=xm8725OffsetPass?XM8725::OffsetX(xm8725Pass):0;
            const INT32 xm8725ShiftY=xm8725OffsetPass?XM8725::OffsetY(xm8725Pass):0;
            CpstPal=xm8725OffsetPass?xm8725Ink[xm8725Pass]:xm8725NormalPalette;
            nCpstType=xm8725OffsetPass?(xm8725NormalType|CTT_CARE):xm8725NormalType;
		for (dy = 0; dy < by; dy++) {
			for (dx = 0; dx < bx; dx++) {
				INT32 ex, ey;

				if (nFlip & 1) {
					ex = (bx - dx - 1);
				} else {
					ex = dx;
				}

				if (nFlip & 2) {
					ey = (by - dy - 1);
				} else {
					ey = dy;
				}

				nCpstX = x + (ex << 4) + xm8725ShiftX;
				nCpstY = y + (ey << 4) + xm8725ShiftY;

//				nCpstTile = n + (dy << 4) + dx;								// normal version
				nCpstTile = (n & ~0x0F) + (dy << 4) + ((n + dx) & 0x0F);	// pgear fix
				
				if (Sfa2ObjHack) {
					// hack to not render blue squares on high score screen after name entry - they should be hidden by priority from what I can see
					if (nCpstTile == 0x1a410 && (a & 0x1f) == 0 && (nCpstX == 0x40 || nCpstX == 0x100) && nCpstY == 0x50 && v == 0 && nLevelFrom == 0 && nLevelTo == 0) continue;
					if (nCpstTile == 0x1a411 && (a & 0x1f) == 0 && (nCpstX == 0x50 || nCpstX == 0x110) && nCpstY == 0x50 && v == 0 && nLevelFrom == 0 && nLevelTo == 0) continue;
					//if (ps[2] == 0xa410) bprintf(PRINT_NORMAL, _T("%x, %x, %x, %x, %x, %x, %x\n"), nCpstTile, a & 0x1f, nCpstX, nCpstY, v, nLevelFrom, nLevelTo);
				}
				
				nCpsBlend = (blendtable) ? blendtable[nCpstTile] : 0;
				nCpstTile <<= 7;						// Find real tile address					

                // Match the native CPS2 GFX address wrap; unwrapped tile
                // addresses often exceed physical decoded graphics length.
                const unsigned xm8726TileOffset=(unsigned)nCpstTile & nCpsGfxMask;
                if(xm8726UseUnion && CpsGfx && nCpsGfxLen>=128u &&
                   xm8726TileOffset<=nCpsGfxLen-128u){
                    XM8726::State().Stamp(CpsGfx+xm8726TileOffset,
                        nCpstX,nCpstY,(unsigned)nCpstFlip);
                }
                // Copy the current framebuffer only WHERE the combined
                // fighter body is opaque. Let the normal CPS renderer draw
                // the offset tile, then undo any pixels within that body.
                XM8728::Mask::Backup xm8728Saved;
                if(xm8725OffsetPass && xm8728Mask.ready)
                    xm8728Mask.Save(xm8728Saved,(const std::uint8_t*)pBurnDraw,
                        nBurnPitch,nCpstX,nCpstY);
                pCpstOne();
                if(xm8774Cursor!=XM8773::None && !xm8725OffsetPass &&
                   CpsGfx && xm8726TileOffset<=nCpsGfxLen-128u && CpstPal){
                    XM8773::State().StampTile(xm8774Cursor,nCpstX,nCpstY,
                        CpsGfx+xm8726TileOffset,(unsigned)nCpstFlip,CpstPal,
                        CpsPal+(27u<<4),(unsigned)nBurnBpp);
                }
                if(xm8725OffsetPass && xm8728Mask.ready)
                    xm8728Mask.Restore(xm8728Saved,(std::uint8_t*)pBurnDraw,
                        nBurnPitch,nCpstX,nCpstY);
				nCpsBlend = 0;
			}
		}

        }
        CpstPal=xm8725NormalPalette;
        nCpstType=xm8725NormalType;
        if(xm8725DoCopies && !xm8726UseUnion)
            XM8725::ObjectDrawn((unsigned)GetCurrentFrame(),(unsigned)(bx*by));
        if(xm8725DoCopies)xm8728Mask.Report();
		CpstPal = pStockPal;	// never leave it pointing at nPortraitPal
	}


    // XM87.73: original cursor pixels are rendered once above selection grading.

    XM8798Latency::state().afterObjects();
	return 0;
}

// Final crash sprites

INT32 FcrashObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	Get = CpsRam90 + 0x50c8;
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;
		INT32 n, y, x, a;
		
		y = BURN_ENDIAN_SWAP_INT16(ps[-1]);
		
		if (y == 0x8000) { // end of sprite list
			break;
		}
		
		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]);

		po[0] = n & 0xff;
		po[1] = n >> 8;
		po[2] = a & 0xff;
		po[3] = a >> 8;
		po[4] = x & 0xff;
		po[5] = x >> 8;
		po[6] = y & 0xff;
		po[7] = y >> 8;	

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}

	return 0;
}

INT32 KodbObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	Get = CpsBootlegSpriteRam + 0x1000;
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;
		INT32 n, y, x, a;
		
		y = BURN_ENDIAN_SWAP_INT16(ps[-1]);
		
		if (y == 0xffff) { // end of sprite list
			break;
		}
		
		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]);

		po[0] = n & 0xff;
		po[1] = n >> 8;
		po[2] = a & 0xff;
		po[3] = a >> 8;
		po[4] = x & 0xff;
		po[5] = x >> 8;
		po[6] = y & 0xff;
		po[7] = y >> 8;

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}
	
	return 0;
}

INT32 DinopicObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	Get = CpsBootlegSpriteRam + 0x1000;
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;
		INT32 n, y, x, a;
		
		y = BURN_ENDIAN_SWAP_INT16(ps[-1]);
		
		if (y == 0x8000) { // end of sprite list
			break;
		}
		
		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]);
		
		po[0] = n & 0xff;
		po[1] = n >> 8;
		po[2] = a & 0xff;
		po[3] = a >> 8;
		po[4] = x & 0xff;
		po[5] = x >> 8;
		po[6] = y & 0xff;
		po[7] = y >> 8;

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}

	return 0;
}

INT32 DaimakaibObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	// writes a blank sprite, followed by end of sprite list marker, start at 0x10 to ignore these
	Get = CpsBootlegSpriteRam + 0x1010;
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;
		INT32 n, y, x, a;
		
		y = BURN_ENDIAN_SWAP_INT16(ps[-1]);
		
		if (y == 0x8000) { // end of sprite list
			break;
		}
		
		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]);
		
		n = GfxRomBankMapper(GFXTYPE_SPRITES, n);
		if (n == -1) continue;
		
		n |= (y & 0x6000) << 3; // high bits of address

		po[0] = n & 0xff;
		po[1] = n >> 8;
		po[2] = a & 0xff;
		po[3] = a >> 8;
		po[4] = x & 0xff;
		po[5] = x >> 8;
		po[6] = y & 0xff;
		po[7] = y >> 8;

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}

	return 0;
}

INT32 WofhObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	Get = CpsRam90 + 0x1000;
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;
		INT32 n, y, x, a;
		
		y = BURN_ENDIAN_SWAP_INT16(ps[-1]);
		
		if (y == 0x8000) { // end of sprite list
			break;
		}
		
		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]);

		po[0] = n & 0xff;
		po[1] = n >> 8;
		po[2] = a & 0xff;
		po[3] = a >> 8;
		po[4] = x & 0xff;
		po[5] = x >> 8;
		po[6] = y & 0xff;
		po[7] = y >> 8;

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}

	return 0;
}

INT32 Sf2mdtObjGet()
{
	INT32 i;
	UINT8 *pg, *po;
	struct ObjFrame* pof;
	UINT8* Get = NULL;
	
	pof = of + nGetNext;

	pof->nCount = 0;

	po = pof->Obj;
	pof->nShiftX = -0x40;
	pof->nShiftY = -0x10;

	Get = CpsBootlegSpriteRam + 0x1000;
	
	if (Get==NULL) return 1;

	// Make a copy of all active sprites in the list
	for (pg = Get, i = 0; i < nMax; pg += 8, i++) {
		UINT16* ps = (UINT16*)pg;
		INT32 n, y, x, a;
		
		y = BURN_ENDIAN_SWAP_INT16(ps[-1]);
		
		if (y == 0x8000) { // end of sprite list
			break;
		}
		
		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]) + 0x03;
		
		po[0] = n & 0xff;
		po[1] = n >> 8;
		po[2] = a & 0xff;
		po[3] = a >> 8;
		po[4] = x & 0xff;
		po[5] = x >> 8;
		po[6] = y & 0xff;
		po[7] = y >> 8;

		pof->nCount++;
		po += 8;
	}

	nGetNext++;
	if (nGetNext >= nFrameCount) {
		nGetNext = 0;
	}

	return 0;
}

INT32 FcrashObjDraw(INT32 nLevelFrom,INT32 nLevelTo)
{
	INT32 i; UINT16 *ps; INT32 nPsAdd;
	struct ObjFrame *pof;
	(void)nLevelFrom; (void)nLevelTo;

	// Draw the earliest frame we have in history
	pof=of+nGetNext;

	// Point to Obj list
	ps=(UINT16 *)pof->Obj;

	nPsAdd=4;

	// Go through all the Objs
	for (i=0; i<pof->nCount; i++,ps+=nPsAdd) {
		INT32 x,y,n,a; INT32 nFlip;

		n = BURN_ENDIAN_SWAP_INT16(ps[0]);
		a = BURN_ENDIAN_SWAP_INT16(ps[1]);
		x = BURN_ENDIAN_SWAP_INT16(ps[2]);
		y = BURN_ENDIAN_SWAP_INT16(ps[3]);
		
		x &= 0x1ff;
		y &= 0x1ff;
		
		x -= 16;
		y = 224 - y;

		// Find the palette for the tiles on this sprite
		CpstPal = CpsPal + ((a & 0x1F) << 4);

		nFlip=(a>>5)&3;		

		// Take care with tiles if the sprite goes off the screen
		if (x<0 || y<0 || x+(1<<4)>(nCpsScreenWidth-1) || y+(1<<4)>(nCpsScreenHeight-1)) {
			nCpstType=CTT_16X16 | CTT_CARE;
		} else {
			nCpstType=CTT_16X16;
		}

		nCpstFlip=nFlip;
		nCpstX=x;
		nCpstY=y;
		nCpstTile = n;
		nCpstTile <<= 7;
		CpstOneObjDoX[0]();
	}
	
	return 0;
}

// xmvsf3v3: the sprite lag buffers (the Obj pointers are rebuilt by CpsObjInit, never saved).
// The Z buffer and its counters aren't saved: only drawing changes them, so they'd make a state
// depend on which frames were drawn (netplay replays and run-ahead's hidden frames aren't), and
// each frame's Z values start above what's already in the buffer
INT32 CpsObjVisualScan(INT32 nAction)
{
	if (!CpsXmvSixEnabled || !(nAction & ACB_DRIVER_DATA)) return 0;

	struct BurnArea ba;
	memset(&ba, 0, sizeof(ba));
	ba.Data = ObjMem;
	ba.nLen = (nMax << 3) * nFrameCount;
	ba.szName = "XMvsSF 3v3 sprite lag buffers";
	BurnAcb(&ba);

	SCAN_VAR(nGetNext);
	for (INT32 i = 0; i < nFrameCount; i++) {
		SCAN_VAR(of[i].nShiftX);
		SCAN_VAR(of[i].nShiftY);
		SCAN_VAR(of[i].nCount);
	}

	if (nAction & ACB_WRITE) {
		if (nGetNext < 0 || nGetNext >= nFrameCount) return 1;
		for (INT32 i = 0; i < nFrameCount; i++) {
			if (of[i].nCount < 0 || of[i].nCount > nMax) return 1;
		}
	}

	return 0;
}
