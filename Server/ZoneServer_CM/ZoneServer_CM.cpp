#include "ZoneServer_CM.h"

#include <limits>

ZoneServer_CM* ZoneServer_CM::m_pThis = NULL;
TemplateManager	g_csTemplateManager;

ZoneServer_CM::ZoneServer_CM()
	: ServerBase_CM()
{
	m_pThis = this;

	m_listSession = NULL;
	m_iSessionCount = 0;
	m_iChannelCount = 0;

	m_iGlobalSvrIndex = 0;
	m_pGlobalSvrSocket = NULL;

	m_iMainSvrIndex = 0;
	m_pMainSvrSocket = NULL;
	m_iSvrConnectRetryTime = 0;

	m_pPoolClientSession = NULL;
	m_pPoolPlayerCharacter = NULL;
	m_pPoolNPC_Equip = NULL;
	m_pPoolNPC = NULL;
	m_pPoolCharacterInfo = NULL;
	m_pPoolAccountItem = NULL;
	m_pPoolCharacterItem = NULL;
	m_pPoolItem = NULL;
	m_pPoolZone = NULL;
	m_pPoolChannel = NULL;
	m_pPoolSubZone = NULL;
}

ZoneServer_CM::~ZoneServer_CM()
{
}

BOOL	ZoneServer_CM::Initialize(UINT32 svrID)
{
	if (ServerBase_CM::Initialize(svrID) == FALSE)
	{
		return FALSE;
	}

	g_pWSEngine = new PxWebSocketEngine;

	m_pPoolClientSession = new _MemoryPool<stClientSession>(2048, "stClientSession");

	g_pIOCPEngine->m_strDebugFileNameHeader = "zone_cm";

	// CB setting
	g_pIOCPEngine->m_pSocketConnCB = OnConnectSocket;
	g_pIOCPEngine->m_pSocketDisconnCB = OnCloseSocket;
	g_pIOCPEngine->m_pDBServerLostCB = OnDBServerLost;

	g_pWSEngine->m_pWSSessionConnCB = OnConnectWSSession;
	g_pWSEngine->m_pWSSessionCloseCB = OnCloseWSSession;
	g_pWSEngine->m_iEndMarkerVal = g_pIOCPEngine->m_iEndMarkerVal;
	
	InitPacketClientCBFunction();
	InitPacketServerCBFunction();
	InitGraphQLCBFunction();

	g_pIOCPEngine->StartEngineV2(0, m_iMySvrPort[1], 0);
	g_pWSEngine->Init(m_iMySvrPort[0]);

	if (!ConnectServers())
	{
		printf("ConnectServers Failed\n");
		return FALSE;
	}

	m_pPoolPlayerCharacter = new _MemoryPool<stPlayerCharacter>(2048, "stPlayerCharacter");
	m_pPoolNPC_Equip = new _MemoryPool<stNPC_Equip>(512, "stNPC_Equip");
	m_pPoolNPC = new _MemoryPool<stNPC>(512, "stNPC");
	m_pPoolCharacterInfo = new _MemoryPool<stCharacterInfo>(2048, "stCharacterInfo");
	m_pPoolAccountItem = new _MemoryPool<stAccountItem>(1024, "stAccountItem");
	m_pPoolCharacterItem = new _MemoryPool<stCharacterItem>(1024, "stCharacterItem");
	m_pPoolItem = new _MemoryPool<stItem>(4096, "stItem");
	m_pPoolZone = new _MemoryPool<stZone>(32, "stZone");
	m_pPoolChannel = new _MemoryPool<stChannel>(64, "stChannel");
	m_pPoolSubZone = new _MemoryPool<stSubZone>(32, "stSubZone");

	if (!g_csTemplateManager.Initialize())
	{
		printf("g_csTemplateManager.Initialize Failed\n");
	}

	g_csGraphQLManager.Init("https://dev-api.luxrobo.link/graphql");

	InitDBCashes();
	InitGameDatas();

	return TRUE;
}

void	ZoneServer_CM::Release()
{
	ReleaseGameDatas();

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
	if (m_pPoolPlayerCharacter) { delete		m_pPoolPlayerCharacter; m_pPoolPlayerCharacter = NULL; }
	if (m_pPoolNPC_Equip) { delete		m_pPoolNPC_Equip; m_pPoolNPC_Equip = NULL; }
	if (m_pPoolNPC) { delete		m_pPoolNPC; m_pPoolNPC = NULL; }
	if (m_pPoolPlayerCharacter) { delete		m_pPoolPlayerCharacter; m_pPoolPlayerCharacter = NULL; }
	if (m_pPoolCharacterInfo) { delete	m_pPoolCharacterInfo; m_pPoolCharacterInfo = NULL; }
	if (m_pPoolAccountItem) { delete	m_pPoolAccountItem; m_pPoolAccountItem = NULL; }
	if (m_pPoolCharacterItem) { delete	m_pPoolCharacterItem; m_pPoolCharacterItem = NULL; }
	if (m_pPoolItem) { delete	m_pPoolItem; m_pPoolItem = NULL; }
	if (m_pPoolZone) { delete	m_pPoolZone; m_pPoolZone = NULL; }
	if (m_pPoolChannel) { delete	m_pPoolChannel; m_pPoolChannel = NULL; }
	if (m_pPoolSubZone) { delete	m_pPoolSubZone; m_pPoolSubZone = NULL; }
}

