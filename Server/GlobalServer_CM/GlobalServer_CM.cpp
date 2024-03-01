#include "GlobalServer_CM.h"

#include <limits>

#include <boost/algorithm/string.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

GlobalServer_CM* GlobalServer_CM::m_pThis = NULL;
TemplateManager	g_csTemplateManager;

GlobalServer_CM::GlobalServer_CM()
	: ServerBase_CM()
{
	m_pThis = this;

	m_iGUIDChunkCountResetSkipCount = 0;

	m_pLoginSvrSocket = NULL;

	m_iMainSvrInfoUpdateTime = 0;
	m_iMainSvrRegionCount = 0;
	ZeroMemory(m_iMainSvrGroupCount, sizeof(m_iMainSvrGroupCount));
	ZeroMemory(m_iMainSvrIdxCount, sizeof(m_iMainSvrIdxCount));
	for (int i = 0; i < 16; ++i)
	{
		for (int j = 0; j < 32; ++j)
		{
			for (int k = 0; k < 128; ++k)
			{
				m_stMainSvrInfos[i][i][k].Init();
			}
		}
	}

	m_iZoneSvrRegionCount = 0;
	ZeroMemory(m_iZoneSvrGroupCount, sizeof(m_iZoneSvrGroupCount));
	ZeroMemory(m_iZoneSvrIdxCount, sizeof(m_iZoneSvrIdxCount));
	for (int i = 0; i < 16; ++i)
	{
		for (int j = 0; j < 32; ++j)
		{
			for (int k = 0; k < 128; ++k)
			{
				m_stZoneSvrInfos[i][i][k].Init();
			}
		}
	}
}

GlobalServer_CM::~GlobalServer_CM()
{
}

BOOL	GlobalServer_CM::Initialize(UINT32 svrID)
{
	if (ServerBase_CM::Initialize(svrID) == FALSE)
	{
		return FALSE;
	}

	g_pIOCPEngine->m_strDebugFileNameHeader = "global_cm";

	// CB setting
	g_pIOCPEngine->m_pSocketConnCB = OnConnectSocket;
	g_pIOCPEngine->m_pSocketDisconnCB = OnCloseSocket;
	g_pIOCPEngine->m_pDBServerLostCB = OnDBServerLost;

	InitPacketServerCBFunction();

	g_pIOCPEngine->StartEngineV2(0, m_iMySvrPort[1], 0);

	SetServerInfos();

	UINT64		elapsedSec = (UINT64)m_iCurrentTime - GUID_SERVER_START_SEC;
	m_iGUIDTimeStamp = (UINT32)(elapsedSec / (UINT64)GUID_SERVER_TIME_STAMP_ADD_SEC);
	m_iLastGUIDTimeStamp = m_iGUIDTimeStamp;
	m_iGUIDChunkCount = DBGetChunkCount(m_iGUIDTimeStamp);

	if (!g_csTemplateManager.Initialize())
	{
		printf("g_csTemplateManager.Initialize Failed\n");
	}

	InitDBCashes();

	return TRUE;
}

void	GlobalServer_CM::Release()
{
	ServerBase_CM::Release();
	g_csTemplateManager.Release();

}

void	GlobalServer_CM::Update()
{
	ServerBase_CM::Update();

	UINT64		elapsedSec = (UINT64)m_iCurrentTime - GUID_SERVER_START_SEC;
	UINT32		timeStamp = (UINT32)(elapsedSec / (UINT64)GUID_SERVER_TIME_STAMP_ADD_SEC);

	if (timeStamp != m_iLastGUIDTimeStamp)
	{
		if (m_iGUIDChunkCountResetSkipCount <= 0)
		{
			m_iGUIDTimeStamp = timeStamp;
			m_iLastGUIDTimeStamp = timeStamp;
			m_iGUIDChunkCount = DBGetChunkCount(timeStamp);
		}
		else
		{
			m_iLastGUIDTimeStamp = timeStamp;
			--m_iGUIDChunkCountResetSkipCount;
		}
	}

	UpdateMainSvrInfo();

	g_pIOCPEngine->ProcRcvBuf_Server();
	g_pIOCPEngine->ProcSocketDisconnect_Server();

	ServerBase_CM::PostUpdate();
}

BOOL	GlobalServer_CM::OnConnectSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	return TRUE;
}

BOOL	GlobalServer_CM::OnCloseSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	if (pSocket->bInsideConnect)
	{
		HASHMAP_UI32::iterator itor_svrSocket = m_pThis->m_csAdminSvrSocket.m_HashMapUI32.begin();
		while (itor_svrSocket != m_pThis->m_csAdminSvrSocket.m_HashMapUI32.end())
		{
			PX_SOCKET* pSvrSocket = (PX_SOCKET*)itor_svrSocket->second;
			if (pSvrSocket == pSocket)
			{
				m_pThis->m_csAdminSvrSocket.RemoveObject(itor_svrSocket->first);
				break;
			}
			++itor_svrSocket;
		}

		int svrIndex = pSocket->SKUID;
		if (m_pThis->m_aServerInfo[svrIndex].type == SVR_LOGIN)
		{
			m_pThis->m_iLoginSvrIndex = 0;
			m_pThis->m_pLoginSvrSocket = NULL;
		}
		else if (m_pThis->m_aServerInfo[svrIndex].type == SVR_MAIN)
		{

		}
	}

	return TRUE;
}

BOOL	GlobalServer_CM::OnDBServerLost(PVOID	pData)
{
	g_pIOCPEngine->dbgprint("OnDBServerLost\n");

	return TRUE;
}

