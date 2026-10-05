#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <algorithm>

static HWND gameWindow;
static WNDPROC oldWindowProc;
static bool fullscreen;
static bool initialized;
static bool firstPresentation;
static RECT monitorBounds;
static LONG savedStyle, savedExStyle;
static HDC logicalDC;
static HBITMAP logicalBitmap;
static HGDIOBJ oldLogicalBitmap;

static void logLine(const wchar_t *format, ...) {
    wchar_t path[MAX_PATH], line[512];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash) return;
    wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"Oxyd-native-display.log");
    va_list args; va_start(args, format);
    _vsnwprintf_s(line, _countof(line), _TRUNCATE, format, args); va_end(args);
    FILE *file = nullptr;
    if (_wfopen_s(&file, path, L"a, ccs=UTF-8") == 0) {
        fwprintf(file, L"%s\n", line); fclose(file);
    }
}

static RECT scaledRect(int width, int height) {
    int scale = std::max(1, std::min(width / 640, height / 480));
    int x = (width - 640 * scale) / 2, y = (height - 480 * scale) / 2;
    return RECT{x, y, x + 640 * scale, y + 480 * scale};
}

static BOOL nearestBlt(HDC destination, const RECT &viewport, int scale,
    const POINT (&points)[2], HDC source, int sx, int sy, int sw, int sh, DWORD rop) {
    int saved = SaveDC(destination);
    SetMapMode(destination, MM_TEXT);
    SetWindowOrgEx(destination, 0, 0, nullptr);
    SetViewportOrgEx(destination, 0, 0, nullptr);
    SetStretchBltMode(destination, COLORONCOLOR);
    BOOL ok = StretchBlt(destination,
        viewport.left + points[0].x * scale, viewport.top + points[0].y * scale,
        (points[1].x - points[0].x) * scale, (points[1].y - points[0].y) * scale,
        source, sx, sy, sw, sh, rop);
    if (saved) RestoreDC(destination, saved);
    return ok;
}

static bool isGameDC(HDC dc) {
    return fullscreen && gameWindow && !IsIconic(gameWindow) && WindowFromDC(dc) == gameWindow;
}

static bool ensureLogicalFrame() {
    if (logicalDC) return true;
    BITMAPINFO info = {}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 640; info.bmiHeader.biHeight = -480;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
    void *bits = nullptr;
    logicalDC = CreateCompatibleDC(nullptr);
    logicalBitmap = CreateDIBSection(logicalDC, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!logicalDC || !logicalBitmap) {
        if (logicalBitmap) DeleteObject(logicalBitmap);
        if (logicalDC) DeleteDC(logicalDC);
        logicalBitmap = nullptr; logicalDC = nullptr; return false;
    }
    oldLogicalBitmap = SelectObject(logicalDC, logicalBitmap);
    memset(bits, 0, 640 * 480 * sizeof(uint32_t));
    return true;
}

static void clearLogicalFrame() {
    if (logicalDC) {
        SelectObject(logicalDC, oldLogicalBitmap);
        DeleteObject(logicalBitmap); DeleteDC(logicalDC);
    }
    logicalDC = nullptr; logicalBitmap = nullptr; oldLogicalBitmap = nullptr;
}

