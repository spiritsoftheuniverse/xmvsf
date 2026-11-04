#include "cps.h"
#include "xmvsf_stage87_17_provenance.h"
#include "xmvsf_stage87_18_source_color.h"
#include "m68000_debug.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
// XM87.16 68000 CPS2 sprite-write provenance probe; only active with env flag.

// CPS - Memory

UINT32 CpsMProt[4];
UINT32 CpsBID[3];

static UINT8 *CpsMem=NULL,*CpsMemEnd=NULL;
UINT8 *CpsRam90=NULL;
UINT8 *CpsZRamC0=NULL,*CpsZRamF0=NULL,*CpsEncZRom=NULL;
UINT8 *CpsSavePal=NULL;
UINT8 *CpsSaveReg[MAX_RASTER + 1];
UINT8 *CpsSaveFrg[2];
static UINT8 *CpsSaveRegData = NULL;
static UINT8 *CpsSaveFrgData = NULL;
UINT8 *CpsRam660=NULL,*CpsRam708=NULL,*CpsReg=NULL,*CpsFrg=NULL;
UINT8 *CpsRamFF=NULL;

CpsMemScanCallback CpsMemScanCallbackFunction = NULL;

// This routine is called first to determine how much memory is needed
// and then to set up all the pointers.
static INT32 CpsMemIndex()
{
	UINT8*  Next; Next =  CpsMem;

	CpsRam90	  = Next; Next += 0x030000;							// Video Ram
	CpsRamFF	  = Next; Next += 0x010000;							// Work Ram
	CpsReg		  = Next; Next += 0x000100;							// Registers

	CpsSavePal    = Next; Next += 0x002000;							// Draw Copy of Correct Palette

	if (((Cps == 2) && !Cps2DisableQSnd) || Cps1Qs == 1) {
		CpsZRamC0 = Next; Next += 0x001000;							// Z80 c000-cfff
		CpsZRamF0 = Next; Next += 0x001000;							// Z80 f000-ffff
	}

	if (Cps == 2) {
		CpsRam660 = Next; Next += 0x004000;							// Extra Memory
		CpsRam708 = Next; Next += 0x010000;							// Obj Ram
		CpsFrg    = Next; Next += 0x000010;							// 'Four' Registers (Registers at 0x400000)

		ZBuf      = (UINT16*)Next; Next += nCpsScreenWidth * nCpsScreenHeight * 2;	// Sprite Masking Z buffer

		CpsSaveRegData = Next; Next += 0x0100 * (MAX_RASTER + 1);	// Draw Copy of registers
		CpsSaveFrgData = Next; Next += 0x0010 * 2;					// Draw Copy of 'Four' Registers
		CpsSaveFrg[0] = CpsSaveFrgData;
		CpsSaveFrg[1] = CpsSaveFrgData + 0x10;

		for (INT32 i = 0; i < MAX_RASTER + 1; i++) {
			CpsSaveReg[i] = CpsSaveRegData + i * 0x0100;
		}

	} else {
		CpsSaveRegData = Next; Next += 0x0100;						// Draw Copy of registers
		CpsSaveFrgData = Next; Next += 0x0010;						// Draw Copy of 'Four' Registers

		CpsSaveReg[0] = CpsSaveRegData;
		CpsSaveFrg[0] = CpsSaveFrgData;
		CpsSaveFrg[1] = CpsSaveFrgData + 0x10;
	}

	CpsMemEnd = Next;

	return 0;
}

static INT32 AllocateMemory()
{
	INT32 nLen;

	CpsMem = NULL;													// Find out how much memory is needed
	CpsMemIndex();
	nLen = CpsMemEnd - (UINT8*)0;

	if ((CpsMem = (UINT8*)BurnMalloc(nLen)) == NULL) {
		return 1;
	}

	memset(CpsMem, 0, nLen);										// blank all memory
	CpsMemIndex();													// Index the allocated memory

	return 0;
}

