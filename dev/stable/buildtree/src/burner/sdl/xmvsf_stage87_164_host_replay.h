// XMVSF 87.164 — host-aware F9 session movie.
// Preserve the original 87.61 recorder for old .bin/.fs files. This separate
// v2 format adds a host timeline: game sessions, menu choices and full native
// save-state checkpoints at every host-side reset/option discontinuity.
// Runs only when XM8764_REPLAY_ROOT (record) or XM8764_REPLAY_SESSION (play)
// is provided. No DHC, ROM script or Stable data is modified.
#ifndef XMVSF_STAGE87_164_HOST_REPLAY_H
#define XMVSF_STAGE87_164_HOST_REPLAY_H
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
namespace XM8764 {
enum { OFF=0, RECORD=1, PLAY=2, FAILED=3, COMPLETE=4 };
struct Host {
    int training=0,versus=0,owner=0,damage=0,menu=0,selection=0;
    int hitboxes=0,display=0,meter=0,dizzy=1,frameData=0,stage=0;
    int stageResolved=0,tournament=0;
    int selectPhase=0,selectFrames=0,selectPicks=0;
    int selectCandidates=0,selectCarry=0;
};
struct Slot { UINT8* address=nullptr; unsigned width=0,type=0; };
struct Segment {
    unsigned session=0,index=0,count=0;
    int mode=0;
    Host host;
    std::string reason,input,state;
    bool complete=false;
};
struct Context {
    int mode=OFF;
    bool initialized=false,insideGame=false,split=true,exitSession=false,failed=false;
    bool fast=true;
    unsigned session=0,segment=0,frames=0,allFrames=0,playIndex=0;
    unsigned totalSegments=0;
    std::string root,folder,journalPath,reason="SESSION_START",error;
    FILE* journal=nullptr;
    FILE* stream=nullptr;
    std::vector<Slot> slots;
    std::vector<Segment> segments;
    Host restored,lastRecordedHost;
    bool hostChanged=false,hostRecorded=false;
    // The user-visible recording indicator must not disappear simply
    // because v2 replaced the old F9 recorder. 180 render frames of
    // completion/error feedback survive F9 stop or a failed write.
    unsigned noticeFrames=0;
    bool lastSaved=false;
    bool sealed=false,unsealedPrefix=false;
    // Before F9: keep one native checkpoint at the first pre-combat VS
    // frame and up to two seconds of subsequent inputs (not save states
    // each frame). F9 can then start slightly late but replay from VS entry.
    bool preActive=false,preExpired=false,preAttempted=false;
    int preLastStatus=-1;
    unsigned preInputWidth=0;
    Host preHost;
    std::string preStatePath;
    std::vector<unsigned> preSlotLayout;
    std::vector<std::vector<UINT8>> preFrames;
    ~Context(){if(stream)std::fclose(stream);if(journal)std::fclose(journal);}
};
inline Context& context(){static Context x;return x;}
inline int makeDir(const char* p){
#ifdef _WIN32
    return _mkdir(p);
#else
    return mkdir(p,0775);
#endif
}
inline bool exists(const std::string& path){
    std::FILE* f=std::fopen(path.c_str(),"rb");
    if(!f)return false;
    std::fclose(f);return true;
}
inline void status(const std::string& message){
    Context& c=context();
    std::fprintf(stderr,"XM87.164 HOST_REPLAY %s\n",message.c_str());
    std::fflush(stderr);
    // A permanent status log OUTSIDE the session folder is essential:
    // even failing to mkdir/open the very first manifest must leave a
    // visible reason where RUN_TEST.bat and its packaging expect it.
    const char* primary=std::getenv("XM8764_REPLAY_STATUS_FILE");
    if(primary && *primary){
        std::FILE* f=std::fopen(primary,"ab");
        if(f){std::fprintf(f,"%s\n",message.c_str());std::fclose(f);}
    }
    // Playback diagnostics must not modify the capture session journal.
    const char* replaySession=std::getenv("XM8764_REPLAY_SESSION");
    if(!c.folder.empty() && !(replaySession && *replaySession)){
        const std::string path=c.folder+"/replay_v2_status.log";
        std::FILE* f=std::fopen(path.c_str(),"ab");
        if(f){std::fprintf(f,"%s\n",message.c_str());std::fclose(f);}
    }
}
inline bool fail(const char* why){
    Context& c=context();
    c.mode=FAILED;c.failed=true;c.error=why;
    c.lastSaved=false;c.noticeFrames=180;
    status(std::string("REPLAY_DIVERGED ")+why);
    return false;
}
inline bool write32(std::FILE* f,std::uint32_t x){
    UINT8 b[4]={(UINT8)x,(UINT8)(x>>8),(UINT8)(x>>16),(UINT8)(x>>24)};
    return std::fwrite(b,1,4,f)==4u;
}
inline bool read32(std::FILE* f,std::uint32_t& x){
    UINT8 b[4];if(std::fread(b,1,4,f)!=4u)return false;
    x=(std::uint32_t)b[0]|((std::uint32_t)b[1]<<8)|
      ((std::uint32_t)b[2]<<16)|((std::uint32_t)b[3]<<24);
    return true;
}
inline bool collect(){
    Context& c=context();c.slots.clear();
    for(unsigned i=0;i<1024;++i){
        BurnInputInfo info={};
        if(BurnDrvGetInputInfo(&info,i))break;
        if(!info.pVal)continue;
        const unsigned width=(info.nType & BIT_GROUP_ANALOG)?2u:1u;
        Slot slot;slot.address=info.pVal;slot.type=(unsigned)info.nType;
        slot.width=width;c.slots.push_back(slot);
    }
    return !c.slots.empty() && c.slots.size()<=1024;
}
inline void releaseInputs(){
    for(const Slot& s:context().slots)
        for(unsigned i=0;i<s.width;++i)s.address[i]=0;
}
inline void hostWrite(std::FILE* f,const Host& h){
    std::fprintf(f,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
      h.training,h.versus,h.owner,h.damage,h.menu,h.selection,h.hitboxes,
      h.display,h.meter,h.dizzy,h.frameData,h.stage,
      h.stageResolved,h.tournament,h.selectPhase,h.selectFrames,
      h.selectPicks,h.selectCandidates,h.selectCarry);
}
inline bool hostRead(std::istringstream& stream,Host& h){
    return bool(stream>>h.training>>h.versus>>h.owner>>h.damage>>
        h.menu>>h.selection>>h.hitboxes>>h.display>>h.meter>>
        h.dizzy>>h.frameData>>h.stage>>h.stageResolved>>h.tournament>>
        h.selectPhase>>h.selectFrames>>h.selectPicks>>
        h.selectCandidates>>h.selectCarry);
}
// Only *semantic* host transitions create new native save-state files.
// selectFrames advances every real frame during character selection,
// while selectCandidates advances during combat detection. Comparing the
// entire Host struct (87.164/165) checkpointed EVERY FRAME, which caused
// thousands of oversized .fs files and interrupted F9 recordings.
inline bool requiresCheckpoint(const Host& a,const Host& b){
    return a.training!=b.training || a.versus!=b.versus ||
        a.owner!=b.owner || a.damage!=b.damage ||
        a.menu!=b.menu || a.selection!=b.selection ||
        a.hitboxes!=b.hitboxes || a.display!=b.display ||
        a.meter!=b.meter || a.dizzy!=b.dizzy ||
        a.frameData!=b.frameData || a.stage!=b.stage ||
        a.stageResolved!=b.stageResolved || a.tournament!=b.tournament ||
        a.selectPhase!=b.selectPhase || a.selectPicks!=b.selectPicks ||
        a.selectCarry!=b.selectCarry;
}
inline bool loadManifest(){
    Context& c=context();
    c.segments.clear();c.sealed=false;c.unsealedPrefix=false;
    std::ifstream f((c.folder+"/manifest.tsv").c_str(),std::ios::binary);
    if(!f)return fail("V2_MANIFEST_MISSING");
    std::string line;
    if(!std::getline(f,line) || line!="XM8764V2\t1")
        return fail("V2_MANIFEST_SIGNATURE");
    bool sawFinal=false;
    unsigned finalFrames=0,finalSessions=0,finalSegments=0;
    unsigned lastEventFrame=0,maxEventFrame=0;
    std::string finalReason;
    while(std::getline(f,line)){
        if(line.empty())continue;
        std::istringstream in(line);std::string key;
        in>>key;
        if(sawFinal)return fail("V2_TRAILING_RECORD_AFTER_FINAL");
        if(key=="FINAL"){
            if(!(in>>finalFrames>>finalSessions>>finalSegments>>finalReason) ||
               finalReason.empty())return fail("V2_FINAL_FORMAT");
            std::string extra;
            if(in>>extra)return fail("V2_FINAL_TRAILING_DATA");
            if(finalReason!="F9_STOP" &&
               finalReason!="APPLICATION_EXIT")
                return fail("V2_FINAL_STOP_REASON");
            sawFinal=true;
        }else if(key=="EVENT"){
            unsigned eventSession=0,absolute=0,x=0,y=0;
            std::string verb,extra;
            if(!(in>>eventSession>>absolute>>verb>>x>>y) || verb.empty() ||
               (in>>extra))
                return fail("V2_MANIFEST_EVENT_FORMAT");
            if(absolute<lastEventFrame)
                return fail("V2_NONMONOTONIC_EVENT");
            lastEventFrame=absolute;
            if(absolute>maxEventFrame)maxEventFrame=absolute;
        }else if(key=="SEG"){
            Segment s;
            if(!(in>>s.session>>s.index>>s.mode>>s.reason>>
                  s.input>>s.state) || !hostRead(in,s.host))
                return fail("V2_MANIFEST_SEGMENT_FORMAT");
            c.segments.push_back(s);
        }else if(key=="DONE"){
            unsigned session=0,index=0,count=0;
            if(!(in>>session>>index>>count))
                return fail("V2_MANIFEST_DONE_FORMAT");
            bool found=false;
            for(Segment& s:c.segments){
                if(s.session==session && s.index==index){
                    if(s.complete)return fail("V2_DUPLICATE_DONE");
                    s.complete=true;s.count=count;found=true;break;
                }
            }
            if(!found)return fail("V2_ORPHAN_DONE");
        }else{
            return fail("V2_UNKNOWN_MANIFEST_VERB");
        }
    }
    if(c.segments.empty())return fail("V2_NO_SEGMENTS");
    unsigned countFrames=0,countSessions=0;
    for(unsigned i=0;i<c.segments.size();++i){
        const Segment& seg=c.segments[i];
        if(!seg.complete||!seg.count)return fail("V2_INCOMPLETE_SEGMENT");
        if(seg.session==0 || seg.index>9999u)return fail("V2_SEGMENT_ID");
        if(seg.input.find('/')!=std::string::npos ||
           seg.input.find('\\')!=std::string::npos ||
           seg.state.find('/')!=std::string::npos ||
           seg.state.find('\\')!=std::string::npos ||
           seg.input.find("..")!=std::string::npos ||
           seg.state.find("..")!=std::string::npos)
            return fail("V2_SEGMENT_PATH_NOT_LOCAL");
        if(!i){
            if(seg.session!=1u || seg.index!=0u)
                return fail("V2_FIRST_SEGMENT_NOT_SESSION_1");
            countSessions=1u;
        }else{
            const Segment& prev=c.segments[i-1];
            if(seg.session==prev.session){
                if(seg.index!=prev.index+1u)return fail("V2_SEGMENT_ORDER");
            }else{
                if(seg.session!=prev.session+1u || seg.index!=0u)
                    return fail("V2_SESSION_ORDER");
                ++countSessions;
            }
        }
        if(seg.count>10000000u-countFrames)
            return fail("V2_TOO_MANY_FRAMES");
        countFrames+=seg.count;
    }
    if(maxEventFrame>countFrames)return fail("V2_EVENT_BEYOND_LAST_INPUT");
    c.sealed=sawFinal;
    c.unsealedPrefix=!sawFinal;
    if(sawFinal){
        if(finalFrames!=countFrames ||
           finalSessions!=countSessions ||
           finalSegments!=c.segments.size())
            return fail("V2_FINAL_TOTAL_MISMATCH");
        status("V2_MANIFEST_SEALED frames="+std::to_string(countFrames)+
            " sessions="+std::to_string(countSessions)+
            " segments="+std::to_string(c.segments.size())+
            " reason="+finalReason);
    }else{
        status("V2_UNSEALED_PREFIX frames="+std::to_string(countFrames)+
            " segments="+std::to_string(c.segments.size())+
            " explicit_selection_only");
    }
    status("V2_MANIFEST_READY segments="+std::to_string(c.segments.size()));
    return true;
}
inline void initialize(){
    Context& c=context();
    if(c.initialized)return;
    c.initialized=true;
    const char* session=std::getenv("XM8764_REPLAY_SESSION");
    if(session&&*session){
        c.folder=session;c.mode=PLAY;
        c.fast=!(std::getenv("XM8764_FAST_FORWARD") &&
            std::strcmp(std::getenv("XM8764_FAST_FORWARD"),"0")==0);
        if(!loadManifest())return;
        return;
    }
    const char* root=std::getenv("XM8764_REPLAY_ROOT");
    if(root&&*root)c.root=root;
}
inline bool isPlayback(){initialize();return context().mode==PLAY;}
inline bool isRecording(){initialize();return context().mode==RECORD;}
inline int recordingFrames(){
    initialize();
    const Context& c=context();
    if(c.mode!=RECORD)return 0;
    return c.allFrames<2147483600u?int(c.allFrames)+1:2147483600;
}
// 1=recording, 2=success notice, -1=failed notice, 0=off.
inline int visualStatus(bool advance=true){
    initialize();Context& c=context();
    if(c.mode==RECORD)return 1;
    if(c.mode==FAILED)return -1;
    if(c.noticeFrames>0u){
        if(advance)--c.noticeFrames;
        return c.lastSaved?2:-1;
    }
    return 0;
}
inline bool active(){initialize();return context().mode==RECORD ||
                                            context().mode==PLAY;}
inline void journal(const char* event,unsigned x=0,unsigned y=0){
    Context& c=context();
    if(c.mode!=RECORD||!c.journal)return;
    std::fprintf(c.journal,"EVENT\t%u\t%u\t%s\t%u\t%u\n",
        c.session,c.allFrames,event,x,y);
    std::fflush(c.journal);
}
inline void finishAtNormalExit(); // registered AFTER Context construction
inline bool startRecording(){
    Context& c=context();initialize();
    // An earlier failed save may leave a partial stream/manifest open.
    // Never reuse it or write new frames into a previous recording.
    if(c.stream){std::fclose(c.stream);c.stream=nullptr;}
    if(c.journal){std::fclose(c.journal);c.journal=nullptr;}
    c.folder.clear();c.error.clear();
    c.noticeFrames=0;c.lastSaved=false;
    if(c.root.empty())return fail("V2_RECORD_ROOT_UNSET");
    // RUN_TEST.bat pre-creates this directory, but fail visibly if it
    // cannot be created or has become inaccessible. Never silently
    // invent success because a previous directory happened to exist.
    if(makeDir(c.root.c_str())!=0){
        const std::string marker=c.root+"/_recording_write_test.tmp";
        std::FILE* probe=std::fopen(marker.c_str(),"wb");
        if(!probe)return fail("V2_RECORD_ROOT_NOT_WRITABLE");
        std::fclose(probe);
        std::remove(marker.c_str());
    }
    bool found=false;
    for(unsigned n=1;n<=9999;++n){
        char name[48];std::snprintf(name,sizeof(name),"/session_%04u",n);
        const std::string folder=c.root+name;
        if(exists(folder+"/manifest.tsv"))continue;
        if(makeDir(folder.c_str())==0){
            c.folder=folder;found=true;break;
        }
    }
    if(!found)return fail("V2_RECORD_DIRECTORY_UNAVAILABLE");
    c.journalPath=c.folder+"/manifest.tsv";
    c.journal=std::fopen(c.journalPath.c_str(),"wb");
    if(!c.journal)return fail("V2_MANIFEST_CREATE_FAILED");
    if(std::fputs("XM8764V2\t1\n",c.journal)==EOF ||
       std::fflush(c.journal)!=0){
        std::fclose(c.journal);c.journal=nullptr;
        return fail("V2_MANIFEST_HEADER_WRITE_FAILED");
    }
    // This handler is registered after the function-local Context
    // destructor, so it is invoked before teardown, after the
    // current RunMessageLoop has finished. Even closing the window
    // without pressing F9 writes the FINAL seal.
    static bool registered=false;
    if(!registered){
        if(std::atexit(finishAtNormalExit)!=0)
            return fail("V2_EXIT_FINALIZER_REGISTRATION_FAILED");
        registered=true;
    }
    c.mode=RECORD;c.insideGame=false;c.split=true;c.session=0;c.segment=0;
    c.frames=0;c.allFrames=0;c.totalSegments=0;
    c.reason="SESSION_START";c.failed=false;
    c.sealed=false;c.unsealedPrefix=false;
    c.hostRecorded=false;c.lastSaved=false;c.noticeFrames=0;
    // Do not point the permanent replay runner at an empty recording
    // until F9 actually finishes a playable multi-session manifest.
    status("RECORD_START "+c.folder);
    return true;
}
inline bool closeSegment(){
    Context& c=context();
    if(!c.stream)return true;
    const int flushRc=std::fflush(c.stream);
    const int closeRc=std::fclose(c.stream);
    c.stream=nullptr;
    if(flushRc!=0 || closeRc!=0)
        return fail("V2_INPUT_FLUSH_OR_CLOSE_FAILED");
    if(c.mode==RECORD && c.journal){
        if(std::fprintf(c.journal,"DONE\t%u\t%u\t%u\n",
                c.session,c.segment,c.frames)<0 ||
           std::fflush(c.journal)!=0)
            return fail("V2_MANIFEST_DONE_WRITE_FAILED");
        ++c.totalSegments; // counts only verified, committed DONE entries
        status("SEGMENT_SAVED session="+std::to_string(c.session)+
            " segment="+std::to_string(c.segment)+
            " frames="+std::to_string(c.frames));
    }
    return true;
}
// Seal a movie only after the final verified input segment is DONE.
// The native application may exit while F9 is still active; that is
// an intentional APPLICATION_EXIT stop, not an incomplete recording.
inline bool finalizeRecording(const char* reason){
    Context& c=context();initialize();
    if(c.mode!=RECORD)return false;
    if(!closeSegment())return false;
    c.insideGame=false;
    if(!c.journal)return fail("V2_MISSING_OPEN_MANIFEST");
    if(c.allFrames==0u){
        std::fclose(c.journal);c.journal=nullptr;
        c.mode=OFF;c.lastSaved=false;c.noticeFrames=180;
        status("RECORD_EMPTY no emulated frames; recording not published");
        return false;
    }
    const char* why=(reason&&*reason)?reason:"F9_STOP";
    // Explicit footer is part of the immutable manifest format. The
    // FINAL record is not present in the user's real 87.166 capture;
    // that capture remains valid as an unsealed, complete-frame prefix.
    if(std::fprintf(c.journal,"FINAL\t%u\t%u\t%u\t%s\n",
            c.allFrames,c.session,c.totalSegments,why)<0 ||
       std::fflush(c.journal)!=0)
        return fail("V2_FINAL_WRITE_FAILED");
    const int manifestClose=std::fclose(c.journal);c.journal=nullptr;
    if(manifestClose!=0)return fail("V2_MANIFEST_FINAL_CLOSE_FAILED");
    // Keep the existing original/latest directory pointer contract.
    // A failed publish is an explicit error; never claim success.
    const std::string latest=c.root+"/latest.txt";
    std::FILE* pointer=std::fopen(latest.c_str(),"wb");
    if(!pointer){
        c.mode=OFF;c.lastSaved=false;c.noticeFrames=180;
        status("RECORD_SAVE_ERROR cannot write latest.txt");
        return false;
    }
    const int wrote=std::fprintf(pointer,"%s\n",c.folder.c_str());
    const int closeCode=std::fclose(pointer);
    if(wrote<0 || closeCode!=0){
        c.mode=OFF;c.lastSaved=false;c.noticeFrames=180;
        status("RECORD_SAVE_ERROR failed writing latest.txt");
        return false;
    }
    c.lastSaved=true;c.noticeFrames=180;c.sealed=true;
    status("RECORD_SAVED total_frames="+std::to_string(c.allFrames)+
        " session_count="+std::to_string(c.session)+
        " segments="+std::to_string(c.totalSegments)+
        " reason="+why+" folder="+c.folder);
    c.mode=OFF;
    return true;
}
inline void finishAtNormalExit(){
    Context& c=context();
    if(c.mode==RECORD)finalizeRecording("APPLICATION_EXIT");
}
inline bool toggleF9(){
    initialize();Context& c=context();
    if(c.mode==PLAY){status("F9_REJECTED_PLAYBACK_ACTIVE");return false;}
    if(c.mode==RECORD)return finalizeRecording("F9_STOP");
    return startRecording();
}
inline void hostEvent(const char* event,unsigned detail=0){
    Context& c=context();initialize();
    if(c.mode!=RECORD)return;
    journal(event,detail);
    c.reason=event;c.split=true;
}
inline void checkpoint(const char* reason){
    Context& c=context();initialize();
    if(c.mode!=RECORD || !c.insideGame)return;
    journal("HOST_STATE_LOAD",0);
    c.reason=reason;c.split=true;
}
inline int nextMode(){
    initialize();Context& c=context();
    if(c.mode!=PLAY)return -2;
    if(c.playIndex>=c.segments.size())return -1;
    return c.segments[c.playIndex].mode;
}
inline void gameBegin(int gameMode){
    Context& c=context();initialize();
    if(c.mode==RECORD){
        if(c.insideGame)return;
        ++c.session;c.segment=0;c.insideGame=true;c.split=true;
        c.hostRecorded=false;c.reason="SESSION_START";
        journal("GAME_BEGIN",(unsigned)gameMode);
    }else if(c.mode==PLAY){
        if(c.playIndex>=c.segments.size()){fail("GAME_BEGIN_NO_SEGMENT");return;}
        if(c.segments[c.playIndex].mode!=gameMode){
            fail("GAME_MODE_MISMATCH");return;
        }
        c.session=c.segments[c.playIndex].session;
        c.insideGame=true;c.exitSession=false;
        status("GAME_BEGIN session="+std::to_string(c.session)+
            " mode="+std::to_string(gameMode));
    }
}
inline void gameEnd(){
    Context& c=context();initialize();
    if(c.mode==RECORD && c.insideGame){
        if(!closeSegment()){c.insideGame=false;return;}
        journal("GAME_END",0);
    }
    if(c.mode==PLAY && c.stream){
        std::fclose(c.stream);c.stream=nullptr;
    }
    c.insideGame=false;c.exitSession=false;
}
inline bool openRecord(const Host& host){
    Context& c=context();
    if(c.stream){
        if(!closeSegment())return false;
        ++c.segment;
    }
    c.frames=0;
    char stem[48];std::snprintf(stem,sizeof(stem),
        "s%04u_seg%04u",c.session,c.segment);
    const std::string input=std::string(stem)+"_inputs.bin";
    const std::string state=std::string(stem)+"_start.fs";
    if(!collect())return fail("V2_NO_EMULATED_INPUT_SLOTS");
    const std::string statePath=c.folder+"/"+state;
    bool usePreroll=c.segment==0u&&c.preActive&&!c.preExpired&&
        !c.preFrames.empty()&&c.preHost.training==host.training&&
        c.preHost.versus==host.versus&&c.preHost.stageResolved==host.stageResolved;
    unsigned inputWidth=0;
    std::vector<unsigned> layout;
    for(const Slot& slot:c.slots){
        inputWidth+=slot.width;
        layout.push_back((slot.type<<8)|slot.width);
    }
    usePreroll=usePreroll&&c.preInputWidth==inputWidth&&
               c.preSlotLayout==layout;
    if(usePreroll){
        std::ifstream source(c.preStatePath.c_str(),std::ios::binary);
        std::ofstream dest(statePath.c_str(),std::ios::binary|std::ios::trunc);
        if(source&&dest){dest<<source.rdbuf();usePreroll=bool(dest);}
        else usePreroll=false;
    }
    if(!usePreroll&&BurnStateSave(const_cast<char*>(statePath.c_str()),1)!=0)
        return fail("V2_CHECKPOINT_SAVE_FAILED");
    const std::string inputPath=c.folder+"/"+input;
    c.stream=std::fopen(inputPath.c_str(),"wb");
    if(!c.stream)return fail("V2_INPUT_OPEN_FAILED");
    const char* driver=BurnDrvGetTextA(DRV_NAME);
    char rom[32]={};
    if(driver)std::strncpy(rom,driver,sizeof(rom)-1);
    if(std::fwrite("XM8761R1",1,8,c.stream)!=8u||
       !write32(c.stream,(std::uint32_t)c.slots.size())||
       std::fwrite(rom,1,32,c.stream)!=32u)
        return fail("V2_INPUT_HEADER_WRITE_FAILED");
    for(const Slot& s:c.slots){
        UINT8 info[2]={(UINT8)s.type,(UINT8)s.width};
        if(std::fwrite(info,1,2,c.stream)!=2u)
            return fail("V2_INPUT_LAYOUT_WRITE_FAILED");
    }
    if(usePreroll){
        // Identical binary input format and frame indices as normal v2,
        // but start at the earlier saved native VS-entry boundary.
        for(const auto& bytes:c.preFrames){
            if(bytes.size()!=inputWidth || !write32(c.stream,c.frames) ||
               std::fwrite(bytes.data(),1,inputWidth,c.stream)!=inputWidth)
                return fail("V2_PREROLL_INPUT_WRITE_FAILED");
            ++c.frames;++c.allFrames;
        }
    }
    std::fprintf(c.journal,"SEG\t%u\t%u\t%d\t%s\t%s\t%s\t",
        c.session,c.segment,host.versus?2:host.training?1:0,
        usePreroll?"PRECOMBAT_VS_ENTRY":c.reason.c_str(),
        input.c_str(),state.c_str());
    hostWrite(c.journal,usePreroll?c.preHost:host);
    if(usePreroll){
        status("VERSUS_PREROLL_APPLIED frames="+std::to_string(c.frames)+
               " checkpoint="+state);
        c.preActive=false;
    }
    std::fputc('\n',c.journal);std::fflush(c.journal);
    c.split=false;
    status("CHECKPOINT_SAVED session="+std::to_string(c.session)+
        " segment="+std::to_string(c.segment)+
        " reason="+c.reason);
    return true;
}
inline bool openPlay(){
    Context& c=context();
    if(c.playIndex>=c.segments.size())return fail("V2_SEGMENT_EOF");
    const Segment& s=c.segments[c.playIndex];
    if(s.session!=c.session)return fail("V2_SEGMENT_SESSION_MISMATCH");
    if(!collect())return fail("V2_PLAY_INPUT_LAYOUT_UNAVAILABLE");
    c.stream=std::fopen((c.folder+"/"+s.input).c_str(),"rb");
    if(!c.stream)return fail("V2_INPUT_SEGMENT_MISSING");
    char magic[8],rom[32],live[32]={};
    std::uint32_t count=0;
    const char* driver=BurnDrvGetTextA(DRV_NAME);
    if(driver)std::strncpy(live,driver,sizeof(live)-1);
    if(std::fread(magic,1,8,c.stream)!=8u ||
       std::memcmp(magic,"XM8761R1",8)!=0 ||
       !read32(c.stream,count) || count!=c.slots.size() ||
       std::fread(rom,1,32,c.stream)!=32u ||
       std::memcmp(rom,live,32)!=0)
        return fail("V2_SEGMENT_ROM_OR_INPUT_MISMATCH");
    for(const Slot& slot:c.slots){
        UINT8 info[2];
        if(std::fread(info,1,2,c.stream)!=2u ||
           info[0]!=(UINT8)slot.type || info[1]!=(UINT8)slot.width)
            return fail("V2_SEGMENT_LAYOUT_MISMATCH");
    }
    unsigned bytes=4u;
    for(const Slot& slot:c.slots)bytes+=slot.width;
    const long pos=std::ftell(c.stream);
    if(pos<0||std::fseek(c.stream,0,SEEK_END)!=0)
        return fail("V2_INPUT_SEEK_FAILED");
    const long end=std::ftell(c.stream);
    if(end-pos!=long(s.count)*long(bytes) ||
       std::fseek(c.stream,pos,SEEK_SET)!=0)
        return fail("V2_INPUT_SIZE_MISMATCH");
    const std::string state=c.folder+"/"+s.state;
    if(BurnStateLoad(const_cast<char*>(state.c_str()),1,NULL)!=0)
        return fail("V2_NATIVE_CHECKPOINT_LOAD_FAILED");
    c.frames=0;c.restored=s.host;c.hostChanged=true;
    status("CHECKPOINT_LOADED session="+std::to_string(s.session)+
        " segment="+std::to_string(s.index)+
        " frames="+std::to_string(s.count)+" reason="+s.reason);
    return true;
}
// These frames are sampled AFTER InputMake but BEFORE BurnDrvFrame.
// Only one native state is saved on the ROM's first pre-combat transition.
// Never roll the native machine backwards or generate synthetic inputs.
inline void captureVersusPreroll(const Host& host){
    Context& c=context();
    if(c.mode!=OFF || c.root.empty() || (!host.training&&!host.versus))
        return;
    const char* name=BurnDrvGetTextA(DRV_NAME);
    if(!name||std::strncmp(name,"xmvsf",5)!=0)return;
    cpu_core_config* cpu=GetCpuCoreConfig(0);
    if(!cpu||!cpu->open||!cpu->read||!cpu->close)return;
    cpu->open(0);
    const int nativeStatus=cpu->read(0xFF5000u)&255u;
    cpu->close();
    // Selection is 00; an advancing pre-combat status (02,04,06)
    // starts the native VS screen. 08 is live combat.
    if(nativeStatus==0 && c.preLastStatus!=0){
        c.preAttempted=false;c.preExpired=false;c.preActive=false;
        c.preFrames.clear();c.preSlotLayout.clear();
    }
    const int previous=c.preLastStatus;
    c.preLastStatus=nativeStatus;
    if(nativeStatus>=8 || (status==0 && c.preActive)){
        c.preActive=false;c.preFrames.clear();
        return;
    }
    if(!c.preActive&&!c.preAttempted&&nativeStatus>0&&nativeStatus<8&&
       previous==0&&host.selectPhase>=3){
        c.preAttempted=true;
        if(!collect())return;
        c.preHost=host;
        c.preSlotLayout.clear();c.preInputWidth=0;
        for(const Slot& slot:c.slots){
            c.preInputWidth+=slot.width;
            c.preSlotLayout.push_back((slot.type<<8)|slot.width);
        }
        c.preStatePath=c.root+"/.versus_preroll_start.fs";
        if(BurnStateSave(const_cast<char*>(c.preStatePath.c_str()),1)!=0){
            status("VERSUS_PREROLL_STATE_FAILED");return;
        }
        c.preActive=true;c.preExpired=false;c.preFrames.clear();
        status("VERSUS_PREROLL_ARMED status="+std::to_string(nativeStatus));
    }
    if(!c.preActive)return;
    if(c.preFrames.size()>=120u){
        c.preActive=false;c.preExpired=true;c.preFrames.clear();
        status("VERSUS_PREROLL_EXPIRED 120 frames");
        return;
    }
    std::vector<UINT8> bytes;
    bytes.reserve(c.preInputWidth);
    for(const Slot& slot:c.slots)
        for(unsigned i=0;i<slot.width;++i)bytes.push_back(slot.address[i]);
    c.preFrames.push_back(bytes);
}
enum FrameResult { NO_REPLAY=0, FRAME_READY=1, END_GAME=2, DIVERGED=3 };
inline FrameResult beforeFrame(const Host& host){
    Context& c=context();initialize();
    if(c.mode!=RECORD && c.mode!=PLAY){
        if(c.mode==OFF)captureVersusPreroll(host);
        return NO_REPLAY;
    }
    if(!c.insideGame)gameBegin(host.versus?2:host.training?1:0);
    if(c.mode==FAILED)return DIVERGED;
    if(c.mode==RECORD){
        // The game's host UI consumes SDL gamepad/keyboard events outside
        // BurnDrvGetInputInfo. Snapshot its actual resolved state whenever
        // menu navigation, player ownership, mode or Training options change.
        if(c.hostRecorded && requiresCheckpoint(host,c.lastRecordedHost)){
            journal("HOST_MENU_STATE_CHANGED",0);
            c.reason="HOST_MENU_STATE_CHANGED";c.split=true;
        }
        c.lastRecordedHost=host;c.hostRecorded=true;
        if(c.split || !c.stream)if(!openRecord(host))return DIVERGED;
        if(!write32(c.stream,c.frames))return fail("V2_FRAME_WRITE_FAILED"),DIVERGED;
        for(const Slot& slot:c.slots)
            if(std::fwrite(slot.address,1,slot.width,c.stream)!=slot.width)
                return fail("V2_INPUT_WRITE_FAILED"),DIVERGED;
        ++c.frames;++c.allFrames;
        return FRAME_READY;
    }
    if(!c.stream && !openPlay())return DIVERGED;
    if(c.frames==c.segments[c.playIndex].count){
        std::fclose(c.stream);c.stream=nullptr;
        ++c.playIndex;
        if(c.playIndex>=c.segments.size()){
            c.mode=COMPLETE;c.exitSession=true;
            status("PLAY_END complete_frames="+std::to_string(c.allFrames));
            releaseInputs();return END_GAME;
        }
        if(c.segments[c.playIndex].session!=c.session){
            c.exitSession=true;releaseInputs();
            status("SESSION_END next_session="+
                std::to_string(c.segments[c.playIndex].session));
            return END_GAME;
        }
        if(!openPlay())return DIVERGED;
    }
    std::uint32_t index=~0u;
    if(!read32(c.stream,index)||index!=c.frames)
        return fail("V2_FRAME_INDEX_MISMATCH"),DIVERGED;
    std::vector<UINT8> bytes;
    for(const Slot& slot:c.slots){
        UINT8 value[2]={};
        if(std::fread(value,1,slot.width,c.stream)!=slot.width)
            return fail("V2_TRUNCATED_FRAME"),DIVERGED;
        for(unsigned j=0;j<slot.width;++j)bytes.push_back(value[j]);
    }
    unsigned position=0;
    for(const Slot& slot:c.slots)
        for(unsigned j=0;j<slot.width;++j)slot.address[j]=bytes[position++];
    ++c.frames;++c.allFrames;
    if(c.allFrames==1u || c.allFrames%600u==0u)
        status("PLAY_PROGRESS frame="+std::to_string(c.allFrames)+
            " segment="+std::to_string(c.playIndex+1u)+"/"+
            std::to_string(c.segments.size()));
    return FRAME_READY;
}
inline unsigned fastFrames(){
    initialize();Context& c=context();
    if(c.mode!=PLAY || !c.fast)return 0u;
    const char* value=std::getenv("XM8764_FAST_FORWARD");
    if(!value||!*value)return 32u;
    const int n=std::atoi(value);
    if(n<=0)return 0u;
    return n>128?128u:(unsigned)n;
}
} // namespace XM8764
#endif
