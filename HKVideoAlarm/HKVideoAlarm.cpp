// HKVideoAlarm.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "HKVideoAlarm.h"
#include <iostream>
#include "HCNetSDK.h"
#include "DataType.h"
#include "DataBaseMgr.h"
#include "Log.h"
CLog g_Log;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

BOOL g_InitStatus = FALSE;

HKDevices m_HKDevices;
HKVideos m_HKVideos;

typedef struct DEVICE_INFO
{
	char szAddr[20];
	int nlLoginHandle;
	int nAlarmHandle;
}DeviceHandle;

CArray<DEVICE_INFO, DEVICE_INFO&> AlarmDevices;


bool FindHKVideo(CString strAddr, CString strChannelID, HKVideo& video)
{
	for (int i = 0; i < m_HKVideos.GetCount(); i++)
	{
		HKVideo v = m_HKVideos.GetAt(i);
		CString strDevAddr = v.szIP;
		if ((strDevAddr == strAddr) && (v.channel == atoi(strChannelID)))
		{
			memcpy(&video, &v, sizeof(HKVideo));
			return true; 
		}
	}
	cout << "未查找信息 " << strAddr << " 通道 " << strChannelID << endl;

	return false;
}

void InsertAlarmToDB(CString strAddr, CString strChannelID, CString strName, int nType)
{
	CDataBaseMgr db;
	if (db.Init())
	{
		CTime tm = CTime::GetCurrentTime();
		CString strTime = tm.Format("%Y-%m-%d %H:%M:%S");
		if (db.IsNotHandled(strAddr, strChannelID) == 1)
			return;
		CString strSql;
		CString strAlarmDescription;
		switch (nType)
		{
		case 0:
			/*			strAlarmDescription = "信号丢失";*/
			break;
		case 2:
			strAlarmDescription = "信号丢失";
			break;
		case 3:
			strAlarmDescription = "移动侦测";
			break;
		case 6:
			strAlarmDescription = "遮挡报警";
			break;
		case 9:
			strAlarmDescription = "视频信号异常";
			break;
		default:
			break;
		}
		CString strResourId, strResourceName;
		if (db.GetHK_IDandFloorName(strAddr, strChannelID, strResourId, strResourceName))
		{
			strSql.Format("INSERT INTO ibs_alarm_record (item_name,resource_name,floor,alarm_description,alarm_level,alarm_type,alarm_category,alarm_time,paramenter_code,resource_id) VALUES(\'%s\',\'视频监控\',\'%s\',\'%s\',3,4,0,\'%s\',\'%s-%s\',%d)", strName, strResourceName, strAlarmDescription, strTime, strAddr, strChannelID, atoi(strResourId));		
			cout << strSql << endl;
			db.ExcuteSQL(strSql);
		}

		strSql.Format("UPDATE ibs_cctvunit_device SET alarm_status = 1,status = 0 WHERE channelId = \'%s\' and ip = \'%s\' ", strChannelID, strAddr);
		cout << strSql << endl;
		db.ExcuteSQL(strSql);
	}
	db.Release();
}