namespace XM8717 {
Source slots[2][1024];
bool enabled=false;
}
namespace XM8718 { bool enabled=false; }
// XM87.16: The 68000 really writes CPS2 object RAM through direct mapped memory.
// For the dedicated audit only, replace WRITE mappings with faithful handlers,
// leaving READ and FETCH mapped normally. Capture the actual guest writer PC and
// register context before deciding whether any ownership inference is possible.
// This never marks body/FX and never changes visible palette selection.
static FILE* xm8716SpriteWrites = NULL;
static unsigned long long xm8716WriteRecords = 0;
static unsigned long long xm8716SkippedRecords = 0;
static unsigned xm8716TraceStartFrame = ~0u;
static const unsigned long long xm8716RecordLimit = 1500000ull;
static inline bool XM8716TraceEnabled() { return xm8716SpriteWrites != NULL; }
static inline unsigned XM8716SpriteOffset(UINT32 address) { return address & 0x1fffu; }
static inline unsigned XM8716SpriteBankBase() { return (unsigned)(nCpsObjectBank & 1) * 0x8000u; }
static inline UINT16 XM8716ObjectWord(const UINT8* obj, unsigned index) {
  return (UINT16)(((unsigned)obj[index * 2u + 1u] << 8) | obj[index * 2u]);
}
static void XM8716ObserveSpriteWrite(UINT32 address, unsigned size, UINT32 value) {
  if (!XM8716TraceEnabled() && !XM8717::enabled) return;
  // Emit only when the write covers the attribute word (+6 / +7).
  const unsigned off=XM8716SpriteOffset(address);
  const unsigned inSlot = off & 7u;
  if (inSlot > 7u || (inSlot + size <= 6u)) return;
  const unsigned frame=(unsigned)GetCurrentFrame();
  if (xm8716TraceStartFrame == ~0u) xm8716TraceStartFrame=frame;
  const unsigned slot=off >> 3;
  const UINT8* const obj=CpsRam708 + XM8716SpriteBankBase() + (off & ~7u);
  const UINT16 x=XM8716ObjectWord(obj,0);
  const UINT16 y=XM8716ObjectWord(obj,1);
  const UINT16 tile=XM8716ObjectWord(obj,2);
  const UINT16 attr=XM8716ObjectWord(obj,3);
  const UINT32 pc=XM8716TraceEnabled()?SekGetPC(-1):0u;
  const UINT32 a0=SekDbgGetRegister(SEK_REG_A0);
  // Full debug-register collection is reserved for the optional full writer trace.
  const UINT32 a1=XM8716TraceEnabled()?SekDbgGetRegister(SEK_REG_A1):0u;
  const UINT32 a2=XM8716TraceEnabled()?SekDbgGetRegister(SEK_REG_A2):0u;
  const UINT32 a3=XM8716TraceEnabled()?SekDbgGetRegister(SEK_REG_A3):0u;
  const UINT32 a4=XM8716TraceEnabled()?SekDbgGetRegister(SEK_REG_A4):0u;
  const UINT32 a5=XM8716TraceEnabled()?SekDbgGetRegister(SEK_REG_A5):0u;
  // XM87.17: record real source address for the exact bank and raw slot.
  // This metadata follows only the sprite write; never changes emulated RAM.
  if (XM8717::enabled) {
    auto& rec=XM8717::slots[nCpsObjectBank&1][slot];
    rec.pc=pc; rec.a0=a0; rec.frame=frame;
    rec.owner=XM8717::Owner(a0);rec.valid=1u;
  }
  // The established, verbose Stage87.16 audit is independent and optional.
  if (!XM8716TraceEnabled()) return;
  if (xm8716WriteRecords >= xm8716RecordLimit) { ++xm8716SkippedRecords; return; }
  const UINT32 d0=SekDbgGetRegister(SEK_REG_D0);
  const UINT32 d1=SekDbgGetRegister(SEK_REG_D1);
  // CPU-state coordinates are read, never modified; these are world coordinates,
  // not evidence that this particular sprite belongs to either actor.
  const auto ramByte=[](unsigned off)->unsigned {
    return CpsRamFF ? (unsigned)CpsRamFF[(off&0xffffu)^1u] : 0u;
  };
  const auto ramWord=[&](unsigned off)->unsigned {
    return (ramByte(off)<<8)|ramByte(off+1u);
  };
  const unsigned status=ramByte(0x5000u);
  const unsigned p1x=ramWord(0x400cu),p1y=ramWord(0x4010u);
  const unsigned p2x=ramWord(0x440cu),p2y=ramWord(0x4410u);
  const unsigned p1anim=ramWord(0x4006u),p2anim=ramWord(0x4406u);
  std::fprintf(xm8716SpriteWrites,
    "%u\t%u\t%u\t%06X\t%u\t%08X\t%06X\t%06X\t%06X\t%06X\t%06X\t%06X\t%08X\t%08X\t%04X\t%04X\t%04X\t%04X\t%u\t%08X\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\n",
    frame,(unsigned)(nCpsObjectBank&1),slot,(unsigned)(address&0xffffffu),size,
    pc,a0,a1,a2,a3,a4,a5,d0,d1,x,y,tile,attr,off,value,
    status,p1x,p1y,p2x,p2y,p1anim,p2anim);
  ++xm8716WriteRecords;
  if ((xm8716WriteRecords & 8191ull)==0ull) std::fflush(xm8716SpriteWrites);
}
static void __fastcall XM8716WriteObjectByte(UINT32 address, UINT8 value) {
  const unsigned off=XM8716SpriteOffset(address);
  CpsRam708[XM8716SpriteBankBase() + (off ^ 1u)]=value;
  XM8716ObserveSpriteWrite(address,1,value);
}
static void __fastcall XM8716WriteObjectWord(UINT32 address, UINT16 value) {
  const unsigned off=XM8716SpriteOffset(address);
  const unsigned base=XM8716SpriteBankBase();
  CpsRam708[base+off]=(UINT8)(value & 255u);
  CpsRam708[base+((off+1u)&0x1fffu)]=(UINT8)(value >> 8);
  XM8716ObserveSpriteWrite(address,2,value);
}
static void __fastcall XM8716WriteObjectLong(UINT32 address, UINT32 value) {
  const unsigned off=XM8716SpriteOffset(address);
  const unsigned base=XM8716SpriteBankBase();
  const unsigned high=(value>>16)&0xffffu, low=value&0xffffu;
  const unsigned off2=(off+2u)&0x1fffu;
  CpsRam708[base+off]=(UINT8)(high&255u);
  CpsRam708[base+((off+1u)&0x1fffu)]=(UINT8)(high>>8);
  CpsRam708[base+off2]=(UINT8)(low&255u);
  CpsRam708[base+((off2+1u)&0x1fffu)]=(UINT8)(low>>8);
  XM8716ObserveSpriteWrite(address,4,value);
}
static void XM8716MapSpriteWriteAudit() {
  if (!XM8716TraceEnabled() && !XM8717::enabled) return;
  // Match all four 0x2000 mirrors of CPS2 object RAM; READ is untouched.
  for (UINT32 address=0x708000u;address<0x710000u;address+=0x2000u)
    SekMapHandler(4,address,address+0x1fffu,MAP_WRITE);
}
static void XM8716CloseSpriteTrace() {
  if (xm8716SpriteWrites) {
    std::fflush(xm8716SpriteWrites);
    std::fclose(xm8716SpriteWrites); xm8716SpriteWrites=NULL;
    std::fprintf(stderr,"XM87.16.2 WRITE_PROVENANCE recorded=%llu capped=%llu\n",
      xm8716WriteRecords,xm8716SkippedRecords);
  }
}

