#ifndef XMVSF_STAGE87_203_STAGE_CAPTURE_H
#define XMVSF_STAGE87_203_STAGE_CAPTURE_H
// Stage87.203 read-only CPS2 scenery telemetry. No emulated writes, no
// graphics mutations, no new renderer, and no automatic completeness claims.
// The caller supplies read-only state from the authoritative CPS2 frame.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace XM87203Stage {
static const unsigned MAX_SLICES=10u;
static const unsigned MAP_BYTES=0x4000u;
static const unsigned PALETTE_BYTES=0x1800u;
static const unsigned ROW_BYTES=0x800u;
static const unsigned OAM_BYTES=0x2000u;
static const unsigned MAX_FRAMES=36000u; // bounded 10-min recording

inline int mkdirExclusive(const char* path){
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path,0755);
#endif
}
inline bool isMarvelVsCapcom(const char* driver){
    return driver && std::strncmp(driver,"mvsc",4)==0;
}
// FBNeo CPS2 RAM/register bytes are host-ordered under LSB_FIRST.
// The old big-endian interpretation converted valid 0x908000 tilemaps
// into unmapped 0x808000 addresses; stage_capture.zip proved this.
inline unsigned le16(const unsigned char *p,unsigned offset){
    return unsigned(p[offset])|(unsigned(p[offset+1u])<<8u);
}
inline unsigned gfxMapBank(const unsigned char *p,unsigned regOffset){
    return (le16(p,regOffset)<<8u)&0xFFC000u;
}
inline unsigned rowBank(const unsigned char *p,unsigned regOffset){
    return (le16(p,regOffset)<<8u)&0xFFF800u;
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
    unsigned frame=0,phase=0,stage=~0u,sliceCount=0,objectBank=0;
    bool captureActive=false; // hardware capture gate, NEVER XMVSF ROM action phase
    const char* driver=nullptr;
    // Original Marvel vs. Capcom CPS2 ROM graphics, decoded by FBNeo.
    // Stored privately under test/ for authentic atlas and sprite extraction.
    const unsigned char* gfx=nullptr;
    unsigned gfxSize=0,gfxMask=0,gfxScrollOffset[4]={};
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
    bool gfxReady=false,gfxAttempted=false;
    unsigned stage=0,lastFrame=~0u,frames=0u,mapWrites=0u,rowWrites=0u,paletteWrites=0u;
    unsigned long long objectWrites=0;
    unsigned activeBankFrames=0,missingBankFrames=0;
    bool mapSetComplete=false;
    std::uint64_t mapHash[MAX_SLICES][3]={},rowHash[MAX_SLICES]={},palHash=0u;
    bool mapKnown[MAX_SLICES][3]={},rowKnown[MAX_SLICES]={},palKnown=false;
    std::FILE *status=nullptr,*framesFile=nullptr,*registers=nullptr;
    std::FILE *maps=nullptr,*rows=nullptr,*palettes=nullptr,*objects=nullptr;
    std::string root,sessionDir;
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
        if(!status){
            std::fprintf(stderr,"XM87.203 ERROR cannot_open_root_status=%s\n",root.c_str());
            failed=true;return false;
        }
        // Never append a second ROM run to an older capture's raw streams.
        // mkdir is exclusive, so interrupted/incomplete sessions are never
        // reused or silently destroyed, including the uploaded 24k-frame run.
        for(unsigned serial=1u;serial<=9999u;++serial){
            char name[32];std::snprintf(name,sizeof(name),"session_%04u",serial);
            // Build the *first* candidate from capture root, not the
            // not-yet-initialized sessionDir. Otherwise it creates an
            // orphan ./session_XXXX and substr(root.size()) throws.
            const std::string candidate=root+"/"+name;
            if(mkdirExclusive(candidate.c_str())==0){
                sessionDir=candidate;break;
            }
        }
        if(sessionDir.empty()){
            statusLine("ERROR",0u,"no_available_session_directory");
            failed=true;close();return false;
        }
        framesFile=std::fopen((sessionDir+"/frames.tsv").c_str(),"wb");
        registers=std::fopen((sessionDir+"/raster.bin").c_str(),"wb");
        maps=std::fopen((sessionDir+"/maps.bin").c_str(),"wb");
        rows=std::fopen((sessionDir+"/row_scroll.bin").c_str(),"wb");
        palettes=std::fopen((sessionDir+"/palettes.bin").c_str(),"wb");
        objects=std::fopen((sessionDir+"/oam.bin").c_str(),"wb");
        enabled=status&&framesFile&&registers&&maps&&rows&&palettes&&objects;
        if(!enabled){
            std::fprintf(stderr,"XM87.203 ERROR could_not_open_capture_files dir=%s\n",root.c_str());
            failed=true;close();return false;
        }
        if(std::fputs("frame\tstage\tphase\tslices\tscroll1x\tscroll1y\tscroll2x\tscroll2y\tscroll3x\tscroll3y\tobjectBank\n",framesFile)==EOF){
            statusLine("ERROR",0u,"frames_header_write_failed");
            failed=true;close();return false;
        }
        // The root status.log remains the single source for the open MSYS
        // terminal; latest.txt points the offline verifier to this run.
        std::FILE* latest=std::fopen((root+"/latest.txt").c_str(),"wb");
        if(!latest){
            statusLine("ERROR",0u,"capture_latest_pointer_open_failed");
            failed=true;close();return false;
        }
        const size_t prefix=root.size()+1u;
        // Guard against malformed paths without throwing or corrupting the
        // persistent latest.txt pointer. No arbitrary substr() offsets.
        if(sessionDir.size()<=prefix ||
           sessionDir.compare(0,root.size(),root)!=0 ||
           sessionDir[root.size()]!='/' ||
           sessionDir.find('/',prefix)!=std::string::npos){
            std::fclose(latest);
            statusLine("ERROR",0u,"invalid_capture_session_directory");
            failed=true;close();return false;
        }
        const std::string relative=sessionDir.substr(prefix);
        const int wrote=std::fprintf(latest,"%s\n",relative.c_str());
        const int closed=std::fclose(latest);
        if(wrote<=0 || closed!=0){
            statusLine("ERROR",0u,"capture_latest_pointer_write_failed");
            failed=true;close();return false;
        }
        statusLine("CAPTURE_START",0u,
            "read_only=1 format=XM87203A1 no_level_complete_without_verification");
        statusLine("SESSION_DIR",0u,sessionDir.c_str());
        return true;
    }
    void fail(unsigned frame,const char*why){
        if(failed)return;
        statusLine("ERROR",frame,why);
        failed=true;enabled=false;close();
    }
    bool dumpDecodedGfx(const Frame& f){
        if(gfxAttempted)return gfxReady;
        const char* opt=std::getenv("XM87203_EXPORT_GFX");
        if(opt && std::strcmp(opt,"0")==0){
            gfxAttempted=true;
            statusLine("INCOMPLETE",f.frame,"decoded_gfx_export_disabled;_sprite_art_missing");
            return false;
        }
        if(!f.gfx || !f.gfxSize)return false; // allow ROM initialization to finish
        gfxAttempted=true;
        // Bounded to prevent a corrupted driver length from filling disk.
        if(f.gfxSize>256u*1024u*1024u){
            fail(f.frame,"decoded_gfx_size_out_of_range");
            return false;
        }
        const std::uint64_t signature=hash64(f.gfx,f.gfxSize);
        char name[100];
        std::snprintf(name,sizeof(name),"mvc1_decoded_gfx_%08X_%016llX.bin",
            f.gfxSize,static_cast<unsigned long long>(signature));
        const std::string dest=sessionDir+"/"+name;
        std::FILE* stream=std::fopen(dest.c_str(),"rb");
        if(stream){
            // Immutable hash-named local artifact; never truncate an older run.
            if(std::fseek(stream,0,SEEK_END)!=0 ||
               std::ftell(stream)!=static_cast<long>(f.gfxSize)){
                std::fclose(stream);
                fail(f.frame,"existing_decoded_gfx_artifact_length_mismatch");
                return false;
            }
            std::fclose(stream);
        }else{
            stream=std::fopen(dest.c_str(),"wb");
            if(!stream){fail(f.frame,"decoded_gfx_open_failed");return false;}
            unsigned written=0;
            while(written<f.gfxSize){
                const unsigned block=std::min(1024u*1024u,f.gfxSize-written);
                if(std::fwrite(f.gfx+written,1,block,stream)!=block){
                    std::fclose(stream);
                    fail(f.frame,"decoded_gfx_write_failed");return false;
                }
                written+=block;
            }
            const int flushResult=std::fflush(stream);
            const int closeResult=std::fclose(stream);
            if(flushResult!=0 || closeResult!=0){
                fail(f.frame,"decoded_gfx_close_failed");return false;
            }
        }
        std::FILE* idx=std::fopen((sessionDir+"/gfx_index.tsv").c_str(),"ab");
        if(!idx){fail(f.frame,"gfx_index_open_failed");return false;}
        const int n=std::fprintf(idx,"%u\t%s\t%u\t%08X\t%u\t%u\t%u\t%016llX\t%s\n",
            f.frame,f.driver?f.driver:"unknown",f.gfxSize,f.gfxMask,
            f.gfxScrollOffset[1],f.gfxScrollOffset[2],f.gfxScrollOffset[3],
            static_cast<unsigned long long>(signature),name);
        const bool valid=n>0 && std::fflush(idx)==0;
        const int closed=std::fclose(idx);
        if(!valid || closed!=0){fail(f.frame,"gfx_index_write_failed");return false;}
        gfxReady=true;
        char info[192];
        std::snprintf(info,sizeof(info),"rom=%s decoded_bytes=%u fnv64=%016llX file=%s",
            f.driver?f.driver:"unknown",f.gfxSize,
            static_cast<unsigned long long>(signature),name);
        statusLine("GFX_READY",f.frame,info);
        return true;
    }
    void observe(const Frame& f){
        // The MVC1 stage-phase address has NOT been ROM-verified. Record
        // original CPS2 registers whenever MVC1 is running, including the
        // transition frames; offline coverage analysis excludes menus.
        if(!f.captureActive || !isMarvelVsCapcom(f.driver))return;
        if(!initialized && !openFiles())return;
        if(!enabled || failed || f.sliceCount==0u ||
           f.sliceCount>MAX_SLICES || !f.reg[0] || !f.frg[0] ||
           !f.palette || !f.oam || f.frame==lastFrame)return;
        if(!gfxAttempted)dumpDecodedGfx(f);
        if(failed)return;
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
            char msg[140];
            if(stage==~0u){
                std::snprintf(msg,sizeof(msg),
                    "source_rom=%s stage_id=unverified native_cps2=1",
                    f.driver?f.driver:"unknown");
                statusLine("SOURCE_BEGIN",f.frame,msg);
            }else{
                std::snprintf(msg,sizeof(msg),"stage=%u begin",stage);
                statusLine("STAGE_BEGIN",f.frame,msg);
            }
        }
        // Real MVC registers must point into 0x900000..0x930000.
        // This checks source-backed graphics RAM, not a guessed ROM stage ID.
        unsigned validBanks=0,missingBanks=0;
        for(unsigned s=0;s<f.sliceCount;++s){
            for(unsigned l=0;l<3u;++l){
                const unsigned addr=f.mapBank[s][l];
                if(addr==0u)continue; // original ROM boot/reset frame
                if(addr>=0x900000u && addr+MAP_BYTES<=0x930000u){
                    ++validBanks;
                    if(!f.map[s][l])++missingBanks;
                }else{
                    ++missingBanks;
                }
            }
        }
        if(validBanks>0u || missingBanks>0u){
            ++activeBankFrames;
            if(missingBanks>0u)++missingBankFrames;
            else missingBankFrames=0u;
        }
        if(missingBankFrames>=5u){
            fail(f.frame,"MVC_TILEMAP_ADDRESS_INVALID_OR_UNMAPPED_check_CPS2_register_byte_order");
            return;
        }
        if(frames>180u && mapWrites==0u){
            fail(f.frame,"MVC_TILEMAPS_EMPTY_after_180_frames_stop_before_more_data_lost");
            return;
        }
        if(!mapSetComplete && mapKnown[0][0] && mapKnown[0][1] && mapKnown[0][2]){
            mapSetComplete=true;
            statusLine("MAP_SOURCE_READY",f.frame,"scroll1_scroll2_scroll3_native_banks_observed_not_yet_builder_verified");
        }
        // Frame metadata and every raster slice are kept at their original
        // emulation-frame position. One-frame-lagged OAM is not time-shifted.
        if(std::fprintf(framesFile,"%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
            f.frame,f.stage,f.phase,f.sliceCount,le16(f.reg[0],0x0c),
            le16(f.reg[0],0x0e),le16(f.reg[0],0x10),le16(f.reg[0],0x12),
            le16(f.reg[0],0x14),le16(f.reg[0],0x16),f.objectBank)<0)
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
                "source_rom=%s stage_id=%s frames=%u map_snapshots=%u row_snapshots=%u palette_snapshots=%u oam_frames=%llu still_recording=1",
                f.driver?f.driver:"unknown",stage==~0u?"unverified":"observed",
                frames,mapWrites,rowWrites,paletteWrites,objectWrites);
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
