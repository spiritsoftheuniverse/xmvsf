#include "cps.h"
#include "xmvsf_stage87_25_direct_outline.h" // Existing fighter outline enable gate
#include "xmvsf_stage87_26_seamless_bloom.h" // Stage87.26 exterior-only fighter glow
#include "xmvsf_stage87_28_interior_mask.h" // ROM-derived fighter opacity stencil
extern bool xm8657PaletteActive;
extern int xm8658GameStatus;
#include "xmvsf_stage86_64_strong_noir.h" // XM86.64 strong saturated dark backgrounds
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "../../../burner/sdl/xmvsf_stage86_49_state.h" // XM86.49 scoped overlay
// Referenced by native palette handling before the later OBJ capture declarations.
extern int xm77StageResolved;
extern XM8649VisualState xm8649Visual;
// CPS - Draw

UINT8 CpsRecalcPal = 0;			// Flag - If it is 1, recalc the whole palette

static INT32 LayerCont;
INT32 nStartline, nEndline;
INT32 nRasterline[MAX_RASTER + 2];

INT32 nCpsLcReg = 0;						// Address of layer controller register
INT32 CpsLayEn[6] = {0, 0, 0, 0, 0, 0};	// bits for layer enable
INT32 MaskAddr[4] = {0, 0, 0, 0};

INT32 nCpsScreenWidth = 384;
INT32 nCpsScreenHeight = 224;
INT32 nCpsGlobalXOffset = 0;
INT32 nCpsGlobalYOffset = 0;

INT32 CpsLayer1XOffs = 0;
INT32 CpsLayer2XOffs = 0;
INT32 CpsLayer3XOffs = 0;
INT32 CpsLayer1YOffs = 0;
INT32 CpsLayer2YOffs = 0;
INT32 CpsLayer3YOffs = 0;

INT32 Cps1DisableBgHi = 0;
INT32 CpsDisableRowScroll = 0;

INT32 Cps1OverrideLayers = 0;
INT32 nCps1Layers[4] = { -1, -1, -1, -1 };
INT32 nCps1LayerOffs[3] = { -1, -1, -1 };

static void Cps1Layers();
static void Cps2Layers();

typedef INT32  (*CpsObjDrawDoFn)(INT32,INT32);
typedef INT32  (*CpsScrXDrawDoFn)(UINT8 *,INT32,INT32);
typedef void (*CpsLayersDoFn)();
typedef INT32  (*CpsrPrepareDoFn)();
typedef INT32  (*CpsrRenderDoFn)();

CpsObjDrawDoFn  CpsObjDrawDoX;
CpsScrXDrawDoFn CpsScr1DrawDoX;
CpsScrXDrawDoFn CpsScr3DrawDoX;
CpsLayersDoFn   CpsLayersDoX;
CpsrPrepareDoFn CpsrPrepareDoX;
CpsrRenderDoFn  CpsrRenderDoX;

void DrawFnInit()
{
	if(Cps == 2) {
		CpsLayersDoX   = Cps2Layers;
		CpsScr1DrawDoX = Cps2Scr1Draw;
		CpsScr3DrawDoX = Cps2Scr3Draw;
		CpsObjDrawDoX  = Cps2ObjDraw;
		CpsrPrepareDoX = Cps2rPrepare;
		CpsrRenderDoX  = Cps2rRender;
	} else {
		CpsLayersDoX   = Cps1Layers;
		CpsScr1DrawDoX = Cps1Scr1Draw;
		CpsScr3DrawDoX = Cps1Scr3Draw;
		CpsObjDrawDoX  = Cps1ObjDraw;
		CpsrPrepareDoX = Cps1rPrepare;
		CpsrRenderDoX  = Cps1rRender;
	}
}

static INT32 DrawScroll1(INT32 i)
{
	// Draw Scroll 1
	INT32 nOff, nScrX, nScrY;
	UINT8 *Find;

	nOff = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x02)));
	if (Cps1OverrideLayers && nCps1LayerOffs[0] != -1) {
		nOff = BURN_ENDIAN_SWAP_INT16(nCps1LayerOffs[0]);
	}

	// Get scroll coordinates
	nScrX = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x0c))); // Scroll 1 X
	nScrY = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x0e))); // Scroll 1 Y

	nScrX += 0x40 - nCpsGlobalXOffset;

//	bprintf(PRINT_NORMAL, _T("1 %x, %x, %x\n"), nOff, nScrX, nScrY);

	nScrX += CpsLayer1XOffs;
	nScrY += 0x10 - nCpsGlobalYOffset;
	nScrY += CpsLayer1YOffs;
	nOff <<= 8;
	nOff &= 0xffc000;
	Find = CpsFindGfxRam(nOff, 0x4000);
	if (Find == NULL) {
		return 1;
	}
	CpsScr1DrawDoX(Find, nScrX, nScrY);
	return 0;
}

static INT32 DrawScroll2Init(INT32 i)
{
	// Draw Scroll 2
	INT32 nScr2Off; INT32 n;

	nScr2Off = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x04)));
	if (Cps1OverrideLayers && nCps1LayerOffs[1] != -1) {
		nScr2Off = BURN_ENDIAN_SWAP_INT16(nCps1LayerOffs[1]);
	}

	// Get scroll coordinates
	nCpsrScrX= BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x10))); // Scroll 2 X
	nCpsrScrY= BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x12))); // Scroll 2 Ytess

	// Get row scroll information
	n = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x22)));

	nScr2Off <<= 8;

	nCpsrScrX += 0x40 - nCpsGlobalXOffset;

