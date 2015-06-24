
#pragma once
#include "global.h"
#include "xbStructDef.h"

BOOL StringToWString(const std::string &str,std::wstring &wstr);
BOOL WStringToString(const std::wstring &wstr,std::string &str);

class CurlInitialize
{
public:
	CurlInitialize();
	~CurlInitialize();
protected:
	BOOL m_bStatus;
public:
	BOOL GetStatus();
};
//==========================

typedef struct _tagThreadCtrl {
	HANDLE m_hThread;
	HANDLE m_hEvent[2];//0:quit;1:resolve;
	HANDLE m_hPipe[2]; //communication to thread;

	PVOID  m_Parameter;
	DWORD (*ThreadProc)(_tagThreadCtrl*);
	DWORD m_dwThreadId;
	WORD  m_wStatus;
}ThreadCtrl,*PThreadCtrl;


DWORD WINAPI CommonThreadProc(LPVOID);
PThreadCtrl CreateXbThread(PVOID,DWORD (*ThreadProc)(PThreadCtrl));
BOOL ResumeXbThread(PThreadCtrl);
VOID QuitXbThread(PThreadCtrl);
VOID DestoryXbThread(PThreadCtrl);
//==========================================
DWORD UpdateThreadProc(PThreadCtrl);
DWORD TaskThreadProc(PThreadCtrl);
//==========================================
int ScanLogicalDrive(char **disk);