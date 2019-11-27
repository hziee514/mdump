#include "mdump.h"
#include <dbghelp.h>
#include <tchar.h>

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess,
                                         DWORD dwPid,
                                         HANDLE hFile,
                                         MINIDUMP_TYPE DumpType,
                                         CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                         CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                         CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

TCHAR dmpName[MAX_PATH] = {0};

LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS *pExceptionInfo);

/*void WINAPI setup(const LPCSTR dmpFile)
{

}*/

extern "C" BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            ::GetModuleFileName(hinstDLL, dmpName, MAX_PATH);
            _tcscat(dmpName, _T(".dmp"));
            ::SetUnhandledExceptionFilter(ExceptionHandler);
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    LONG nRet = EXCEPTION_CONTINUE_SEARCH;
    HMODULE hDbgHlp = ::LoadLibrary("dbghelp.dll");
    if (hDbgHlp == NULL)
    {
        return nRet;
    }
    MINIDUMPWRITEDUMP pfDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHlp, "MiniDumpWriteDump");
    if (pfDump == NULL)
    {
        return nRet;
    }
    HANDLE hFile = ::CreateFile(dmpName,
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExceptionInfo;
        ExInfo.ClientPointers = NULL;

        if (pfDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL))
        {
            nRet = EXCEPTION_EXECUTE_HANDLER;
        }

        ::CloseHandle(hFile);
    }
    return nRet;
}
