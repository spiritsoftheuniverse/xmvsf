// Opt-in private CPS2/MVC1 Motorola 68000 source dump. No emulator writes.
#ifndef XM87203_M68K_SNAPSHOT_H
#define XM87203_M68K_SNAPSHOT_H
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace XM87203M68K {
inline bool save(const std::string& path,const unsigned char* data,unsigned size){
    FILE* f=std::fopen(path.c_str(),"wb");
    if(!f)return false;
    const bool ok=std::fwrite(data,1,size,f)==size;
    const int closed=std::fclose(f);
    return ok && closed==0;
}
inline void once(const char* driver,unsigned frame,
                 const unsigned char* rom,unsigned romSize,
                 const unsigned char* opcodes,unsigned codeSize){
    static bool attempted=false;
    if(attempted)return;
    const char* dir=std::getenv("XM87203_M68K_DUMP_DIR");
    if(!dir || !*dir || !driver || std::strncmp(driver,"mvsc",4)!=0 ||
       !rom || !opcodes || !romSize || !codeSize)return;
    attempted=true;
    if(codeSize>romSize || romSize>0x1000000u){
        std::fprintf(stderr,"XM87.203 M68K snapshot rejected bad lengths\n");return;
    }
    const std::string root(dir);
    const bool ok=save(root+"/mvc_program_data_host_words.bin",rom,romSize) &&
                  save(root+"/mvc_opcodes_decrypted_host_words.bin",opcodes,codeSize);
    FILE* f=std::fopen((root+"/program_source.tsv").c_str(),"wb");
    if(f){
        std::fprintf(f,"frame\tdriver\trom_bytes\tdecrypted_bytes\tword_order\tstatus\n%u\t%s\t%u\t%u\thost_word_swapped\t%s\n",
                     frame,driver,romSize,codeSize,ok?"CAPTURED":"ERROR");
        std::fclose(f);
    }
    std::fprintf(stderr,"XM87.203 M68K_PROGRAM_SNAPSHOT %s dir=%s frame=%u\n",
                 ok?"CAPTURED":"ERROR",dir,frame);
}
}
#endif