void	ZoneServer_CM::Update()
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

	// Server Connect 시도
	ConnectServers();

	std::list<stZone*>::iterator itor_zone = m_listZone.begin();
	while (itor_zone != m_listZone.end())
	{
		stZone* pZone = *itor_zone;
		UpdateZone(pZone);
		++itor_zone;
	}

	g_pWSEngine->ProcRcvBuf_Client();
	g_pIOCPEngine->ProcRcvBuf_Server();
	g_csGraphQLManager.Update();

	g_pWSEngine->ProcWSSessionDisconnect();
	g_pIOCPEngine->ProcSocketDisconnect_Server();

	ServerBase_CM::PostUpdate();
}

BOOL	ZoneServer_CM::OnConnectSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	return TRUE;
}

BOOL	ZoneServer_CM::OnCloseSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	if (pSocket->bInsideConnect)
	{
		int svrIndex = pSocket->SKUID;
		if (m_pThis->m_aServerInfo[svrIndex].type == SVR_MAIN)
		{
			m_pThis->m_pMainSvrSocket = NULL;
		}
		else if (m_pThis->m_aServerInfo[svrIndex].type == SVR_GLOBAL)
		{
			m_pThis->m_pGlobalSvrSocket = NULL;
		}
	}

	return TRUE;
}

BOOL	ZoneServer_CM::OnConnectWSSession(SPWSSession pWSSession)
{

	return TRUE;
}

BOOL	ZoneServer_CM::OnCloseWSSession(SPWSSession pWSSession)
{
	if (pWSSession->m_pData)
	{
		stClientSession* pSession = (stClientSession*)pWSSession->m_pData;

		m_pThis->FreeClientSession(pSession);
		pWSSession->m_pData = NULL;
	}

	return TRUE;
}

stClientSession* ZoneServer_CM::GetClientSession()
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

void	ZoneServer_CM::FreeClientSession(stClientSession* pSession)
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

void	ZoneServer_CM::FreeClientSessionData(stClientSession* pSession)
{
	pSession->pWSSession = nullptr;

	if (pSession->pPC)
	{
		stPlayerCharacter* pPC = pSession->pPC;

		DBProcUpdatePlayerCharacter1(pPC);

		if (pPC->pZone)
		{
			RemoveCharacterFromZone(pPC->pZone, pPC);
			pPC->pZone = NULL;
		}

		if (pPC->pSubZone)		// 서브존에 스폰된 캐릭터는 채널에서 플레이어 삭제 전파를 안한다.
		{
			if (pPC->pChannel)
			{
				RemoveCharacterFromChannel(pPC->pChannel, pPC, FALSE);
				pPC->pChannel = NULL;
			}

			RemoveCharacterFromSubZone(pPC->pSubZone, pPC);
			pPC->pSubZone = NULL;
		}
		else
		{
			if (pPC->pChannel)
			{
				RemoveCharacterFromChannel(pPC->pChannel, pPC);
				pPC->pChannel = NULL;
			}
		}

		ReleasePlayerCharacter(pSession->pPC);

		pSession->pPC = NULL;
		pPC->pSession = NULL;
	}

	stAccount* pAccount = &pSession->account;

	std::list<stAccountItem*>::iterator itor_ACItem = pAccount->listAccountItem.begin();
	while (itor_ACItem != pAccount->listAccountItem.end())
	{
		ReleaseAccountItem(*itor_ACItem);
		++itor_ACItem;
	}
	pAccount->listAccountItem.clear();

	if (pSession->connectState != ESCS_DisconnectByOtherConnect)
	{
		m_csAdminClientSession.RemoveObject(pAccount->ACGUID);

	}
}

stClientSession* ZoneServer_CM::GetConnectedClientSession(UINT64 SHPGUID)
{
	stClientSession* pResultSession = (stClientSession*)m_csAdminClientSession.GetObjectByKey(SHPGUID);
	return pResultSession;
}

BOOL	ZoneServer_CM::OnDBServerLost(PVOID	pData)
{
	g_pIOCPEngine->dbgprint("OnDBServerLost\n");

	return TRUE;
}