//	bprintf(PRINT_NORMAL, _T("2 %x, %x, %x\n"), nScr2Off, nCpsrScrX, nCpsrScrY);

	nCpsrScrX += CpsLayer2XOffs;
	nCpsrScrX &= 0x03FF;

	nCpsrScrY += 0x10 - nCpsGlobalYOffset;
	nCpsrScrY += CpsLayer2YOffs;
	nCpsrScrY &= 0x03FF;

	nScr2Off &= 0xFFC000;
	CpsrBase = CpsFindGfxRam(nScr2Off, 0x4000);
	if (CpsrBase == NULL) {
		return 1;
	}

	CpsrRows = NULL;

	if ((n & 1) && !CpsDisableRowScroll) {
		INT32 nTab, nStart;
		// Find row scroll table:

		nTab = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x08)));
		nStart = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x20)));

		nTab <<= 8;
		nTab &= 0xFFF800; // Vampire - Row scroll effect in VS screen background

		CpsrRows = (UINT16 *)CpsFindGfxRam(nTab, 0x0800);

		// Find start offset
		nCpsrRowStart = nStart + 16;
	}

	CpsrPrepareDoX();
	return 0;
}

inline static INT32 DrawScroll2Exit()
{
	CpsrBase = NULL;
	nCpsrScrX = 0;
	nCpsrScrY = 0;
	CpsrRows = NULL;
	return 0;
}

inline static INT32 DrawScroll2Do()
{
	if (CpsrBase == NULL) {
		return 1;
	}
	CpsrRenderDoX();
	return 0;
}

static INT32 DrawScroll3(INT32 i)
{
	// Draw Scroll 3
	INT32 nOff, nScrX, nScrY;
	UINT8 *Find;

	nOff = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x06)));
	if (Cps1OverrideLayers && nCps1LayerOffs[2] != -1) {
		nOff = BURN_ENDIAN_SWAP_INT16(nCps1LayerOffs[2]);
	}

	// Get scroll coordinates
	nScrX = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x14))); // Scroll 3 X
	nScrY = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[i] + 0x16))); // Scroll 3 Y

	nScrX += 0x40 - nCpsGlobalXOffset;

//	bprintf(PRINT_NORMAL, _T("3 %x, %x, %x\n"), nOff, nScrX, nScrY);

	nScrX += CpsLayer3XOffs;
	nScrY += 0x10 - nCpsGlobalYOffset;
	nScrY += CpsLayer3YOffs;

	nOff <<= 8;
	nOff &= 0xffc000;
	Find=CpsFindGfxRam(nOff, 0x4000);
	if (Find == NULL) {
		return 1;
	}
	CpsScr3DrawDoX(Find, nScrX, nScrY);
	return 0;
}

// Star Control bit and other info's RE by loic.petit @ https://gitlab.com/loic.petit/cps2-reverse/-/blob/master/DLs/DL-0921/doc/star-raster.md
static INT32 DrawStar(INT32 nLayer)
{
	INT32 nStar, nStarXPos, nStarYPos, nStarColour;
	UINT8* pStar = CpsStar + (nLayer << 12);

	for (nStar = 0; nStar < 0x1000; nStar++) {
		nStarColour = pStar[nStar];

		if (nStarColour != 0x0F) {
			nStarXPos = (((nStar >> 8) << 5) - BURN_ENDIAN_SWAP_INT16(*((INT16*)(CpsSaveReg[0] + 0x18 + (nLayer << 2)))) + (nStarColour & 0x1F) - 64) & 0x01FF;
			nStarYPos = ((nStar & 0xFF) - BURN_ENDIAN_SWAP_INT16(*((INT16*)(CpsSaveReg[0] + 0x1A + (nLayer << 2)))) - 16) & 0xFF;

			if (nStarXPos < nCpsScreenWidth && nStarYPos < nCpsScreenHeight) {
				nStarColour = ((nStarColour & 0xE0) >> 1) + ((GetCurrentFrame() >> 4) % ((nStarColour & 0x80) ? 0xe : 0xf));
				PutPix(pBurnDraw + (nBurnPitch * nStarYPos) + (nBurnBpp * nStarXPos), CpsPal[0x0800 + (nLayer << 9) + nStarColour]);
			}
		}
	}

	return 0;
}

static void Cps1Layers()
{
  INT32 Draw[4]={-1,-1,-1,-1};
  INT32 nDrawMask=0;
  INT32 i=0;

  nDrawMask=1; // Sprites always on
  LayerCont = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[0] + nCpsLcReg)));
  // Get correct bits from Layer Controller
  if (LayerCont & CpsLayEn[1]) nDrawMask|=2;
  if (LayerCont & CpsLayEn[2]) nDrawMask|=4;
  if (LayerCont & CpsLayEn[3]) nDrawMask|=8;
  nDrawMask&=nBurnLayer;   // User choice of layers to display
  
  // Layer control:
  Draw[0]=(LayerCont>>12)&3; // top layer
  Draw[1]=(LayerCont>>10)&3;
  Draw[2]=(LayerCont>> 8)&3;
  Draw[3]=(LayerCont>> 6)&3; // bottom layer (most covered up)
  
  if (Cps1OverrideLayers) {
	nDrawMask = 1;
	Draw[0] = nCps1Layers[0];
	Draw[1] = nCps1Layers[1];
	Draw[2] = nCps1Layers[2];
	Draw[3] = nCps1Layers[3];
	if (Draw[1] != -1) nDrawMask |= 2;
	if (Draw[2] != -1) nDrawMask |= 4;
	if (Draw[3] != -1) nDrawMask |= 8;
	nDrawMask &= nBurnLayer;
  }
  
  // Check for repeated layers and if there are any, the lower layer is omitted
