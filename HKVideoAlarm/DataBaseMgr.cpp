#include "pch.h"
#include "DataBaseMgr.h"

#define  SQL_BUFFER_LEN 1024

CMutex qLock;

CDataBaseMgr* GetDBPtr()
{
	qLock.Lock();
	return new CDataBaseMgr;
}

void RemoveDBPtr(CDataBaseMgr* pObject)
{
	delete pObject;
	qLock.Unlock();
}


CDataBaseMgr::CDataBaseMgr(void)
{

}


CDataBaseMgr::~CDataBaseMgr(void)
{

}

bool CDataBaseMgr::Init(CString strUID,CString strPassWord,CString strDSN)
{
	bool bRet = false;
	if(!m_DbAdo.CreateInstance())
	{
		return false;
	}
	m_DbAdo.SetConnectionString(strDSN,strUID,strPassWord);
	if(!m_DbAdo.OpenConnection())
	{
		return false;
	}
	return true;
}

bool CDataBaseMgr::Init(const char* szConnect /*= NULL*/)
{
	CString strConnectStr = szConnect;
	bool bRet = false;
	if (!m_DbAdo.CreateInstance())
	{
		return false;
	}
	if (szConnect == NULL)
	{
		CString strFilePath = GetAppPath() + "db.ini";
		char connectStr[200] = { 0 };
		DWORD dwRet = GetPrivateProfileString("DBInfo", "ConnectString", "", connectStr, 200, strFilePath);
		if (dwRet <= 0)
		{
			return false;
		}
		strConnectStr = connectStr;
	}
	m_DbAdo.SetConnectionString(strConnectStr);
	if (!m_DbAdo.OpenConnection())
	{
		return false;
	}
	return true;
}

CString CDataBaseMgr::GetAppPath() const
{
	TCHAR szPath[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szPath, MAX_PATH);
	CString strPath(szPath);
	strPath = strPath.Left(strPath.ReverseFind(_T('\\')) + 1);
	return strPath;
}

void CDataBaseMgr::Release()
{
	m_DbAdo.CloseConnection();
}

bool CDataBaseMgr::ExcuteSQL(CString strSql)
{
	CSingleLock lk(&m_Sec,TRUE);
	char szSQL[SQL_BUFFER_LEN] = {0};
	memcpy(szSQL,strSql.GetBuffer(strSql.GetLength()),strSql.GetLength());
	strSql.ReleaseBuffer();
	if(!m_DbAdo.IsConnecting())
	{
		m_DbAdo.OpenConnection();
		return false;
	}
	return m_DbAdo.Execute(szSQL);
}

void CDataBaseMgr::CheckDBConnect()
{
	CSingleLock lk(&m_Sec,TRUE);
	if(!m_DbAdo.IsConnecting())
		m_DbAdo.OpenConnection();
}

bool CDataBaseMgr::GetDBStatus()
{
	return m_DbAdo.IsConnecting();
}

BOOL CDataBaseMgr::GetHK_IDandFloorName(CString strAddr,CString strChannelID,CString& strResourceId,CString& strFloorName)
{
	CSingleLock lk(&m_Sec,TRUE);
	CString strSql;
	strSql.Format("SELECT id,name FROM ibs_resource WHERE id = (SELECT resource_id FROM ibs_cctvunit_device WHERE channelId = \'%s\' AND ip = \'%s\')",strChannelID, strAddr);

	char szSQL[SQL_BUFFER_LEN] = {0};
	memcpy(szSQL,strSql.GetBuffer(strSql.GetLength()),strSql.GetLength());
	strSql.ReleaseBuffer();
	try 
	{
		bool bOpen = m_DbAdo.OpenRecordset(szSQL);
		if(bOpen)
		{
			if(m_DbAdo.GetRecordCount()>0)
			{
				m_DbAdo.GetFieldValue("id",strResourceId);
				m_DbAdo.GetFieldValue("name",strFloorName);
				return TRUE;
			}

		}else{
			CheckDBConnect();
			return FALSE;
		}
	}
	catch (_com_error &err)
	{
		TRACE(_T("数据库操作失败! 错误信息:%s, 文件:%s, 行:%d.\n"), err.ErrorMessage(), __FILE__, __LINE__);	 		
		return FALSE;
	}
	return FALSE;
}

