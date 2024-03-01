#include "MainServer_CM.h"

#include <limits>

MainServer_CM* MainServer_CM::m_pThis = NULL;
TemplateManager	g_csTemplateManager;

MainServer_CM::MainServer_CM()
	: ServerBase_CM()
{
	m_pThis = this;

	m_listSession = NULL;
	m_iSessionCount = 0;

	m_iGlobalSvrIndex = 0;
	m_pGlobalSvrSocket = NULL;
	m_iLoginSvrIndex = 0;
	m_pLoginSvrSocket = NULL;
	m_iSvrConnectRetryTime = 0;

	m_iZoneSvrCount = 0;
	ZeroMemory(m_stZoneSvrInfos, sizeof(m_stZoneSvrInfos));
	m_iRecommendZoneSvrID = 0;

	m_iSvrInfoUpdateTime = 0;
	m_iAuthKeyIndexForZoneSvr = 0;
	m_bRegionServerRole = FALSE;

	m_pPoolClientSession = NULL;
	m_pPoolPlayerCharacter = NULL;
	m_pPoolCharacterInfo = NULL;
	m_pPoolAccountItem = NULL;
	m_pPoolCharacterItem = NULL;
	m_pPoolItem = NULL;
}

MainServer_CM::~MainServer_CM()
{
}

BOOL	MainServer_CM::Initialize(UINT32 svrID)
{
	if (ServerBase_CM::Initialize(svrID) == FALSE)
	{
		return FALSE;
	}

	g_pWSEngine = new PxWebSocketEngine;

	m_pPoolClientSession = new _MemoryPool<stClientSession>(2048, "stClientSession");

	g_pIOCPEngine->m_strDebugFileNameHeader = "main_cm";

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

	InitServerInfos();

	if (!ConnectServers())
	{
		printf("ConnectServers Failed\n");
		return FALSE;
	}

	// Auth Key Table Init
	srand(timeGetTime());
	for (int i = 0; i < AUTH_KEY_TABLE_COUNT; ++i)
	{
		m_pAuthKeyTableForZoneSvr[i] = rand();
	}

	m_pPoolPlayerCharacter = new _MemoryPool<stPlayerCharacter>(2048, "stPlayerCharacter");
	m_pPoolCharacterInfo = new _MemoryPool<stCharacterInfo>(2048, "stCharacterInfo");
	m_pPoolAccountItem = new _MemoryPool<stAccountItem>(1024, "stAccountItem");
	m_pPoolCharacterItem = new _MemoryPool<stCharacterItem>(1024, "stCharacterItem");
	m_pPoolItem = new _MemoryPool<stItem>(4096, "stItem");

	if (!g_csTemplateManager.Initialize())
	{
		printf("g_csTemplateManager.Initialize Failed\n");
	}

	g_csGraphQLManager.Init("https://dev-api.luxrobo.link/graphql");

	InitDBCashes();

	return TRUE;
}

void	MainServer_CM::Release()
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
	if (m_pPoolPlayerCharacter) { delete	m_pPoolPlayerCharacter; m_pPoolPlayerCharacter = NULL; }
	if (m_pPoolCharacterInfo) { delete	m_pPoolCharacterInfo; m_pPoolCharacterInfo = NULL; }
	if (m_pPoolAccountItem) { delete	m_pPoolAccountItem; m_pPoolAccountItem = NULL; }
	if (m_pPoolCharacterItem) { delete	m_pPoolCharacterItem; m_pPoolCharacterItem = NULL; }
	if (m_pPoolItem) { delete	m_pPoolItem; m_pPoolItem = NULL; }
}

void	MainServer_CM::Update()
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

	g_pWSEngine->ProcRcvBuf_Client();
	g_pIOCPEngine->ProcRcvBuf_Server();
	g_csGraphQLManager.Update();

	g_pWSEngine->ProcWSSessionDisconnect();
	g_pIOCPEngine->ProcSocketDisconnect_Server();

	UpdateSvrInfos();

	ServerBase_CM::PostUpdate();
}