#define CRP(a,b) if (Draw[a]==Draw[b]) Draw[b]=-1;
  CRP(0,1) CRP(0,2) CRP(0,3) CRP(1,2) CRP(1,3) CRP(2,3)
#undef CRP

  for (i = 0; i < 2; i++) {
	  if (LayerCont & CpsLayEn[4 + i]) {
		  DrawStar(i);
	  }
  }

  // prepare layer 2
  DrawScroll2Init(0);

  // draw layers, bottom -> top
  for (i=3;i>=0;i--)
  {
    INT32 n=Draw[i]; // Find out which layer to draw

    if (n==0) {
	  if (nDrawMask & 1)  CpsObjDrawDoX(0,7);

	  if (!Cps1DisableBgHi && i+1 < 4) {
		nBgHi=1;
		switch (Draw[i+1]) {
			case 1:
				if (nDrawMask & 2) 	DrawScroll1(0);
				break;
			case 2:
				if (nDrawMask & 4)  DrawScroll2Do();
				break;
			case 3:
				if (nDrawMask & 8)  DrawScroll3(0);
				break;
		}
		nBgHi=0;
	  }
	}

    // Then Draw the scroll layer on top
    switch (n) {
		case 1:
			if (nDrawMask & 2) DrawScroll1(0);
			break;
		case 2:
			if (nDrawMask & 4) DrawScroll2Do();
			break;
		case 3:
			if (nDrawMask & 8) DrawScroll3(0);
			break;
	}
  }

  DrawScroll2Exit();
}

static void Cps2Layers()
{
	INT32 Draw[MAX_RASTER][4];
	INT32 Prio[MAX_RASTER][4];
	INT32 nDrawMask[MAX_RASTER];

	CpsObjDrawInit();

	INT32 nSlice = 0;
	do {
		LayerCont = BURN_ENDIAN_SWAP_INT16(*((UINT16 *)(CpsSaveReg[nSlice] + nCpsLcReg)));

		// Determine which layers are enabled
		nDrawMask[nSlice] = 1;								// Sprites always on
		if (LayerCont & CpsLayEn[1]) nDrawMask[nSlice] |= 2;
		if (LayerCont & CpsLayEn[2]) nDrawMask[nSlice] |= 4;
		if (LayerCont & CpsLayEn[3]) nDrawMask[nSlice] |= 8;
		nDrawMask[nSlice] &= nBurnLayer;					// User choice of layers to display

		// Determine layerö priority:
		Draw[nSlice][3] = (LayerCont >> 12) & 3;			// top layer
		Draw[nSlice][2] = (LayerCont >> 10) & 3;
		Draw[nSlice][1] = (LayerCont >> 8) & 3;
		Draw[nSlice][0] = (LayerCont >> 6) & 3;				// bottom layer (most covered up)

		// Determine layer-sprite priority (layer >= sprites -> layer on top)
		INT32 nLayPri = (CpsSaveFrg[0][4] << 8) | CpsSaveFrg[0][5];	// Layer priority register at word (400004)
		Prio[nSlice][3] = (nLayPri >> 12) & 7;
		Prio[nSlice][2] = (nLayPri >> 8) & 7;
		Prio[nSlice][1] = (nLayPri >> 4) & 7;
		Prio[nSlice][0] = 0;

		// Check for repeated layers (if found, discard the lower layer)
#define CRP(a, b) if (Draw[nSlice][a] == Draw[nSlice][b]) Draw[nSlice][b] = -1;
		CRP(3, 2) CRP(3, 1) CRP(2, 1) CRP(3, 0) CRP(2, 0) CRP(1, 0)
#undef CRP

		// Pre-process priorities
		// Higher priority layers must have higher layer-sprite priorities
		// N.B. If this is not the case, masking effects may occur (not emulated)
#if 0
		// Raise sprite priorities of top layers if needed
		INT32 nHighPrio = 0;
		for (INT32 i = 0; i < 4; i++) {
			if (Draw[nSlice][i] > 0) {
				if (Prio[nSlice][Draw[nSlice][i]] < nHighPrio) {
					Prio[nSlice][Draw[nSlice][i]] = nHighPrio;
				} else {
					nHighPrio = Prio[nSlice][Draw[nSlice][i]];
				}
			}
		}
#else
		// Lower sprite priorities of bottom layers if needed
		INT32 nHighPrio = 9999;
		for (INT32 i = 3; i >= 0; i--) {
			if (Draw[nSlice][i] > 0) {
				if (Prio[nSlice][Draw[nSlice][i]] > nHighPrio) {
					Prio[nSlice][Draw[nSlice][i]] = nHighPrio;
				} else {
					nHighPrio = Prio[nSlice][Draw[nSlice][i]];
				}
			}
		}
#endif
		nSlice++;
	} while (nSlice < MAX_RASTER && nRasterline[nSlice]);

	INT32 nPrevPrio = -1;
	for (INT32 nCurrPrio = 0; nCurrPrio < 8; nCurrPrio++) {

		nSlice = 0;
		do {
			for (INT32 i = 0; i < 4; i++) {
				if ((Draw[nSlice][i] >= 0) && Prio[nSlice][Draw[nSlice][i]] == nCurrPrio) {

					// Render sprites between the previous layer and this one
					if ((nDrawMask[0] & 1) && (nPrevPrio < nCurrPrio)) {
						CpsObjDrawDoX(nPrevPrio + 1, nCurrPrio);
						nPrevPrio = nCurrPrio;
					}

					nStartline = nRasterline[nSlice];
					nEndline = nRasterline[nSlice + 1];
					if (!nEndline) {
						nEndline = nCpsScreenHeight;
					}

					// Render layer
					switch (Draw[nSlice][i]) {
						case 1:
							if (nDrawMask[nSlice] & 2) {
								DrawScroll1(nSlice);
							}
							break;
						case 2:
							if (nDrawMask[nSlice] & 4) {
								DrawScroll2Init(nSlice);
								DrawScroll2Do();
								DrawScroll2Exit();
							}
							break;
						case 3:
							if (nDrawMask[nSlice] & 8) {
								DrawScroll3(nSlice);
							}
							break;
					}
				}
			}
			nSlice++;
		}  while (nSlice < MAX_RASTER && nRasterline[nSlice]);
	}

	// Render highest priority sprites
	if ((nDrawMask[0] & 1) && (nPrevPrio < 7)) {
		CpsObjDrawDoX(nPrevPrio + 1, 7);
	}
}