void	GlobalServer_CM::SetServerInfos()
{
	for (UINT32 i = 0; i < m_iServerNum; ++i)
	{
		int a = m_aServerInfo[i].svrRegion;
		int b = m_aServerInfo[i].svrGroup;
		int c = m_aServerInfo[i].svrIndex;

		if (m_aServerInfo[i].type == SVR_MAIN)
		{
			m_stMainSvrInfos[a][b][c].pSocket = NULL;
			m_stMainSvrInfos[a][b][c].pServerInfo = &m_aServerInfo[i];
			m_csAdminMainSvrInfo.AddObject(&m_stMainSvrInfos[a][b], m_aServerInfo[i].svrID);

			if (m_aServerInfo[i].svrRegion + 1 > m_iMainSvrRegionCount)
			{
				m_iMainSvrRegionCount = m_aServerInfo[i].svrRegion + 1;
			}

			if (m_aServerInfo[i].svrGroup + 1 > m_iMainSvrGroupCount[a])
			{
				m_iMainSvrGroupCount[a] = m_aServerInfo[i].svrGroup + 1;
			}

			if (m_aServerInfo[i].svrIndex + 1 > m_iMainSvrIdxCount[a][b])
			{
				m_iMainSvrIdxCount[a][b] = m_aServerInfo[i].svrIndex + 1;
			}
		}
		else if (m_aServerInfo[i].type == SVR_ZONE)
		{
			m_stZoneSvrInfos[a][b][c].pSocket = NULL;
			m_stZoneSvrInfos[a][b][c].pServerInfo = &m_aServerInfo[i];
			m_csAdminZoneSvrInfo.AddObject(&m_stZoneSvrInfos[a][b], m_aServerInfo[i].svrID);

			if (m_aServerInfo[i].svrRegion + 1 > m_iZoneSvrRegionCount)
			{
				m_iZoneSvrRegionCount = m_aServerInfo[i].svrRegion + 1;
			}

			if (m_aServerInfo[i].svrGroup + 1 > m_iZoneSvrGroupCount[a])
			{
				m_iZoneSvrGroupCount[a] = m_aServerInfo[i].svrGroup + 1;
			}

			if (m_aServerInfo[i].svrIndex + 1 > m_iZoneSvrIdxCount[a][b])
			{
				m_iZoneSvrIdxCount[a][b] = m_aServerInfo[i].svrIndex + 1;
			}
		}
	}
}

BOOL	GlobalServer_CM::ConnectServers()
{

	return	TRUE;
}

void	GlobalServer_CM::ProcessChunkCountAdd()
{
	if (m_iGUIDChunkCount == GUID_SERVER_CHUNK_COUNT_MAX)
	{
		++m_iGUIDTimeStamp;
		m_iGUIDChunkCount = 0;
		++m_iGUIDChunkCountResetSkipCount;

		DBInsertChunkCount(m_iGUIDTimeStamp, m_iGUIDChunkCount);
	}
	else
	{
		++m_iGUIDChunkCount;
		DBUpdateChunkCount(m_iGUIDTimeStamp, m_iGUIDChunkCount);
	}
}

void	GlobalServer_CM::SendTimeTagInfoAll()
{
	std::list<PX_SOCKET*>::iterator itor_sock = g_pIOCPEngine->m_listServerSocket.begin();
	while (itor_sock != g_pIOCPEngine->m_listServerSocket.end())
	{
		SendTimeTagInfo(*itor_sock);
		++itor_sock;
	}
}

void	GlobalServer_CM::SendTimeTagInfo(PX_SOCKET* pSocket)
{
	ProcessChunkCountAdd();
	UINT32	timeTag = m_iGUIDTimeStamp << 12 | m_iGUIDChunkCount;

	PX_BUFFER* pToSvr = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvr)
	{
		pToSvr->WriteUI32(timeTag);
		g_pIOCPEngine->SendBuffer(pToSvr, PKS_INFORM_TIME_TAG);
	}
}

void	GlobalServer_CM::UpdateMainSvrInfo()
{
	if (m_iCurrentTime - m_iMainSvrInfoUpdateTime > SERVER_INFO_UPDATE_REQUEST_TIME)
	{
		m_iMainSvrInfoUpdateTime = m_iCurrentTime;

		for (int svrRegion = 0; svrRegion < m_iMainSvrRegionCount; ++svrRegion)
		{
			BOOL bFindRegionRole = FALSE;
			for (int svrGroup = 0; svrGroup < m_iMainSvrGroupCount[svrRegion]; ++svrGroup)
			{
				for (int svrIdx = 0; svrIdx < m_iMainSvrIdxCount[svrRegion][svrGroup]; ++svrIdx)
				{
					if (m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].isRegionServerRole == TRUE)
					{
						if (m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].pSocket != NULL)
						{
							bFindRegionRole = TRUE;
							break;
						}
						else
						{
							m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].isRegionServerRole = FALSE;
						}
					}
				}
			}

			if (bFindRegionRole == FALSE)
			{
				for (int svrGroup = 0; svrGroup < m_iMainSvrGroupCount[svrRegion]; ++svrGroup)
				{
					for (int svrIdx = 0; svrIdx < m_iMainSvrIdxCount[svrRegion][svrGroup]; ++svrIdx)
					{
						if (m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].pSocket != NULL)
						{
							m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].isRegionServerRole = TRUE;

							PX_BUFFER* pToMainSvr = PxObjectManager::GetSendBufferObj(m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].pSocket);
							if (pToMainSvr)
							{
								pToMainSvr->WriteUI8(1);
								g_pIOCPEngine->SendBuffer(pToMainSvr, PKS_SET_REGION_SERVER_ROLE);
							}
						}
					}
				}
			}
		}
	}
}