#pragma once

#include "DBAdo.h"

typedef struct STRU_HK_VIDEO
{
	char szIP[20];
	int port;
	char user[60];
	char password[60];
	char name[100];
	int channel;
}HKVideo;

typedef struct STRU_HK_DEVICE
{
	char szIP[20];
	int port;
	char user[60];
	char password[60];
	char name[100];
}HKDevice;

typedef CArray<HKDevice, HKDevice> HKDevices;

typedef CArray<HKVideo, HKVideo> HKVideos;

class CDataBaseMgr
{
public:
	CDataBaseMgr(void);
	~CDataBaseMgr(void);
	bool Init(CString strUID,CString strPassWord,CString strDSN);
	bool Init(const char* szConnect = NULL);
	CString GetAppPath() const;
	void Release();
protected:
	CDBAdo m_DbAdo;
public:
	bool ExcuteSQL(CString strSql);
	void CheckDBConnect();
	bool GetDBStatus();
protected:
	CCriticalSection m_Sec;
public:
	BOOL GetHK_IDandFloorName(CString strAddr,CString strChannelID,CString& strResourceId,CString& strFloorName);
	int IsNotHandled(CString strAddr, CString strChannelID);

	void GetHKDevices(HKDevices& devList);
	void GetHKVideos(HKVideos& videoList);

};

CDataBaseMgr* GetDBPtr();

void RemoveDBPtr(CDataBaseMgr* pObject);