BOOL	ZoneServer_CM::ConnectServers()
{
	if (m_iCurrentTime - m_iSvrConnectRetryTime > SERVER_RECONNECT_TRY_TIME)
	{
		m_iSvrConnectRetryTime = m_iCurrentTime;
		stServerInfo* pMySvrInfo = &m_aServerInfo[m_iMySvrIndex];

		for (UINT32 i = 0; i < m_iServerNum; ++i)
		{
			if (m_aServerInfo[i].type == SVR_MAIN)
			{
				if (pMySvrInfo->svrRegion == m_aServerInfo[i].svrRegion && pMySvrInfo->svrGroup == m_aServerInfo[i].svrGroup)
				{
					if (m_pMainSvrSocket == NULL)
					{
						m_pMainSvrSocket = g_pIOCPEngine->ConnectTo(m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server, SVR_ZONE, m_iMySvrID);
						if (!m_pMainSvrSocket)
						{
							printf("Connect Servers Faild IP:%s,port:%d \n", m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server);
							continue;
						}
						else
						{
							m_iMainSvrIndex = i;
							m_pMainSvrSocket->SKUID = i;

							UINT32	accountCount = 0;// DBGetAccountCount();
							UINT32	channelCount = 0;// DBGetAccountCount();

							PX_BUFFER* pToSvrMain = PxObjectManager::GetSendBufferObj(m_pMainSvrSocket);
							if (pToSvrMain)
							{
								pToSvrMain->WriteUI32(accountCount);
								pToSvrMain->WriteUI32(channelCount);
								g_pIOCPEngine->SendBuffer(pToSvrMain, PKS_ZONE_SERVER_INFO);
							}
						}
					}
				}
			}
			else if (m_aServerInfo[i].type == SVR_GLOBAL)
			{
				if (m_pGlobalSvrSocket == NULL)
				{
					m_pGlobalSvrSocket = g_pIOCPEngine->ConnectTo(m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server, SVR_ZONE, m_iMySvrID);

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

BOOL	ZoneServer_CM::InitGameDatas()
{
	// mapTemplate 의 Zone 들을 미리 준비한다. SubZone은 채널 생성시에 들어간다..
	HASHMAP_UI32::iterator	itor_MT = g_csTemplateManager.m_csAdminMapTemplate.m_HashMapUI32.begin();
	while (itor_MT != g_csTemplateManager.m_csAdminMapTemplate.m_HashMapUI32.end())
	{
		stMapTemplate* pMT = (stMapTemplate*)((*itor_MT).second);

		if (pMT->isSubZone == FALSE)
		{
			stZone* pZone = m_pPoolZone->Allocate();
			pZone->Init();
			pZone->ZONEGUID = m_csGUIDFactory.RequestGUID(TGO_TYPE_ZONE);
			pZone->mapTID = pMT->TID;

			m_csAdminZone.AddObject(pZone, pZone->mapTID);
			m_listZone.push_back(pZone);
		}
		++itor_MT;
	}

	return TRUE;
}

void	ZoneServer_CM::ReleaseGameDatas()
{
	while (!m_listZone.empty())
	{
		stZone* pZone = m_listZone.front();
		ReleaseZone(pZone);
		m_listZone.pop_front();
	}
}

void	ZoneServer_CM::ReleasePlayerCharacter(stPlayerCharacter* pPC)
{
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (pPC->equipItem[i])
		{
			ReleaseItem(pPC->equipItem[i]);
			pPC->equipItem[i] = NULL;
		}
	}

	std::list<stCharacterItem*>::iterator itor_CHItem = pPC->listCharacterItem.begin();
	while (itor_CHItem != pPC->listCharacterItem.end())
	{
		stCharacterItem* pCHItem = *itor_CHItem;
		ReleaseCharacterItem(pCHItem);
		++itor_CHItem;
	}
	pPC->listCharacterItem.clear();

	m_pPoolPlayerCharacter->Free(pPC);
}

void	ZoneServer_CM::ReleaseNPC_Equip(stNPC_Equip* pNPCEquip)
{
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (pNPCEquip->equipItem[i])
		{
			ReleaseItem(pNPCEquip->equipItem[i]);
			pNPCEquip->equipItem[i] = NULL;
		}
	}

	m_pPoolNPC_Equip->Free(pNPCEquip);
}

void	ZoneServer_CM::ReleaseNPC(stNPC* pNPC)
{
	m_pPoolNPC->Free(pNPC);
}

void	ZoneServer_CM::ReleaseAccountItem(stAccountItem* pACItem)
{
	m_pPoolAccountItem->Free(pACItem);
}

void	ZoneServer_CM::ReleaseCharacterItem(stCharacterItem* pCHItem)
{
	m_pPoolCharacterItem->Free(pCHItem);
}

void	ZoneServer_CM::ReleaseItem(stItem* pItem)
{

	m_pPoolItem->Free(pItem);
}

void	ZoneServer_CM::ReleaseZone(stZone* pZone)
{
	m_pPoolZone->Free(pZone);
}

void	ZoneServer_CM::ReleaseChannel(stChannel* pChannel)
{
	--m_iChannelCount;
	m_pPoolChannel->Free(pChannel);
}

void	ZoneServer_CM::ReleaseSubZone(stSubZone* pSubZone)
{
	m_pPoolSubZone->Free(pSubZone);
}









