
#include "util.h"
#include "xbStructDef.h"
#include "curl\curl.h"

BOOL StringToWString(const std::string &str,std::wstring &wstr)
{    
	int nLen = (int)str.length();    
	wstr.resize(nLen,L' ');

	int nResult = MultiByteToWideChar(CP_ACP,0,(LPCSTR)str.c_str(),nLen,(LPWSTR)wstr.c_str(),nLen);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}
//wstring高字节不为0，返回FALSE
BOOL WStringToString(const std::wstring &wstr,std::string &str)
{    
	int nLen = (int)wstr.length();    
	str.resize(nLen,' ');

	int nResult = WideCharToMultiByte(CP_ACP,0,(LPCWSTR)wstr.c_str(),nLen,(LPSTR)str.c_str(),nLen,NULL,NULL);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}

// --------------------------------------
CurlInitialize::CurlInitialize(){
	m_bStatus = FALSE;
	if (::curl_global_init(CURL_GLOBAL_WIN32)==CURLE_OK)
		m_bStatus = TRUE;
}

CurlInitialize::~CurlInitialize(){
	::curl_global_cleanup();
	m_bStatus = FALSE;
}
BOOL CurlInitialize::GetStatus(){
	return m_bStatus;
}

DWORD WINAPI CommonThreadProc(LPVOID Parameter) {
	PThreadCtrl pThreadCtrl=(PThreadCtrl)Parameter;
	if (!Parameter)
		return 1;
	return pThreadCtrl->ThreadProc(pThreadCtrl);
}

PThreadCtrl CreateXbThread(PVOID Parameter,DWORD (*ThreadProc)(PThreadCtrl)){
	int i=0;
	DWORD dwThreadId=0;
	HANDLE hThread=INVALID_HANDLE_VALUE;
	HANDLE hPipe[2] ={INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE};
	HANDLE hEvent[2]={INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE};

	PThreadCtrl pThreadCtrl = new ThreadCtrl;
	if (!ThreadProc)
		return NULL;
	if (!CreatePipe(&hPipe[0],&hPipe[1],NULL,1024)) {
		return NULL;
	}
	for ( i=0; i<2; i++) {
		hEvent[i] = CreateEvent(NULL,TRUE,FALSE,NULL);
	}
	hThread=CreateThread(NULL,0,CommonThreadProc,(PVOID)pThreadCtrl,CREATE_SUSPENDED,&dwThreadId);
	if (hPipe[0]==INVALID_HANDLE_VALUE ||
		hPipe[1]==INVALID_HANDLE_VALUE ||
		hEvent[0]==INVALID_HANDLE_VALUE||
		hEvent[1]==INVALID_HANDLE_VALUE||
		hThread==INVALID_HANDLE_VALUE
		) {
			// Initial fail
			for (i=0; i<2; i++) {
				if (hPipe[i]!=INVALID_HANDLE_VALUE)
					CloseHandle(hPipe[i]);
				if (hEvent[i]!=INVALID_HANDLE_VALUE)
					CloseHandle(hEvent[i]);
			}
			if (hThread!=INVALID_HANDLE_VALUE) {
				TerminateThread(hThread,0);
				CloseHandle(hThread);
			}
			return NULL;
	}
	else {		
		pThreadCtrl->m_hThread  = hThread;
		for (i=0; i<2; i++) {
			pThreadCtrl->m_hEvent[i]= hEvent[i];
			pThreadCtrl->m_hPipe[i]= hPipe[i];
		}
		pThreadCtrl->m_Parameter = Parameter;
		pThreadCtrl->ThreadProc  = ThreadProc;
		pThreadCtrl->m_dwThreadId=dwThreadId;
		pThreadCtrl->m_wStatus = 0;
		return pThreadCtrl;
	}
}

BOOL ResumeXbThread(PThreadCtrl pThreadCtrl) {
	if (!pThreadCtrl) {
		return FALSE;
	}
	if (pThreadCtrl->m_wStatus==0) {
		pThreadCtrl->m_wStatus=1;
		ResumeThread(pThreadCtrl->m_hThread);
		return TRUE;
	}
	return FALSE;
}

VOID QuitXbThread(PThreadCtrl pThrdCtrl) {
	if (!pThrdCtrl)
		return ;
	SetEvent(pThrdCtrl->m_hEvent[0]);
	WaitForSingleObject(pThrdCtrl->m_hEvent[0],INFINITE);
}
VOID DestoryXbThread(PThreadCtrl pThrdCtrl) {
	if (!pThrdCtrl)
		return ;

	if (pThrdCtrl->m_hThread!=INVALID_HANDLE_VALUE) {
		CloseHandle(pThrdCtrl->m_hThread);
		pThrdCtrl->m_hThread = INVALID_HANDLE_VALUE;
	}
	for (int i=0; i<2; i++) {
		if (pThrdCtrl->m_hEvent[i]=INVALID_HANDLE_VALUE) {
			CloseHandle(pThrdCtrl->m_hEvent[i]);
			pThrdCtrl->m_hEvent[i] = INVALID_HANDLE_VALUE;
		}
		if (pThrdCtrl->m_hPipe[i]=INVALID_HANDLE_VALUE) {
			CloseHandle(pThrdCtrl->m_hPipe[i]);
			pThrdCtrl->m_hPipe[i] = INVALID_HANDLE_VALUE;
		}
	}
	delete pThrdCtrl;
}

int ScanLogicalDrive(char **disk) {
	int i=0,j=0;
	char *buf=NULL,*p=NULL;
	DWORD dwCount = GetLogicalDriveStringsA(0,buf);
	if (dwCount==0 )
		return 0;

	buf = new char[dwCount];
	GetLogicalDriveStringsA(dwCount,buf);

	for (i=0,j=0,p=buf; i<dwCount;i++) {
		if (buf[i]=='\0') {
			if (GetDriveTypeA(p)==DRIVE_FIXED) {
				(*disk)[j] = p[0];
				p = buf+i+1;
				j++;
			}
		}
	}
	delete []buf;
	return strlen(*disk);
}