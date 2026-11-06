#ifndef XMVSF_STAGE87_203_STAGE_CAPTURE_H
#define XMVSF_STAGE87_203_STAGE_CAPTURE_H
// Stage87.203 read-only CPS2 scenery telemetry. No emulated writes, no
// graphics mutations, no new renderer, and no automatic completeness claims.
// The caller supplies read-only state from the authoritative CPS2 frame.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>

namespace XM87203Stage {
static const unsigned MAX_SLICES=10u;
static const unsigned MAP_BYTES=0x4000u;
static const unsigned PALETTE_BYTES=0x1800u;
static const unsigned ROW_BYTES=0x800u;
static const unsigned OAM_BYTES=0x2000u;
static const unsigned MAX_FRAMES=36000u; // bounded 10-min recording

inline unsigned be16(const unsigned char *p,unsigned offset){
    return (unsigned(p[offset])<<8u)|unsigned(p[offset+1u]);
}
inline std::uint64_t hash64(const unsigned char *p,unsigned n){
    std::uint64_t h=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<n;++i){h ^= p[i];h*=UINT64_C(1099511628211);}
    return h;
}
inline bool put32(std::FILE*f,std::uint32_t n){
    unsigned char b[4]={static_cast<unsigned char>(n),
        static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n>>16),
        static_cast<unsigned char>(n>>24)};
    return std::fwrite(b,1,4,f)==4;
}
inline bool put64(std::FILE*f,std::uint64_t n){
    return put32(f,static_cast<std::uint32_t>(n)) &&
           put32(f,static_cast<std::uint32_t>(n>>32));
}
struct Frame {
    unsigned frame=0,phase=0,stage=0,sliceCount=0,objectBank=0;
    int screenW=384,screenH=224;
    const unsigned char *reg[MAX_SLICES]={};
    const unsigned char *frg[MAX_SLICES]={};
    unsigned rasterStart[MAX_SLICES]={};
    const unsigned char *map[MAX_SLICES][3]={};
    unsigned mapBank[MAX_SLICES][3]={};
    const unsigned char *row[MAX_SLICES]={};
    unsigned rowBank[MAX_SLICES]={};
    const unsigned char *palette=nullptr,*oam=nullptr;
};
struct Recorder {
    bool initialized=false,enabled=false,failed=false,hasStage=false;
    unsigned stage=0,lastFrame=~0u,frames=0u,mapWrites=0u,rowWrites=0u,paletteWrites=0u;
    unsigned long long objectWrites=0;
    std::uint64_t mapHash[MAX_SLICES][3]={},rowHash[MAX_SLICES]={},palHash=0u;
    bool mapKnown[MAX_SLICES][3]={},rowKnown[MAX_SLICES]={},palKnown=false;
    std::FILE *status=nullptr,*framesFile=nullptr,*registers=nullptr;
    std::FILE *maps=nullptr,*rows=nullptr,*palettes=nullptr,*objects=nullptr;
    std::string root;
    ~Recorder(){close();}
    void close(){
        std::FILE *all[]={framesFile,registers,maps,rows,palettes,objects,status};
        for(std::FILE *f:all)if(f)std::fclose(f);
        status=framesFile=registers=maps=rows=palettes=objects=nullptr;
    }
    void statusLine(const char *event,unsigned frame,const char *detail){
        if(status){
            std::fprintf(status,"frame=%u\t%s\t%s\n",frame,event,detail);
            std::fflush(status);
        }
        std::fprintf(stderr,"XM87.203 %s f=%u %s\n",event,frame,detail);
    }
    bool openFiles(){
        const char *path=std::getenv("XM87203_STAGE_CAPTURE_DIR");
        initialized=true;
        if(!path || !*path)return false;
        root=path;
        status=std::fopen((root+"/status.log").c_str(),"ab");
        framesFile=std::fopen((root+"/frames.tsv").c_str(),"ab");
        registers=std::fopen((root+"/raster.bin").c_str(),"ab");
        maps=std::fopen((root+"/maps.bin").c_str(),"ab");
        rows=std::fopen((root+"/row_scroll.bin").c_str(),"ab");
        palettes=std::fopen((root+"/palettes.bin").c_str(),"ab");
        objects=std::fopen((root+"/oam.bin").c_str(),"ab");
        enabled=status&&framesFile&&registers&&maps&&rows&&palettes&&objects;
        if(!enabled){
            std::fprintf(stderr,"XM87.203 ERROR could_not_open_capture_files dir=%s\n",root.c_str());
            failed=true;close();return false;
        }
        std::fputs("frame\tstage\tphase\tslices\tscroll1x\tscroll1y\tscroll2x\tscroll2y\tscroll3x\tscroll3y\tobjectBank\n",framesFile);
        statusLine("CAPTURE_START",0u,"read_only=1 format=XM87203A1 no_level_complete_without_verification");
        return true;
    }
    void fail(unsigned frame,const char*why){
        if(failed)return;
        statusLine("ERROR",frame,why);
        failed=true;enabled=false;close();
    }
    void observe(const Frame& f){
        // Selection/menu is not a valid stage capture.
        if(f.phase!=8u)return;
        if(!initialized && !openFiles())return;
        if(!enabled || failed || f.sliceCount==0u ||
           f.sliceCount>MAX_SLICES || !f.reg[0] || !f.frg[0] ||
           !f.palette || !f.oam || f.frame==lastFrame)return;
        if(frames>=MAX_FRAMES){
            statusLine("INCOMPLETE",f.frame,"capture_frame_limit_reached;_no_completion_claim");
            enabled=false;close();return;
        }
        if(lastFrame!=~0u && f.frame<lastFrame){
            statusLine("SESSION_RESET",f.frame,"native_frame_counter_rewind_new_segment");
            std::memset(mapKnown,0,sizeof(mapKnown));
            std::memset(rowKnown,0,sizeof(rowKnown));palKnown=false;
        }
        if(!hasStage || stage!=f.stage){
            if(hasStage){
                statusLine("INCOMPLETE",f.frame,"previous_stage_left_before_validation");
            }
            stage=f.stage;hasStage=true;
            std::memset(mapKnown,0,sizeof(mapKnown));
            std::memset(rowKnown,0,sizeof(rowKnown));palKnown=false;
            char msg[90];std::snprintf(msg,sizeof(msg),"stage=%u begin",stage);
            statusLine("STAGE_BEGIN",f.frame,msg);
        }
        // Frame metadata and every raster slice are kept at their original
        // emulation-frame position. One-frame-lagged OAM is not time-shifted.
        if(std::fprintf(framesFile,"%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
            f.frame,f.stage,f.phase,f.sliceCount,be16(f.reg[0],0x0c),
            be16(f.reg[0],0x0e),be16(f.reg[0],0x10),be16(f.reg[0],0x12),
            be16(f.reg[0],0x14),be16(f.reg[0],0x16),f.objectBank)<0)
            return fail(f.frame,"frame_tsv_write_failed");
        if(std::fwrite("FRM1",1,4,registers)!=4 ||
           !put32(registers,f.frame)||!put32(registers,f.stage)||
           !put32(registers,f.sliceCount))
            return fail(f.frame,"raster_header_write_failed");
        for(unsigned s=0;s<f.sliceCount;++s){
            if(!f.reg[s] || !f.frg[s] ||
               !put32(registers,f.rasterStart[s]) ||
               std::fwrite(f.reg[s],1,0x100,registers)!=0x100 ||
               std::fwrite(f.frg[s],1,0x10,registers)!=0x10)
                return fail(f.frame,"raster_slice_write_failed");
            for(unsigned layer=0;layer<3;++layer){
                const unsigned char *p=f.map[s][layer];
                if(!p)continue; // missing native bank remains a coverage gap
                const std::uint64_t hash=hash64(p,MAP_BYTES);
                if(!mapKnown[s][layer] || hash!=mapHash[s][layer]){
                    if(std::fwrite("MAP1",1,4,maps)!=4 ||
                       !put32(maps,f.frame)||!put32(maps,f.stage)||
                       !put32(maps,s)||!put32(maps,layer+1u)||
                       !put32(maps,f.mapBank[s][layer]) ||
                       !put64(maps,hash)||
                       std::fwrite(p,1,MAP_BYTES,maps)!=MAP_BYTES)
                        return fail(f.frame,"map_snapshot_write_failed");
                    mapHash[s][layer]=hash;mapKnown[s][layer]=true;
                    ++mapWrites;
                }
            }
            if(f.row[s]){
                const std::uint64_t hash=hash64(f.row[s],ROW_BYTES);
                if(!rowKnown[s] || rowHash[s]!=hash){
                    if(std::fwrite("ROW1",1,4,rows)!=4 ||
                       !put32(rows,f.frame)||!put32(rows,f.stage)||
                       !put32(rows,s)||!put32(rows,f.rowBank[s])||
                       !put64(rows,hash)||
                       std::fwrite(f.row[s],1,ROW_BYTES,rows)!=ROW_BYTES)
                        return fail(f.frame,"row_scroll_write_failed");
                    rowHash[s]=hash;rowKnown[s]=true;++rowWrites;
                }
            }
        }
        const std::uint64_t ph=hash64(f.palette,PALETTE_BYTES);
        if(!palKnown || ph!=palHash){
            if(std::fwrite("PAL1",1,4,palettes)!=4 ||
               !put32(palettes,f.frame)||!put32(palettes,f.stage)||
               !put64(palettes,ph)||
               std::fwrite(f.palette,1,PALETTE_BYTES,palettes)!=PALETTE_BYTES)
                return fail(f.frame,"palette_write_failed");
            palHash=ph;palKnown=true;++paletteWrites;
        }
        if(std::fwrite("OAM1",1,4,objects)!=4 ||
           !put32(objects,f.frame)||!put32(objects,f.stage)||
           !put32(objects,f.objectBank)||
           std::fwrite(f.oam,1,OAM_BYTES,objects)!=OAM_BYTES)
            return fail(f.frame,"oam_write_failed");
        ++frames;++objectWrites;lastFrame=f.frame;
        if((frames%300u)==0u){
            char msg[160];std::snprintf(msg,sizeof(msg),
                "stage=%u frames=%u map_snapshots=%u row_snapshots=%u palette_snapshots=%u oam_frames=%llu still_recording=1",
                stage,frames,mapWrites,rowWrites,paletteWrites,objectWrites);
            statusLine("PROGRESS",f.frame,msg);
            if(std::fflush(framesFile)!=0 || std::fflush(registers)!=0 ||
               std::fflush(maps)!=0 || std::fflush(rows)!=0 ||
               std::fflush(palettes)!=0 ||
               std::fflush(objects)!=0)fail(f.frame,"capture_flush_failed");
        }
    }
};
inline Recorder& state(){static Recorder s;return s;}
} // namespace XM87203Stage
#endif