/*0-信号量报警,1-硬盘满,2-信号丢失,3－移动侦测,4－硬盘未格式化,5-读写硬盘出错,6-遮挡报警,7-制式不匹配, 8-非法访问, 9-视频信号异常,
				  10-录像异常,11- 智能场景变化,12-阵列异常,13-前端/录像分辨率不匹配,14-申请解码资源失败,15-智能侦测, 16-POE供电异常报警,17-闪光灯异常,
18-磁盘满负荷异常报警,19-音频丢失，20-开启录像，21-关闭录像，22-车辆检测算法异常，23-脉冲报警,24-人脸库硬盘异常,25-人脸库变更,26-人脸库图片变更,27-POC异常,28-相机视角异常*/
void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
	if (!g_InitStatus)
		return;

	switch (lCommand)
	{
	case COMM_ALARM_V30:
	{
		NET_DVR_ALARMINFO_V30 struAlarmInfoV30;
		memcpy(&struAlarmInfoV30, pAlarmInfo, sizeof(NET_DVR_ALARMINFO_V30));

		switch (struAlarmInfoV30.dwAlarmType)
		{
		case 0://signal alarm
			cout << "信号量报警" << endl;
			break;
		case 1://hard disk full alarm
			cout << "硬盘满报警" << endl;
			break;
		case 2://video loss alarm
		{
			cout << "信号丢失" << endl;
			CString strStatus, strTemp;
			for (int i = 0; i < MAX_CHANNUM_V30; i++)
			{
				BYTE bStatus = struAlarmInfoV30.byChannel[i];
				if (bStatus)
				{
					CString strChannel, strSql;
					strChannel.Format("%d", i + 1);
					CString strAddr = pAlarmer->sDeviceIP;
					strAddr.Trim();
					HKVideo vFind = { 0 };
					if (FindHKVideo(strAddr, strChannel, vFind))
					{
						InsertAlarmToDB(strAddr, strChannel, vFind.name, 2);
						cout << " 设备IP " << pAlarmer->sDeviceIP << " 通道 " << strChannel << " 名称 " << vFind.name << endl;
					}
				}				
			}
		}
			break;
		case 3:
			cout << "移动侦测" << endl;
			for (int i = 0; i < MAX_CHANNUM_V30; i++)
			{
				BYTE bStatus = struAlarmInfoV30.byChannel[i];
				if (bStatus)
				{
					CString strChannel, strSql;
					strChannel.Format("%d", i + 1);
					CString strAddr = pAlarmer->sDeviceIP;
					strAddr.Trim();
					HKVideo vFind = { 0 };
					if (FindHKVideo(strAddr, strChannel, vFind))
					{
						InsertAlarmToDB(strAddr, strChannel, vFind.name, 3);
						cout << " 设备IP " << pAlarmer->sDeviceIP << " 通道 " << strChannel << " 名称 " << vFind.name << endl;
					}
				}
			}			
			break;
		case 6:
			cout << "遮挡报警" << endl;
			for (int i = 0; i < MAX_CHANNUM_V30; i++)
			{
				BYTE bStatus = struAlarmInfoV30.byChannel[i];
				if (bStatus)
				{
					CString strChannel, strSql;
					strChannel.Format("%d", i + 1);
					CString strAddr = pAlarmer->sDeviceIP;
					strAddr.Trim();
					HKVideo vFind = { 0 };
					if (FindHKVideo(strAddr, strChannel, vFind))
					{
						InsertAlarmToDB(strAddr, strChannel, vFind.name, 6);
						cout << " 设备IP " << pAlarmer->sDeviceIP << " 通道 " << strChannel << " 名称 " << vFind.name << endl;
					}
				}
			}
			break;
		case 9:
			cout << "视频信号异常" << endl;
			for (int i = 0; i < MAX_CHANNUM_V30; i++)
			{
				BYTE bStatus = struAlarmInfoV30.byChannel[i];
				if (bStatus)
				{
					CString strChannel, strSql;
					strChannel.Format("%d", i + 1);
					CString strAddr = pAlarmer->sDeviceIP;
					strAddr.Trim();
					HKVideo vFind = { 0 };
					if (FindHKVideo(strAddr, strChannel, vFind))
					{
						InsertAlarmToDB(strAddr, strChannel, vFind.name, 9);
						cout << " 设备IP " << pAlarmer->sDeviceIP << " 通道 " << strChannel << " 名称 " << vFind.name << endl;
					}
				}
			}
			break;
		default:
			break;
		}
	}
	break;
	default:
		break;
	}
	return;
}