static BOOL presentFrame() {
    RECT client; GetClientRect(gameWindow, &client);
    RECT viewport = scaledRect(client.right, client.bottom);
    int scale = (viewport.right - viewport.left) / 640;
    HDC destination = GetDC(gameWindow);
    if (!destination) return FALSE;
    RECT bars[4] = {
        {0, 0, client.right, viewport.top},
        {0, viewport.bottom, client.right, client.bottom},
        {0, viewport.top, viewport.left, viewport.bottom},
        {viewport.right, viewport.top, client.right, viewport.bottom}
    };
    for (const RECT &bar : bars) FillRect(destination, &bar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    if (!firstPresentation) {
        firstPresentation = true;
        logLine(L"Present: desktop=%ldx%ld scale=%d viewport=(%ld,%ld)-(%ld,%ld); logical frame=640x480",
            client.right, client.bottom, scale, viewport.left, viewport.top,
            viewport.right, viewport.bottom);
    }
    POINT points[2] = {{0,0}, {640,480}};
    BOOL ok = nearestBlt(destination, viewport, scale, points, logicalDC, 0, 0, 640, 480, SRCCOPY);
    ReleaseDC(gameWindow, destination); return ok;
}

static bool logicalDestination(HDC dc, int &x, int &y, int &w, int &h) {
    if (!isGameDC(dc) || !ensureLogicalFrame()) return false;
    POINT points[2] = {{x,y}, {x+w,y+h}};
    if (!LPtoDP(dc, points, 2)) return false;
    x = points[0].x; y = points[0].y;
    w = points[1].x - x; h = points[1].y - y;
    return true;
}

static HDC logicalSource(HDC dc) {
    return isGameDC(dc) && ensureLogicalFrame() ? logicalDC : dc;
}

extern "C" BOOL WINAPI OxBitBlt(HDC dc, int x, int y, int w, int h, HDC src, int sx, int sy, DWORD rop) {
    src = logicalSource(src);
    if (!logicalDestination(dc, x, y, w, h)) return BitBlt(dc, x, y, w, h, src, sx, sy, rop);
    BOOL ok = BitBlt(logicalDC, x, y, w, h, src, sx, sy, rop);
    return ok ? presentFrame() : FALSE;
}

extern "C" BOOL WINAPI OxStretchBlt(HDC dc, int x, int y, int w, int h,
    HDC src, int sx, int sy, int sw, int sh, DWORD rop) {
    src = logicalSource(src);
    if (!logicalDestination(dc, x, y, w, h)) return StretchBlt(dc, x, y, w, h, src, sx, sy, sw, sh, rop);
    BOOL ok = StretchBlt(logicalDC, x, y, w, h, src, sx, sy, sw, sh, rop);
    return ok ? presentFrame() : FALSE;
}

extern "C" BOOL WINAPI OxAlphaBlend(HDC dc, int x, int y, int w, int h,
    HDC src, int sx, int sy, int sw, int sh, BLENDFUNCTION blend) {
    src = logicalSource(src);
    if (!logicalDestination(dc, x, y, w, h)) return AlphaBlend(dc, x, y, w, h, src, sx, sy, sw, sh, blend);
    // Preserve the game's alpha composition at 640x480 before scaling pixels.
    BOOL ok = AlphaBlend(logicalDC, x, y, w, h, src, sx, sy, sw, sh, blend);
    return ok ? presentFrame() : FALSE;
}

static int WINAPI OxStretchDIBits(HDC dc, int x, int y, int w, int h,
    int sx, int sy, int sw, int sh, const void *bits, const BITMAPINFO *info, UINT usage, DWORD rop) {
    if (!logicalDestination(dc, x, y, w, h))
        return StretchDIBits(dc, x, y, w, h, sx, sy, sw, sh, bits, info, usage, rop);
    int result = StretchDIBits(logicalDC, x, y, w, h, sx, sy, sw, sh, bits, info, usage, rop);
    if (result != GDI_ERROR) presentFrame();
    return result;
}

static BOOL WINAPI OxGetClientRect(HWND window, LPRECT rect) {
    if (fullscreen && window == gameWindow && !IsIconic(window)) {
        if (!rect) return FALSE;
        *rect = RECT{0, 0, 640, 480}; return TRUE;
    }
    return GetClientRect(window, rect);
}

static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    if (fullscreen) {
        if (message == WM_WINDOWPOSCHANGING && lp && !IsIconic(window)) {
            WINDOWPOS *pos = (WINDOWPOS*)lp;
            if (!(pos->flags & SWP_HIDEWINDOW) && pos->x > -30000 && pos->y > -30000) {
                pos->x = monitorBounds.left; pos->y = monitorBounds.top;
                pos->cx = monitorBounds.right - monitorBounds.left;
                pos->cy = monitorBounds.bottom - monitorBounds.top;
                pos->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
            }
        } else if (message == WM_STYLECHANGING && wp == GWL_STYLE) {
            STYLESTRUCT *style = (STYLESTRUCT*)lp;
            style->styleNew = (style->styleNew & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME)) | WS_POPUP;
        } else if (message == WM_SIZE && wp != SIZE_MINIMIZED) {
            lp = MAKELPARAM(640, 480);
        } else if (message == WM_ERASEBKGND) {
            RECT rect; GetClientRect(window, &rect);
            FillRect((HDC)wp, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH)); return 1;
        } else if (message == WM_DISPLAYCHANGE) {
            MONITORINFO info = {}; info.cbSize = sizeof(info);
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info)) {
                monitorBounds = info.rcMonitor;
                SetWindowPos(window, nullptr, monitorBounds.left, monitorBounds.top,
                    monitorBounds.right-monitorBounds.left, monitorBounds.bottom-monitorBounds.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                firstPresentation = false;
            }
        } else if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST &&
            message != WM_MOUSEWHEEL && message != WM_MOUSEHWHEEL) {
            RECT client; GetClientRect(window, &client);
            RECT viewport = scaledRect(client.right, client.bottom);
            int scale = (viewport.right - viewport.left) / 640;
            int x = std::clamp(((short)LOWORD(lp) - viewport.left) / scale, 0L, 639L);
            int y = std::clamp(((short)HIWORD(lp) - viewport.top) / scale, 0L, 479L);
            lp = MAKELPARAM(x, y);
        }
    }
    if (message == WM_NCDESTROY) {
        LRESULT result = CallWindowProcA(oldWindowProc, window, message, wp, lp);
        clearLogicalFrame();
        gameWindow = nullptr; oldWindowProc = nullptr; fullscreen = false; return result;
    }
    return CallWindowProcA(oldWindowProc, window, message, wp, lp);
}

