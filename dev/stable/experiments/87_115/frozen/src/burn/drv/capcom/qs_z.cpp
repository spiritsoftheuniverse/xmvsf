#include "cps.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern int xm14TrainingMode;
extern int xm87102DamageMode;
// QSound - Z80

static INT32 nQsndZBank = 0;
static UINT8 QscCmd[2] = {0, 0};

// Map in the memory for the current 0x8000-0xc000 music bank
static INT32 QsndZBankMap()
{
	UINT32 nOff;
	UINT8* Bank;
	nOff = nQsndZBank << 14;
	nOff += (Cps2Turbo) ? 0x10000 : 0x8000; // sfz3mix .22 z80 rom stored in mame memmap format (banks start at 0x10000, instead of 0x8000 like fbneo)

	if (Cps1Qs == 0) {
		if (nOff + 0x4000 > nCpsZRomLen) {			// End of bank is out of range
			nOff = 0;
		}
		Bank = CpsZRom + nOff;
	} else {
		if (nOff + 0x4000 > (nCpsZRomLen / 2)) {
			nOff = 0;
		}
		Bank = CpsZRom - (nCpsZRomLen / 2) + nOff;
	}

	// Read and fetch the bank
	ZetMapArea(0x8000, 0xbfff, 0, Bank);
	if (Cps1Qs == 0) {
		ZetMapArea(0x8000, 0xbfff, 2, Bank, CpsZRom + nOff);
	} else {
		ZetMapArea(0x8000, 0xbfff, 2, Bank);
	}

	return 0;
}

// XM87.102: actual QSound DSP register writes, provenance only. No audio mutation.
static void xm87102ObserveQSoundCommand(unsigned frame,unsigned command,unsigned value){
    if(!xm14TrainingMode||xm87102DamageMode!=1)return;
    static bool checked=false;
    static std::FILE* out=nullptr;
    static unsigned count=0;
    if(!checked){
        checked=true;
        const char* dir=std::getenv("XM87102_KO_TRACE_DIR");
        if(!dir||!*dir)return;
        char path[2048];
        const size_t len=std::strlen(dir);

        std::snprintf(path,sizeof(path),"%s%sxm87102_ko_qsound_commands.tsv",dir,
                      len && (dir[len-1]=='\\'||dir[len-1]=='/')?"":"\\");
        out=std::fopen(path,"wb");
        if(out)std::fputs("frame\tdsp_register\tdsp_value\n",out);
        else std::fprintf(stderr,"XM87.102 QSOUND_TRACE_OPEN_FAILED %s\n",path);
    }
    if(out&&count<300000u){
        std::fprintf(out,"%u\t%02X\t%04X\n",frame,command,value);
        if((++count%256u)==0u)std::fflush(out);
        if(count==1u)std::fprintf(stderr,"XM87.102 QSOUND_WRITE trace active\n");
    }
}

void __fastcall QsndZWrite(UINT16 a, UINT8 d)
{
	if (a == 0xd000) {
		QscCmd[0] = d;
		return;
	}
	if (a == 0xd001) {
		QscCmd[1] = d;
		return;
	}
	if (a == 0xd002) {
		QscWrite(d, (QscCmd[0] << 8) | QscCmd[1]);
		xm87102ObserveQSoundCommand((unsigned)GetCurrentFrame(),(unsigned)d,
		    ((unsigned)QscCmd[0]<<8)|(unsigned)QscCmd[1]);
//		bprintf(PRINT_NORMAL, _T("QSound command %02X %04X sent.\n"), d, (QscCmd[0] << 8) | QscCmd[1]);
		return;
	}
	if (a == 0xd003) {
		INT32 nNewBank = d & 0x0f;
		if (nQsndZBank != nNewBank) {
			nQsndZBank = nNewBank;
			QsndZBankMap();
		}
	}
}

UINT8 __fastcall QsndZRead(UINT16 a)
{
	if (a == 0xd007) {						// DSP status
		return QscRead();
	}
	return 0;
}

INT32 QsndZInit()
{
	if (nCpsZRomLen < 0x8000) {				// Not enough Z80 Data
		return 1;
	}
	if (CpsZRom == NULL) {
		return 1;
	}

	ZetInit(0);
	ZetOpen(0);

	ZetSetReadHandler(QsndZRead);
	ZetSetWriteHandler(QsndZWrite);

	// Read and fetch first 0x8000 of Rom
	if (Cps1Qs) {
		ZetMapArea(0x0000, 0x7FFF, 0, CpsZRom - (nCpsZRomLen / 2));
		ZetMapArea(0x0000, 0x7FFF, 2, CpsZRom, CpsZRom - (nCpsZRomLen / 2));	// If it tries to fetch this area
	} else {
		ZetMapArea(0x0000, 0x7FFF, 0 ,CpsZRom);
		ZetMapArea(0x0000, 0x7FFF, 2, CpsZRom);
	}

	// Map first Bank of Rom
	nQsndZBank = 0;
	QsndZBankMap();

	ZetMapArea(0xC000, 0xCFFF, 0, CpsZRamC0);
	ZetMapArea(0xC000, 0xCFFF, 1, CpsZRamC0);
	ZetMapArea(0xC000, 0xCFFF, 2, CpsZRamC0);

	ZetMemCallback(0xD000, 0xEFFF, 0);
	ZetMemCallback(0xD000, 0xEFFF, 1);

	if (Cps1Qs) {
		ZetMapArea(0xD000, 0xEFFF, 2, CpsZRom, CpsZRom - (nCpsZRomLen / 2));	// If it tries to fetch this area
	} else {
		ZetMapArea(0xD000, 0xEFFF, 2, CpsZRom);
	}

	ZetMapArea(0xF000, 0xFFFF, 0, CpsZRamF0);
	ZetMapArea(0xF000, 0xFFFF, 1, CpsZRamF0);
	ZetMapArea(0xF000, 0xFFFF, 2, CpsZRamF0);

	ZetClose();

	QscCmd[0] = QscCmd[1] = 0;

	return 0;
}

INT32 QsndZExit()
{
	ZetExit();
	return 0;
}

// Scan the current QSound z80 state
INT32 QsndZScan(INT32 nAction)
{
	ZetScan(nAction);					// Scan Z80
	SCAN_VAR(nQsndZBank);
	SCAN_VAR(QscCmd);

	if (nAction & ACB_WRITE) {			// If write, bank could have changed
		ZetOpen(0);
		QsndZBankMap();
		ZetClose();
	}

	return 0;
}