void CpsClearScreen()
{
	if (Cps == 1) {
		switch (nBurnBpp) {
			case 4: {
				UINT32* pClear = (UINT32*)pBurnDraw;
				UINT32 nColour = (fFakeDip & 1) ? 0 : CpsPal[0xbff ^ 15];
				for (INT32 i = 0; i < nCpsScreenWidth * nCpsScreenHeight / 8; i++) {
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
				}
				break;
			}

			case 3: {
				UINT8* pClear = pBurnDraw;
				UINT8 r = (CpsPal[0xbff ^ 15] >> 0) & 0xFF;
				UINT8 g = (CpsPal[0xbff ^ 15] >> 8) & 0xFF;
				UINT8 b = (CpsPal[0xbff ^ 15] >> 16) & 0xFF;
				if (fFakeDip & 1) r = g = b = 0;

				for (INT32 i = 0; i < nCpsScreenWidth * nCpsScreenHeight; i++) {
					*pClear++ = r;
					*pClear++ = g;
					*pClear++ = b;
				}
				break;
			}

			case 2: {
				UINT32* pClear = (UINT32*)pBurnDraw;
				UINT32 nColour = CpsPal[0xbff ^ 15] | CpsPal[0xbff ^ 15] << 16;
				if (fFakeDip & 1) nColour = 0;

				for (INT32 i = 0; i < nCpsScreenWidth * nCpsScreenHeight / 16; i++) {
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
				}
				break;
			}
		}
	} else {
		memset(pBurnDraw, 0, nCpsScreenWidth * nCpsScreenHeight * nBurnBpp);
	}
}

// XM86.57: 0x0c00 native CPS2 palette entries; sprite page is 32*16.
// Keep these arrays stable while Cps2ObjDraw renders (including Wily's
// temporary sprite-only passes). This never modifies any emulated palette RAM.
bool xm8657PaletteActive=false;
UINT32 xm8657MonoPal[512]={};
UINT32 xm8659InkPal[512]={};
UINT32 xm8657HudPal[512]={};
UINT32 xm8657FxPal[512]={};
UINT32 xm8658SceneryPal[512]={};
static UINT32 xm8657OriginalPal[0xC00]={};
static bool xm8657Eligible(){
    if(Cps!=2 || nBurnBpp!=4 || !CpsPal)return false;
    const char* enabled=std::getenv("XM8649_WILY_RENDER");
    if(!enabled || enabled[0]!='1' || enabled[1]!='\0')return false;
    const char* name=BurnDrvGetTextA(DRV_NAME);
    return name && std::strncmp(name,"xmvsf",5)==0;
}
static void xm8657BeginNativeGrade(){
    xm8657PaletteActive=false;
    if(!xm8657Eligible())return;
    const unsigned frame=(unsigned)GetCurrentFrame();
    // XM87.57: the selected custom Wily stage owns its scenery and grading.
    // Native scroll/background palettes are never presented on that stage,
    // except during ROM-native full-screen effects. Keep their original white
    // for the flash classifier in BOTH normal and Double Feature modes.
    // Fighter-specific monochrome, HUD and FX palette arrays remain unchanged.
    const bool wilyNativeUnmodifiedBg=(xm77StageResolved==1 &&
             xm8649Visual.sceneActive && xm8649Visual.stage==5);
    static bool reportedWilyNativeBg=false;
    if(wilyNativeUnmodifiedBg && !reportedWilyNativeBg){
        std::fprintf(stderr,"XM87.57 NATIVE_WILY_BACKGROUND original_palettes=1 double_feature_preserved=1\n");
        reportedWilyNativeBg=true;
    }
    // XM86.64: Magneto is deliberately grayscale. No ROM costume matcher;
    // that experimental path never tinted in live gameplay.
    for(unsigned i=0;i<0xC00u;++i){
        const UINT32 c=CpsPal[i];
        xm8657OriginalPal[i]=c;
        CpsPal[i]=(wilyNativeUnmodifiedBg && i>=0x200u)
                   ? c : xm8664Background(c,frame);
        if(i<512u){
            xm8657MonoPal[i]=xm8657Gray(c);
            xm8659InkPal[i]=xm8659FighterInk(c);
            xm8657HudPal[i]=xm8657HalfSaturation(c);
            xm8657FxPal[i]=c;
            xm8658SceneryPal[i]=xm8664Background(c,frame);
        }
    }
    xm8657PaletteActive=true;
    static bool first=true;
    if(first){
        unsigned long originalSum=0,gradedSum=0,nonblack=0;
        for(unsigned i=0x200u;i<0xc00u;++i){
            const auto a=xm8657Unpack(xm8657OriginalPal[i]);
            const auto b=xm8657Unpack(CpsPal[i]);
            const unsigned lum=xm8657Luma(a.r,a.g,a.b);
            if(lum<4u)continue;
            originalSum+=lum;gradedSum+=xm8657Luma(b.r,b.g,b.b);++nonblack;
        }
        std::fprintf(stderr,"XM86.64 BG_30_PERCENT_70_SAT sample_entries=%lu avg_original=%lu avg_graded=%lu; Wily graded independently\n",
            nonblack,nonblack?originalSum/nonblack:0ul,nonblack?gradedSum/nonblack:0ul);
        std::fprintf(stderr,"XM86.64 NATIVE_PALETTE backgrounds=30pct_peak77 saturation70_hue35s magneto=grayscale fighter_ink96 hud_rgb50 fx_color wily_independent_dark\n");
        first=false;
    }
}
static void xm8657RestoreNativePalette(){
    if(xm8657PaletteActive)std::memcpy(CpsPal,xm8657OriginalPal,sizeof(xm8657OriginalPal));
}