BOOL	MainServer_CM::OnConnectSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	return TRUE;
}

BOOL	MainServer_CM::OnCloseSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	if (pSocket->bInsideConnect)
	{
		int svrIndex = pSocket->SKUID;
		if (m_pThis->m_aServerInfo[svrIndex].type == SVR_LOGIN)
		{
			m_pThis->m_pLoginSvrSocket = NULL;
		}
		else if (m_pThis->m_aServerInfo[svrIndex].type == SVR_GLOBAL)
		{
			m_pThis->m_pGlobalSvrSocket = NULL;
		}
		else if (m_pThis->m_aServerInfo[svrIndex].type == SVR_ZONE)
		{
			stZoneServerInfo* pZoneSvrInfo = (stZoneServerInfo*)pSocket->pData;
			if (pZoneSvrInfo)
			{
				pZoneSvrInfo->pSocket = NULL;
			}
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

BOOL	MainServer_CM::OnConnectWSSession(SPWSSession pWSSession)
{

	return TRUE;
}

BOOL	MainServer_CM::OnCloseWSSession(SPWSSession pWSSession)
{
	if (pWSSession->m_pData)
	{
		stClientSession* pSession = (stClientSession*)pWSSession->m_pData;

		m_pThis->FreeClientSession(pSession);
		pWSSession->m_pData = NULL;
	}

	return TRUE;
}

stClientSession* MainServer_CM::GetClientSession()
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

void	MainServer_CM::FreeClientSession(stClientSession* pSession)
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

void	MainServer_CM::FreeClientSessionData(stClientSession* pSession)
{
	pSession->pWSSession = nullptr;

	if (pSession->pPC)
	{
		stPlayerCharacter* pPC = pSession->pPC;

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

stClientSession* MainServer_CM::GetConnectedClientSession(UINT64 SHPGUID)
{
	stClientSession* pResultSession = (stClientSession*)m_csAdminClientSession.GetObjectByKey(SHPGUID);
	return pResultSession;
}

BOOL	MainServer_CM::OnDBServerLost(PVOID	pData)
{
	g_pIOCPEngine->dbgprint("OnDBServerLost\n");

	return TRUE;
}

void	MainServer_CM::InitServerInfos()
{
	stServerInfo* pMyServerInfo = &m_aServerInfo[m_iMySvrIndex];

	for (UINT32 i = 0; i < m_iServerNum; ++i)
	{
		if (m_aServerInfo[i].type == SVR_ZONE &&
			m_aServerInfo[i].svrRegion == pMyServerInfo->svrRegion &&
			m_aServerInfo[i].svrGroup == pMyServerInfo->svrGroup)
		{
			int index = m_aServerInfo[i].svrIndex;

			m_stZoneSvrInfos[index].iAccountCount = 0;
			m_stZoneSvrInfos[index].pSocket = NULL;
			m_stZoneSvrInfos[index].pServerInfo = &m_aServerInfo[i];

			m_csAdminZoneSvrInfo.AddObject(&m_stZoneSvrInfos[index], m_aServerInfo[i].svrID);

			if (m_aServerInfo[i].svrRegion + 1 > m_iZoneSvrCount)
			{
				m_iZoneSvrCount = m_aServerInfo[i].svrRegion + 1;
			}
		}
	}
}

BOOL	MainServer_CM::ConnectServers()
{
	if (m_iCurrentTime - m_iSvrConnectRetryTime > SERVER_RECONNECT_TRY_TIME)
	{
		m_iSvrConnectRetryTime = m_iCurrentTime;
		for (UINT32 i = 0; i < m_iServerNum; ++i)
		{
			if (m_aServerInfo[i].type == SVR_LOGIN)
			{
				if (m_pLoginSvrSocket == NULL)
				{
					m_pLoginSvrSocket = g_pIOCPEngine->ConnectTo(m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server, SVR_MAIN, m_iMySvrID);
					if (!m_pLoginSvrSocket)
					{
						printf("Connect Servers Faild IP:%s,port:%d \n", m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server);
						continue;
					}
					else
					{
						m_iLoginSvrIndex = i;
						m_pLoginSvrSocket->SKUID = i;

						UINT32	accountCount = 0;// DBGetAccountCount();

						PX_BUFFER* pToSvrLogin = PxObjectManager::GetSendBufferObj(m_pLoginSvrSocket);
						if (pToSvrLogin)
						{
							pToSvrLogin->WriteUI32(accountCount);
							g_pIOCPEngine->SendBuffer(pToSvrLogin, PKS_MAIN_SERVER_INFO);
						}
					}
				}
			}
			else if (m_aServerInfo[i].type == SVR_GLOBAL)
			{
				if (m_pGlobalSvrSocket == NULL)
				{
					m_pGlobalSvrSocket = g_pIOCPEngine->ConnectTo(m_aServerInfo[i].private_ip, m_aServerInfo[i].port_server, SVR_MAIN, m_iMySvrID);

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

void	MainServer_CM::ReleasePlayerCharacter(stPlayerCharacter* pPC)
{
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (pPC->equipItem[i])
		{
			ReleaseItem(pPC->equipItem[i]);
			pPC->equipItem[i] = NULL;
		}
	}

	while (!pPC->listCharacterItem.empty())
	{
		stCharacterItem* pCHItem = pPC->listCharacterItem.front();
		ReleaseCharacterItem(pCHItem);
		pPC->listCharacterItem.pop_front();
	}
	pPC->listCharacterItem.clear();

	m_pPoolPlayerCharacter->Free(pPC);
}

void	MainServer_CM::ReleaseAccountItem(stAccountItem* pACItem)
{
	m_pPoolAccountItem->Free(pACItem);
}

void	MainServer_CM::ReleaseCharacterItem(stCharacterItem* pCHItem)
{
	m_pPoolCharacterItem->Free(pCHItem);
}

void	MainServer_CM::ReleaseItem(stItem* pItem)
{
	//m_csAdminItem.RemoveObject(pItem->itemGUID);
	m_pPoolItem->Free(pItem);
}

UINT32	MainServer_CM::FindBestZoneSvrID()
{
	UINT32	bestIndex = 0;
	UINT32	minUser = SERVER_MAXUSER;

	for (int i = 0; i < m_iZoneSvrCount; ++i)
	{
		if (m_stZoneSvrInfos[i].pSocket != NULL && m_stZoneSvrInfos[i].iAccountCount < minUser)
		{
			bestIndex = m_stZoneSvrInfos[i].pServerInfo->svrID;
			minUser = m_stZoneSvrInfos[i].iAccountCount;
		}
	}
	return bestIndex;
}

UINT32	MainServer_CM::GetNextAuthKeyForZoneSvr()
{
	UINT32 retVal = m_pAuthKeyTableForZoneSvr[m_iAuthKeyIndexForZoneSvr++];
	if (m_iAuthKeyIndexForZoneSvr >= AUTH_KEY_TABLE_COUNT)	m_iAuthKeyIndexForZoneSvr = 0;
	return retVal;
}

void	MainServer_CM::UpdateSvrInfos()
{
	if (m_iCurrentTime - m_iSvrInfoUpdateTime > SERVER_INFO_UPDATE_REQUEST_TIME)
	{
		m_iSvrInfoUpdateTime = m_iCurrentTime;

		m_iRecommendZoneSvrID = FindBestZoneSvrID();

		HASHMAP_UI32::iterator itor_zoneSvrInfo = m_csAdminZoneSvrInfo.m_HashMapUI32.begin();
		while (itor_zoneSvrInfo != m_csAdminZoneSvrInfo.m_HashMapUI32.end())
		{
			stZoneServerInfo* pZoneSvrInfo = (stZoneServerInfo*)itor_zoneSvrInfo->second;
			if (pZoneSvrInfo->pSocket != NULL)
			{
				PX_BUFFER* pToZoneSvr = PxObjectManager::GetSendBufferObj(pZoneSvrInfo->pSocket);
				if (pToZoneSvr)
				{
					g_pIOCPEngine->SendBuffer(pToZoneSvr, PKS_ZONE_SERVER_INFO);
				}
			}
			++itor_zoneSvrInfo;
		}
	}
}

void	MainServer_CM::InitPlayerCharacterByMyProfile(stPlayerCharacter* pPC, NestoQL::MyProfileOutput& myProfile)
{
	stCharacterInfo* pCHInfo = pPC->pCharacterInfo;

	pPC->CHGUID = myProfile.id;
	wcsncpy_s(pCHInfo->nickname, myProfile.nickname, NICKNAME_LENGTH);
	strncpy_s(pCHInfo->nickname_UTF8, myProfile.nickname_UTF8, NICKNAME_LENGTH_UTF8);
	strncpy_s(pPC->birthDate, myProfile.birthdate, BIRTHDATE_LENGTH);
	strncpy_s(pPC->thumbnailUrl, myProfile.thumbnailUrl, THUMBNAIL_URL_LENGTH);
	strncpy_s(pPC->mainBadgeUrl, myProfile.mainBadgeUrl, MAINBADGE_URL_LENGTH);
	pPC->statusType = myProfile.statusType;
	wcsncpy_s(pPC->statusDescription, myProfile.statusDescription, STATUS_DESCRIPTION_LENGTH);
	strncpy_s(pPC->statusDescription_UTF8, myProfile.statusDescription_UTF8, STATUS_DESCRIPTION_LENGTH_UTF8);
	strncpy_s(pPC->personalSpaceUrl, myProfile.personalSpaceUrl, PERSONAL_SPACE_URL_LENGTH);
	pPC->level = myProfile.level;
	pPC->totalExperienceForNextLevel = myProfile.totalExperienceForNextLevel;
	pPC->point = myProfile.point;
	pPC->literacyExp = myProfile.literacyExp;
	pPC->imaginationExp = myProfile.imaginationExp;
	pPC->narrativeExp = myProfile.narrativeExp;
	pPC->sociabilityExp = myProfile.sociabilityExp;

	pCHInfo->customize[ECCT_FaceShape] = myProfile.avatarProducts.faceShape.productId;
	pCHInfo->customize[ECCT_Eye] = myProfile.avatarProducts.eye.productId;
	pCHInfo->customize[ECCT_Mouth] = myProfile.avatarProducts.mouth.productId;
	pCHInfo->customize[ECCT_SkinColor] = myProfile.avatarProducts.skinColor.productId;
	pCHInfo->customize[ECCT_FaceSticker] = myProfile.avatarProducts.faceSticker.productId;
	pCHInfo->customize[ECCT_Hair] = myProfile.avatarProducts.hair.productId;
	pCHInfo->item_Accessory = myProfile.avatarProducts.accessory.productId;

	NestoQL::UserProductAvatar* pUAP[] = {&myProfile.avatarProducts.top,&myProfile.avatarProducts.bottom,&myProfile.avatarProducts.onepiece,
										&myProfile.avatarProducts.shoe,&myProfile.avatarProducts.accessory};
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (pUAP[i] == NULL)
		{
			pPC->equipItem[i] = NULL;
			continue;
		}

		if (pUAP[i]->productId == 0)
		{
			pPC->equipItem[i] = NULL;
		}
		else
		{
			stItem* pItem = m_pPoolItem->Allocate();
			pItem->Init();

			pItem->itemGUID.GUID = pPC->CHGUID;
			pItem->itemGUID.productID = pUAP[i]->productId;

			pItem->ACGUID = pPC->ACGUID;
			pItem->count = 1;
			pItem->itemStatus = EIS_Equip;
		}
	}
}