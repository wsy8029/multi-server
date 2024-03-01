#include "ServerBase_CM.h"

ServerBase_CM* ServerBase_CM::m_pThisBase = NULL;

ServerBase_CM::ServerBase_CM()
{
	m_pThisBase = this;
	m_bLoopStop = FALSE;
	m_iServerNum = 0;
	ZeroMemory(m_aServerInfo, sizeof(m_aServerInfo));
	
	m_iMySvrID = 0;
	m_iMySvrIndex = 0;
	m_iMySvrRegion = 0;
	m_iMySvrGroup = 0;
	ZeroMemory(m_iMySvrPort, sizeof(m_iMySvrPort));

	m_iCurrentTime = 0;
	ZeroMemory(m_strCurrentTime, sizeof(m_strCurrentTime));
	m_iCurTick = 0;
	m_iCurTickDiff = 0;

	m_pDriver = NULL;
}

ServerBase_CM::~ServerBase_CM()
{

}

BOOL	ServerBase_CM::Initialize(UINT32 svrID)
{
	g_pIOCPEngine = new	PxIOCPEngine;

	m_pDriver = get_driver_instance();
	m_csDBConn[DB_COMMON].Init(m_pDriver, DB_COMMON, "../Data_Setting/ServerSetting.json", "01234567896543210123456789123456", "0123456789ABCDEF", FPPrepareDBCommands);

	if (!GetServerIPTables())
	{
		printf("GetServerIPTables Failed\n");
		delete g_pIOCPEngine;
		return FALSE;																	// 종료하자^^
	}

	PxFrameMemory::InitManager(1024 * 1024 * 1);										// 1 MB

	m_csGUIDFactory.m_strGUIDDBTable = "2_guidtimechunk";
	m_csGUIDFactory.Init(&m_csDBConn[DB_COMMON]);

	std::list<std::string> listIP;
	g_pIOCPEngine->GetLocalIPAddress(&listIP);

	BOOL	bFindMyServerInfo = FALSE;
	for (UINT32 i = 0; i < m_iServerNum; ++i)
	{
		if (m_aServerInfo[i].svrID == svrID)
		{
			std::list<std::string>::iterator itor_ip = listIP.begin();
			while (itor_ip != listIP.end())
			{
				std::string localIP = *itor_ip;
				if (strcmp(localIP.c_str(), m_aServerInfo[i].private_ip) == 0)
				{
					bFindMyServerInfo = TRUE;
					
					m_iMySvrPort[0] = m_aServerInfo[i].port_client_TCP;
					m_iMySvrPort[1] = m_aServerInfo[i].port_server;
					g_pIOCPEngine->m_iEndMarkerVal = m_aServerInfo[i].endMarker;
					m_iMySvrID = m_aServerInfo[i].svrID;
					m_iMySvrIndex = (UINT16)i;
					m_iMySvrRegion = m_aServerInfo[i].svrRegion;
					m_iMySvrGroup = m_aServerInfo[i].svrGroup;
					break;
				}
				++itor_ip;
			}
			if (bFindMyServerInfo)	break;
		}
	}

	g_pIOCPEngine->log.Init(svrID);

	if (bFindMyServerInfo == FALSE)
	{
		printf("bFindMyServerInfo Failed\n");

#ifdef PACKET_LOG
		g_pIOCPEngine->log.Print("Failed - MyServerInfo \n");
		std::string localip = "Local IP : ";
		std::list<std::string>::iterator itor_ip = listIP.begin();
		while (itor_ip != listIP.end())
		{
			localip += " : " + *itor_ip;
			++itor_ip;
		}
		localip += "\n";
		g_pIOCPEngine->log.Print(localip);

		std::string Infoip = "DB Info IP : \n";
		for (UINT32 i = 0; i < m_iServerNum; ++i)
		{
			Infoip += g_pIOCPEngine->log.FString("- SERVER_TYPE : %d , Private_ip : %s \n", m_aServerInfo[i].type, m_aServerInfo[i].private_ip);
		}
		g_pIOCPEngine->log.Print(Infoip);
#endif

		delete g_pIOCPEngine;
		g_pIOCPEngine = NULL;
		return FALSE;
	}

	SetPacketVersion_Server(g_pIOCPEngine->m_iPacketServerVersion);
	SetPacketVersion_Client(g_pIOCPEngine->m_iPacketClientVersion);
	SetPacketVersion_Client_UDP(g_pIOCPEngine->m_iPacketClientUDPVersion);

	time(&m_iCurrentTime);

	g_pIOCPEngine->m_pDBServerLostCB = OnDBServerLost;

	return TRUE;
}

