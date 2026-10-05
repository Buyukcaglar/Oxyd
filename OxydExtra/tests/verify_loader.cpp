#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int wmain(int argc, wchar_t **argv) {
    if (argc != 2) return 2;
    wchar_t command[2*MAX_PATH]; swprintf_s(command, L"\"%s\"", argv[1]);
    STARTUPINFOW startup = {}; startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(argv[1], command, nullptr, nullptr, FALSE,
        DEBUG_ONLY_THIS_PROCESS, nullptr, nullptr, &startup, &process)) {
        printf("CreateProcess failed: %lu\n", GetLastError()); return 1;
    }
    bool stoppedAtOriginalEntry = false, sawCompanion = false;
    LPVOID originalEntry = nullptr; unsigned char savedByte = 0; SIZE_T transferred;
    ULONGLONG deadline = GetTickCount64()+10000;
    while (GetTickCount64() < deadline) {
        DEBUG_EVENT event = {};
        if (!WaitForDebugEvent(&event, 1000)) continue;
        DWORD status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            unsigned char *base = (unsigned char*)event.u.CreateProcessInfo.lpBaseOfImage;
            originalEntry = base + 0xead8;
            ReadProcessMemory(process.hProcess, originalEntry, &savedByte, 1, &transferred);
            unsigned char breakpoint = 0xcc;
            if (!WriteProcessMemory(process.hProcess, originalEntry, &breakpoint, 1, &transferred)) break;
            FlushInstructionCache(process.hProcess, originalEntry, 1);
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            wchar_t path[MAX_PATH] = {};
            if (event.u.LoadDll.hFile) {
                GetFinalPathNameByHandleW(event.u.LoadDll.hFile, path, MAX_PATH, FILE_NAME_NORMALIZED);
                if (wcsstr(path, L"oxscale.dll")) {
                    sawCompanion = true; wprintf(L"Loaded companion: %s\n", path);
                }
                CloseHandle(event.u.LoadDll.hFile);
            }
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            auto &exception = event.u.Exception.ExceptionRecord;
            if (exception.ExceptionCode == EXCEPTION_BREAKPOINT && exception.ExceptionAddress == originalEntry) {
                stoppedAtOriginalEntry = true;
                printf("PASS: Windows loader and new bootstrap reached original VB6 entry point.\n");
                printf("Stopped before executing the original entry point; game and screen mode were not started.\n");
                TerminateProcess(process.hProcess, 0);
                ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
                break;
            }
            if (exception.ExceptionCode != EXCEPTION_BREAKPOINT) {
                printf("Loader exception: 0x%08lX\n", exception.ExceptionCode); status = DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            printf("Process exited before original entry: 0x%08lX\n", event.u.ExitProcess.dwExitCode);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status); break;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
    }
    if (!stoppedAtOriginalEntry) TerminateProcess(process.hProcess, 1);
    WaitForSingleObject(process.hProcess, 1000);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    return stoppedAtOriginalEntry && sawCompanion ? 0 : 1;
}
