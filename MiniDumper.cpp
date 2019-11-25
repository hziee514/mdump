#include "StdAfx.h"
#include "MiniDumper.h"

#include <stdlib.h>
#include <tchar.h>

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess,
										 DWORD dwPid,
										 HANDLE hFile,
										 MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

MiniDumper gMiniDumper;

static BOOL g_bRestartApp = FALSE;

extern "C"
{
	void WINAPI SetRestart(BOOL bRestartApp)
	{
		g_bRestartApp = bRestartApp;
	}
};


TCHAR							MiniDumper::m_szAppName[MAX_PATH] = { 0 };
TCHAR							MiniDumper::m_szDmpName[MAX_PATH] = { 0 };
LPTOP_LEVEL_EXCEPTION_FILTER	MiniDumper::m_oldHandler = NULL;

MiniDumper::MiniDumper(void)
{
	::GetModuleFileName(NULL, m_szAppName, MAX_PATH);
	::GetModuleFileName(NULL, m_szDmpName, MAX_PATH);
	_tcscat(m_szDmpName, _T(".dmp"));
	m_oldHandler = SetUnhandledExceptionFilter(MiniDumpHandler);
}

MiniDumper::~MiniDumper(void)
{
	::SetUnhandledExceptionFilter(m_oldHandler);
}

LONG MiniDumper::MiniDumpHandler(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
	LONG nRet = EXCEPTION_CONTINUE_SEARCH;

	HMODULE hDbgHlp = ::LoadLibrary(_T("dbghelp.dll"));
	if (hDbgHlp != NULL)
	{
		MINIDUMPWRITEDUMP pfDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDbgHlp, "MiniDumpWriteDump" );
		if (pfDump != NULL)
		{
			HANDLE hFile = CreateFile(m_szDmpName,
				GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
		}
	}

	if (m_oldHandler != NULL)
	{
		return m_oldHandler(pExceptionInfo);
	}

	if (g_bRestartApp)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// Start the child process.
		CreateProcess(NULL, m_szAppName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return nRet;
}
