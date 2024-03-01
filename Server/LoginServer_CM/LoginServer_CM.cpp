#include "LoginServer_CM.h"

#include <limits>

LoginServer_CM* LoginServer_CM::m_pThis = NULL;
TemplateManager	g_csTemplateManager;

LoginServer_CM::LoginServer_CM()
	: ServerBase_CM()
{
	m_pThis = this;

	m_listSession = NULL;
	m_iSessionCount = 0;

	m_iAuthKeyIndex = 0;

	m_iGlobalSvrIndex = 0;
	m_pGlobalSvrSocket = NULL;

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
	ZeroMemory(m_iRecommendMainSvrID, sizeof(m_iRecommendMainSvrID));
	
	m_iSvrUpdateTime = 0;
	m_iSvrConnectRetryTime = 0;

	m_pPoolClientSession = NULL;
}

LoginServer_CM::~LoginServer_CM()
{
}

BOOL	LoginServer_CM::Initialize(UINT32 svrID)
{
	if (ServerBase_CM::Initialize(svrID) == FALSE)
	{
		return FALSE;
	}

	g_pWSEngine = new PxWebSocketEngine;

	m_pPoolClientSession = new _MemoryPool<stClientSession>(2048, "stClientSession");

	g_pIOCPEngine->m_strDebugFileNameHeader = "login_cm";

	// CB setting
	g_pIOCPEngine->m_pSocketConnCB = OnConnectSocket;
	g_pIOCPEngine->m_pSocketDisconnCB = OnCloseSocket;
	g_pIOCPEngine->m_pDBServerLostCB = OnDBServerLost;

	g_pWSEngine->m_pWSSessionConnCB = OnConnectWSSession;
	g_pWSEngine->m_pWSSessionCloseCB = OnCloseWSSession;
	g_pWSEngine->m_iEndMarkerVal = g_pIOCPEngine->m_iEndMarkerVal;

	InitPacketClientCBFunction();
	InitPacketServerCBFunction();

	g_pIOCPEngine->StartEngineV2(0, m_iMySvrPort[1], 0);
	g_pWSEngine->Init(m_iMySvrPort[0]);

	InitServerInfos();

	if (!ConnectServers())
	{
		printf("ConnectServers Failed\n");
		return FALSE;
	}

	if (!g_csTemplateManager.Initialize())
	{
		printf("g_csTemplateManager.Initialize Failed\n");
	}

	g_csGraphQLManager.Init_NoAlloc();

	// Auth Key Table Init
	srand(timeGetTime());
	for (int i = 0; i < AUTH_KEY_TABLE_COUNT; ++i)
	{
		m_pAuthKeyTable[i] = rand();
	}

	InitDBCashes();
	
	return TRUE;
}

void	LoginServer_CM::Release()
{
	ServerBase_CM::Release();
	g_csTemplateManager.Release();
	g_csGraphQLManager.Release();

	if (g_pWSEngine)
	{
		g_pWSEngine->Release();

		delete g_pWSEngine;
		g_pWSEngine = NULL;
	}

	HASHMAP_UI64::iterator	iter = m_csAdminClientSession.m_HashMapUI64.begin();
	while (iter != m_csAdminClientSession.m_HashMapUI64.end())
	{
		stClientSession* pSession = (stClientSession*)((*iter).second);
		m_pPoolClientSession->Free(pSession);
		++iter;
	}
	m_csAdminClientSession.RemoveObjectAll();
	m_listSession = NULL;
	m_iSessionCount = 0;

	if (m_pPoolClientSession) { delete	m_pPoolClientSession; m_pPoolClientSession = NULL; }
}