static void DoDraw(INT32 Recalc)
{
	CtvReady();								// Point to correct tile drawing functions

	// xmvsf3v3: a state load restores the effective palette itself; rereading palette RAM would lose it
	if (!CpsPalConsumeRestoredCache()) {
		if (CpsRecalcPal || bCpsUpdatePalEveryFrame) GetPalette(0, 6);
		if (Recalc || bCpsUpdatePalEveryFrame) CpsPalUpdate(CpsSavePal);		// recalc whole palette if needed
	}

    xm8657BeginNativeGrade();
    CpsClearScreen();
    // Initialization occurs BEFORE any native fighter sprites are drawn.
    XM8726::State().Begin(Cps==2 && xm8657PaletteActive &&
        xm8658GameStatus==8 && XM8725::Enabled(),
        nCpsScreenWidth,nCpsScreenHeight,nBurnBpp);
    CpsLayersDoX();
    // XM87.209: the 76-pixel legacy stun meter MUST NOT be painted here.
    // It was an independent native software draw layered under the new
    // 61x6 PNG HUD, causing the user-visible double rainbow/immobile tip.
    // The existing SDL HUD (vid_sdl2.cpp::xm8733Overlay) is the single
    // presentation owner: one pass, PNG base -> live fill -> moving inset.
    // Leave CPS2 layer order, original fighter pixels and hitboxes intact.
    XM8726::State().Composite(pBurnDraw,nBurnBpp,nBurnPitch,
        (unsigned)GetCurrentFrame());
    xm8657RestoreNativePalette(); // original palette for later emulation and capture
}

// STAGE 44: donor-only, opt-in native CPS-2 layer capture.
// Captures rendered scroll layers, original composite and raw sprite-list metadata.
// NO modifications to the source game's emulation or the live XMVSF renderer.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <cstdint>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

extern void XM44WriteBufferedSprites(FILE*, int);
extern void XM47ResetCapturedTiles();
extern void XM47WriteSpriteTiles(FILE*);
static bool xm44Initialised = false;
static bool xm44Enabled = false;
static bool xm44Capturing = false;
static bool xm44Done = false;
static bool xm44Auto = false;
static unsigned long long xm44TotalSamples=0;
static unsigned xm44Batches=0;
static int xm44Sample = 0;
static int xm44RawFrame = 0;
static int xm44Limit = 180;
static int xm44Step = 1;
static std::string xm44Dir;
static std::string xm44Root;
static void xm44Status(const char* event,const std::string& detail) {
    const char* statusRoot=std::getenv("XM87203_STAGE_CAPTURE_DIR");
    if(statusRoot && *statusRoot) {
        const std::string path=std::string(statusRoot)+"/status.log";
        FILE* f=std::fopen(path.c_str(),"ab");
        if(f) {
            std::fprintf(f,"frame=%d\t%s\t%s\n",GetCurrentFrame(),event,detail.c_str());
            std::fclose(f);
        }
    }
    std::fprintf(stderr,"XM44 CLEAN_RIP %s frame=%d %s\n",
        event,GetCurrentFrame(),detail.c_str());
}
static int xm44Mkdir(const char* path) {
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path,0755);
#endif
}
static FILE* xm44Index = NULL;
static FILE* xm44Sprites = NULL;
static FILE* xm47Tiles = NULL;
static FILE* xm47Palettes = NULL;

