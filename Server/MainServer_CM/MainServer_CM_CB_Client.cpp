#include "MainServer_CM.h"

void	MainServer_CM::InitPacketClientCBFunction()
{
	g_pWSEngine->SetPacketClientCB(PK_HAND_SHAKE, 0, ProcHandShake_v0);
	g_pWSEngine->SetPacketClientCB(PK_ALIVE_CHECK, 0, ProcAliveCheck_v0);
	g_pWSEngine->SetPacketClientCB(PK_ERROR_LOG, 0, ProcErrorLog_v0);
	g_pWSEngine->SetPacketClientCB(PK_CONNECT_MAIN_SERVER_AUTH, 0, ProcConnectMainServerAuth_v0);
	g_pWSEngine->SetPacketClientCB(PK_ACCOUNT_LOGOUT, 0, ProcAccountLogout_v0);
	
	g_pWSEngine->SetPacketClientCB(PK_REQUEST_ZONE_ENTER, 0, ProcRequestZoneEnter_v0);
}

BOOL	MainServer_CM::ProcHandShake_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	UINT8 svrIndex = PX_RECV_GET_UI8(pBuffer);

	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
	if (pToClient != nullptr)
	{
		pToClient->WriteUI8(svrIndex);
		pToClient->WriteUI32(pWSSession->m_iWSSessionUID);
		g_pWSEngine->SendBuffer(pToClient, PK_HAND_SHAKE);
	}

	return TRUE;
}

BOOL	MainServer_CM::ProcAliveCheck_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	return TRUE;
}

BOOL	MainServer_CM::ProcAccountLogout_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	pWSSession->Close();

	return TRUE;
}

BOOL	MainServer_CM::ProcErrorLog_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	UINT8 msgLineCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < msgLineCount; ++i)
	{
		UINT8	type = PX_RECV_GET_UI8(pBuffer);
		CHAR log[256];
		PX_RECV_GET_STRINGN(log, pBuffer, 256);

		if (strlen(log) > 0)
		{
			m_pThis->m_cmdInsertErrorLog1.SetUInt(1, type);
			m_pThis->m_cmdInsertErrorLog1.SetString(2, log);
			m_pThis->m_cmdInsertErrorLog1.ExecuteUpdate();
		}
	}

	UINT8 stackTraceLineCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < stackTraceLineCount; ++i)
	{
		UINT8	type = PX_RECV_GET_UI8(pBuffer);
		CHAR log[256];
		PX_RECV_GET_STRINGN(log, pBuffer, 256);

		if (strlen(log) > 0)
		{
			m_pThis->m_cmdInsertErrorLog1.SetUInt(1, type);
			m_pThis->m_cmdInsertErrorLog1.SetString(2, log);
			m_pThis->m_cmdInsertErrorLog1.ExecuteUpdate();
		}
	}

	return TRUE;
}

BOOL	MainServer_CM::ProcConnectMainServerAuth_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	UINT64 ACGUID = PX_RECV_GET_UI64(pBuffer);
	UINT32 authKey = PX_RECV_GET_UI32(pBuffer);
	
	stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(ACGUID);
	if (!pSession)																		// 유저에게 접속 실패를 알린다.. 해당 guid가 없다
	{
		PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
		if (pToClient != nullptr)
		{
			pToClient->WriteUI8(1);
			g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_MAIN_SERVER_AUTH);
			
			pWSSession->Close();
		}
		return FALSE;
	}

	pSession->RefreshAliveCheck();

	if (pSession->authKeyFromLoginServer != authKey)
	{
		PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
		if (pToClient != nullptr)
		{
			pToClient->WriteUI8(2);
			g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_MAIN_SERVER_AUTH);

			pWSSession->Close();
		}
		return FALSE;
	}

	if (pWSSession->m_pData != NULL)
	{
		stClientSession* pRemoveSession = (stClientSession*)pWSSession->m_pData;
		m_pThis->FreeClientSession(pRemoveSession);
	}

	pWSSession->m_pData = (PVOID)pSession;													// per Socket Data Set!!
	pSession->pWSSession = pWSSession;
	pSession->connectState = ESCS_ConnectedToServer;

	// Request GraphQL MyProfile
	std::string strFields = "";
	NestoQL::MyProfileOutput::SerializeField(strFields);
	g_csGraphQLManager.QueryGraphQL(ACGUID, NestoQL::EGQLMT_MyProfile, NestoQL::QUERY, NULL, strFields, pSession->accessToken, pSession);

	return TRUE;
}

BOOL	MainServer_CM::ProcRequestZoneEnter_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	stAccount* pAC = &pSession->account;
	stPlayerCharacter* pPC = pSession->pPC;

	UINT32 bestZoneSvrID = m_pThis->m_iRecommendZoneSvrID;
	UINT8	result = 0;
	if (pPC == NULL)
	{
		result = 1;
	}
	else if (bestZoneSvrID == 0)
	{
		result = 2;
	}

	// 클라이언트에게 결과 알림
	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
	if (pToClient != nullptr)
	{
		pToClient->WriteUI8(result);
		g_pWSEngine->SendBuffer(pToClient, PK_REQUEST_ZONE_ENTER);
	}

	if (result == 0)
	{
		pPC->zoneSvrID = bestZoneSvrID;

		// Zone 서버에게 접속 알림 .. 서버 그룹에서 최적의 Zone 서버를 구해서 클라에게 알려준다..
		stZoneServerInfo* pZoneSvrInfo = (stZoneServerInfo*)m_pThis->m_csAdminZoneSvrInfo.GetObjectByKey(bestZoneSvrID);

		PX_BUFFER* pToZoneSvr = PxObjectManager::GetSendBufferObj(pZoneSvrInfo->pSocket);
		if (pToZoneSvr)
		{
			pSession->authKeyForZoneServer = m_pThis->GetNextAuthKeyForZoneSvr();

			pToZoneSvr->WriteUI64(pAC->ACGUID);
			pToZoneSvr->WriteUI32(pSession->authKeyForZoneServer);
			pToZoneSvr->WriteUI64(pPC->CHGUID);

			pAC->WritePacket_Server_v0(pToZoneSvr);
			pPC->WritePacket_Server_v0(pToZoneSvr);

			g_pIOCPEngine->SendBuffer(pToZoneSvr, PKS_ZONE_SERVER_CONNECT_PREPARE);
		}

		// Global Svr 에게 유일 접속 처리..
		PX_BUFFER* pToGlobalSvr = PxObjectManager::GetSendBufferObj(m_pThis->m_pGlobalSvrSocket);
		if (pToGlobalSvr)
		{
			pToGlobalSvr->WriteUI8(pAC->selectedSvrRegion);
			pToGlobalSvr->WriteUI32(bestZoneSvrID);
			pToGlobalSvr->WriteUI64(pAC->ACGUID);
			g_pIOCPEngine->SendBuffer(pToGlobalSvr, PKS_ACCOUNT_CONNECT_UNIQUE);
		}
	}

	return TRUE;
}