void	LoginServer_CM::Update()
{
	ServerBase_CM::Update();

	// alive check
	EnterCriticalSection(&g_pWSEngine->m_csListWSSession_CritSec);
	std::list<SPWSSession>::iterator	itor_ws = g_pWSEngine->m_listWSSession.begin();
	while (itor_ws != g_pWSEngine->m_listWSSession.end())
	{
		std::list<SPWSSession>::iterator itor_ws_next = std::next(itor_ws, 1);
		SPWSSession		pWS = *itor_ws;
		stClientSession* pSession = (stClientSession*)pWS->m_pData;
		if (pSession)
		{
			pSession->aliveCheckTick += m_iCurTickDiff;
			if (pSession->aliveCheckCount == 0)
			{
				//if (pSession->aliveCheckTick > 30000)		// 30초
				if (pSession->aliveCheckTick > 300000)		// 300초
				{
					PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWS);
					if (pToClient != nullptr)
					{
						pToClient->WriteUI8(1);
						g_pWSEngine->SendBuffer(pToClient, PK_ALIVE_CHECK);
					}

					++pSession->aliveCheckCount;
					pSession->aliveCheckTick = 0;
				}
			}
			else
			{
				if (pSession->aliveCheckCount < 4)
				{
					//if (pSession->aliveCheckTick > 10000)		// 10초
					if (pSession->aliveCheckTick > 100000)		// 100초
					{
						PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWS);
						if (pToClient != nullptr)
						{
							pToClient->WriteUI8(1);
							g_pWSEngine->SendBuffer(pToClient, PK_ALIVE_CHECK);
						}

						++pSession->aliveCheckCount;
						pSession->aliveCheckTick = 0;
					}
				}
				else
				{
					if (pSession->isRequestDisconn == 0)
					{
						pWS->Close();
						pSession->isRequestDisconn = 1;
					}
				}
			}
		}
		itor_ws = itor_ws_next;
	}
	LeaveCriticalSection(&g_pWSEngine->m_csListWSSession_CritSec);

	g_pWSEngine->ProcRcvBuf_Client();
	g_pIOCPEngine->ProcRcvBuf_Server();
	g_csGraphQLManager.Update();
	
	g_pWSEngine->ProcWSSessionDisconnect();
	g_pIOCPEngine->ProcSocketDisconnect_Server();
	
	UpdateSvrInfos();

	ServerBase_CM::PostUpdate();
}

BOOL	LoginServer_CM::OnConnectSocket(PVOID	pData)			// WebSocket 사용하면 여기로는 클라이언트 안탄다.. 서버간 연결만 탄다.
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	if (pSocket->bInsideConnect == FALSE)
	{
	}

	return TRUE;
}

BOOL	LoginServer_CM::OnCloseSocket(PVOID	pData)				// WebSocket 사용하면 여기로는 클라이언트 안탄다.. 서버간 연결만 탄다.
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	if (pSocket->bInsideConnect)
	{
		int svrIndex = pSocket->SKUID;
		if (m_pThis->m_aServerInfo[svrIndex].type == SVR_MAIN)
		{
			stMainServerInfo* pMainSvrInfo = (stMainServerInfo*)pSocket->pData;
			if (pMainSvrInfo)
			{
				pMainSvrInfo->pSocket = NULL;
			}
		}
		else if (m_pThis->m_aServerInfo[svrIndex].type == SVR_GLOBAL)
		{
			m_pThis->m_pGlobalSvrSocket = NULL;
		}
	}
	else
	{
		if (pSocket->pData)
		{
			stClientSession* pSession = (stClientSession*)pSocket->pData;

			m_pThis->FreeClientSession(pSession);
			pSocket->pData = NULL;
		}
	}

	return TRUE;
}

BOOL	LoginServer_CM::OnConnectWSSession(SPWSSession pWSSession)
{
	stClientSession* pSession = m_pThis->GetClientSession();
	pWSSession->m_pData = (PVOID)pSession;
	pSession->pWSSession = pWSSession;

	return TRUE;
}

BOOL	LoginServer_CM::OnCloseWSSession(SPWSSession pWSSession)
{
	if (pWSSession->m_pData)
	{
		stClientSession* pSession = (stClientSession*)pWSSession->m_pData;

		m_pThis->FreeClientSession(pSession);
		pWSSession->m_pData = NULL;
	}

	return TRUE;
}

stClientSession* LoginServer_CM::GetClientSession()
{
	stClientSession* newobj = m_pPoolClientSession->Allocate();
	newobj->Init();

	newobj->prev = NULL;
	newobj->next = m_listSession;
	if (m_listSession)	m_listSession->prev = newobj;
	m_listSession = newobj;
	++m_iSessionCount;

	return newobj;
}

void	LoginServer_CM::FreeClientSession(stClientSession* pSession)
{
	// 다른 캐릭터들에게 캐릭터 떠남 전파
	if (pSession == m_listSession)
	{
		m_listSession = pSession->next;
		if (m_listSession) m_listSession->prev = NULL;
	}
	else
	{
		if (pSession->prev) pSession->prev->next = pSession->next;
		if (pSession->next) pSession->next->prev = pSession->prev;
	}
	--m_iSessionCount;

	FreeClientSessionData(pSession);

	m_pPoolClientSession->Free(pSession);
}