static std::string xm44Path(const char* name) {
    return xm44Dir + "/" + name;
}
static bool xm44WriteBytes(const std::string& name, const unsigned char* data, size_t size) {
    FILE* f = std::fopen(xm44Path(name.c_str()).c_str(), "wb");
    if (!f) { std::fprintf(stderr,"XM44: WRITE_FAILED %s\n",name.c_str()); return false; }
    bool ok = std::fwrite(data, 1, size, f) == size;
    std::fclose(f);
    return ok;
}
static uint64_t xm44Hash(const std::vector<unsigned char>& data) {
    uint64_t h = UINT64_C(14695981039346656037);
    for (unsigned char c: data) { h ^= c; h *= UINT64_C(1099511628211); }
    return h;
}
static void xm44Init() {
    if (xm44Initialised) return;
    xm44Initialised = true;
    const char* enabled = std::getenv("XM44_CAPTURE");
    if (!enabled || std::strcmp(enabled,"1") != 0 || Cps != 2) return;
    const char* driver = BurnDrvGetTextA(DRV_NAME);
    // Donor-only: never capture normal XMVSF play.
    if (!driver || std::strncmp(driver,"mvsc",4) != 0) return;
    const char* dir = std::getenv("XM44_CAPTURE_DIR");
    if (!dir || !*dir) {
        std::fprintf(stderr,"XM44: set XM44_CAPTURE_DIR to an existing directory\n");
        return;
    }
    xm44Root=dir;
    xm44Dir=xm44Root;
    const char* automatic=std::getenv("XM44_AUTO");
    xm44Auto=automatic && std::strcmp(automatic,"1")==0;
    const char* max = std::getenv("XM44_FRAMES");
    if (max && *max) xm44Limit = std::atoi(max);
    if (xm44Limit < 1 || xm44Limit > 1200) xm44Limit = 180;
    const char* step = std::getenv("XM44_STEP");
    if (step && *step) xm44Step = std::atoi(step);
    if (xm44Step < 1 || xm44Step > 30) xm44Step = 1;
    if (nBurnBpp != 4) {
        std::fprintf(stderr,"XM44: NEEDS_32_BIT_VIDEO nBurnBpp=%d\n", nBurnBpp);
        return;
    }
    xm44Enabled = true;
    std::fprintf(stderr,"XM44: ARMED donor=%s dir=%s max=%d step=%d\n",
        driver,xm44Dir.c_str(),xm44Limit,xm44Step);
    xm44Status("CLEAN_RIP_ARMED","donor="+std::string(driver)+
        " samples="+std::to_string(xm44Limit)+
        " step="+std::to_string(xm44Step)+
        (xm44Auto?" mode=CONTINUOUS_AUTO_BATCH":
                   " trigger="+xm44Root+"/START.flag"));
}
static void xm44Close() {
    if (xm44Index) { std::fclose(xm44Index); xm44Index = NULL; }
    if (xm44Sprites) { std::fclose(xm44Sprites); xm44Sprites = NULL; }
    if (xm47Tiles) { std::fclose(xm47Tiles); xm47Tiles = NULL; }
    if (xm47Palettes) { std::fclose(xm47Palettes); xm47Palettes = NULL; }
}
// Stage 45: render with CPS2's normal raster-sliced, priority-ordered layer engine.
// layer 0 exports the complete background composite (all three scrolls, no sprites).
// layers 1-3 isolate each scroll but still honor its raster slices. All uses of the
// global drawing-layer mask are restored before normal game rendering resumes.
static void xm44DrawBackground(int layer) {
    const INT32 savedMask = nBurnLayer;
    nBurnLayer = (layer == 0) ? 0x0E : (1 << layer);
    Cps2Layers();
    nBurnLayer = savedMask;
}
// Render twice over distinct colors. A pixel unchanged between the two passes
// was written by the source layer; unchanged clear pixels become alpha zero.
// This preserves real black scenery pixels (single-color chroma keys cannot).
static bool xm44RenderLayer(int layer, std::vector<unsigned char>& rgba) {
    const int w=nCpsScreenWidth, h=nCpsScreenHeight;
    if (!pBurnDraw || w<=0 || h<=0 || nBurnPitch<w*4) return false;
    std::vector<unsigned char> black((size_t)w*h*4), white((size_t)w*h*4);
    unsigned char* outputs[2]={black.data(),white.data()};
    const unsigned char fills[2]={0x17,0xE9};
    for(int pass=0; pass<2; ++pass) {
        for(int y=0; y<h; ++y) {
            unsigned char* dest=pBurnDraw+(size_t)y*nBurnPitch;
            for (int x=0;x<w;++x) {
                dest[x*4+0] = fills[pass];
                dest[x*4+1] = fills[pass];
                dest[x*4+2] = fills[pass];
                dest[x*4+3] = 0;
            }
        }
        xm44DrawBackground(layer);
        for(int y=0; y<h; ++y)
            std::memcpy(outputs[pass]+(size_t)y*w*4,pBurnDraw+(size_t)y*nBurnPitch,(size_t)w*4);
    }
    rgba.resize((size_t)w*h*4);
    for(size_t i=0; i<(size_t)w*h; ++i) {
        const unsigned char* a=&black[i*4];
        const unsigned char* b=&white[i*4];
        bool painted = a[0]==b[0] && a[1]==b[1] && a[2]==b[2];
        rgba[i*4+0]=a[2]; rgba[i*4+1]=a[1]; rgba[i*4+2]=a[0]; rgba[i*4+3]=painted?255:0;
    }
    return true;
}
static void xm44CaptureAfterDraw() {
    xm44Init();
    if (!xm44Enabled || xm44Done) return;
    if (!xm44Capturing) {
        const std::string marker=xm44Root+"/START.flag";
        if(!xm44Auto){
            FILE* go=std::fopen(marker.c_str(),"rb");
            if(!go)return;
            std::fclose(go);
        }
        // Shot Factory-style capture: preserve the trigger in an exclusive
        // session directory. Never overwrite existing rip files.
        xm44Dir.clear();
        for(unsigned n=1;n<=9999u;++n) {
            char name[32];
            std::snprintf(name,sizeof(name),"shot_%04u",n);
            const std::string candidate=xm44Root+"/"+name;
            if(xm44Mkdir(candidate.c_str())==0) {
                xm44Dir=candidate;break;
            }
        }
        if(xm44Dir.empty()) {
            xm44Status("ERROR","CLEAN_RIP_NO_FREE_SESSION");
            xm44Enabled=false;return;
        }
        if(xm44Auto){
            FILE* origin=std::fopen(xm44Path("AUTOMATIC_BATCH.txt").c_str(),"wb");
            if(!origin){
                xm44Status("ERROR","CLEAN_RIP_AUTO_PROVENANCE_WRITE_FAILED dir="+xm44Dir);
                xm44Enabled=false;return;
            }
            std::fprintf(origin,"source=mvsc native_renderer\nautomatic=1\nbatch=%u\n",
                         xm44Batches+1u);
            if(std::fclose(origin)!=0){
                xm44Status("ERROR","CLEAN_RIP_AUTO_PROVENANCE_CLOSE_FAILED");
                xm44Enabled=false;return;
            }
        }else if(std::rename(marker.c_str(),xm44Path("START.flag").c_str())!=0) {
            xm44Status("ERROR","CLEAN_RIP_MOVE_TRIGGER_FAILED dir="+xm44Dir);
            xm44Enabled=false;return;
        }
        xm44Sample=0;
        xm44RawFrame=0;
        xm44Index = std::fopen(xm44Path("layers.tsv").c_str(),"wb");
        xm44Sprites = std::fopen(xm44Path("sprites.tsv").c_str(),"wb");
        xm47Tiles = std::fopen(xm44Path("tiles.bin").c_str(),"wb");
        xm47Palettes = std::fopen(xm44Path("palettes.bin").c_str(),"wb");
        if (!xm44Index || !xm44Sprites || !xm47Tiles || !xm47Palettes) {
            std::fprintf(stderr,"XM44: FAILED_TO_OPEN_EXPORTS dir=%s\n",xm44Dir.c_str());
            xm44Close(); xm44Enabled=false; return;
        }
        std::fprintf(xm44Index,"sample\temulated_frame\tlayer\tfile\tfnv64\twidth\theight\tscroll_x\tscroll_y\n");
        std::fprintf(xm44Sprites,"sample\tobject_index\tx\ty\ttile\tattributes\tpriority\tpalette\twidth_tiles\theight_tiles\n");
        XM47ResetCapturedTiles();
        xm44Capturing=true;
        xm44Status("CLEAN_RIP_START","batch="+
            std::to_string(xm44Batches+1u)+" dir="+xm44Dir+
            " painter=native_CPS2 layers=0,1,2,3 transparent_two_fill=1"+
            (xm44Auto?" automatic=1":" manual=1"));
    }
    ++xm44RawFrame;
    if ((xm44RawFrame-1)%xm44Step) return;
    const int w=nCpsScreenWidth,h=nCpsScreenHeight;
    if (!pBurnDraw || nBurnBpp != 4 || w<1 || h<1 || nBurnPitch<w*4) return;
    // Copy row by row, as pitch need not be tightly packed.
    std::vector<unsigned char> original((size_t)h*nBurnPitch);
    std::memcpy(original.data(),pBurnDraw,original.size());
    int oldStart=nStartline,oldEnd=nEndline,oldBg=nBgHi;
    nStartline=0; nEndline=h; nBgHi=0;
    // l0 = the CPS2-composited scenery, already ordered and raster-correct.
    // l1..l3 = isolated scrolling layers, for later map/animation extraction.
    for(int layer=0;layer<=3;++layer) {
        std::vector<unsigned char> rgba;
        if(!xm44RenderLayer(layer,rgba))continue;
        const uint64_t hash=xm44Hash(rgba);
        char filename[80]; std::snprintf(filename,sizeof(filename),"l%d_%06d.rgba",layer,xm44Sample);
        if (!xm44WriteBytes(filename,rgba.data(),rgba.size())) continue;
        // A whole composite has no single scroll offset: -1 signals this.
        const int regX=0x0c+(layer-1)*4;
        const int regY=regX+2;
        const int sx=(layer==0) ? -1 : BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsSaveReg[0]+regX)));
        const int sy=(layer==0) ? -1 : BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsSaveReg[0]+regY)));
        std::fprintf(xm44Index,"%d\t%d\t%d\t%s\t%016llx\t%d\t%d\t%d\t%d\n",xm44Sample,GetCurrentFrame(),layer,filename,(unsigned long long)hash,w,h,sx,sy);
    }
    nStartline=oldStart; nEndline=oldEnd; nBgHi=oldBg;
    // Restore original gameplay output BEFORE returning from CpsDraw.
    std::memcpy(pBurnDraw,original.data(),original.size());
    XM44WriteBufferedSprites(xm44Sprites,xm44Sample);
    // One 2052-byte palette record per captured frame: UINT32 sample ID,
    // 512 UINT32 native BurnHighCol colors, in CPS2 sprite-palette order.
    const UINT32 paletteSample = (UINT32)xm44Sample;
    if (CpsPal && xm47Palettes) {
        std::fwrite(&paletteSample,sizeof(paletteSample),1,xm47Palettes);
        std::fwrite(CpsPal,sizeof(UINT32),512,xm47Palettes);
    }
    XM47WriteSpriteTiles(xm47Tiles);
    std::fflush(xm44Index);std::fflush(xm44Sprites);
    if (xm47Tiles) std::fflush(xm47Tiles);
    if (xm47Palettes) std::fflush(xm47Palettes);
    ++xm44Sample;
    ++xm44TotalSamples;
    if (xm44Sample >= xm44Limit) {
        xm44Close(); xm44Done=true;
        FILE* marker = std::fopen(xm44Path("CAPTURE_COMPLETE.txt").c_str(),"wb");
        if (marker) { std::fprintf(marker,"samples=%d\n",xm44Sample); std::fclose(marker); }
        ++xm44Batches;
        xm44Status("CLEAN_RIP_BURST_DONE","samples="+
            std::to_string(xm44Sample)+" total_samples="+
            std::to_string(xm44TotalSamples)+" batch="+
            std::to_string(xm44Batches)+" dir="+xm44Dir+
            " animation_and_world_coverage_unverified=1");
        // Continuous mode auto-starts next independent batch on the next
        // original CPS2 draw frame. Manual START.flag remains opt-in.
        xm44Done=false;
        xm44Capturing=false;
    }
}

