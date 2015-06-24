#include "InitConf.h"
#include "json\json.h"
#include "curl\curl.h"

std::string GetModulePath(HMODULE hModule){
	char buf[1024] = {'\0'};
	std::string strDir;

	::GetModuleFileNameA( hModule, buf, MAX_PATH);
	PathRemoveFileSpecA(buf);
	strDir.append(buf);
	return strDir;
}

std::string GetFilePathFromFile(std::string szFile) {
	std::string strDir;
	char buf[1024] = {'\0'};

	strcpy_s(buf,szFile.data());
	PathRemoveFileSpecA(buf);
	strDir.append(buf);
	return strDir;
}

void InitDir(){
	char buf[1024] = {'\0'};

	std::string BaseDir = GetModulePath(NULL);
	std::string tmpDir;

	// ----------------------------------
	tmpDir = BaseDir+"\\UpdateDir" ;_mkdir(tmpDir.data());
	// ----------------------------------
	tmpDir = BaseDir+"\\Data" ;	_mkdir(tmpDir.data());
	// ----------------------------------
	tmpDir = BaseDir+"\\Conf" ;	_mkdir(tmpDir.data());
	// ----------------------------------
	tmpDir = BaseDir+"\\Temp" ;	_mkdir(tmpDir.data());
	// ----------------------------------
}
// --------------------------------------
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	FILE *__fpw = (FILE *)userp;
	fwrite(contents, size, nmemb, __fpw);
	return size * nmemb;
}

BOOL GetResourceFromHttp(const char *urls,const char *filename)
{
	FILE* __fpw=fopen(filename, "wb");
	if(!__fpw)
		return FALSE;

	char url[1024];
	_snprintf_s(url, sizeof(url), "%s",urls);

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, __fpw);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	long len = ftell(__fpw);
	fclose(__fpw);

	if(res==CURLE_OK){
		if (len==0)
		{
			DeleteFileA(filename);
			return FALSE;
		}
		return TRUE;
	}
	else{
		DeleteFileA(filename);
		return FALSE;
	}
}

//Get config file from server
BOOL GetConfFromServ(std::string &serverUrl,std::string &fileName) {
	// get config file
	std::string xbSpeedConf = fileName;
	std::string serConfUrl  = serverUrl;
	Json::Value	__jsonRoot;
	while(true){
		if ( PathFileExistsA(xbSpeedConf.data()) ) {
			Json::Reader r;
			std::ifstream fJson(xbSpeedConf);

			if (!r.parse(fJson,__jsonRoot,false) )
				return FALSE;
			if(!__jsonRoot.isObject())
				return FALSE;
		}
		else {
			if (GetResourceFromHttp(serverUrl.data(),xbSpeedConf.data())) {
				continue;
			}
			else {
				return FALSE;
			}
		}
		return TRUE;

	}
	return FALSE;
}

PTaskConfDef LoadLocalShareConf() {
	PTaskConfDef taskConf = NULL;
	std::string dataDir;
	std::string BaseDir = GetModulePath(NULL);

	dataDir = BaseDir + "\\Conf\\taskdefine.conf";
	if (PathFileExistsA(dataDir.data())) {
		taskConf = CreateTaskConfDef(dataDir);
		if (!taskConf || taskConf->m_code!=0) {
			DeleteFileA(dataDir.data());
			DestroyTaskConfDef(taskConf);
			taskConf = NULL;
		}
	}
	return taskConf;
}

