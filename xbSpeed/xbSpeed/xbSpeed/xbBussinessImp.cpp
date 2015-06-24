
#include "xbBusinessImp.h"
#include <iostream>
#include "xldl.h"
#include "DownWrapper.h"
#include "InitConf.h"
extern DownWrapper* g_pWapper;
//==============================================
void PrintTaskInfo(DownTaskInfo &info);

DWORD UpdateThreadProc(PThreadCtrl pThrdCtrl) {
	//load local update.conf

	//check finish download

	//start pending download
	while (true){
	//add or remove download

	//check server conf period 3600s
		std::cout<<"UpdateThreadProc -- running"<<std::endl;
		Sleep(5000);
	}
	return 0;
}

void AddToSharing(mapCurSharingTask &mapSharingStatus,int status) {
	std::string szTaskTD,szTaskCFG;

	mapCurSharingTask::iterator it;
	std::string szTmp;
	std::string BaseDir = GetModulePath(NULL);
	for (it = mapSharingStatus.begin();
		 it!=mapSharingStatus.end();
		 it++) {
			 if (!it->second) {
				 continue;
			 }

			 if (it->second->nPrepareTDCFG==0) {
				 szTaskCFG = it->second->matchPath;
				 szTaskCFG.append(".td.cfg");
				 if (!PathFileExistsA(szTaskCFG.data())) {
					 if (it->second->hTaskCfgObj!=NULL) // task running
						 continue;

					 AddToSharing(it->second,true);
				 }
				 else {
					 it->second->nPrepareTDCFG=1;
				 }
			 }
			 else if (it->second->nPrepareTDCFG==1){
				 if (it->second->hTaskObj!=NULL)
					 continue;
				 // add
				 if (it->second->nStatus==0 || it->second->nStatus==2) {
					 szTaskTD = it->second->matchPath;
					 szTaskTD.append(".td");
					 MoveFileA(it->second->matchPath.data(),szTaskTD.data());
					 AddToSharing(it->second,false);
				 }
			 }
/*			if (it->second->nStatus==0) {
				// add
				// check and prepare .td and .cfg
				szTaskTD = it->second->matchPath;
				szTaskTD.append(".td");
				szTaskCFG= szTaskTD;
				szTaskCFG.append(".cfg");
				if (!PathFileExistsA(szTaskTD.data())) {
					if (PathFileExistsA(it->second->matchPath.data())) {
						MoveFileA(it->second->matchPath.data(),szTaskTD.data());
					}
					else {
						// Not Found
						it->second->nStatus=4;
					}
				}
				if (PathFileExistsA(szTaskCFG.data())) {
					// .td.cfg exist
					it->second->nPrepareTDCFG=0;
					szTmp = BaseDir + "\\Data\\temp.td.cfg";
					MoveFileA(szTmp.data(),szTaskCFG.data());
				}

				AddToSharing(it->second);
			}
			else if (it->second->nStatus==2) {
				;//new
				// check and prepare .td and .cfg
				// add
				// check and prepare .td and .cfg
				szTaskTD = it->second->matchPath;
				szTaskTD.append(".td");
				szTaskCFG= szTaskTD;
				szTaskCFG.append(".cfg");
				if (!PathFileExistsA(szTaskTD.data())) {
					if (PathFileExistsA(it->second->matchPath.data())) {
						MoveFileA(it->second->matchPath.data(),szTaskTD.data());
					}
					it->second->nStatus=4;
				}
				if (!PathFileExistsA(szTaskCFG.data())) {
					szTmp = BaseDir + "\\Data\\temp.td.cfg";
					MoveFileA(szTmp.data(),szTaskCFG.data());
				}

				AddToSharing(it->second);
			}*/
	}
}

void AddToSharing(PCurSharingTask item,bool bForCfg) {
	std::wstring wstr;
	std::string strTmp;
	DownTaskParam downTaskParam;
	HANDLE hTask = NULL;

	if (!item) {
		return;
	}
	if (bForCfg) {
		wstr.clear();
		StringToWString(item->item.name,wstr);
		wstr.append(L".td.cfg");
		StrCpyW(downTaskParam.szFilename, wstr.data());

		wstr.clear();
		StringToWString(item->item.tdConfigUrl,wstr);
		StrCpyW(downTaskParam.szTaskUrl, wstr.data());
		downTaskParam.IsOnlyOriginal = TRUE;

		wstr.clear();
		strTmp = GetFilePathFromFile(item->matchPath);
		StringToWString(strTmp,wstr);
		wstr.append(L"\\");
		StrCpyW(downTaskParam.szSavePath, wstr.data());

		downTaskParam.IsOnlyOriginal = TRUE;
		hTask = g_pWapper->TaskCreate(downTaskParam);
		if (hTask) {
			g_pWapper->TaskStart(hTask);
			item->hTaskCfgObj = hTask;
		}
	}
	else {
		wstr.clear();
		StringToWString(item->item.name,wstr);
		StrCpyW(downTaskParam.szFilename, wstr.data());

		wstr.clear();
		StringToWString(item->item.downloadUrl,wstr);
		StrCpyW(downTaskParam.szTaskUrl, wstr.data());

		wstr.clear();
		strTmp = GetFilePathFromFile(item->matchPath);
		StringToWString(strTmp,wstr);
		wstr.append(L"\\");
		StrCpyW(downTaskParam.szSavePath, wstr.data());

		downTaskParam.IsOnlyOriginal = FALSE;

		hTask = g_pWapper->TaskCreate(downTaskParam);
		if (hTask) {
			g_pWapper->TaskStart(hTask);
			item->hTaskObj = hTask;
			item->nStatus  = 1;//run
		}
	}
}