void	ServerBase_CM::Release()
{
	PxFrameMemory::Release();

	m_csGUIDFactory.Release();

	if (g_pIOCPEngine)
	{
		g_pIOCPEngine->ReleaseEngine();

		delete	g_pIOCPEngine;
		g_pIOCPEngine = NULL;
	}

	ReleaseDBCommands();

	// disconnect from server, terminate client library 
	for (int i = 0; i < DB_CONN_COUNT; ++i)
	{
		m_csDBConn[i].Destroy();
	}

	PxMemoryManager::DestroyInstance();
}

void	ServerBase_CM::Update()
{
	static bool isFirstTime = true;

	time(&m_iCurrentTime);		
	localtime_s(&m_tmCurrentTime, &m_iCurrentTime);
	strftime(m_strCurrentTime, 128, "%Y-%m-%d %H:%M:%S", &m_tmCurrentTime);			// for db save

	DWORD	curTick = timeGetTime();
	if (isFirstTime)
	{
		m_iCurTickDiff = 0;
		m_iCurTick = curTick;
		isFirstTime = false;
	}
	else
	{
		m_iCurTickDiff = curTick - m_iCurTick;
		m_iCurTick = curTick;
	}

	for (int i = 0; i < DB_CONN_COUNT; ++i)
	{
		m_csDBConn[i].Update();
	}

}

void	ServerBase_CM::PostUpdate()
{
	if (g_pIOCPEngine->m_bEnd)		// 종료 처리!
	{
		m_bLoopStop = TRUE;
	}

	PxFrameMemory::Clear();
}

BOOL	ServerBase_CM::OnDBServerLost(PVOID	pData)
{
	g_pIOCPEngine->dbgprint("OnDBServerLost\n");

	for (int i = 0; i < DB_CONN_COUNT; ++i)
	{
		m_pThisBase->m_csDBConn[i].FixConnection();
	}

	return TRUE;
}

BOOL	ServerBase_CM::FPPrepareDBCommands(THMySQLConnector* pConn)
{
	return m_pThisBase->PrepareDBCommands(pConn);
}

BOOL	ServerBase_CM::PrepareDBCommands(THMySQLConnector* pConn)
{
	ReleaseDBCommands();

	m_cmdGetServerIP1.PrepareStatement(pConn->m_pConn,"SELECT type, svrRegion, svrGroup, svrIndex, svrName, svrStatus, maintenanceEndTime, privateIP, publicIP, portClientTCP, portClientUDP, portServer, endMarker FROM " + pConn->m_strServerIPTable);

	return TRUE;
}

void	ServerBase_CM::ReleaseDBCommands()
{
	m_cmdGetServerIP1.Destroy();
}

BOOL	ServerBase_CM::GetServerIPTables()
{
	//서버 ip얻어오자
	sql::ResultSet* res = m_cmdGetServerIP1.ExecuteAndGetResultSet();
	if(res != NULL)
	{
		while (res->next())
		{
			stServerInfo* pSvrInfo = &m_aServerInfo[m_iServerNum];

			pSvrInfo->index = (UINT8)m_iServerNum;
			std::string svrType = res->getString(1).c_str();
			pSvrInfo->type = ParseServerType(svrType.c_str());
			pSvrInfo->svrRegion = (UINT8)res->getUInt(2);
			pSvrInfo->svrGroup = (UINT8)res->getUInt(3);
			pSvrInfo->svrIndex = (UINT8)res->getUInt(4);
			pSvrInfo->svrID = GetSvrID(pSvrInfo->type, pSvrInfo->svrRegion, pSvrInfo->svrGroup, pSvrInfo->svrIndex);
			strncpy_s(pSvrInfo->svrName_UTF8, res->getString(5).c_str(), 49);
			pSvrInfo->status = (SERVER_STATUS)res->getUInt(6);
			pSvrInfo->maintenanceEndTime = res->getString(7);
			strncpy_s(pSvrInfo->private_ip, res->getString(8).c_str(), 16);
			strncpy_s(pSvrInfo->public_ip, res->getString(9).c_str(), 16);
			pSvrInfo->port_client_TCP = (UINT16)res->getUInt(10);
			pSvrInfo->port_client_UDP = (UINT16)res->getUInt(11);
			pSvrInfo->port_server = (UINT16)res->getUInt(12);
			pSvrInfo->endMarker = (UINT16)res->getUInt(13);

			++m_iServerNum;
		}

		delete res;

		return TRUE;
	}

	return FALSE;
}

UINT16	ServerBase_CM::GetSvrIndexBySvrID(UINT32 svrID)
{
	for (int i = 0; i < (int)m_iServerNum; ++i)
	{
		if (m_aServerInfo[i].svrID == svrID)
		{
			return m_aServerInfo[i].svrIndex;
		}
	}
	return 0;
}

UINT8	ServerBase_CM::GetIndexBySvrID(UINT32 svrID)
{
	for (int i = 0; i < (int)m_iServerNum; ++i)
	{
		if (m_aServerInfo[i].svrID == svrID)
		{
			return m_aServerInfo[i].index;
		}
	}
	return 0;
}