int CDataBaseMgr::IsNotHandled(CString strAddr, CString strChannelID)
{
	CSingleLock lk(&m_Sec,TRUE);
	CString strSql;
	strSql.Format("SELECT * FROM ibs_alarm_record WHERE paramenter_code=\'%s-%s\'  AND manager_status=0",strAddr,strChannelID);
	char szSQL[SQL_BUFFER_LEN] = {0};
	memcpy(szSQL,strSql.GetBuffer(strSql.GetLength()),strSql.GetLength());
	strSql.ReleaseBuffer();
	try 
	{
		bool bOpen = m_DbAdo.OpenRecordset(szSQL);
		if(bOpen)
		{
			if(m_DbAdo.GetRecordCount()>0)
			{
				return 1;
			}else{
				return 0;
			}

		}else{
			CheckDBConnect();
			return -1;
		}
	}
	catch (_com_error &err)
	{
		TRACE(_T("数据库操作失败! 错误信息:%s, 文件:%s, 行:%d.\n"), err.ErrorMessage(), __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

void CDataBaseMgr::GetHKDevices(HKDevices& devList)
{
	CSingleLock lk(&m_Sec, TRUE);
	CString strSql = "SELECT * FROM ibs_cctvunit WHERE type = \'hk\'  ";
	char szSQL[SQL_BUFFER_LEN] = { 0 };
	memcpy(szSQL, strSql.GetBuffer(strSql.GetLength()), strSql.GetLength());
	strSql.ReleaseBuffer();
	devList.RemoveAll();
	try
	{
		bool bOpen = m_DbAdo.OpenRecordset(szSQL);
		if (bOpen)
		{
			m_DbAdo.MoveToFirst();
			CString strAddr, strUser, strPswd, strName, strChannel;
			int nPort = 0, nChannel = 0;
			while (!m_DbAdo.IsEndRecordset())
			{
				HKDevice dev = { 0 };
				m_DbAdo.GetFieldValue("ip", strAddr);
				strcpy_s(dev.szIP, strAddr);
				m_DbAdo.GetFieldValue("port", nPort);
				dev.port = nPort;

				m_DbAdo.GetFieldValue("username", strUser);
				strcpy_s(dev.user, strUser);
				m_DbAdo.GetFieldValue("password", strPswd);
				strcpy_s(dev.password, strPswd);

				m_DbAdo.GetFieldValue("name", strName);
				strcpy_s(dev.name, strName);

				devList.Add(dev);
				m_DbAdo.MoveToNext();
			}
		}
	}
	catch (_com_error &err)
	{
		TRACE(_T("数据库操作失败! 错误信息:%s, 文件:%s, 行:%d.\n"), err.ErrorMessage(), __FILE__, __LINE__);
		return;
	}
	return;
}

void CDataBaseMgr::GetHKVideos(HKVideos& videoList)
{
	CSingleLock lk(&m_Sec, TRUE);
	CString strSql = "SELECT t.ip,t.port,t.username,t.password,t1.name,t1.channelId FROM ibs_cctvunit_device as t1,ibs_cctvunit as t WHERE t1.cctvunit_id = t.id AND t.type = \'hk\' ";
	char szSQL[SQL_BUFFER_LEN] = { 0 };
	memcpy(szSQL, strSql.GetBuffer(strSql.GetLength()), strSql.GetLength());
	strSql.ReleaseBuffer();
	videoList.RemoveAll();
	try
	{
		bool bOpen = m_DbAdo.OpenRecordset(szSQL);
		if (bOpen)
		{
			m_DbAdo.MoveToFirst();
			CString strAddr, strUser, strPswd, strName, strChannel;
			int nPort = 0, nChannel = 0;
			while (!m_DbAdo.IsEndRecordset())
			{
				HKVideo dev = { 0 };
				m_DbAdo.GetFieldValue("ip", strAddr);
				strcpy_s(dev.szIP, strAddr);
				m_DbAdo.GetFieldValue("port", nPort);
				dev.port = nPort;

				m_DbAdo.GetFieldValue("username", strUser);
				strcpy_s(dev.user, strUser);
				m_DbAdo.GetFieldValue("password", strPswd);
				strcpy_s(dev.password, strPswd);

				m_DbAdo.GetFieldValue("name", strName);
				strcpy_s(dev.name, strName);
				m_DbAdo.GetFieldValue("channelId", strChannel);
				dev.channel = atoi(strChannel);

				videoList.Add(dev);
				m_DbAdo.MoveToNext();
			}
		}
	}
	catch (_com_error &err)
	{
		TRACE(_T("数据库操作失败! 错误信息:%s, 文件:%s, 行:%d.\n"), err.ErrorMessage(), __FILE__, __LINE__);
		return;
	}
	return;
}