void	LoginServer_CM::FreeClientSessionData(stClientSession* pSession)
{
	pSession->pWSSession = nullptr;
}

stClientSession* LoginServer_CM::GetConnectedClientSession(UINT64 ACGUID)
{
	stClientSession* pResultSession = (stClientSession*)m_csAdminClientSession.GetObjectByKey(ACGUID);
	return pResultSession;
}

BOOL	LoginServer_CM::OnDBServerLost(PVOID	pData)
{
	g_pIOCPEngine->dbgprint("OnDBServerLost\n");

	return TRUE;
}

void	LoginServer_CM::InitServerInfos()
{
	for (UINT32 i = 0; i < m_iServerNum; ++i)
	{
		int a = m_aServerInfo[i].svrRegion;
		int b = m_aServerInfo[i].svrGroup;
		int c = m_aServerInfo[i].svrIndex;

		if (m_aServerInfo[i].type == SVR_MAIN)
		{
			m_stMainSvrInfos[a][b][c].iConnectCount = 0;
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
	}
}

BOOL	LoginServer_CM::ConnectServers()
{
	if (m_iCurrentTime - m_iSvrConnectRetryTime > SERVER_RECONNECT_TRY_TIME)
	{
		m_iSvrConnectRetryTime = m_iCurrentTime;

		for (UINT32 i = 0; i < m_iServerNum; ++i)
		{
			if (m_aServerInfo[i].type == SVR_GLOBAL)
			{
				if (m_pGlobalSvrSocket == NULL)
				{
					m_pGlobalSvrSocket = g_pIOCPEngine->ConnectTo(m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server, SVR_LOGIN, m_iMySvrID);

					if (!m_pGlobalSvrSocket)
					{
						printf("Connect Servers Faild index:%d , IP:%s,port:%d \n", m_aServerInfo[i].index, m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server);
						continue;
					}
					else
					{
						m_iGlobalSvrIndex = i;
						m_pGlobalSvrSocket->SKUID = i;
					}
				}
			}
		}
	}

	return	TRUE;
}

UINT32	LoginServer_CM::GetNextAuthKey()
{
	UINT32 retVal = m_pAuthKeyTable[m_iAuthKeyIndex++];
	if (m_iAuthKeyIndex >= AUTH_KEY_TABLE_COUNT)	m_iAuthKeyIndex = 0;
	return retVal;
}

UINT32	LoginServer_CM::FindBestMainSvrID(UINT8	svrRegion)
{
	UINT32	bestID = 0;
	UINT32	minUser = SERVER_MAXUSER;

	for (int svrGroup = 0; svrGroup < m_iMainSvrGroupCount[svrRegion]; ++svrGroup)
	{
		for (int svrIdx = 0; svrIdx < m_iMainSvrIdxCount[svrRegion][svrGroup]; ++svrIdx)
		{
			if (m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].pSocket != NULL && m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].iConnectCount < minUser)
			{
				bestID = m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].pServerInfo->svrID;
				minUser = m_stMainSvrInfos[svrRegion][svrGroup][svrIdx].iConnectCount;
			}
		}
	}
	return bestID;
}

void	LoginServer_CM::UpdateSvrInfos()
{
	if (m_iCurrentTime - m_iSvrUpdateTime > SERVER_INFO_UPDATE_REQUEST_TIME)
	{
		m_iSvrUpdateTime = m_iCurrentTime;

		for (int i = 0; i < m_iMainSvrRegionCount; ++i)
		{
			m_iRecommendMainSvrID[i] = FindBestMainSvrID((UINT8)i);
		}

		HASHMAP_UI32::iterator itor_mainSvrInfo = m_csAdminMainSvrInfo.m_HashMapUI32.begin();
		while (itor_mainSvrInfo != m_csAdminMainSvrInfo.m_HashMapUI32.end())
		{
			stMainServerInfo* pMainSvrInfo = (stMainServerInfo*)itor_mainSvrInfo->second;
			if (pMainSvrInfo->pSocket != NULL)
			{
				PX_BUFFER* pToMainSvr = PxObjectManager::GetSendBufferObj(pMainSvrInfo->pSocket);
				if (pToMainSvr)
				{
					g_pIOCPEngine->SendBuffer(pToMainSvr, PKS_MAIN_SERVER_INFO);
				}
			}
			++itor_mainSvrInfo;
		}
	}
}