// Map the correct bank of obj memory to the 68000 address space (including mirrors).
void CpsDoMapObjectBanks(INT32 nBank)
{
	nCpsObjectBank = nBank;

	if (nCpsObjectBank) {
		SekMapMemory(CpsRam708 + 0x8000, 0x708000, 0x709FFF, MAP_RAM);
		SekMapMemory(CpsRam708 + 0x8000, 0x70A000, 0x70BFFF, MAP_RAM);
		SekMapMemory(CpsRam708 + 0x8000, 0x70C000, 0x70DFFF, MAP_RAM);
		SekMapMemory(CpsRam708 + 0x8000, 0x70E000, 0x70FFFF, MAP_RAM);
	} else {
		SekMapMemory(CpsRam708, 0x708000, 0x709FFF, MAP_RAM);
		SekMapMemory(CpsRam708, 0x70A000, 0x70BFFF, MAP_RAM);
		SekMapMemory(CpsRam708, 0x70C000, 0x70DFFF, MAP_RAM);
		SekMapMemory(CpsRam708, 0x70E000, 0x70FFFF, MAP_RAM);
	}
	XM8716MapSpriteWriteAudit();
}

void CpsMapObjectBanks(INT32 nBank)
{
	if (nBank != nCpsObjectBank) {
		nCpsObjectBank = nBank;

		CpsDoMapObjectBanks(nBank);
	}
}

