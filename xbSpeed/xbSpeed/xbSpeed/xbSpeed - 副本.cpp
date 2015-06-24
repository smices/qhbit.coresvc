/*
#include <Windows.h>

#include "Shlwapi.h"
#pragma comment(lib,"shlwapi.lib") 

#include <string>
#include <tchar.h>

#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <direct.h>
//#include <errno.h>
//#include <tchar.h>

#include "xldl.h"
#include "DownWrapper.h"
*/

#include <iostream>
#include <tchar.h>

#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib") 
#include <string>
#include <iomanip>
#include "xldl.h"
#include "DownWrapper.h"


DownWrapper *pWapper = NULL;
// 加载xldl.dll
DownWrapper* LoadDll()
{
	WCHAR szModulePath[MAX_PATH] = {0};
	::GetModuleFileName(NULL, szModulePath, MAX_PATH);
	::PathRemoveFileSpec(szModulePath);

	WCHAR szDllpath[MAX_PATH] = {0};
	::PathCombine(szDllpath, szModulePath, L"xldl.dll");
	OutputDebugString(szDllpath);

	DownWrapper* pWapper = new DownWrapper(szDllpath);
	return pWapper;
}


// 打印任务信息
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
		std::wcout << endl << setiosflags(ios::left) << setw(20) << "Status" << setw(20) << "Progress " 
			<< setw(15) << "File Size (B)" << setw(15) << "Download Size (B)" << endl; 
		bTitle = false;
	}
	wcout << setw(20) << stat 
		<< setiosflags(ios::fixed) << setprecision(2) << info.fPercent * 100 << "%\t\t" 
		<< setw(15) << info.nTotalSize
		<< setw(15) << info.nTotalDownload << endl; 
}

void main_loop()
{
	// add download task
/*	DownTaskParam downTaskParam;
	StrCpyW(downTaskParam.szTaskUrl ,L"http://download-cf.jetbrains.com/webide/PhpStorm-8.0.3.exe");
	StrCpyW(downTaskParam.szFilename,L"PhpStorm8.exe");

//	StrCpyW(downTaskParam.szSavePath,L"d:\\");

	WCHAR szModulePath[MAX_PATH] = {0};
	::GetModuleFileName(NULL, szModulePath, MAX_PATH);
	::PathRemoveFileSpec(szModulePath);
	StrCpyW(downTaskParam.szSavePath, szModulePath);

//	downTaskParam.szTaskUrl  = L"http://download-cf.jetbrains.com/webide/PhpStorm-8.0.3.exe";
//	downTaskParam.szFilename = L"PhpStorm-8.x.exe";
//	downTaskParam.szSavePath = L"d:\\";
	_tprintf(downTaskParam.szSavePath);
*/
	DownTaskParam downTaskParam;

/*	std::wcout << "Input url: ";
 	std::wstring strUrl;
 	std::wcin >> strUrl;
	StrCpyW(downTaskParam.szTaskUrl, strUrl.c_str());*/
	StrCpyW(downTaskParam.szTaskUrl ,L"http://download-cf.jetbrains.com/webide/PhpStorm-8.0.3.exe");

/*	std::wcout << "Input filename: ";
	std::wstring strFilename;
	std::wcin >> strFilename;
	StrCpyW(downTaskParam.szFilename, strFilename.c_str());*/
	StrCpyW(downTaskParam.szFilename,L"PhpStorm8.exe");

	WCHAR szModulePath[MAX_PATH] = {0};
	::GetModuleFileName(NULL, szModulePath, MAX_PATH);
	::PathRemoveFileSpec(szModulePath);
	StrCpyW(downTaskParam.szSavePath, szModulePath);
	HANDLE hTask = pWapper->TaskCreate(downTaskParam);

	// start task
	pWapper->TaskStart(hTask);

	//poll taskinfo;
	while(true)
	{
		DownTaskInfo info;
		pWapper->TaskQueryEx(hTask, info);
		PrintTaskInfo(info);
		if (info.stat == TSC_COMPLETE || info.stat == TSC_ERROR)
			break;
		Sleep(1000);
	}

/*	pWapper->TaskDelete(hTask);
	DownTaskParam deleteParam;
	StrCpyW(deleteParam.szFilename, info.szFilename);
	StrCpyW(deleteParam.szSavePath, downTaskParam.szSavePath);
	pWapper->DelTempFile(deleteParam);*/

}

// main entry
int main (int argc, char **argv)
{
	// read config file;

	//init td plugin
	pWapper= LoadDll();
	if (pWapper==NULL)
		return 1;
	main_loop();
	pWapper->UnInit();
	delete pWapper;
	pWapper = NULL;

	return 0;
}