// XMVSF Stage86.49 (Stage77 provenance): isolated CPS2 sprite output. The native draw buffer is restored
// byte-for-byte, and normal character select / native stages never enter here.
extern int xm77StageResolved;
extern XM8649VisualState xm8649Visual;
// XM86.54: permanent stage05 native-object ownership classification.
// Only the disposable sprite capture is filtered; DoDraw() remains intact.
bool xm8651SpriteCaptureActive=false;
bool xm8651SpriteCaptureStats=false;
int xm8654ConsideredObjects=0;
int xm8654FilteredObjects=0;
int xm8654RetainedObjects=0;
int xm8654SceneryObjects=0;
int xm8654FilteredPriority[8]={0};
INT32 xm77SpriteWidth=0, xm77SpriteHeight=0, xm77SpriteValid=0;
static std::vector<UINT8> xm77Pixels;
const UINT8* xm77SpritePixels(){return xm77SpriteValid?xm77Pixels.data():NULL;}
static void xm77CaptureSprites(){
    xm77SpriteValid=0;
    // XM87.56: the Wily stage needs its native fighter/FX layer in BOTH modes.
    // Double Feature remains an independent palette/lighting switch.
    if((xm77StageResolved!=1 && xm77StageResolved!=2) || !xm8649Visual.sceneActive || Cps!=2 || !pBurnDraw ||
       nBurnBpp!=4 || nCpsScreenWidth!=384 || nCpsScreenHeight!=224 || nBurnPitch<384*4)
        return;
    const int w=384,h=224;
    const size_t bytes=(size_t)nBurnPitch*h;
    std::vector<UINT8> original(bytes),a((size_t)w*h*4),b((size_t)w*h*4);
    memcpy(original.data(),pBurnDraw,bytes);
    const INT32 savedMask=nBurnLayer,savedStart=nStartline,savedEnd=nEndline,savedHi=nBgHi;
    nBurnLayer=1; // CPS2 OBJ only. Native scroll layers are NOT drawn in temporary pass.
    xm8651SpriteCaptureActive=true; // only the temporary two-fill pass is filtered
    xm8654ConsideredObjects=0; xm8654FilteredObjects=0;
    xm8654RetainedObjects=0; xm8654SceneryObjects=0;
    for(int priority=0;priority<8;++priority)xm8654FilteredPriority[priority]=0;
    for(int pass=0;pass<2;++pass){
        xm8651SpriteCaptureStats=(pass==0);
        const UINT8 fill=pass?0xE9:0x17;
        for(int y=0;y<h;++y){
            UINT8* row=pBurnDraw+(size_t)y*nBurnPitch;
            for(int x=0;x<w;++x){
                row[x*4+0]=fill;row[x*4+1]=fill;row[x*4+2]=fill;row[x*4+3]=0;
            }
        }
        nStartline=0;nEndline=h;nBgHi=0;
        Cps2Layers();
        std::vector<UINT8>& dst=pass?b:a;
        for(int y=0;y<h;++y)memcpy(dst.data()+(size_t)y*w*4,
                                      pBurnDraw+(size_t)y*nBurnPitch,(size_t)w*4);
    }
    xm8651SpriteCaptureStats=false;
    xm8651SpriteCaptureActive=false;
    nBurnLayer=savedMask;nStartline=savedStart;nEndline=savedEnd;nBgHi=savedHi;
    memcpy(pBurnDraw,original.data(),bytes); // original screen remains pristine
    xm77Pixels.resize((size_t)w*h*4);
    for(size_t i=0;i<(size_t)w*h;++i){
        const UINT8* q=&a[4*i],*r=&b[4*i];
        UINT8* dst=&xm77Pixels[4*i];
        dst[0]=q[2];dst[1]=q[1];dst[2]=q[0];
        dst[3]=(q[0]==r[0] && q[1]==r[1] && q[2]==r[2])?255:0;
    }
    xm77SpriteWidth=w;xm77SpriteHeight=h;xm77SpriteValid=1;
}


