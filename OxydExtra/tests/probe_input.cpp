#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>

template<class Fn> Fn method(IUnknown *object, unsigned slot) {
    return reinterpret_cast<Fn>((*reinterpret_cast<void***>(object))[slot]);
}
static bool report(const wchar_t *name, HRESULT hr) {
    wprintf(L"%s: 0x%08lX\n", name, (unsigned long)hr);
    return SUCCEEDED(hr);
}
int wmain(int argc, wchar_t **argv) {
    if (argc > 2) return 2;
    HANDLE context = INVALID_HANDLE_VALUE; ULONG_PTR cookie = 0;
    if (argc == 2) {
        ACTCTXW a = {}; a.cbSize = sizeof(a); a.lpSource = argv[1];
        context = CreateActCtxW(&a);
        if (context == INVALID_HANDLE_VALUE) {
            wprintf(L"CreateActCtx failed: %lu\n", GetLastError()); return 1;
        }
        if (!ActivateActCtx(context, &cookie)) {
            wprintf(L"ActivateActCtx failed: %lu\n", GetLastError()); ReleaseActCtx(context); return 1;
        }
        wprintf(L"Local manifest activated.\n");
    }
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 1;
    CLSID cls; IID iid;
    CLSIDFromString(L"{E1211353-8E94-11D1-8808-00C04FC2C602}", &cls);
    IIDFromString(L"{FAFA3599-8B72-11D2-90B2-00C04FC2C602}", &iid);
    IUnknown *dx = nullptr, *input = nullptr, *mouse = nullptr;
    int result = 1;
    hr = CoCreateInstance(cls, nullptr, CLSCTX_INPROC_SERVER, iid, (void**)&dx);
    if (report(L"DirectX7 CoCreateInstance", hr)) {
        typedef HRESULT (STDMETHODCALLTYPE *CreateInput)(IUnknown*, IUnknown**);
        hr = method<CreateInput>(dx, 10)(dx, &input);
        if (report(L"DirectInputCreate", hr)) {
            typedef HRESULT (STDMETHODCALLTYPE *CreateDevice)(IUnknown*, BSTR, IUnknown**);
            BSTR device = SysAllocString(L"GUID_SysMouse");
            hr = method<CreateDevice>(input, 5)(input, device, &mouse);
            SysFreeString(device);
            if (report(L"CreateDevice GUID_SysMouse", hr)) result = 0;
        }
        wchar_t modulePath[MAX_PATH] = {};
        HMODULE module = GetModuleHandleW(L"dx7vb.dll");
        if (module && GetModuleFileNameW(module, modulePath, MAX_PATH)) wprintf(L"Loaded DLL: %s\n", modulePath);
    }
    if (mouse) mouse->Release();
    if (input) input->Release();
    if (dx) dx->Release();
    CoUninitialize();
    if (cookie) DeactivateActCtx(0, cookie);
    if (context != INVALID_HANDLE_VALUE) ReleaseActCtx(context);
    return result;
}
