#pragma once

#include <Windows.h>
#include <dbghelp.h>

class MiniDumper
{
public:
	MiniDumper(void);
	virtual ~MiniDumper(void);

protected:
	static TCHAR							m_szDmpName[MAX_PATH];
	static TCHAR							m_szAppName[MAX_PATH];
	static LPTOP_LEVEL_EXCEPTION_FILTER		m_oldHandler;

protected:
	static LONG WINAPI MiniDumpHandler(struct _EXCEPTION_POINTERS *pExceptionInfo);
};