static unsigned hookRuntimeImports() {
    unsigned changed = 0;
    uint8_t *base = (uint8_t*)GetModuleHandleW(L"msvbvm60.dll");
    if (!base) return 0;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32*)(base + dos->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    IMAGE_IMPORT_DESCRIPTOR *descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + rva);
    for (; descriptor->Name; ++descriptor) {
        if (!descriptor->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA32 *names = (IMAGE_THUNK_DATA32*)(base + descriptor->OriginalFirstThunk);
        IMAGE_THUNK_DATA32 *addresses = (IMAGE_THUNK_DATA32*)(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++addresses) {
            if (IMAGE_SNAP_BY_ORDINAL32(names->u1.Ordinal)) continue;
            const char *name = (const char*)((IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData))->Name;
            void *replacement = nullptr;
            if (strcmp(name, "BitBlt") == 0) replacement = (void*)OxBitBlt;
            else if (strcmp(name, "StretchBlt") == 0) replacement = (void*)OxStretchBlt;
            else if (strcmp(name, "StretchDIBits") == 0) replacement = (void*)OxStretchDIBits;
            else if (strcmp(name, "GetClientRect") == 0) replacement = (void*)OxGetClientRect;
            if (replacement) {
                DWORD previous;
                if (VirtualProtect(&addresses->u1.Function, sizeof(DWORD), PAGE_READWRITE, &previous)) {
                    addresses->u1.Function = (DWORD)(uintptr_t)replacement;
                    DWORD ignored; VirtualProtect(&addresses->u1.Function, sizeof(DWORD), previous, &ignored);
                    ++changed;
                }
            }
        }
    }
    return changed;
}

extern "C" void WINAPI OxScaleInit() {
    if (initialized) return;
    initialized = true;
    BOOL dpi = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    unsigned hooked = hookRuntimeImports();
    logLine(L"Init: x86 native desktop patch; DPI awareness=%d; runtime imports redirected=%u", dpi, hooked);
}