DWORD TaskThreadProc(PThreadCtrl pThrdCtrl) {
	DWORD dwTickCountCur = 0;
	DWORD dwTickCountLast= 0;
	bool updateConf = false;
	PTaskConfDef taskConf = NULL,taskConfNew=NULL;
	std::string szTmp;

	mapCurSharingTask mapSharingStatus;
	//load local taskdefine.conf
	taskConf = LoadLocalShareConf();
	//collect local task source file
	ScanTarget(mapSharingStatus,taskConf);
	while(true){
		//add upload
		AddToSharing(mapSharingStatus,0);
		if (updateConf) {
			taskConfNew = LoadLocalShareConf();
			if (taskConfNew && taskConfNew->m_code==0) {
				TaskItems::iterator it;
				mapCurSharingTask::iterator itm;

				for (itm = mapSharingStatus.begin();
					 itm!= mapSharingStatus.end();
					 itm++
					 ) {
						 if (taskConfNew->m_msg.m_files.find(itm->first) == \
							 taskConfNew->m_msg.m_files.end()) {
							 itm->second->nStatus=3;//remove
						 }
				}

				for (it = taskConfNew->m_msg.m_files.begin();
					 it!= taskConfNew->m_msg.m_files.end();
					 it ++
					) {
						if (!it->second) {
							continue;
						}
						if (mapSharingStatus.find(it->first)!=mapSharingStatus.end()) {
							continue;
						}
						szTmp = ScanTarget(it->second);
						if (!szTmp.empty()) {
							PCurSharingTask item = CreateSharingTaskItem(szTmp,it->second);
							item->nStatus=2;
							mapSharingStatus.insert(mapCurSharingTask::value_type(it->first,item));
						}
				}
				AddToSharing(mapSharingStatus,2);
			}
		}

		DWORD waitRet = WaitForSingleObject(pThrdCtrl->m_hEvent[0],3000);
		if (waitRet==WAIT_OBJECT_0) {
			SetEvent(pThrdCtrl->m_hEvent[1]);
			break;
		}
		else if (waitRet==WAIT_TIMEOUT) {
			////check server task conf updated?
			//check server define task conf period 3600s
			dwTickCountCur = GetTickCount();
			if(dwTickCountCur-dwTickCountLast>10*1000) {
				long version=0;
				if (!taskConf||taskConf->m_code==9) {
					version=0;
				}
				else if(taskConf->m_code==0) {
					version = taskConf->m_msg.m_version;
				}
				else {
					version = 0;
				}
				if (FetchTaskConf(version)==0) {
					updateConf = true;//have new config
				}
				dwTickCountLast = dwTickCountCur;
			}
		}
		//prepare upload *.td and *.cfg

		//add or remove upload

		//report task status
		for (mapCurSharingTask::iterator itRunning = mapSharingStatus.begin();
			itRunning != mapSharingStatus.end();
			itRunning++){
				DownTaskInfo info;
				memset(&info,0,sizeof(info));
				if (itRunning->second->nPrepareTDCFG==0 && !itRunning->second->hTaskCfgObj) {
					continue;
				}
				if (itRunning->second->nPrepareTDCFG==0 && itRunning->second->hTaskCfgObj!=NULL) {
					;//view .td.cfg downing status
					g_pWapper->TaskQueryEx(itRunning->second->hTaskObj, info);
					PrintTaskInfo(info);
					if (info.stat == NOITEM)
						continue;
					else if (info.stat == TSC_COMPLETE) {
						itRunning->second->nPrepareTDCFG=1;
						continue;
					}
					else if (info.stat == TSC_ERROR) {
						szTmp = itRunning->second->matchPath;
						szTmp.append(".td.cfg");
						if (PathFileExistsA(szTmp.data())) {
							itRunning->second->nPrepareTDCFG=1;
							g_pWapper->TaskPause(itRunning->second->hTaskCfgObj);
							g_pWapper->TaskDelete(itRunning->second->hTaskCfgObj);
							itRunning->second->hTaskCfgObj = NULL;
						}
						continue;
					}
					else {
						continue;
					}
				}
				if (itRunning->second->nPrepareTDCFG==1) {
					if (itRunning->second->nStatus==1) {
						g_pWapper->TaskQueryEx(itRunning->second->hTaskObj, info);
						PrintTaskInfo(info);
						if (info.stat == NOITEM)
							continue;
						else if (info.stat == TSC_COMPLETE)
							continue;
						else if (info.stat == TSC_ERROR)
						{
							continue;
						}
					}
					else if (itRunning->second->nStatus==3) {
						;//
					}
				}
		}
	}
	DestroyTaskConfDef(taskConf);

	return 0;
}
