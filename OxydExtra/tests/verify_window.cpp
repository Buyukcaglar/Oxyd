#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    HMODULE runtime = LoadLibraryW(L"msvbvm60.dll");
    HMODULE companion = LoadLibraryW(L"oxscale.dll");
    if (!runtime || !companion) return 1;
    auto init = (void(WINAPI*)())GetProcAddress(companion, "OxScaleInit");
    auto full = (HRESULT(WINAPI*)(void*,HWND,LONG))GetProcAddress(companion, "OxEnterFullscreen");
    auto windowed = (HRESULT(WINAPI*)(void*,HWND,LONG))GetProcAddress(companion, "OxEnterWindowed");
    auto blt = (BOOL(WINAPI*)(HDC,int,int,int,int,HDC,int,int,DWORD))GetProcAddress(companion, "BitBlt");
    if (!init || !full || !windowed || !blt) return 2;
    init();
    DEVMODEW before = {}; before.dmSize = sizeof(before);
    EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &before);
    WNDCLASSA cls = {}; cls.lpfnWndProc = DefWindowProcA; cls.hInstance = GetModuleHandleW(nullptr);
    cls.lpszClassName = "OxScaleHiddenVerification";
    if (!RegisterClassA(&cls)) return 3;
    HWND window = CreateWindowExA(0, cls.lpszClassName, "Hidden test", WS_OVERLAPPEDWINDOW,
        0, 0, 640, 480, nullptr, nullptr, cls.hInstance, nullptr);
    if (!window || FAILED(full(nullptr, window, 0x11))) return 4;
    RECT actual; GetClientRect(window, &actual);
    MONITORINFO monitor = {}; monitor.cbSize = sizeof(monitor);
    GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor);
    if (actual.right != monitor.rcMonitor.right-monitor.rcMonitor.left ||
        actual.bottom != monitor.rcMonitor.bottom-monitor.rcMonitor.top) return 5;
    if (IsWindowVisible(window)) return 6;
    auto *base = (uint8_t*)runtime;
    auto *nt = (IMAGE_NT_HEADERS32*)(base+((IMAGE_DOS_HEADER*)base)->e_lfanew);
    auto *imports = (IMAGE_IMPORT_DESCRIPTOR*)(base+nt->OptionalHeader.DataDirectory[1].VirtualAddress);
    bool virtualRectVerified = false;
    for (; imports->Name; ++imports) {
        if (!imports->OriginalFirstThunk) continue;
        auto *names=(IMAGE_THUNK_DATA32*)(base+imports->OriginalFirstThunk);
        auto *addresses=(IMAGE_THUNK_DATA32*)(base+imports->FirstThunk);
        for (;names->u1.AddressOfData;++names,++addresses) {
            if (IMAGE_SNAP_BY_ORDINAL32(names->u1.Ordinal)) continue;
            const char *name=(const char*)((IMAGE_IMPORT_BY_NAME*)(base+names->u1.AddressOfData))->Name;
            if (strcmp(name,"GetClientRect")==0) {
                auto getRect=(BOOL(WINAPI*)(HWND,LPRECT))(uintptr_t)addresses->u1.Function;
                RECT logical; getRect(window,&logical);
                virtualRectVerified = logical.right==640 && logical.bottom==480;
            }
        }
    }
    if (!virtualRectVerified) return 7;
    BITMAPINFO info = {}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=640; info.bmiHeader.biHeight=-480;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    uint32_t *sourceBits=nullptr,*copyBits=nullptr;
    HDC source=CreateCompatibleDC(nullptr), copy=CreateCompatibleDC(nullptr), screen=GetDC(window);
    HBITMAP sourceBitmap=CreateDIBSection(source,&info,DIB_RGB_COLORS,(void**)&sourceBits,nullptr,0);
    HBITMAP copyBitmap=CreateDIBSection(copy,&info,DIB_RGB_COLORS,(void**)&copyBits,nullptr,0);
    if (!sourceBitmap || !copyBitmap || !screen) return 8;
    HGDIOBJ oldSource=SelectObject(source,sourceBitmap),oldCopy=SelectObject(copy,copyBitmap);
    for (int i=0;i<640*480;++i) sourceBits[i]=(i*97)&0xffffff;
    // A hidden window cannot display pixels, but the game-sized framebuffer
    // and subsequent capture must still preserve every input pixel.
    blt(screen,0,0,640,480,source,0,0,SRCCOPY);
    if (!blt(copy,0,0,640,480,screen,0,0,SRCCOPY)) return 9;
    GdiFlush();
    for (int i=0;i<640*480;++i) if ((copyBits[i]&0xffffff)!=sourceBits[i]) return 10;
    ReleaseDC(window,screen); SelectObject(source,oldSource); SelectObject(copy,oldCopy);
    DeleteObject(sourceBitmap); DeleteObject(copyBitmap); DeleteDC(source); DeleteDC(copy);
    if (FAILED(windowed(nullptr,nullptr,8))) return 11;
    DestroyWindow(window);
    DEVMODEW after={}; after.dmSize=sizeof(after); EnumDisplaySettingsW(nullptr,ENUM_CURRENT_SETTINGS,&after);
    if (before.dmPelsWidth!=after.dmPelsWidth || before.dmPelsHeight!=after.dmPelsHeight ||
        before.dmBitsPerPel!=after.dmBitsPerPel || before.dmDisplayFrequency!=after.dmDisplayFrequency) return 12;
    printf("PASS: hidden native borderless window %ldx%ld; VB client 640x480; 307,200 framebuffer pixels; desktop mode unchanged.\n", actual.right,actual.bottom);
    return 0;
}