PTaskConfDef CreateTaskConfDef(std::string szJSConf) {
	int nSise = 0,i=0;
	bool bErr = false;

	Json::Value jsValue;
	Json::Reader reader;
	std::ifstream fJson(szJSConf);
	PTaskConfDef pTaskConf = NULL;

	if (!reader.parse(fJson,jsValue,false) || !jsValue.isObject())
		return NULL;

	if (!jsValue.isMember("code")||!jsValue["code"].isIntegral()) {
		return NULL;
	}

	pTaskConf = new TaskConfDef;
	pTaskConf->m_code = jsValue["code"].asInt64();
	pTaskConf->m_msg.m_version=0;
	if (pTaskConf->m_code!=0) {
		return pTaskConf;
	}
	
	if (   !jsValue.isMember("msg") 
		|| !(jsValue["msg"].isMember("version")&&(jsValue["msg"]["version"].isInt())) 
		|| !(jsValue["msg"].isMember("files")&&jsValue["msg"]["files"].isArray())
		) {
			delete pTaskConf;
			return NULL;
	}
	pTaskConf->m_msg.m_version = jsValue["msg"]["version"].asUInt();

	bErr = false;

	nSise = jsValue["msg"]["files"].size();
	for (i=0; i < nSise; i++) {
		Json::Value item = jsValue["msg"]["files"][i];

		if (!(item.isMember("fileName")&&item["fileName"].isString()) \
			||!(item.isMember("storage")&&item["storage"].isString()) \
			||!(item.isMember("fileSize")&&item["fileSize"].isIntegral()) \
			||!(item.isMember("fileHash")&&item["fileHash"].isString()) \
			||!(item.isMember("uploadSpeed")&&item["uploadSpeed"].isIntegral()) \
			||!(item.isMember("downloadUrl")&&item["downloadUrl"].isString()) \
			||!(item.isMember("tdConfigUrl")&&item["tdConfigUrl"].isString())
			) {
				bErr = true;
				break;
		}
	}
	if (bErr) {
		delete pTaskConf;
		return NULL;
	}
	for (i=0; i < nSise; i++) {
		Json::Value item = jsValue["msg"]["files"][i];
		PTaskItem tagitem = new TaskItem;
		tagitem->name = item["fileName"].asString();
		tagitem->storage = item["storage"].asString();
		tagitem->size =  item["fileSize"].asInt64();
		tagitem->hash = item["fileHash"].asString();
		tagitem->uploadSpeed =  item["uploadSpeed"].asInt64();
		tagitem->downloadSpeed = 0 ;//item["downloadSpeed"].asInt64();
		tagitem->downloadUrl = item["downloadUrl"].asString();
		tagitem->tdConfigUrl = item["tdConfigUrl"].asString();

		if (pTaskConf->m_msg.m_files.find(tagitem->name)!=pTaskConf->m_msg.m_files.end()) {
			delete tagitem;
		}
		else {
			pTaskConf->m_msg.m_files[tagitem->name] = tagitem;
		}
	}

	return pTaskConf;
}

void DestroyTaskConfDef(PTaskConfDef pTaskConf) {
	TaskItems::iterator it;
	if (!pTaskConf)
		return ;
	if (pTaskConf->m_code!=0)
		return ;
	for( it=pTaskConf->m_msg.m_files.begin();
		it!=pTaskConf->m_msg.m_files.end();
		it++ 
		) {
		PTaskItem tagitem = it->second;
		delete tagitem;
		it->second = NULL;
	}
	delete pTaskConf;
}

int FetchTaskConf(long version) {
	std::string urlServ="http://ctr.datacld.com/api/ctr?svc=xbspeed";
	std::string urlTmp;
	char buf[256]={0};
	PTaskConfDef taskConf = NULL;
	// check update info
	// check our config file
	std::string tmpDir,dataDir;

	std::string BaseDir = GetModulePath(NULL);

	dataDir = BaseDir + "\\Conf\\taskdefine.conf";

	sprintf(buf,"&cfv=%u",version);
	urlTmp = urlServ + buf;
	tmpDir = BaseDir+"\\Temp\\taskdefine.conf";
	if (PathFileExistsA(tmpDir.data())) {
		DeleteFileA(tmpDir.data());
	}

	if (!GetConfFromServ(urlTmp,tmpDir)) {
		return 1;
	}

	taskConf = CreateTaskConfDef(tmpDir);
	if (!taskConf) {
		return 2;
	}
	if (taskConf->m_code==0) {
		MoveFileExA(tmpDir.data(),dataDir.data(),MOVEFILE_REPLACE_EXISTING);
		DeleteFileA(tmpDir.data());
		DestroyTaskConfDef(taskConf);
		return 0;
	}
	else if (taskConf->m_code==9) {
		DestroyTaskConfDef(taskConf);
		return -1;
	}
	else {
		DeleteFileA(tmpDir.data());
		DestroyTaskConfDef(taskConf);
		return 3;
	}
	DestroyTaskConfDef(taskConf);
	return 0;
}

