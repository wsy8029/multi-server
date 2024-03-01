#include "MainServer_CM.h"

void	MainServer_CM::InitPacketServerCBFunction()
{
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_END, 0, ProcsServerEnd_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_TYPE_ANNOUNCE, 0, ProcsServerTypeAnnounce_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_REQUEST_TIME_TAG, 0, ProcsRequestTimeTag_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_INFORM_TIME_TAG, 0, ProcsInformTimeTag_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_ALIVE_CHECK, 0, ProcsServerAliveCheck_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_MAIN_SERVER_INFO, 0, ProcMainServerInfo_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ZONE_SERVER_INFO, 0, ProcZoneServerInfo_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_MAIN_SERVER_CONNECT_PREPARE, 0, ProcsMainServerConnectPrepare_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ZONE_SERVER_CONNECT_PREPARE_RESULT, 0, ProcZoneServerConnectPrepareResult_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ACCOUNT_LOGOUT, 0, ProcsAccountLogout_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SET_REGION_SERVER_ROLE, 0, ProcsSetRegionServerRole_v0);
}

BOOL	MainServer_CM::ProcsServerEnd_v0 (PX_SOCKET*	pSocket,BYTE*	pBuffer,int		buffLen)			// 서버 원격 제어등을 이용하여 종료시
{
	g_pIOCPEngine->ReserveEndEngine();
	return TRUE;
}

BOOL	MainServer_CM::ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLenn)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8 eServerType = PX_RECV_GET_UI8(pBuffer);
	UINT32 svrID = PX_RECV_GET_UI32(pBuffer);

	UINT16 index = m_pThis->GetIndexBySvrID(svrID);

	pSocket->SKUID = index;

	if (eServerType == SVR_ZONE)
	{
		stZoneServerInfo* pZoneSvrInfo = (stZoneServerInfo*)m_pThis->m_csAdminZoneSvrInfo.GetObjectByKey(svrID);
		if (pZoneSvrInfo != NULL)
		{
			pZoneSvrInfo->pSocket = pSocket;
			pSocket->pData = pZoneSvrInfo;
		}
	}

	return TRUE;
}

BOOL	MainServer_CM::ProcsRequestTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32 timeTag = PX_RECV_GET_UI32(pBuffer);
	m_pThis->m_csGUIDFactory.ReserveNextTimeTag(timeTag);

	return TRUE;
}

BOOL	MainServer_CM::ProcsInformTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32 timeTag = PX_RECV_GET_UI32(pBuffer);
	m_pThis->m_csGUIDFactory.SetTimeTag(timeTag);

	return TRUE;
}

BOOL	MainServer_CM::ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	PX_BUFFER* pToSvrReply = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrReply)
	{
		g_pIOCPEngine->SendBuffer(pToSvrReply, PKS_SERVER_ALIVE_CHECK);
	}

	return TRUE;
}

BOOL	MainServer_CM::ProcMainServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32	accountCount = m_pThis->m_iSessionCount;

	PX_BUFFER* pToSvrResponse = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrResponse)
	{
		pToSvrResponse->WriteUI32(accountCount);
		g_pIOCPEngine->SendBuffer(pToSvrResponse, PKS_MAIN_SERVER_INFO);
	}

	return TRUE;
}

BOOL	MainServer_CM::ProcZoneServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	stZoneServerInfo* pZSI = (stZoneServerInfo*)pSocket->pData;
	pZSI->iAccountCount = PX_RECV_GET_UI32(pBuffer);
	pZSI->iChannelCount = PX_RECV_GET_UI32(pBuffer);

	return TRUE;
}

BOOL	MainServer_CM::ProcsMainServerConnectPrepare_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT64 ACGUID = PX_RECV_GET_UI64(pBuffer);
	UINT8 selectedSvrRegion = PX_RECV_GET_UI8(pBuffer);
	UINT32 authKey = PX_RECV_GET_UI32(pBuffer);

	stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(ACGUID);
	if (pSession) //이미 존재하는 세션
	{
		if (pSession->connectState == ESCS_WaitForConnectToServer)
		{
			pSession->authKeyFromLoginServer = authKey;

			PX_BUFFER* pToSvrResponse = PxObjectManager::GetSendBufferObj(pSocket);
			if (pToSvrResponse)
			{
				pToSvrResponse->WriteUI64(ACGUID);
				pToSvrResponse->WriteUI8(0);
				g_pIOCPEngine->SendBuffer(pToSvrResponse, PKS_MAIN_SERVER_CONNECT_PREPARE_RESULT);
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
	pSession->authKeyFromLoginServer = authKey;

	pSession->platformType = PX_RECV_GET_UI8(pBuffer);
	PX_RECV_GET_STRINGN(pSession->accessToken, pBuffer, GQL_TOKEN_SIZE);
	PX_RECV_GET_STRINGN(pSession->refreshToken, pBuffer, GQL_TOKEN_SIZE);

	stAccount* pAccount = &pSession->account;
	m_pThis->m_csAdminClientSession.AddObject(pSession, ACGUID);
	pAccount->ACGUID = ACGUID;
	pAccount->selectedSvrRegion = selectedSvrRegion;

	PX_BUFFER* pToSvrResponse = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrResponse)
	{
		pToSvrResponse->WriteUI64(ACGUID);
		pToSvrResponse->WriteUI8(0);
		g_pIOCPEngine->SendBuffer(pToSvrResponse, PKS_MAIN_SERVER_CONNECT_PREPARE_RESULT);
	}

	return TRUE;
}

BOOL	MainServer_CM::ProcZoneServerConnectPrepareResult_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	stZoneServerInfo* pZSI = (stZoneServerInfo*)pSocket->pData;

	UINT64	ACGUID = PX_RECV_GET_UI64(pBuffer);
	UINT8 result = PX_RECV_GET_UI8(pBuffer);

	stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(ACGUID);
	if (pSession == NULL) return FALSE;

	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
	if (pToClient != nullptr)
	{
		pToClient->WriteUI8(result);

		if (result == 0)	// 성공이라면 메인 서버에 접속하기 위한 추가 정보 전송
		{
			pToClient->WriteUI32(pSession->authKeyForZoneServer);
			pToClient->WriteString(pZSI->pServerInfo->public_ip);
			pToClient->WriteUI16(pZSI->pServerInfo->port_client_TCP);
			pToClient->WriteUI16(pZSI->pServerInfo->endMarker);
		}

		g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_ZONE_SERVER_INFO);
	}
	
	return TRUE;
}

BOOL	MainServer_CM::ProcsAccountLogout_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
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

BOOL	MainServer_CM::ProcsSetRegionServerRole_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8	isRegionServerRole = PX_RECV_GET_UI8(pBuffer);
	m_pThis->m_bRegionServerRole = isRegionServerRole;

	return TRUE;
}
