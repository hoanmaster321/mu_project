#include "StdAfx.h"
#include "StackWalker.h"
#include <tchar.h>
#include <time.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")


static TCHAR s_szExceptionLogFileName[_MAX_PATH] = _T("\\exceptions.log");  // default
static BOOL s_bUnhandledExeptionFilterSet = FALSE;

// Log Set Internals
int LogMDay;
int LogMonth;
int LogMYear;
BOOL GetTime = 0;

class StackWalkerToConsole : public StackWalker
{
private:
	FILE* file;
	char szFile[256];
	time_t ltime;

public:
	StackWalkerToConsole() : file(nullptr)
	{
		CreateDirectoryA("STACK_ERROR", 0);
		time(&ltime);
		struct tm today;
		localtime_s(&today, &ltime);
		wsprintfA(szFile, "STACK_ERROR\\stack_%04d%02d%02d_%02d%02d%02d.log",
			today.tm_year + 1900, today.tm_mon + 1, today.tm_mday,
			today.tm_hour, today.tm_min, today.tm_sec);
		fopen_s(&file, szFile, "w");
	}

	~StackWalkerToConsole()
	{
		if (file)
		{
			fclose(file);
			file = nullptr;
		}
	}
	char* GetStackLogFileName()
	{
		return szFile;
	}
	FILE* GetFile() const
	{
		return file;
	}
protected:
	virtual void OnOutput(LPCTSTR szText)
	{
		if (file)
		{
			fprintf(file, "%s\n", szText);
			fflush(file);
		}
	}
};

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	return NULL;
}
//
//static BOOL PreventSetUnhandledExceptionFilter()
//{
//	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
//	if (hKernel32 == NULL)
//		return FALSE;
//	void* pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
//	if (pOrgEntry == NULL)
//		return FALSE;
//
//#ifdef _M_IX86
//	// Code for x86:
//	// 33 C0                xor         eax,eax
//	// C2 04 00             ret         4
//	unsigned char szExecute[] = { 0x33, 0xC0, 0xC2, 0x04, 0x00 };
//#elif _M_X64
//	// 33 C0                xor         eax,eax
//	// C3                   ret
//	unsigned char szExecute[] = { 0x33, 0xC0, 0xC3 };
//#else
//#error "The following code only works for x86 and x64!"
//#endif
//
//	DWORD dwOldProtect = 0;
//	BOOL  bProt = VirtualProtect(pOrgEntry, sizeof(szExecute), PAGE_EXECUTE_READWRITE, &dwOldProtect);
//
//	SIZE_T bytesWritten = 0;
//	BOOL   bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, szExecute, sizeof(szExecute),
//		&bytesWritten);
//
//	if ((bProt != FALSE) && (dwOldProtect != PAGE_EXECUTE_READWRITE))
//	{
//		DWORD dwBuf;
//		VirtualProtect(pOrgEntry, sizeof(szExecute), dwOldProtect, &dwBuf);
//	}
//	return bRet;
//}


BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 == NULL)
	{
		return FALSE;
	}

	void* pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if (pOrgEntry == NULL)
	{
		return FALSE;
	}

	DWORD dwOldProtect = 0;
	SIZE_T jmpSize = 5;
#ifdef _M_X64
	jmpSize = 13;
#endif
	BOOL bProt = VirtualProtect(pOrgEntry, jmpSize,
		PAGE_EXECUTE_READWRITE, &dwOldProtect);
	BYTE newJump[20];
	void* pNewFunc = &MyDummySetUnhandledExceptionFilter;
#ifdef _M_IX86
	DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
	dwOrgEntryAddr += jmpSize; // add 5 for 5 op-codes for jmp rel32
	DWORD dwNewEntryAddr = (DWORD)pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
	// JMP rel32: Jump near, relative, displacement relative to next instruction.
	newJump[0] = 0xE9;  // JMP rel32
	memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
#elif _M_X64
	newJump[0] = 0x49;  // MOV R15, ...
	newJump[1] = 0xBF;  // ...
	memcpy(&newJump[2], &pNewFunc, sizeof(pNewFunc));
	//pCur += sizeof (ULONG_PTR);
	newJump[10] = 0x41;  // JMP R15, ...
	newJump[11] = 0xFF;  // ...
	newJump[12] = 0xE7;  // ...
#endif
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, newJump, jmpSize, &bytesWritten);

	if (bProt != FALSE)
	{
		DWORD dwBuf;
		VirtualProtect(pOrgEntry, jmpSize, dwOldProtect, &dwBuf);
	}
	return bRet;
}

static LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS* pExPtrs)
{
#ifdef _M_IX86
	if (pExPtrs && pExPtrs->ExceptionRecord && pExPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		static char MyStack[1024 * 128];
		__asm mov eax, offset MyStack[1024 * 128];
		__asm mov esp, eax;
	}
#endif

	CreateDirectoryA("STACK_ERROR", NULL);

	SYSTEMTIME st;
	GetLocalTime(&st);
	char szDumpFile[MAX_PATH];
	sprintf_s(szDumpFile, sizeof(szDumpFile), "STACK_ERROR\\crash_%04d%02d%02d_%02d%02d%02d.dmp",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	HANDLE hDumpFile = CreateFileA(szDumpFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pExPtrs;
		mdei.ClientPointers = FALSE;

		MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
			MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpScanMemory |
			MiniDumpWithDataSegs
		);

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, dumpType, &mdei, NULL, NULL);
		CloseHandle(hDumpFile);
	}

	StackWalkerToConsole sw;
	FILE* fp = sw.GetFile();
	if (fp && pExPtrs && pExPtrs->ExceptionRecord)
	{
		fprintf(fp, "================================================================\n");
		fprintf(fp, "MU ONLINE CRASH REPORT\n");
		fprintf(fp, "Exception Code   : 0x%08X\n", pExPtrs->ExceptionRecord->ExceptionCode);
		fprintf(fp, "Exception Address: 0x%p\n", pExPtrs->ExceptionRecord->ExceptionAddress);
		fprintf(fp, "Exception Flags  : 0x%08X\n", pExPtrs->ExceptionRecord->ExceptionFlags);
		if (pExPtrs->ContextRecord)
		{
#ifdef _M_IX86
			fprintf(fp, "EAX: 0x%08X  EBX: 0x%08X  ECX: 0x%08X  EDX: 0x%08X\n",
				pExPtrs->ContextRecord->Eax, pExPtrs->ContextRecord->Ebx, pExPtrs->ContextRecord->Ecx, pExPtrs->ContextRecord->Edx);
			fprintf(fp, "ESI: 0x%08X  EDI: 0x%08X  EBP: 0x%08X  ESP: 0x%08X  EIP: 0x%08X\n",
				pExPtrs->ContextRecord->Esi, pExPtrs->ContextRecord->Edi, pExPtrs->ContextRecord->Ebp, pExPtrs->ContextRecord->Esp, pExPtrs->ContextRecord->Eip);
#endif
		}
		fprintf(fp, "Dump File        : %s\n", szDumpFile);
		fprintf(fp, "================================================================\n\n");
		fflush(fp);
	}

	if (pExPtrs)
	{
		sw.ShowCallstack(GetCurrentThread(), pExPtrs->ContextRecord);
	}

	char szNotice[512];
	sprintf_s(szNotice, sizeof(szNotice),
		"Game Da Bi Crash!\n\n"
		"Ma loi (Exception): 0x%08X\n"
		"Dia chi (Address): 0x%p\n\n"
		"File Minidump va Log da duoc luu tai:\n"
		"- %s\n"
		"- %s\n\n"
		"Vui long gui file trong thu muc STACK_ERROR.",
		pExPtrs && pExPtrs->ExceptionRecord ? pExPtrs->ExceptionRecord->ExceptionCode : 0,
		pExPtrs && pExPtrs->ExceptionRecord ? pExPtrs->ExceptionRecord->ExceptionAddress : 0,
		szDumpFile,
		sw.GetStackLogFileName());
	MessageBoxA(NULL, szNotice, "MU Crash Reporter", MB_OK | MB_ICONERROR);

	return EXCEPTION_EXECUTE_HANDLER;
}

static void InitUnhandledExceptionFilter()
{
	TCHAR szModName[_MAX_PATH];
	if (GetModuleFileName(NULL, szModName, sizeof(szModName) / sizeof(TCHAR)) != 0)
	{
		_tcscpy_s(s_szExceptionLogFileName, szModName);
		_tcscat_s(s_szExceptionLogFileName, _T(".exp.log"));
	}
	if (s_bUnhandledExeptionFilterSet == FALSE)
	{
		// set global exception handler (for handling all unhandled exceptions)
		SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
#if defined _M_X64 || defined _M_IX86
		PreventSetUnhandledExceptionFilter();
#endif
		s_bUnhandledExeptionFilterSet = TRUE;
	}
}

void StartStackLogging()
{
	InitUnhandledExceptionFilter();
}