std::string ScanTarget(PTaskItem pItem) {
	char disk[30]={'\0'};
	std::string szTarget="";
	char *p = disk;
	if (!pItem) {
		return szTarget;
	}
	int nCount = ScanLogicalDrive(&p);
	int i=0,j=0;
	std::string szPath,itPath,secdPath,itPathTD;

	// scan root level
	for (i=0; i<nCount; i++) {
		szPath.clear();
		szPath.append(1,disk[i]);
		szPath.append(":\\");

		// scan all disk root
		{
			itPath = szPath;
			itPath.append(pItem->storage);
			itPath.append("\\");
			itPath.append(pItem->name);
			itPathTD = itPath;
			itPathTD.append(".td");
			if (PathFileExistsA(itPath.data()) || PathFileExistsA(itPathTD.data())) {
				return itPath;
			}
		}
		// scan second
		itPath = szPath;
		itPath.append("*");
		WIN32_FIND_DATAA fd;
		HANDLE hFindFile = FindFirstFileA(itPath.data(), &fd);  
		if(hFindFile == INVALID_HANDLE_VALUE) {
			::FindClose(hFindFile);
			continue;
		}
		while(true) {
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				itPath = szPath;
				itPath.append(fd.cFileName);
				itPath.append("\\");

				{
					secdPath = itPath;
					secdPath.append(pItem->storage);
					secdPath.append("\\");
					secdPath.append(pItem->name);
					itPathTD = secdPath;
					itPathTD.append(".td");
					if (PathFileExistsA(secdPath.data())||PathFileExistsA(itPathTD.data())) {
						::FindClose(hFindFile);
						return secdPath;
					}
				}
			}
			if (FindNextFileA(hFindFile, &fd) == FALSE) {
				break;
			}
		}
		::FindClose(hFindFile);
	}
	return szTarget;
}

PCurSharingTask CreateSharingTaskItem(std::string strPath,const PTaskItem item) {
	PCurSharingTask pItem= new CurSharingTask;
	if (!pItem) {
		return NULL;
	}
	pItem->nStatus=0;
	pItem->hTaskObj=NULL;
	pItem->hTaskCfgObj=NULL;
	pItem->matchPath = strPath;
	pItem->item.name = item->name;
	pItem->item.storage = item->storage;
	pItem->item.size = item->size;
	pItem->item.hash = item->hash;
	pItem->item.uploadSpeed = item->uploadSpeed;
	pItem->item.downloadSpeed = item->downloadSpeed;
	pItem->item.downloadUrl = item->downloadUrl;
	pItem->item.tdConfigUrl = item->tdConfigUrl;
	pItem->nPrepareTDCFG = 0;
	return pItem;
}

