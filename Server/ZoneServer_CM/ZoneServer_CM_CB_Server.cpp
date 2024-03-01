#include "ZoneServer_CM.h"

void	ZoneServer_CM::InitPacketServerCBFunction()
{
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_END, 0, ProcsServerEnd_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_TYPE_ANNOUNCE, 0, ProcsServerTypeAnnounce_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_REQUEST_TIME_TAG, 0, ProcsRequestTimeTag_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_INFORM_TIME_TAG, 0, ProcsInformTimeTag_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_ALIVE_CHECK, 0, ProcsServerAliveCheck_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ZONE_SERVER_INFO, 0, ProcZoneServerInfo_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ZONE_SERVER_CONNECT_PREPARE, 0, ProcZoneServerConnectPrepare_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_INIT_SERVER_DATA, 0, ProcInitServerData_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ACCOUNT_LOGOUT, 0, ProcsAccountLogout_v0);
}

BOOL	ZoneServer_CM::ProcsServerEnd_v0 (PX_SOCKET*	pSocket,BYTE*	pBuffer,int		buffLen)			// 서버 원격 제어등을 이용하여 종료시
{
	g_pIOCPEngine->ReserveEndEngine();
	return TRUE;
}

BOOL	ZoneServer_CM::ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLenn)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8 eServerType = PX_RECV_GET_UI8(pBuffer);
	UINT32 svrID = PX_RECV_GET_UI32(pBuffer);

	UINT16 index = m_pThis->GetIndexBySvrID(svrID);

	pSocket->SKUID = index;

	return TRUE;
}

BOOL	ZoneServer_CM::ProcsRequestTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32 timeTag = PX_RECV_GET_UI32(pBuffer);
	m_pThis->m_csGUIDFactory.ReserveNextTimeTag(timeTag);

	return TRUE;
}

BOOL	ZoneServer_CM::ProcsInformTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32 timeTag = PX_RECV_GET_UI32(pBuffer);
	m_pThis->m_csGUIDFactory.SetTimeTag(timeTag);

	return TRUE;
}

BOOL	ZoneServer_CM::ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	PX_BUFFER* pToSvrReply = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrReply)
	{
		g_pIOCPEngine->SendBuffer(pToSvrReply, PKS_SERVER_ALIVE_CHECK);
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcZoneServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32	accountCount = m_pThis->m_iSessionCount;
	UINT32	channelCount = m_pThis->m_iChannelCount;

	PX_BUFFER* pToSvrResponse = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrResponse)
	{
		pToSvrResponse->WriteUI32(accountCount);
		pToSvrResponse->WriteUI32(channelCount);
		g_pIOCPEngine->SendBuffer(pToSvrResponse, PKS_ZONE_SERVER_INFO);
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcZoneServerConnectPrepare_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT64 ACGUID = PX_RECV_GET_UI64(pBuffer);
	UINT32 authKey = PX_RECV_GET_UI32(pBuffer);

	stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(ACGUID);
	if (pSession) //이미 존재하는 세션
	{
		if (pSession->connectState == ESCS_WaitForConnectToServer)
		{
			pSession->authKeyFromMainServer = authKey;

			PX_BUFFER* pToSvrResponse = PxObjectManager::GetSendBufferObj(pSocket);
			if (pToSvrResponse)
			{
				pToSvrResponse->WriteUI64(ACGUID);
				pToSvrResponse->WriteUI8(0);
				g_pIOCPEngine->SendBuffer(pToSvrResponse, PKS_ZONE_SERVER_CONNECT_PREPARE_RESULT);
			}

			g_pIOCPEngine->dbgprint("ConnectPrepare (already exist) - %I64u\n", ACGUID);

			return FALSE;
		}
		else
		{
			// 이미 접속된 계정 차단
			pSession->connectState = ESCS_DisconnectByOtherConnect;
			m_pThis->m_csAdminClientSession.RemoveObject(ACGUID);

			if (pSession->pWSSession != nullptr)
			{
				//로그아웃 패킷 보냄
				PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
				if (pToClient != nullptr)
				{
					pToClient->WriteUI8(1);		// 1: 다른 사람 접속에 의해 로그아웃
					g_pWSEngine->SendBuffer(pToClient, PK_ACCOUNT_LOGOUT);
				}
				g_pIOCPEngine->dbgprint("ConnectPrepare (logout by other conn) - %I64u\n", ACGUID);
			}

			m_pThis->FreeClientSessionData(pSession);
		}
	}

	pSession = m_pThis->GetClientSession();
	pSession->Init();
	pSession->connectState = ESCS_WaitForConnectToServer;
	pSession->authKeyFromMainServer = authKey;

	UINT64 CHGUID = PX_RECV_GET_UI64(pBuffer);

	stAccount* pAC = &pSession->account;
	pAC->ACGUID = ACGUID;

	stPlayerCharacter* pPC = m_pThis->m_pPoolPlayerCharacter->Allocate();
	pPC->Init();
	pPC->CHGUID = CHGUID;
	pPC->ACGUID = ACGUID;
	pSession->pPC = pPC;
	pPC->pSession = pSession;
	pPC->pAccount = &pSession->account;

	stCharacterInfo* pCHInfo = (stCharacterInfo*)m_pThis->m_csAdminCharacterInfo.GetObjectByKey(CHGUID);
	if (pCHInfo == NULL)
	{
		pCHInfo = m_pThis->m_pPoolCharacterInfo->Allocate();
		pCHInfo->Init();
		pCHInfo->CHGUID = CHGUID;

		m_pThis->m_listCharacterInfo.push_back(pCHInfo);
		m_pThis->m_csAdminCharacterInfo.AddObject(pCHInfo, CHGUID);
	}
	pPC->pCharacterInfo = pCHInfo;

	pAC->ReadPacket_Server_v0(pBuffer, m_pThis->m_pPoolAccountItem);
	pPC->ReadPacket_Server_v0(pBuffer, m_pThis->m_pPoolCharacterItem, m_pThis->m_pPoolItem);

	m_pThis->m_csAdminClientSession.AddObject(pSession, ACGUID);

	PX_BUFFER* pToSvrResponse = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrResponse)
	{
		pToSvrResponse->WriteUI64(ACGUID);
		pToSvrResponse->WriteUI8(0);
		g_pIOCPEngine->SendBuffer(pToSvrResponse, PKS_ZONE_SERVER_CONNECT_PREPARE_RESULT);
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcInitServerData_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	return TRUE;
}

BOOL	ZoneServer_CM::ProcsAccountLogout_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT64	ACGUID = PX_RECV_GET_UI64(pBuffer);

	stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(ACGUID);
	if (pSession == NULL) return FALSE;

	// 이미 접속된 계정 차단
	pSession->connectState = ESCS_DisconnectByOtherConnect;
	m_pThis->m_csAdminClientSession.RemoveObject(ACGUID);

	//로그아웃 패킷 보냄
	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
	if (pToClient != nullptr)
	{
		pToClient->WriteUI8(1);		// 1: 다른 사람 접속에 의해 로그아웃
		g_pWSEngine->SendBuffer(pToClient, PK_ACCOUNT_LOGOUT);
	}

	m_pThis->FreeClientSessionData(pSession);

	return TRUE;
}