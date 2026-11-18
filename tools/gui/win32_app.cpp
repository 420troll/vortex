#define NOMINMAX
#include "audio/audio_session.h"
#include "audio/winmm_output.h"
#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
constexpr wchar_t kClassName[]=L"TROLLfxVortexLabWindow";
constexpr int kOpen=100,kPlay=101,kPause=102,kStop=103,kLoop=104,kExport=105;
constexpr int kMorph=200,kTime=201,kWet=202,kInput=203,kOutput=204;
constexpr int kProgressLeft=300,kProgressRight=301;
constexpr UINT kExportFinished=WM_APP+1;

vortex::AudioSession g_session;
vortex::WinMmOutput g_output_device(g_session);
HWND g_window=nullptr,g_status=nullptr,g_file=nullptr,g_position=nullptr,g_morph_value=nullptr;
HWND g_time_value=nullptr,g_wet_value=nullptr,g_input_value=nullptr,g_output_value=nullptr;
HWND g_morph_slider=nullptr,g_time_slider=nullptr,g_wet_slider=nullptr,g_input_slider=nullptr,g_output_slider=nullptr;
HWND g_left_meter=nullptr,g_right_meter=nullptr,g_loop_box=nullptr;
bool g_loaded=false,g_morph_dirty=false,g_time_dirty=false,g_wet_dirty=false,g_input_dirty=false,g_output_dirty=false;
bool g_loop=true,g_exporting=false;
float g_morph=0.5f,g_morph_time=25.0f,g_wet=0.35f,g_input=1.0f,g_output=1.0f;
float g_meter_left=0.0f,g_meter_right=0.0f;
std::thread g_export_thread;
std::mutex g_export_mutex;
std::wstring g_export_message;
bool g_export_success=false;