void ScanTarget(mapCurSharingTask &mapColl,PTaskConfDef taskConf) {
	char disk[30]={'\0'};
	char *p = disk;
	if (!taskConf) {
		return ;
	}
	if (taskConf->m_code!=0) {
		return ;
	}
	int nCount = ScanLogicalDrive(&p);
	int i=0,j=0;
	std::string szPath,itPath,secdPath,itPathTD;

	// scan root level
	for (i=0; i<nCount; i++) {
		szPath.clear();
		szPath.append(1,disk[i]);
		szPath.append(":\\");
		
		for (TaskItems::iterator it = taskConf->m_msg.m_files.begin(); 
			it != taskConf->m_msg.m_files.end();
			it++) {
			itPath = szPath;
			itPath.append(it->second->storage);
			itPath.append("\\");
			itPath.append(it->second->name);
			itPathTD = itPath;
			itPathTD.append(".td");
			if (PathFileExistsA(itPath.data()) || PathFileExistsA(itPathTD.data())) {
				if (mapColl.find(it->second->name)==mapColl.end()) {
					PCurSharingTask item = CreateSharingTaskItem(itPath,it->second);
					mapColl.insert(mapCurSharingTask::value_type(it->second->name,item));
				}
			}
		}
		// scan second
		itPath = szPath;
		itPath.append("*");
		WIN32_FIND_DATAA fd;
		HANDLE hFindFile = FindFirstFileA(itPath.data(), &fd);  
		if(hFindFile == INVALID_HANDLE_VALUE) {
			::FindClose(hFindFile);
			continue;
		}
		while(true) {
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				itPath = szPath;
				itPath.append(fd.cFileName);
				itPath.append("\\");
				
				for (TaskItems::iterator it = taskConf->m_msg.m_files.begin();
					it != taskConf->m_msg.m_files.end();
					it++) {
						secdPath = itPath;
						secdPath.append(it->second->storage);
						secdPath.append("\\");
						secdPath.append(it->second->name);
						itPathTD = secdPath;
						itPathTD.append(".td");
						if (PathFileExistsA(secdPath.data())||PathFileExistsA(itPathTD.data())) {
							if (mapColl.find(it->second->name)==mapColl.end()) {
								PCurSharingTask item = CreateSharingTaskItem(secdPath,it->second);
								mapColl.insert(mapCurSharingTask::value_type(it->second->name,item));
							}
						}
				}
			}
			if (FindNextFileA(hFindFile, &fd) == FALSE) {
				break;
			}
		}
		::FindClose(hFindFile);
	}
}

PUpdateConfDef LoadLocalUpdateConf() {
	PUpdateConfDef updateConf = NULL;
	std::string dataDir;
	std::string BaseDir = GetModulePath(NULL);

	dataDir = BaseDir + "\\Conf\\Update.conf";
	if (PathFileExistsA(dataDir.data())) {
		updateConf = CreateUpdateConfDef(dataDir);
		if (!updateConf || updateConf->m_code!=0) {
			DeleteFileA(dataDir.data());
			DestroyUpdateConfDef(updateConf);
			updateConf = NULL;
		}
	}
	return updateConf;
}

