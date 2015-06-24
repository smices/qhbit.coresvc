
#include "global.h"
#include <iostream>

#include "xldl.h"
#include "DownWrapper.h"

#include "xbStructDef.h"
#include "util.h"
#include "InitConf.h"

#include "curl\curl.h"

DownWrapper* g_pWapper = NULL;

DownWrapper* LoadDll();
void UnloadDll(DownWrapper** pWapper);
void PrintTaskInfo(DownTaskInfo &info);

int GlobalInitialize(CurlInitialize &curl){
	// check and init our dir
	InitDir();

	// Initialize curl
	if (!curl.GetStatus())
		return 1;
	// Load download engine
	g_pWapper = LoadDll();

	if (!g_pWapper)
		return 2;

	// Initialize download engine
	if (!g_pWapper->Init()) {
		UnloadDll(&g_pWapper);
		return 3;
	}
	g_pWapper->LimitUploadSpeed(1024,30);
	return 0;
}

/*
1.main_thread:get ctrl code
2.update_thread:download pkg
3.task_thread:upload task
*/
int _tmain(int argc, _TCHAR* argv[]) {
	CurlInitialize curl;
	if (GlobalInitialize(curl)!=0)
		return 1;
	//start business thread
	PThreadCtrl pUpdateThrdCtrl=CreateXbThread((PVOID)NULL,UpdateThreadProc);
	PThreadCtrl pShardTaskThrdCtrl=CreateXbThread((PVOID)NULL,TaskThreadProc);
	if (pUpdateThrdCtrl && pShardTaskThrdCtrl) {
		//ResumeXbThread(pUpdateThrdCtrl);
		ResumeXbThread(pShardTaskThrdCtrl);
		while(true) {
			//input ?
			char cmd = std::cin.peek();
			if (cmd=='q') {
				QuitXbThread(pShardTaskThrdCtrl);
				break;
			}
			//-->send command to thread
			//monitor thread?
			Sleep(5000);
		}
	}
	if (pUpdateThrdCtrl) {
		DestoryXbThread(pUpdateThrdCtrl);
	}
	if (pShardTaskThrdCtrl) {
		DestoryXbThread(pShardTaskThrdCtrl);
	}
	std::wcout << "waiting quit" << std::endl;
	Sleep(5000);

	// Uninitialize Engine
	UnloadDll(&g_pWapper);
	return 0;
}

// Load xldl.dll
DownWrapper* LoadDll()
{
	WCHAR szModulePath[MAX_PATH] = {0};
	::GetModuleFileName(NULL, szModulePath, MAX_PATH);
	::PathRemoveFileSpecW(szModulePath);

	WCHAR szDllpath[MAX_PATH] = {0};
	::PathCombine(szDllpath, szModulePath, L"xldl.dll");
	OutputDebugString(szDllpath);

	DownWrapper* pWapper = new DownWrapper(szDllpath);
	return pWapper;
}

void UnloadDll(DownWrapper** Wapper){
	if (!Wapper)
		return ;
	if ((*Wapper)!=NULL)
	{
		(*Wapper)->UnInit();
		delete (*Wapper);
		(*Wapper) = NULL;
	}
}

// Print task info
void PrintTaskInfo(DownTaskInfo &info)
{
	using namespace std;
	std::wstring stat(L"unknown");
	switch(info.stat)
	{
	case NOITEM: stat = L"NOITEM";
		break;
	case TSC_ERROR: stat = L"ERROR";
		break;
	case TSC_PAUSE: stat = L"PAUSE";
		break;
	case TSC_DOWNLOAD: stat = L"DOWNLOAD";
		break;
	case TSC_COMPLETE: stat = L"COMPLETE";
		break;
	case TSC_STARTPENDING: stat = L"STARTPENDING";
		break;
	case TSC_STOPPENDING : stat = L"STOPPENDING";
		break;
	}

	static bool bTitle = true;
	if (bTitle)
	{
		std::wcout << endl << setiosflags(ios::left) <<setw(20)<< "Filename" << setw(20) << "Status" << setw(20) << "Progress " 
			<< setw(15) << "File Size (B)" << setw(15) << "Download Size (B)" << endl; 
		bTitle = false;
	}
	wcout <<setw(20)<< info.szFilename << setw(20) << stat 
		<< setiosflags(ios::fixed) << setprecision(2) << info.fPercent * 100 << "%\t\t" 
		<< setw(15) << info.nTotalSize
		<< setw(15) << info.nTotalDownload << endl; 
}
