#define NOMINMAX
#include <Windows.h>
#include <string>

namespace {
struct WindowSearch { DWORD process_id; HWND found=nullptr; };
BOOL CALLBACK find_process_window(HWND hwnd,LPARAM data) {
    auto* search=reinterpret_cast<WindowSearch*>(data);
    DWORD process_id=0; GetWindowThreadProcessId(hwnd,&process_id);
    if(process_id==search->process_id && IsWindowVisible(hwnd)) { search->found=hwnd; return FALSE; }
    return TRUE;
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=2) return 2;
    std::wstring command=L"\""+std::wstring(argv[1])+L"\"";
    STARTUPINFOW startup{}; startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    if(!CreateProcessW(argv[1],command.data(),nullptr,nullptr,FALSE,CREATE_UNICODE_ENVIRONMENT,nullptr,nullptr,&startup,&process)) return 3;
    CloseHandle(process.hThread);
    HWND window=nullptr;
    for(unsigned attempt=0;attempt<100 && !window;++attempt) {
        WindowSearch search{process.dwProcessId}; EnumWindows(find_process_window,reinterpret_cast<LPARAM>(&search));
        window=search.found;
        if(!window && WaitForSingleObject(process.hProcess,0)==WAIT_OBJECT_0) break;
        Sleep(50);
    }
    if(!window) { TerminateProcess(process.hProcess,4); WaitForSingleObject(process.hProcess,5000); CloseHandle(process.hProcess); return 4; }
    Sleep(500); // Ensure the GUI remains open beyond startup.
    if(WaitForSingleObject(process.hProcess,0)==WAIT_OBJECT_0) { CloseHandle(process.hProcess); return 5; }
    PostMessageW(window,WM_CLOSE,0,0);
    const DWORD result=WaitForSingleObject(process.hProcess,5000);
    if(result!=WAIT_OBJECT_0) TerminateProcess(process.hProcess,6);
    CloseHandle(process.hProcess);
    return result==WAIT_OBJECT_0?0:6;
}