PUpdateConfDef CreateUpdateConfDef(std::string szJSConf) {
	int nSise = 0,i=0;
	bool bErr = false;

	Json::Value jsValue;
	Json::Reader reader;
	std::ifstream fJson(szJSConf);
	PUpdateConfDef pUpdateConf = NULL;

	if (!reader.parse(fJson,jsValue,false) || !jsValue.isObject())
		return NULL;
	if (!jsValue.isMember("code")||!jsValue["code"].isIntegral())
		return NULL;
	pUpdateConf = (PUpdateConfDef)new UpdateConfDef;
	pUpdateConf->m_code = jsValue["code"].asInt64();
	pUpdateConf->m_msg.m_version=0;
	if (pUpdateConf->m_code!=0) {
		return pUpdateConf;
	}
	if (   !jsValue.isMember("msg") 
		|| !(jsValue["msg"].isMember("version")&&(jsValue["msg"]["version"].isInt())) 
		|| !(jsValue["msg"].isMember("files")&&(jsValue["msg"]["files"].isArray())) 
		) {
			delete pUpdateConf;
			return NULL;
	}
	pUpdateConf->m_msg.m_version = jsValue["msg"]["version"].asUInt();
	bErr = false;
	Json::Value jsFiles=jsValue["msg"]["files"];
	nSise = jsFiles.size();
	for (i=0;i<nSise;i++) {
		Json::Value item = jsFiles[i];
		if (!(item.isMember("service")&&item["service"].isString())
			||!(item.isMember("updateMode")&&item["updateMode"].isString())
			||!(item.isMember("LastVersion")&&item["LastVersion"].isString())
			||!(item.isMember("LastVersionCode")&&item["LastVersionCode"].isIntegral())
			||!(item.isMember("ReleaseTime")&&item["ReleaseTime"].isString())
			||!(item.isMember("LowCompatible")&&item["LowCompatible"].isString())
			||!(item.isMember("Arch")&&item["Arch"].isString())
			||!(item.isMember("FileName")&&item["FileName"].isString())
			||!(item.isMember("FileSize")&&item["FileSize"].isIntegral())
			||!(item.isMember("FileHash")&&item["FileHash"].isString())
			||!(item.isMember("Download")&&item["Download"].isString())
			){
				bErr = true;
				break;
		}
	}
	if (bErr) {
		delete pUpdateConf;
		return NULL;
	}
	for (i=0;i<nSise;i++) {
		Json::Value item = jsFiles[i];
		PUpdateItem tagitem = new UpdateItem;
		tagitem->service = item["service"].asString();
		tagitem->updateMode = item["updateMode"].asString();
		tagitem->LastVersion = item["LastVersion"].asString();
		tagitem->LastVersionCode = item["LastVersionCode"].asInt64();
		tagitem->ReleaseTime = item["ReleaseTime"].asString();
		tagitem->LowCompatible = item["LowCompatible"].asString();
		tagitem->Arch = item["Arch"].asString();
		tagitem->FileName = item["FileName"].asString();
		tagitem->FileSize = item["FileSize"].asInt64();
		tagitem->FileHash = item["FileHash"].asString();
		tagitem->Download = item["Download"].asString();
		if (pUpdateConf->m_msg.m_files.find(tagitem->service)!=pUpdateConf->m_msg.m_files.end()) {
			delete tagitem;
		}
		else {
			pUpdateConf->m_msg.m_files[tagitem->service] = tagitem;
		}
	}
	return pUpdateConf;
}

void DestroyUpdateConfDef(PUpdateConfDef pUpdateConf) {
	UpdateItems::iterator it;
	if (!pUpdateConf)
		return;
	if (pUpdateConf->m_code!=0)
		return ;

	for( it=pUpdateConf->m_msg.m_files.begin();
		it!=pUpdateConf->m_msg.m_files.end();
		it++ 
		) {
			PUpdateItem tagitem = it->second;
			delete tagitem;
			it->second = NULL;
	}
	delete pUpdateConf;
}

int FetchUpdateConf(long version) {
	std::string urlServ="http://ctr.datacld.com/api/upgrade";
	std::string urlTmp;
	std::string dataDir,tmpDir;
	char buf[256]={0};
	PUpdateConfDef updateConf=NULL;
	std::string BaseDir = GetModulePath(NULL);
	dataDir = BaseDir + "\\Conf\\update.conf";

	sprintf(buf,"?cfv=%u",version);
	urlTmp = urlServ + buf;
	tmpDir = BaseDir+"\\Temp\\update.conf";
	if (PathFileExistsA(tmpDir.data())) {
		DeleteFileA(tmpDir.data());
	}

	if (!GetConfFromServ(urlTmp,tmpDir)) {
		return 1;
	}

	updateConf = CreateUpdateConfDef(tmpDir);
	if (!updateConf) {
		return 2;
	}
	if (updateConf->m_code==0) {
		MoveFileExA(tmpDir.data(),dataDir.data(),MOVEFILE_REPLACE_EXISTING);
		DeleteFileA(tmpDir.data());
		DestroyUpdateConfDef(updateConf);
		return 0;
	}
	else if (updateConf->m_code==9) {
		DestroyUpdateConfDef(updateConf);
		return -1;
	}
	else {
		DeleteFileA(tmpDir.data());
		DestroyUpdateConfDef(updateConf);
		return 3;
	}
	DestroyUpdateConfDef(updateConf);
	return 0;
}