INT32 __fastcall CPSResetCallback()
{
	// Reset instruction on 68000
	if (((Cps & 1) && !Cps1DisablePSnd) || ((Cps == 2) && !Cps2DisableQSnd)) ZetReset();						// Reset Z80 (CPU #1)

	return 0;
}

// samples:
// empty + 0x3f L, channel 0, sample + 0x00
// empty + 0x3f R, channel 1, sample + 0x40

static void samples_l_start(int channel, int sample, int loop)
{
	BurnSampleChannelPlay(channel + 0x00, sample + 0x00, loop);
}

static int samples_l_playing()
{
	return BurnSampleGetChannelStatus(0x00) == SAMPLE_PLAYING;
}
static int samples_r_playing()
{
	return BurnSampleGetChannelStatus(0x01) == SAMPLE_PLAYING;
}

static void samples_r_start(int channel, int sample, int loop)
{
	BurnSampleChannelPlay(channel + 0x01, sample + 0x40, loop);
}

// ----------------------------------------------------------------------------
static void qsound_cps2turbo_write(UINT32 offset, UINT8 data) //Zero800 allows you to write and signal when a new song is played
{
	UINT8 *m_qsound_sharedram1 = CpsZRamC0;
	UINT8 *m_qsound_sharedram2 = CpsZRamF0;
	if (offset == 0x0001 && (data & 0xff) == 0x00 && m_qsound_sharedram1[0x00] == 0xff) //stop samples sound
	{
		samples_l_start(0, 0, 0);
		samples_r_start(0, 0, 0);
		bprintf(0, _T("Stop: %02x to %x\n"), offset, offset);
	}

	if (offset == 0xffe)
	{
		if (m_qsound_sharedram1[0x80] == 0x77) //stop music condition in audiocpu = C080 (z80 hacked)
		{
			samples_l_start(0, 0, 0); //init L samples
			samples_r_start(0, 0, 0); //init R samples
			m_qsound_sharedram2[0x27] = 0xc9; //Enable Qsound Volume
			m_qsound_sharedram1[0x80] = 0xff; //Reset music id flag
			m_qsound_sharedram1[0x81] = 0xff; //Reset music id flag
			bprintf(0, _T("Clear: %02x to %x\n"), data, offset);
		}
		else
			if (m_qsound_sharedram1[0x81] > 0x00 && m_qsound_sharedram1[0x81] < 0x40) //play music condition in audiocpu = C081 (z80 hacked)
			{
				int data_byte = m_qsound_sharedram1[0x81];
				m_qsound_sharedram1[0x80] = 0xff; //Reset music id flag
				m_qsound_sharedram1[0x81] = 0xff; //Reset music id flag
				bprintf(0, _T("Music: %02x to %x\n"), data_byte, offset);
				samples_l_start(0, data_byte, 1);
				if (samples_l_playing() == 1)
				{
					m_qsound_sharedram2[0x27] = 0x00; //Qsound music volume = 0
				}
				else
					samples_l_start(0, 0, 0); //init L samples

				samples_r_start(0, data_byte, 1);
				if (samples_r_playing() == 1)
				{
					m_qsound_sharedram2[0x27] = 0x00; //Qsound music volume = 0
				}
				else
					samples_r_start(0, 0, 0); //init R samples
			}
			else
				if (m_qsound_sharedram2[0x27] == 0x00 && samples_l_playing() != 1) //if loadstate and qsound volume = 0 && samples stopped
				{
					samples_l_start(0, 0, 0); //init L samples
					samples_r_start(0, 0, 0); //init R samples
					m_qsound_sharedram2[0x27] = 0xc9; //Enable Qsound Volume
					m_qsound_sharedram1[0x80] = 0xff; //Reset music id flag
					m_qsound_sharedram1[0x81] = 0xff; //Reset music id flag
					bprintf(0, _T("Volume0: %02x to %x\n"), data, offset);
				}
				else
					if (m_qsound_sharedram2[0x27] != 0x00 && samples_l_playing() == 1) //if loadstate and qsound volume != 0 && samples playing
					{
						samples_l_start(0, 0, 0); //init L samples
						samples_r_start(0, 0, 0); //init R samples
						m_qsound_sharedram2[0x27] = 0xc9; //Enable Qsound Volume
						m_qsound_sharedram1[0x80] = 0xff; //Reset music id flag
						m_qsound_sharedram1[0x81] = 0xff; //Reset music id flag
						bprintf(0, _T("Volume0: %02x to %x\n"), data, offset);
					}
					else
						if ((data & 0xff) == 0x55) //init samples in rom start
						{
							samples_l_start(0, 0, 0);
							samples_r_start(0, 0, 0);
						}
	}

	m_qsound_sharedram1[offset] = data;	//Play data Qsound
}

