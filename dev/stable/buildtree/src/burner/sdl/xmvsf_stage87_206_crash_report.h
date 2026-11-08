#ifndef XMVSF_STAGE87_206_CRASH_REPORT_H
#define XMVSF_STAGE87_206_CRASH_REPORT_H
// TEST-only Windows SEH crash diagnostics. Native code continues normally;
// on an unhandled exception, write the original exception address, last
// game/render checkpoint, and (when available) a minidump for a stack trace.
// No background recording, no network, no ROM or runtime alteration.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>

namespace XM87206Crash {
inline volatile LONG& phase(){static volatile LONG value=0;return value;}
inline volatile LONG& frame(){static volatile LONG value=0;return value;}
inline void mark(LONG step,LONG frameNumber=-1) {
    InterlockedExchange(&phase(),step);
    if(frameNumber>=0)InterlockedExchange(&frame(),frameNumber);
}
inline LONG WINAPI onUnhandled(EXCEPTION_POINTERS* error){
    CreateDirectoryA("test",nullptr);
    CreateDirectoryA("test\\87.228",nullptr);
    CreateDirectoryA("test\\87.228\\observer",nullptr);
    const DWORD code=error && error->ExceptionRecord?
        error->ExceptionRecord->ExceptionCode:0;
    const void* address=error && error->ExceptionRecord?
        error->ExceptionRecord->ExceptionAddress:nullptr;
    const unsigned long long offset=(unsigned long long)
        ((std::uintptr_t)address-(std::uintptr_t)GetModuleHandleA(nullptr));
    char report[1024]={};
    const int len=std::snprintf(report,sizeof(report),
        "XM87.206 UNHANDLED_EXCEPTION code=0x%08lX address=%p "
        "module_offset=0x%llX phase=%ld emulated_frame=%ld thread=%lu classic=%d\r\n",
        (unsigned long)code,address,offset,(long)phase(),(long)frame(),
        (unsigned long)GetCurrentThreadId(),
        !(std::getenv("XM8755_TOURNAMENT_MODE") &&
          std::strcmp(std::getenv("XM8755_TOURNAMENT_MODE"),"1")==0));
    HANDLE log=CreateFileA("test\\87.228\\observer\\crash_report.log",
           FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
           OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(log!=INVALID_HANDLE_VALUE){
        DWORD written=0;
        if(len>0)WriteFile(log,report,(DWORD)std::min(len,(int)sizeof(report)-1),
                          &written,nullptr);
        CloseHandle(log);
    }
    std::fprintf(stderr,"%s",report);std::fflush(stderr);
    // Optional runtime DLL: do not require DbgHelp.lib in the build.
    HMODULE dll=LoadLibraryA("dbghelp.dll");
    if(dll){
        typedef BOOL (WINAPI* DumpFn)(HANDLE,DWORD,HANDLE,MINIDUMP_TYPE,
            PMINIDUMP_EXCEPTION_INFORMATION,PMINIDUMP_USER_STREAM_INFORMATION,
            PMINIDUMP_CALLBACK_INFORMATION);
        const DumpFn dump=reinterpret_cast<DumpFn>(
            GetProcAddress(dll,"MiniDumpWriteDump"));
        if(dump){
            HANDLE file=CreateFileA("test\\87.228\\observer\\crash.dmp",GENERIC_WRITE,
                FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file!=INVALID_HANDLE_VALUE){
                MINIDUMP_EXCEPTION_INFORMATION info={};
                info.ThreadId=GetCurrentThreadId();
                info.ExceptionPointers=error;
                info.ClientPointers=FALSE;
                dump(GetCurrentProcess(),GetCurrentProcessId(),file,
                    MiniDumpNormal,&info,nullptr,nullptr);
                CloseHandle(file);
            }
        }
        FreeLibrary(dll);
    }
    // Preserve Windows Error Reporting's standard crash behavior/status.
    return EXCEPTION_CONTINUE_SEARCH;
}
inline void rearm(){
    // A native driver / SDL media reinitialization may replace the process
    // unhandled-exception filter; reclaim it on every TEST game entry.
    SetUnhandledExceptionFilter(onUnhandled);
}
inline void install(){
    static bool installed=false;
    if(installed)return;
    installed=true;
    rearm();
    std::fprintf(stderr,
        "XM87.228 CRASH_REPORT armed: test/87.228/observer/crash_report.log and crash.dmp\n");
}
} // namespace XM87206Crash
#else
namespace XM87206Crash {
inline void mark(int,int=-1){}
inline void rearm(){}
inline void install(){}
}
#endif
#endif
