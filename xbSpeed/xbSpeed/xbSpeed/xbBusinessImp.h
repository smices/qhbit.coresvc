
#pragma once
#include "util.h"

DWORD UpdateThreadProc(PThreadCtrl);
DWORD TaskThreadProc(PThreadCtrl);

void AddToSharing(PCurSharingTask item,bool bForCfg = false);
void AddToSharing(mapCurSharingTask &mapSharingStatus,int status=0);