UINT8 __fastcall CPSQSoundC0ReadByte(UINT32 sekAddress)
{
//	bprintf(PRINT_NORMAL, _T("    QS %06X read\n"), sekAddress);

	if (!(sekAddress & 1)) {
		return 0xFF;
	}

	QsndSyncZ80();

	sekAddress &= 0x1FFF;
	return CpsZRamC0[sekAddress >> 1];
}

void __fastcall CPSQSoundC0WriteByte(UINT32 sekAddress, UINT8 byteValue)
{
//	bprintf(PRINT_NORMAL, _T("    QS %06X -> %02X\n"), sekAddress, byteValue);

	if (!(sekAddress & 1)) {
		return;
	}

	sekAddress &= 0x1FFF;

	if (Cps2Turbo) {
		qsound_cps2turbo_write(sekAddress >> 1, byteValue);
	}

#if 1 && defined USE_SPEEDHACKS
	// Sync only when the last byte of the sound command is written
	if (sekAddress == 0x001F) {
		QsndSyncZ80();
	}
#else
	QsndSyncZ80();
#endif

	CpsZRamC0[sekAddress >> 1] = byteValue;
}

UINT8 __fastcall CPSQSoundF0ReadByte(UINT32 sekAddress)
{
//	bprintf(PRINT_NORMAL, _T("    QS %06X read\n"), sekAddress);

	if (!(sekAddress & 1)) {
		return 0xFF;
	}

	QsndSyncZ80();

	sekAddress &= 0x1FFF;
	return CpsZRamF0[sekAddress >> 1];
}

void __fastcall CPSQSoundF0WriteByte(UINT32 sekAddress, UINT8 byteValue)
{
//	bprintf(PRINT_NORMAL, _T("    QS %06X -> %02X\n"), sekAddress, byteValue);

	if (!(sekAddress & 1)) {
		return;
	}

	sekAddress &= 0x1FFF;

#if 1 && defined USE_SPEEDHACKS
	// Sync only when the last byte of the sound command is written
	if (sekAddress == 0x001F) {
		QsndSyncZ80();
	}
#else
	QsndSyncZ80();
#endif

	CpsZRamF0[sekAddress >> 1] = byteValue;
}

// ----------------------------------------------------------------------------

#if 0
UINT8 __fastcall CPSExtraNVRAMReadByte(UINT32 sekAddress)
{
//	bprintf(PRINT_NORMAL, _T("  - 0x%06X read.\n"), sekAddress);

	sekAddress &= 0x3FFF;
	return CpsRam660[sekAddress];
}

void __fastcall CPSExtraNVRAMWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
//	bprintf(PRINT_NORMAL, _T("  - 0x%06X -> %02X\n"), sekAddress, byteValue);

	sekAddress &= 0x3FFF;
	CpsRam660[sekAddress] = byteValue;
}
#endif

// ----------------------------------------------------------------------------

/*
INT32 prevline;

void __fastcall CpsWriteSpriteByte(UINT32 sekAddress, UINT8 byteValue)
{
	if (prevline != SekCurrentScanline()) {
		prevline = SekCurrentScanline();
//		bprintf(PRINT_NORMAL, _T("  - sb (%3i)\n"), prevline);
	}

	sekAddress &= 0x1FFF;
	CpsRam708[sekAddress + nCpsObjectBank * 0x8000] = byteValue;
}

void __fastcall CpsWriteSpriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	if (prevline != SekCurrentScanline()) {
		prevline = SekCurrentScanline();
//		bprintf(PRINT_NORMAL, _T("  - sw (%3i)\n"), prevline);
	}

	sekAddress &= 0x1FFE;
	CpsRam708[sekAddress + nCpsObjectBank * 0x8000 + 1] = wordValue >> 8;
	CpsRam708[sekAddress + nCpsObjectBank * 0x8000 + 0] = wordValue & 255;
}
*/