extern "C" HRESULT WINAPI OxEnterFullscreen(void*, HWND window, LONG) {
    if (!IsWindow(window)) return E_INVALIDARG;
    if (!gameWindow) {
        gameWindow = window;
        savedStyle = GetWindowLongA(window, GWL_STYLE);
        savedExStyle = GetWindowLongA(window, GWL_EXSTYLE);
        oldWindowProc = (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)windowProc);
        if (!oldWindowProc) { gameWindow = nullptr; return E_FAIL; }
    }
    MONITORINFO info = {}; info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info)) return E_FAIL;
    monitorBounds = info.rcMonitor;
    fullscreen = true; firstPresentation = false;
    LONG style = GetWindowLongA(window, GWL_STYLE);
    SetWindowLongA(window, GWL_STYLE, (style & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME)) | WS_POPUP);
    SetWindowLongA(window, GWL_EXSTYLE, savedExStyle & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME));
    BOOL ok = SetWindowPos(window, HWND_TOP, info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top,
        SWP_FRAMECHANGED | SWP_NOACTIVATE);
    logLine(L"Fullscreen: monitor=(%ld,%ld)-(%ld,%ld); native borderless resize=%d; no display-mode call",
        monitorBounds.left, monitorBounds.top, monitorBounds.right, monitorBounds.bottom, ok);
    return ok ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

extern "C" HRESULT WINAPI OxEnterWindowed(void*, HWND, LONG) {
    fullscreen = false;
    if (IsWindow(gameWindow)) {
        SetWindowLongA(gameWindow, GWL_STYLE, savedStyle);
        SetWindowLongA(gameWindow, GWL_EXSTYLE, savedExStyle);
        SetWindowPos(gameWindow, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    logLine(L"Windowed: original game window sizing restored; no display-mode call");
    return S_OK;
}

#ifdef OXSCALE_TEST
int main() {
    struct Case { int w, h, scale, x, y; } cases[] = {
        {640,480,1,0,0}, {1920,1080,2,320,60}, {2560,1600,3,320,80},
        {3840,2160,4,640,120}, {1280,1024,2,0,32}, {1366,768,1,363,144}
    };
    for (auto c : cases) {
        RECT v = scaledRect(c.w, c.h);
        if (v.left != c.x || v.top != c.y || v.right - v.left != 640*c.scale || v.bottom - v.top != 480*c.scale) return 1;
    }
    // Exercise the real GDI scaling path on memory DIBs, without opening a game.
    BITMAPINFO info = {}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 640; info.bmiHeader.biHeight = -480;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
    uint32_t *srcBits = nullptr, *dstBits = nullptr;
    HDC src = CreateCompatibleDC(nullptr), dst = CreateCompatibleDC(nullptr);
    HBITMAP srcBitmap = CreateDIBSection(src, &info, DIB_RGB_COLORS, (void**)&srcBits, nullptr, 0);
    info.bmiHeader.biWidth = 1920; info.bmiHeader.biHeight = -1440;
    HBITMAP dstBitmap = CreateDIBSection(dst, &info, DIB_RGB_COLORS, (void**)&dstBits, nullptr, 0);
    if (!srcBitmap || !dstBitmap) return 2;
    HGDIOBJ oldSrc = SelectObject(src, srcBitmap), oldDst = SelectObject(dst, dstBitmap);
    for (int y=0; y<480; ++y) for (int x=0; x<640; ++x) srcBits[y*640+x] = ((x*43)&255) | (((y*71)&255)<<8) | (((x^y)&255)<<16);
    POINT points[2] = {{0,0},{640,480}}; RECT viewport = {0,0,1920,1440};
    if (!nearestBlt(dst, viewport, 3, points, src, 0, 0, 640, 480, SRCCOPY)) return 3;
    GdiFlush();
    for (int y=0; y<1440; ++y) for (int x=0; x<1920; ++x)
        if ((dstBits[y*1920+x]&0xffffff) != srcBits[(y/3)*640+x/3]) return 4;
    SelectObject(src, oldSrc); SelectObject(dst, oldDst);
    DeleteObject(srcBitmap); DeleteObject(dstBitmap); DeleteDC(src); DeleteDC(dst);
    printf("PASS: 6 desktop geometries; all 2,764,800 output pixels preserve exact 3x nearest-neighbor colors.\n");
    return 0;
}
#endif
