#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>

int wmain(int argc, wchar_t **argv) {
    if (argc != 2) return 2;
    ITypeLib *lib = nullptr;
    HRESULT hr = LoadTypeLibEx(argv[1], REGKIND_NONE, &lib);
    wprintf(L"LoadTypeLibEx: 0x%08lX\n", (unsigned long)hr);
    if (FAILED(hr)) return 1;
    TLIBATTR *la = nullptr;
    lib->GetLibAttr(&la);
    wchar_t guid[40];
    StringFromGUID2(la->guid, guid, 40);
    wprintf(L"TypeLib %s version %u.%u\n", guid, la->wMajorVerNum, la->wMinorVerNum);
    lib->ReleaseTLibAttr(la);
    for (UINT i = 0; i < lib->GetTypeInfoCount(); ++i) {
        ITypeInfo *ti = nullptr; TYPEATTR *a = nullptr; BSTR name = nullptr;
        lib->GetTypeInfo(i, &ti); ti->GetTypeAttr(&a);
        ti->GetDocumentation(MEMBERID_NIL, &name, nullptr, nullptr, nullptr);
        if (a->typekind == TKIND_COCLASS || wcscmp(name, L"IDirectX7") == 0 ||
            wcscmp(name, L"DirectInput") == 0 || wcscmp(name, L"DirectInputDevice") == 0) {
            StringFromGUID2(a->guid, guid, 40);
            wprintf(L"%s GUID=%s kind=%d funcs=%u flags=%x\n", name, guid, a->typekind, a->cFuncs, a->wTypeFlags);
            for (UINT j = 0; j < a->cFuncs; ++j) {
                FUNCDESC *f = nullptr; BSTR fn = nullptr;
                ti->GetFuncDesc(j, &f); ti->GetDocumentation(f->memid, &fn, nullptr, nullptr, nullptr);
                wprintf(L"  %s id=%ld slot=%d params=%d return=%d\n", fn, f->memid, f->oVft / 4, f->cParams, f->elemdescFunc.tdesc.vt);
                SysFreeString(fn); ti->ReleaseFuncDesc(f);
            }
        }
        SysFreeString(name); ti->ReleaseTypeAttr(a); ti->Release();
    }
    lib->Release(); return 0;
}