// ----------------------------------------------------------------------------

UINT8 __fastcall haxx0rReadByte(UINT32 sekAddress)
{
	sekAddress &= 0xFFFF;
	bprintf(PRINT_NORMAL, _T("    QS %06X read (%02X)\n"), sekAddress, CpsEncZRom[sekAddress]);
	return CpsEncZRom[sekAddress];
}

INT32 CpsMemInit()
{
	if (AllocateMemory()) {
		return 1;
	}

	XM8716CloseSpriteTrace();
	xm8716WriteRecords=xm8716SkippedRecords=0;
	xm8716TraceStartFrame=~0u;
	const char* xm8717Path=std::getenv("XM8717_DRAW_PROVENANCE_FILE");
	// XM87.18: source tags are needed by the source-first body mask during
	// normal xmvsfu combat. Other CPS2 games keep their original direct mapping.
	// Explicit XM8718_SOURCE_BODY=0 disables the new feature for A/B comparison.
	const char* xm8718Choice=std::getenv("XM8718_SOURCE_BODY");
	const bool xm8718Allowed=!(xm8718Choice && xm8718Choice[0]=='0' && !xm8718Choice[1]);
	const char* xm8718Driver=BurnDrvGetTextA(DRV_NAME);
	const bool xm8718XMVSF=xm8718Driver && std::strncmp(xm8718Driver,"xmvsf",5)==0;
	const bool xm8718Gameplay=(Cps==2 && xm8718XMVSF && xm8718Allowed);
	XM8718::enabled=xm8718Gameplay;
	XM8717::enabled=(Cps==2 && ((xm8717Path && *xm8717Path) || xm8718Gameplay));
	if (XM8717::enabled) {
		std::memset(XM8717::slots,0,sizeof(XM8717::slots));
		std::fprintf(stderr,"XM87.17 SOURCE_TAGS enabled: tracking 68000 source through lagged object list\n");
		std::fprintf(stderr,"XM87.18 SOURCE_OWNER_GRAY %s game=%s\n",xm8718Gameplay?"enabled":"trace_only",xm8718Driver?xm8718Driver:"unknown");
	}
	const char* auditPath=std::getenv("XM8716_SPRITE_WRITES_FILE");
	// Stage87.16.2: xmvsfu is the STANDARD two-player CPS2 driver, not the
    // CpsXmvSix (six-player hack) variant. That check incorrectly disabled
    // tracing for the game we actually launch. Opt in only by env + CPS2.
    if (Cps==2 && auditPath && *auditPath) {
		xm8716SpriteWrites=std::fopen(auditPath,"w");
		if (xm8716SpriteWrites) {
			std::setvbuf(xm8716SpriteWrites,NULL,_IOFBF,256*1024);
			std::fprintf(xm8716SpriteWrites,"frame\tbank\tslot\taddress\tsize\tpc\ta0\ta1\ta2\ta3\ta4\ta5\td0\td1\txword\tyword\ttileword\tattrword\tbyte_offset\tvalue\tstatus\tp1_world_x\tp1_world_y\tp2_world_x\tp2_world_y\tp1_anim\tp2_anim\n");
			std::fprintf(stderr,"XM87.16.2 WRITE_PROVENANCE enabled file=%s cps=%d six_player=%d\n",auditPath,(int)Cps,(int)CpsXmvSixEnabled);
		} else std::fprintf(stderr,"XM87.16.2 WRITE_PROVENANCE could_not_open file=%s\n",auditPath);
	}

	SekOpen(0);

	SekSetResetCallback(CPSResetCallback);

	if (Cps2Turbo) {
		// hmmmmmm
		bprintf(0, _T("limit 68k to 0x400000\n"));
		nCpsRomLen = 0x400000;
		nCpsCodeLen = 0x400000;
	}

	// Map in memory:
	// 68000 Rom (as seen as is, through read)
	SekMapMemory(CpsRom, 0, nCpsRomLen - 1, MAP_READ);

	// 68000 Rom (as seen decrypted, through fetch)
	if (nCpsCodeLen > 0) {
		// Decoded part (up to nCpsCodeLen)
		SekMapMemory(CpsCode, 0, nCpsCodeLen - 1, MAP_FETCH);
	}
	if (nCpsRomLen > nCpsCodeLen) {
		// The rest (up to nCpsRomLen)
		SekMapMemory(CpsRom + nCpsCodeLen, nCpsCodeLen, nCpsRomLen - 1, MAP_FETCH);
	}

	if (Cps == 2) {
		nCpsObjectBank = -1;
		CpsMapObjectBanks(0);

#if 0
		SekMapHandler(3, 0x660000, 0x663FFF, MAP_RAM);
		SekSetReadByteHandler(3, CPSExtraNVRAMReadByte);
		SekSetWriteByteHandler(3, CPSExtraNVRAMWriteByte);
#else
		SekMapMemory(CpsRam660, 0x660000, 0x663FFF, MAP_RAM);
#endif

//		SekMapHandler(4, 0x708000, 0x709FFF, MAP_WRITE);
//		SekMapHandler(4, 0x70A000, 0x70BFFF, MAP_WRITE);
//		SekMapHandler(4, 0x70C000, 0x70DFFF, MAP_WRITE);
//		SekMapHandler(4, 0x70E000, 0x70FFFF, MAP_WRITE);

//		SekSetWriteByteHandler(4, CpsWriteSpriteByte);
//		SekSetWriteWordHandler(4, CpsWriteSpriteWord);
		if (XM8716TraceEnabled() || XM8717::enabled) {
			SekSetWriteByteHandler(4, XM8716WriteObjectByte);
			SekSetWriteWordHandler(4, XM8716WriteObjectWord);
			SekSetWriteLongHandler(4, XM8716WriteObjectLong);
		}
	}

	SekMapMemory(CpsRam90,		0x900000, 0x92FFFF, MAP_RAM);	// Gfx Ram
	SekMapMemory(CpsRamFF,		0xFF0000, 0xFFFFFF, MAP_RAM);	// Work Ram

	SekSetReadByteHandler(0, CpsReadByte);
	SekSetWriteByteHandler(0, CpsWriteByte);
	SekSetReadWordHandler(0, CpsReadWord);
	SekSetWriteWordHandler(0, CpsWriteWord);

	// QSound
	if ((Cps == 2) && !Cps2DisableQSnd) {
		SekMapHandler(1,	0x618000, 0x619FFF, MAP_RAM);

		SekSetReadByteHandler(1, CPSQSoundC0ReadByte);
		SekSetWriteByteHandler(1, CPSQSoundC0WriteByte);
	}

	if (Cps1Qs == 1) {
		// Map the 1st 32KB of the QSound ROM into the 68K address space
		for (INT32 i = 0x7FFF; i >= 0; i--) {
			CpsEncZRom[(i << 1) + 0] = CpsEncZRom[i];
			CpsEncZRom[(i << 1) + 1] = 0xFF;
		}
		SekMapMemory(CpsEncZRom, 0xF00000, 0xF0FFFF, MAP_ROM);

		// QSound shared RAM
		SekMapHandler(1,	0xF18000, 0xF19FFF, MAP_RAM);
		SekMapHandler(2,	0xF1E000, 0xF1FFFF, MAP_RAM);

		SekSetReadByteHandler(1, CPSQSoundC0ReadByte);
		SekSetWriteByteHandler(1, CPSQSoundC0WriteByte);
		SekSetReadByteHandler(2, CPSQSoundF0ReadByte);
		SekSetWriteByteHandler(2, CPSQSoundF0WriteByte);
	}

	SekClose();

	return 0;
}