HWND control(HWND parent,const wchar_t* cls,const wchar_t* title,DWORD style,int x,int y,int w,int h,int id=0) {
    return CreateWindowExW(0,cls,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
}
void label(HWND parent,const wchar_t* text,int x,int y,int w,int h) {
    control(parent,L"STATIC",text,SS_LEFT,x,y,w,h);
}
HWND slider(HWND parent,int id,int minimum,int maximum,int x,int y,int w) {
    HWND h=control(parent,TRACKBAR_CLASSW,L"",TBS_AUTOTICKS|TBS_HORZ,x,y,w,34,id);
    SendMessageW(h,TBM_SETRANGE,TRUE,MAKELONG(minimum,maximum));
    SendMessageW(h,TBM_SETPOS,TRUE,minimum);
    SendMessageW(h,TBM_SETLINESIZE,0,1);
    SendMessageW(h,TBM_SETPAGESIZE,0,10);
    return h;
}
std::wstring utf8_to_wide(const std::string& value) {
    if(value.empty()) return {};
    const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.c_str(),-1,nullptr,0);
    if(!count) throw std::runtime_error("invalid UTF-8 path");
    std::wstring result(static_cast<std::size_t>(count),L'\0');
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.c_str(),-1,result.data(),count);
    result.pop_back(); return result;
}
std::string wide_to_utf8(const std::wstring& value) {
    const int count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.c_str(),-1,nullptr,0,nullptr,nullptr);
    if(!count) throw std::runtime_error("cannot convert file path to UTF-8");
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.c_str(),-1,result.data(),count,nullptr,nullptr);
    result.pop_back(); return result;
}
void set_status(const std::wstring& text) { if(g_status) SetWindowTextW(g_status,text.c_str()); }
bool post(vortex::AudioCommandType type,float value) {
    if(g_session.post({type,value})) return true;
    set_status(L"Audio command queue is busy; the latest control value will be retried.");
    return false;
}
void flush_controls() {
    if(g_morph_dirty && post(vortex::AudioCommandType::SetMorph,g_morph)) g_morph_dirty=false;
    if(g_time_dirty && post(vortex::AudioCommandType::SetMorphTime,g_morph_time)) g_time_dirty=false;
    if(g_wet_dirty && post(vortex::AudioCommandType::SetWet,g_wet)) g_wet_dirty=false;
    if(g_input_dirty && post(vortex::AudioCommandType::SetInputGain,g_input)) g_input_dirty=false;
    if(g_output_dirty && post(vortex::AudioCommandType::SetOutputGain,g_output)) g_output_dirty=false;
}
void update_control_text() {
    wchar_t text[96];
    swprintf_s(text,L"%0.1f%%",g_morph*100); SetWindowTextW(g_morph_value,text);
    swprintf_s(text,L"%0.0f ms",g_morph_time); SetWindowTextW(g_time_value,text);
    swprintf_s(text,L"%0.0f%% wet",g_wet*100); SetWindowTextW(g_wet_value,text);
    swprintf_s(text,L"%0.0f%%",g_input*100); SetWindowTextW(g_input_value,text);
    swprintf_s(text,L"%0.0f%%",g_output*100); SetWindowTextW(g_output_value,text);
}
void set_slider(HWND hwnd,float& value,bool& dirty,float scale,vortex::AudioCommandType type) {
    const int pos=static_cast<int>(SendMessageW(hwnd,TBM_GETPOS,0,0));
    value=float(pos)/scale;
    dirty=!post(type,value);
    update_control_text();
}
void choose_file() {
    wchar_t path[32768]{};
    OPENFILENAMEW dialog{}; dialog.lStructSize=sizeof(dialog); dialog.hwndOwner=g_window;
    dialog.lpstrFilter=L"WAV audio (*.wav)\0*.wav\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile=path; dialog.nMaxFile=static_cast<DWORD>(std::size(path));
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
    dialog.lpstrDefExt=L"wav";
    if(!GetOpenFileNameW(&dialog)) return;
    try {
        set_status(L"Loading WAV...");
        auto audio=vortex::read_wav(wide_to_utf8(path));
        vortex::validate_realtime_audio(audio);
        g_output_device.shutdown();
        g_session.set_source(std::move(audio));
        g_loaded=true;
        SetWindowTextW(g_file,path);
        SendMessageW(g_morph_slider,TBM_SETPOS,TRUE,500);
        g_morph=0.5f; g_morph_dirty=true; flush_controls();
        set_status(L"Ready — press Play.");
    } catch(const std::exception& error) {
        const auto message=utf8_to_wide(error.what());
        set_status(L"Could not open WAV.");
        MessageBoxW(g_window,message.c_str(),L"TROLLfx Vortex Lab",MB_OK|MB_ICONERROR);
    }
}
void export_file() {
    if(!g_loaded || g_exporting) return;
    wchar_t path[32768]{};
    OPENFILENAMEW dialog{}; dialog.lStructSize=sizeof(dialog); dialog.hwndOwner=g_window;
    dialog.lpstrFilter=L"WAV audio (*.wav)\0*.wav\0\0"; dialog.lpstrFile=path;
    dialog.nMaxFile=static_cast<DWORD>(std::size(path)); dialog.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt=L"wav";
    if(!GetSaveFileNameW(&dialog)) return;
    const auto filename=wide_to_utf8(path);
    const vortex::MorphParams params{g_morph,g_morph_time,g_wet};
    const float input=g_input,output=g_output;
    const auto* source=&g_session.source();
    g_exporting=true; EnableWindow(GetDlgItem(g_window,kOpen),FALSE); EnableWindow(GetDlgItem(g_window,kExport),FALSE);
    set_status(L"Exporting WAV in background...");
    try {
        g_export_thread=std::thread([source,filename,params,input,output]() {
            bool success=false; std::wstring message;
            try {
                const auto rendered=vortex::render_offline(*source,params,input,output);
                vortex::write_wav(filename,rendered);
                success=true; message=L"WAV export complete.";
            } catch(const std::exception& error) {
                try { message=L"Export failed: "+utf8_to_wide(error.what()); }
                catch(...) { message=L"WAV export failed."; }
            }
            { std::lock_guard lock(g_export_mutex); g_export_success=success; g_export_message=std::move(message); }
            PostMessageW(g_window,kExportFinished,0,0);
        });
    } catch(const std::exception&) {
        g_exporting=false; EnableWindow(GetDlgItem(g_window,kOpen),TRUE); EnableWindow(GetDlgItem(g_window,kExport),TRUE);
        set_status(L"Could not start the WAV export worker.");
    }
}
void refresh_status() {
    if(g_output_device.failed()) {
        g_output_device.shutdown();
        const auto error=utf8_to_wide(g_output_device.error_message());
        set_status(L"Audio output stopped: "+error);
    }
    if(!g_loaded) return;
    const auto state=g_session.state();
    const auto frames=g_session.position_frames(),total=g_session.total_frames();
    const auto rate=g_session.source().sample_rate;
    const auto seconds=rate?frames/rate:0,total_seconds=rate?total/rate:0;
    wchar_t text[160];
    const wchar_t* state_text=state==vortex::TransportState::Playing?L"Playing":state==vortex::TransportState::Paused?L"Paused":L"Stopped";
    swprintf_s(text,L"%s — %02llu:%02llu / %02llu:%02llu",state_text,
               static_cast<unsigned long long>(seconds/60),static_cast<unsigned long long>(seconds%60),
               static_cast<unsigned long long>(total_seconds/60),static_cast<unsigned long long>(total_seconds%60));
    SetWindowTextW(g_position,text);
    g_meter_left=std::max(g_session.peak_left(),g_meter_left*0.88f);
    g_meter_right=std::max(g_session.peak_right(),g_meter_right*0.88f);
    SendMessageW(g_left_meter,PBM_SETPOS,WPARAM(std::clamp(int(g_meter_left*100),0,100)),0);
    SendMessageW(g_right_meter,PBM_SETPOS,WPARAM(std::clamp(int(g_meter_right*100),0,100)),0);
}
void create_controls(HWND hwnd) {
    control(hwnd,L"BUTTON",L"Open WAV...",BS_PUSHBUTTON,24,22,120,34,kOpen);
    control(hwnd,L"BUTTON",L"Play",BS_PUSHBUTTON,160,22,80,34,kPlay);
    control(hwnd,L"BUTTON",L"Pause",BS_PUSHBUTTON,248,22,80,34,kPause);
    control(hwnd,L"BUTTON",L"Stop",BS_PUSHBUTTON,336,22,80,34,kStop);
    g_loop_box=control(hwnd,L"BUTTON",L"Loop",BS_AUTOCHECKBOX,438,27,80,26,kLoop);
    SendMessageW(g_loop_box,BM_SETCHECK,BST_CHECKED,0);
    control(hwnd,L"BUTTON",L"Export WAV...",BS_PUSHBUTTON,810,22,140,34,kExport);
    g_file=control(hwnd,L"STATIC",L"No file loaded",SS_LEFT|SS_PATHELLIPSIS,24,68,926,25);
    label(hwnd,L"PLAYBACK",24,106,130,24);
    g_position=control(hwnd,L"STATIC",L"Stopped — 00:00 / 00:00",SS_LEFT,160,106,500,24);
    label(hwnd,L"A",24,154,40,25); label(hwnd,L"MORPH",475,145,180,35); label(hwnd,L"B",925,154,40,25);
    g_morph_slider=slider(hwnd,kMorph,0,1000,46,174,884); SendMessageW(g_morph_slider,TBM_SETPOS,TRUE,500);
    g_morph_value=control(hwnd,L"STATIC",L"50.0%",SS_CENTER,415,211,200,34);
    label(hwnd,L"MORPH TRANSITION",24,260,210,24); g_time_value=control(hwnd,L"STATIC",L"25 ms",SS_RIGHT,830,260,120,24);
    g_time_slider=slider(hwnd,kTime,1,5000,24,282,926); SendMessageW(g_time_slider,TBM_SETPOS,TRUE,25);
    label(hwnd,L"WET / DRY",24,328,150,24); g_wet_value=control(hwnd,L"STATIC",L"35% wet",SS_RIGHT,830,328,120,24);
    g_wet_slider=slider(hwnd,kWet,0,100,24,350,926); SendMessageW(g_wet_slider,TBM_SETPOS,TRUE,35);
    label(hwnd,L"INPUT LEVEL",24,396,150,24); g_input_value=control(hwnd,L"STATIC",L"100%",SS_RIGHT,830,396,120,24);
    g_input_slider=slider(hwnd,kInput,0,200,24,418,926); SendMessageW(g_input_slider,TBM_SETPOS,TRUE,100);
    label(hwnd,L"OUTPUT LEVEL",24,464,150,24); g_output_value=control(hwnd,L"STATIC",L"100%",SS_RIGHT,830,464,120,24);
    g_output_slider=slider(hwnd,kOutput,0,200,24,486,926); SendMessageW(g_output_slider,TBM_SETPOS,TRUE,100);
    label(hwnd,L"LEFT",24,536,55,24); g_left_meter=control(hwnd,PROGRESS_CLASSW,L"",PBS_SMOOTH,82,536,868,22,kProgressLeft);
    label(hwnd,L"RIGHT",24,568,55,24); g_right_meter=control(hwnd,PROGRESS_CLASSW,L"",PBS_SMOOTH,82,568,868,22,kProgressRight);
    SendMessageW(g_left_meter,PBM_SETRANGE32,0,100); SendMessageW(g_right_meter,PBM_SETRANGE32,0,100);
    g_status=control(hwnd,L"STATIC",L"Open a stereo WAV file to begin.",SS_LEFT,24,606,926,26);
    update_control_text();
}
LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam) {
    switch(message) {
    case WM_CREATE:
        g_window=hwnd; create_controls(hwnd); SetTimer(hwnd,1,30,nullptr); return 0;
    case WM_COMMAND:
        switch(LOWORD(wparam)) {
        case kOpen: choose_file(); return 0;
        case kPlay:
            if(!g_loaded) { set_status(L"Open a stereo WAV file first."); return 0; }
            if(!g_output_device.running() && !g_output_device.start()) { const auto error=utf8_to_wide(g_output_device.error_message()); set_status(error); MessageBoxW(hwnd,error.c_str(),L"Audio output error",MB_OK|MB_ICONERROR); return 0; }
            post(vortex::AudioCommandType::Play,1); return 0;
        case kPause: post(vortex::AudioCommandType::Pause,0); return 0;
        case kStop: post(vortex::AudioCommandType::Stop,0); return 0;
        case kExport: export_file(); return 0;
        case kLoop:
            g_loop=SendMessageW(g_loop_box,BM_GETCHECK,0,0)==BST_CHECKED;
            post(vortex::AudioCommandType::SetLoop,g_loop?1.0f:0.0f); return 0;
        }
        break;
    case WM_HSCROLL: {
        const HWND source=reinterpret_cast<HWND>(lparam);
        if(source==g_morph_slider) set_slider(source,g_morph,g_morph_dirty,1000.0f,vortex::AudioCommandType::SetMorph);
        else if(source==g_time_slider) set_slider(source,g_morph_time,g_time_dirty,1.0f,vortex::AudioCommandType::SetMorphTime);
        else if(source==g_wet_slider) set_slider(source,g_wet,g_wet_dirty,100.0f,vortex::AudioCommandType::SetWet);
        else if(source==g_input_slider) set_slider(source,g_input,g_input_dirty,100.0f,vortex::AudioCommandType::SetInputGain);
        else if(source==g_output_slider) set_slider(source,g_output,g_output_dirty,100.0f,vortex::AudioCommandType::SetOutputGain);
        return 0;
    }
    case WM_TIMER: flush_controls(); refresh_status(); return 0;
    case kExportFinished: {
        if(g_export_thread.joinable()) g_export_thread.join();
        std::wstring message; bool success;
        { std::lock_guard lock(g_export_mutex); message=g_export_message; success=g_export_success; }
        g_exporting=false; EnableWindow(GetDlgItem(hwnd,kOpen),TRUE); EnableWindow(GetDlgItem(hwnd,kExport),TRUE);
        set_status(message);
        if(!success) MessageBoxW(hwnd,message.c_str(),L"WAV export",MB_OK|MB_ICONERROR);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd,1); g_session.post({vortex::AudioCommandType::Stop,0}); g_output_device.shutdown();
        if(g_export_thread.joinable()) g_export_thread.join();
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,message,wparam,lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE, PWSTR,int show) {
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_BAR_CLASSES}; InitCommonControlsEx(&controls);
    WNDCLASSEXW wc{sizeof(wc)}; wc.lpfnWndProc=window_proc; wc.hInstance=instance;
    wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1); wc.lpszClassName=kClassName;
    if(!RegisterClassExW(&wc)) return 1;
    HWND hwnd=CreateWindowExW(0,kClassName,L"TROLLfx Vortex Lab",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,1000,700,nullptr,nullptr,instance,nullptr);
    if(!hwnd) return 1;
    ShowWindow(hwnd,show); UpdateWindow(hwnd);
    MSG message{}; while(GetMessageW(&message,nullptr,0,0)>0) { TranslateMessage(&message); DispatchMessageW(&message); }
    return static_cast<int>(message.wParam);
}