void ConnectIBSDevice()
{
	AlarmDevices.RemoveAll();

	cout << "准备重连 IBS Event" << endl;

	CDataBaseMgr db;
	if (db.Init())
	{
		m_HKVideos.RemoveAll();
		db.GetHKVideos(m_HKVideos);
		cout << "视频总数 " << m_HKVideos.GetCount() << endl;
		m_HKDevices.RemoveAll();
		db.GetHKDevices(m_HKDevices);
		for (int i = 0; i < m_HKDevices.GetCount(); i++)
		{
			HKDevice dev = m_HKDevices.GetAt(i);

			DEVICE_INFO devHandle;
			NET_DVR_DEVICEINFO_V30 struLocalDeviceInfo;
			LONG lUserId = -1;
			DWORD Err = 0;
			memset(&struLocalDeviceInfo, 0, sizeof(struLocalDeviceInfo));
			cout << "登陆设备 " << dev.szIP << endl;
			lUserId = NET_DVR_Login_V30(dev.szIP, (WORD)dev.port, dev.user, dev.password, &struLocalDeviceInfo);
			if (lUserId < 0)
			{
				Err = NET_DVR_GetLastError();
				if (Err == NET_DVR_PASSWORD_ERROR)
				{
					cout << "User name password is incorrect!" << endl;
					continue;
				}
				else
				{
					cout << "登陆失败" << endl;
					CString strSql;
					strSql.Format("UPDATE ibs_cctvunit SET status = 0 WHERE ip = \'%s\' ", dev.szIP);
					cout << strSql << endl;
					db.ExcuteSQL(strSql);
					strSql.Format("UPDATE ibs_cctvunit_device SET alarm_status = 1,status = 0 WHERE ip = \'%s\' ", dev.szIP);
					cout << strSql << endl;
					db.ExcuteSQL(strSql);

					if (db.IsNotHandled(dev.szIP,"OnLineStatus"))
					{
						CString strName = "设备" + CString(dev.szIP);
						CString strTime = CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S");
						CString strAlarmDescription = "设备离线";
						strSql.Format("INSERT INTO ibs_alarm_record (item_name,resource_name,alarm_description,alarm_level,alarm_type,alarm_category,alarm_time,paramenter_code) VALUES(\'%s\',\'视频监控\',\'%s\',1,4,0,\'%s\',\'%s-%s\')", strName, strAlarmDescription, strTime, dev.szIP, "OnLineStatus");
						cout << strSql << endl;
						db.ExcuteSQL(strSql);
					}
					continue;
				}
			}

			CString strSql;
			strSql.Format("UPDATE ibs_cctvunit SET status = 1 WHERE ip = \'%s\' ", dev.szIP);
			cout << strSql << endl;
			db.ExcuteSQL(strSql);
			strSql.Format("UPDATE ibs_cctvunit_device SET alarm_status = 0,status = 1 WHERE ip = \'%s\' ", dev.szIP);
			cout << strSql << endl;
			db.ExcuteSQL(strSql);

			LONG lFortifyHandle = NET_DVR_SetupAlarmChan_V30(lUserId);
			if (lFortifyHandle == -1)
			{
				cout << "Set Alarm Fail " << dev.szIP <<endl;
			}
			devHandle.nAlarmHandle = lFortifyHandle;
			devHandle.nlLoginHandle = lUserId;
			strcpy_s(devHandle.szAddr, dev.szIP);
			AlarmDevices.Add(devHandle);
		}
	}
	db.Release();
	g_InitStatus =  TRUE;
}

void DisConnectIBSDevice()
{
	cout << "即将断开所有设备 " << endl;
	for (int i=0;i<AlarmDevices.GetCount();i++)
	{
		DeviceHandle it = AlarmDevices.GetAt(i);
		if (it.nAlarmHandle !=-1)
		{
			cout << "撤防 " << it.szAddr << endl;
			NET_DVR_CloseAlarmChan_V30(it.nAlarmHandle);
		}
		if (it.nlLoginHandle !=-1)
		{
			cout << "登出 " << it.szAddr << endl;
			NET_DVR_Logout(it.nlLoginHandle);
		}
		
	}
	AlarmDevices.RemoveAll();
	g_InitStatus = FALSE;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
			CoInitializeEx(NULL, COINIT_MULTITHREADED);

			NET_DVR_Init();  // init DVR

			NET_DVR_SetDVRMessageCallBack_V30(MessageCallback, NULL);

			while (true)
			{
				ConnectIBSDevice();
				Sleep(15000);
				DisConnectIBSDevice();
			}

			NET_DVR_Cleanup();
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