INT32 CpsMemExit()
{
#if 0
	FILE* fp = fopen("mem.raw", "wb");
	if (fp) {
		fwrite(CpsRam660, 1, 0x4000, fp);
		fclose(fp);
	}
#endif

	XM8716CloseSpriteTrace();
	// Deallocate all used memory
	BurnFree(CpsMem);
	
	CpsMemScanCallbackFunction = NULL;

	return 0;
}

static INT32 ScanRam()
{
	// scan ram:
	struct BurnArea ba;
	memset(&ba, 0, sizeof(ba));

	ba.Data = CpsRam90;  ba.nLen = 0x030000; ba.szName = "CpsRam90";  BurnAcb(&ba);
	ba.Data = CpsRamFF;  ba.nLen = 0x010000; ba.szName = "CpsRamFF";  BurnAcb(&ba);
	ba.Data = CpsReg;    ba.nLen = 0x000100; ba.szName = "CpsReg";    BurnAcb(&ba);

	if (((Cps == 2) && !Cps2DisableQSnd) || Cps1Qs == 1) {
		ba.Data = CpsZRamC0; ba.nLen = 0x001000; ba.szName = "CpsZRamC0"; BurnAcb(&ba);
		ba.Data = CpsZRamF0; ba.nLen = 0x001000; ba.szName = "CpsZRamF0"; BurnAcb(&ba);
	}

	if (Cps == 2) {
		ba.Data = CpsRam708; ba.nLen = 0x010000; ba.szName = "CpsRam708"; BurnAcb(&ba);
		ba.Data = CpsFrg;    ba.nLen = 0x000010; ba.szName = "CpsFrg";    BurnAcb(&ba);
	}

	return 0;
}

