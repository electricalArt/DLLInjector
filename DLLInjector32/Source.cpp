#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

constexpr size_t PROCESS_NAME_LENGTH_MAX = 500;

void ExitError(const char* szcMsg) {
    puts(szcMsg);
    getchar();
    exit(1);
}

//  Finds the process. Returns found process handle.
HANDLE GetProcessHandle(const wchar_t* szcProcessName) {
    DWORD wProcessId = 0;
    HANDLE hSnapshot = 0;
    PROCESSENTRY32 processEntry = { 0 };

    processEntry.dwSize = sizeof(PROCESSENTRY32);
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    Process32First(hSnapshot, &processEntry);
    do {
        if (wcscmp(processEntry.szExeFile, szcProcessName) == 0) {
            wProcessId = processEntry.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &processEntry));

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, true, wProcessId);

    return hProcess;
}

//  Writes string that contains hack .dll path to the process. Stores written memory
//  address to `lpProcessSzcDLLPath` variable.
LPVOID WriteHackDLLToProcess(HANDLE hProcess, const wchar_t* szcDLLPath) {
    LPVOID lpProcessSzcDLLPath = VirtualAllocEx(
        hProcess,
        NULL,
        (wcslen(szcDLLPath) + 1) * sizeof(wchar_t),
        MEM_COMMIT,
        PAGE_READWRITE );
    if (lpProcessSzcDLLPath == NULL) {
        ExitError("ERROR: lpProcessSzcDLLPath is NULL");
    }
    SIZE_T cbWritten = 0;
    WriteProcessMemory(
        hProcess,
        lpProcessSzcDLLPath,
        szcDLLPath,
        (wcslen(szcDLLPath) + 1) * sizeof(wchar_t),
        &cbWritten);

    return lpProcessSzcDLLPath;
}

//  Creates remote thread in the process. This thread starts hack .dll.
HANDLE CreateHackThread(HANDLE hProcess, LPVOID lpProcessSzcDLLPath) {
    HMODULE hKernel32base = GetModuleHandleA("kernel32.dll");
    if (hKernel32base == NULL) {
        ExitError("hKernel32base is NULL");
    }
    auto lpThreadStartAddress = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32base, "LoadLibraryW");
    DWORD wThreadId = 0;
    HANDLE hThread = CreateRemoteThread(
        hProcess,
        NULL,
        0,
        lpThreadStartAddress,
        lpProcessSzcDLLPath,
        0,
        &wThreadId);
    if (hThread == NULL) {
        ExitError("ERROR: hThread is NULL");
    }
    printf("wThreadId: %d\n", wThreadId);

    return hThread;
}

//  Waits for the thread to finish
void WaitThread(HANDLE hThread) {
    DWORD wThreadExitCode = 0;
    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, &wThreadExitCode);
    printf("wThreadExitCode: %d\n", wThreadExitCode);
    if (wThreadExitCode == 0) {
        printf("THREAD ERROR: %d\n", GetLastError());
    }
}

int wmain(int argc, wchar_t* argv[]) {
    if (argv[1] == NULL) {
        ExitError("ERROR: ProcessName is not specified");
    }
    if (argv[2] == NULL) {
        ExitError("ERROR: DLLPath is not specified");
    }

    const wchar_t* szcProcessName = argv[1];
    const wchar_t* szcDLLPath = argv[2];

    HANDLE hProcess = GetProcessHandle(szcProcessName);
    LPVOID lpProcessSzcDLLPath = WriteHackDLLToProcess(hProcess, szcDLLPath);
    HANDLE hThread = CreateHackThread(hProcess, lpProcessSzcDLLPath);
    WaitThread(hThread);

    VirtualFreeEx(
        hProcess,
        lpProcessSzcDLLPath,
        0,
        MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    getchar();

    return 0;
}
