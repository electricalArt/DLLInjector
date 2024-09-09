/*+===================================================================
  File:      DllInjector.exe

  Summary:   DllInjector is an application that searches for specified
             process and injects specified Dll file.

===================================================================+*/

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <easylogging++.h>>
#include <filesystem>

INITIALIZE_EASYLOGGINGPP

//  Configures default logger to output logs to stdout and to log file.
void ConfigureLogger()
{
    el::Configurations loggerConfigs;
    loggerConfigs.setToDefault();
    loggerConfigs.setGlobally(
        el::ConfigurationType::Filename,
        std::filesystem::temp_directory_path().string() + "\\DllInjector.log");
    el::Loggers::reconfigureAllLoggers(loggerConfigs);
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
    LOG_IF(hProcess == NULL, FATAL) << "Could not open specified process.";

    return hProcess;
}

//  Writes string that contains hack .dll path to the process. Stores written memory
//  address to `lpProcessSzcDllPath` variable.
LPVOID WriteHackDllToProcess(HANDLE hProcess, const wchar_t* szcDllPath) {
    LPVOID lpProcessSzcDllPath = VirtualAllocEx(
        hProcess,
        NULL,
        (wcslen(szcDllPath) + 1) * sizeof(wchar_t),
        MEM_COMMIT,
        PAGE_READWRITE );
    if (lpProcessSzcDllPath == NULL) {
        LOG(FATAL) << "VirtualAllocEx() failed to get DMA address";
    }
    SIZE_T cbWritten = 0;
    WriteProcessMemory(
        hProcess,
        lpProcessSzcDllPath,
        szcDllPath,
        (wcslen(szcDllPath) + 1) * sizeof(wchar_t),
        &cbWritten);
    LOG_IF(cbWritten == 0, FATAL) << ".dll file is not written inside process memory";
    LOG(INFO) << "cbWritten: " << cbWritten;

    return lpProcessSzcDllPath;
}

//  Creates remote thread in the process. This thread starts hack .dll.
HANDLE CreateHackThread(HANDLE hProcess, LPVOID lpProcessSzcDllPath) {
    HMODULE hKernel32base = GetModuleHandleA("kernel32.dll");
    if (hKernel32base == NULL) {
        LOG(FATAL) << "hKernel32base is NULL";
    }
    auto lpThreadStartAddress = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32base, "LoadLibraryW");
    LOG_IF(lpThreadStartAddress == NULL, FATAL) << "GetProcAddress() failed to find `LoadLibraryW()` address";
    DWORD wThreadId = 0;
    HANDLE hThread = CreateRemoteThread(
        hProcess,
        NULL,
        0,
        lpThreadStartAddress,
        lpProcessSzcDllPath,
        0,
        &wThreadId);
    LOG_IF(hThread == NULL, FATAL) << "hThread is NULL";
    LOG(INFO) << "wThreadId: " << wThreadId;

    return hThread;
}

//  Waits for the thread to finish
void WaitThread(HANDLE hThread) {
    DWORD wThreadExitCode = 0;
    LOG(INFO) << "Waiting for the library exit...";
    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, &wThreadExitCode);
    LOG(INFO) << "wThreadExitCode: " << wThreadExitCode;
}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: wmain()

  Args:     DllInjector.exe <ProcessName> <DllPath>
-----------------------------------------------------------------F-F*/
int wmain(int argc, wchar_t* argv[]) {
    ConfigureLogger();
    LOG_IF(argv[1] == NULL, FATAL) << "ProcessName is not specified";
    LOG_IF(argv[2] == NULL, FATAL) << "DllPath is not specified";

    const wchar_t* szcProcessName = argv[1];
    const wchar_t* szcDllPath = argv[2];
    LOG(INFO) << "<ProcessName>: " << szcProcessName;
    LOG(INFO) << "<DllPath>: " << szcDllPath;
    LOG_IF(std::filesystem::exists(szcDllPath) == false, FATAL) << "Specified <DllPath> is not exist.";

    std::wstring szcDllAbsolutePath = std::filesystem::absolute(szcDllPath).wstring();
    LOG(INFO) << "Absolute path: " << szcDllAbsolutePath;

    HANDLE hProcess = GetProcessHandle(szcProcessName);
    LPVOID lpProcessSzcDllPath = WriteHackDllToProcess(hProcess, szcDllAbsolutePath.c_str());
    // WriteHackDllToProcess() allocates memory withing the process

    HANDLE hThread = CreateHackThread(hProcess, lpProcessSzcDllPath);
    LOG_IF(hThread, INFO) << "The library succesfully injected.";
    WaitThread(hThread);

    // Free allocated memory within the process
    VirtualFreeEx(
        hProcess,
        lpProcessSzcDllPath,
        0,
        MEM_RELEASE);
    if (hThread)
        CloseHandle(hThread);
    if (hProcess)
        CloseHandle(hProcess);

    return 0;
}