// Scan the current state of the CPS1/2 machine
INT32 CpsAreaScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;

	if (CpsMem == NULL) {
		return 1;
	}

	if (pnMin) {										// Return minimum compatible version
		*pnMin = 0x029521;
	}

	if (nAction & ACB_MEMORY_ROM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data   = CpsRom;
		ba.nLen   = nCpsRomLen;
		ba.szName = "CpsRom";
		BurnAcb(&ba);

		if (nCpsZRomLen) {
			ba.Data   = CpsZRom;
			ba.nLen   = nCpsZRomLen;
			ba.szName = "CpsZRom";
			BurnAcb(&ba);
		}
	}

	if (Cps == 2 || Cps1Qs == 1 || PangEEP == 1 || CpsBootlegEEPROM == 1) {		// Scan EEPROM
		EEPROMScan(nAction, pnMin);
	}

	if (nAction & ACB_MEMORY_RAM) {

		ScanRam();

		if (Cps == 2) {
			memset(&ba, 0, sizeof(ba));
			ba.Data   = CpsRam660;
			ba.nLen   = 0x004000;
			ba.szName = "CpsRam660";
			BurnAcb(&ba);
		}
	}


	if (nAction & ACB_DRIVER_DATA) {					// Scan volatile variables/registers/RAM

		SekScan(nAction);								// Scan 68000 state

		SCAN_VAR(nCpsCyclesExtra);
		SCAN_VAR(nCpsObjectBank);

		if (Cps1OverrideLayers) {
			SCAN_VAR(nCps1Layers);
			SCAN_VAR(nCps1LayerOffs);
		}

		CpsRwScan();									// scan cps_rw.cpp stuff

		if (Cps2Turbo) {
			BurnSampleScan(nAction, pnMin);
		}

		if (nAction & ACB_WRITE) {						// Palette could have changed
			CpsRecalcPal = 1;

			if (Cps == 2) {
				SekOpen(0);
				CpsDoMapObjectBanks(nCpsObjectBank);    // Re-Map object banks
				SekClose();
			}
		}
	}

	if (((Cps == 2) && !Cps2DisableQSnd) || Cps1Qs == 1) {						// Scan QSound chips
		QsndScan(nAction);
	} else {											// Scan PSound chips
		if ((Cps & 1) && !Cps1DisablePSnd) PsndScan(nAction, pnMin);
	}
	
	if (CpsMemScanCallbackFunction) {
		CpsMemScanCallbackFunction(nAction, pnMin);
	}
	
	return 0;
}

// xmvsf3v3: the display state other games rebuild after a load (latched palette, registers and
// 'four' registers, sprite lag buffers), so its states restore exactly, a paused redraw included
INT32 CpsXmvVisualScan(INT32 nAction)
{
	if (!CpsXmvSixEnabled || !(nAction & ACB_DRIVER_DATA)) return 0;

	struct BurnArea ba;
	memset(&ba, 0, sizeof(ba));
	ba.Data = CpsSavePal;		ba.nLen = 0x2000;								ba.szName = "XMvsSF 3v3 latched palette";	BurnAcb(&ba);
	ba.Data = CpsSaveRegData;	ba.nLen = 0x100 * (MAX_RASTER + 1);				ba.szName = "XMvsSF 3v3 latched registers";	BurnAcb(&ba);
	ba.Data = CpsSaveFrgData;	ba.nLen = 0x20;									ba.szName = "XMvsSF 3v3 latched frg";		BurnAcb(&ba);

	if (CpsObjVisualScan(nAction)) return 1;

	return CpsPalVisualScan(nAction);
}