#include "xmvsf_stage87_56_native_white_probe.h"
#include "xmvsf_stage87_57_native_flash_overlay.h"

// First-frame fallback: camera/stage are observed after the emulated frame
// but before VidPaint. If Stage 05 becomes ready on this exact frame, populate
// the native sprite alpha texture now rather than flashing the forest.
void xm8657EnsureCurrentSprites(){
    if(!xm77SpriteValid && xm8649Visual.sceneActive)xm77CaptureSprites();
}

INT32 CpsDraw()
{
	DoDraw(CpsRecalcPal);
    xm8756ProbeNativeWhite(); // read-only pre-Wily native framebuffer sample
    xm8757CaptureNativeFlash(); // snapshot native flash BEFORE the two-fill sprite passes
    xm77CaptureSprites(); // XM86.49 two-fill sprite-only isolation, restores original framebuffer
    xm8757FinalizeNativeFlash(); // native RGB flash with filtered OBJ/FX coverage
    xm8657PaletteActive=false;
	xm44CaptureAfterDraw();

	CpsRecalcPal = 0;
	return 0;
}

INT32 CpsRedraw()
{
	DoDraw(1);
    xm8756ProbeNativeWhite(); // read-only pre-Wily native framebuffer sample
    xm8757CaptureNativeFlash(); // snapshot native flash BEFORE the two-fill sprite passes
    xm77CaptureSprites(); // XM86.49 redraw mode
    xm8757FinalizeNativeFlash();
    xm8657PaletteActive=false;

	CpsRecalcPal = 0;
	return 0;
}

