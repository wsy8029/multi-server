#include "LoginServer_CM.h"

void	LoginServer_CM::InitPacketClientCBFunction()
{
	g_pWSEngine->SetPacketClientCB(PK_HAND_SHAKE, 0, ProcHandShake_v0);
	g_pWSEngine->SetPacketClientCB(PK_INFORM_SEND, 0, ProcInformSend_v0);
	g_pWSEngine->SetPacketClientCB(PK_ALIVE_CHECK, 0, ProcAliveCheck_v0);
	g_pWSEngine->SetPacketClientCB(PK_ERROR_LOG, 0, ProcErrorLog_v0);

	g_pWSEngine->SetPacketClientCB(PK_ACCOUNT_LOGIN, 0, ProcAccountLogin_v0);
	g_pWSEngine->SetPacketClientCB(PK_ACCOUNT_LOGOUT, 0, ProcAccountLogout_v0);
}

BOOL	LoginServer_CM::ProcHandShake_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
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

BOOL	LoginServer_CM::ProcInformSend_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	if (pWSSession->m_pData)
	{
		stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
		pSession->RefreshAliveCheck();

		pSession->platformType = PX_RECV_GET_UI8(pBuffer);
		UINT16 clientVersion = PX_RECV_GET_UI16(pBuffer);

		if (clientVersion < m_pThis->m_stClientInfo[pSession->platformType].clientVersion)
		{
			PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
			if (pToClient != nullptr)
			{
				pToClient->WriteUI8(1);
				pToClient->WriteString((char*)m_pThis->m_stClientInfo[pSession->platformType].marketURL.c_str());
				g_pWSEngine->SendBuffer(pToClient, PK_INFORM_SEND);
			}
			return FALSE;
		}
	}

	return TRUE;
}

BOOL	LoginServer_CM::ProcAliveCheck_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	return TRUE;
}

BOOL	LoginServer_CM::ProcErrorLog_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
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

BOOL	LoginServer_CM::ProcAccountLogin_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	pSession->selectedSvrRegion = PX_RECV_GET_UI8(pBuffer);
	PX_RECV_GET_STRINGN(pSession->accessToken, pBuffer, GQL_TOKEN_SIZE);
	PX_RECV_GET_STRINGN(pSession->refreshToken, pBuffer, GQL_TOKEN_SIZE);

	pSession->ACGUID = g_csGraphQLManager.GetUserNoFromJWTToken(pSession->accessToken);
	if (pSession->ACGUID == 0) return FALSE;

	m_pThis->m_csAdminClientSession.RemoveObject(pSession->ACGUID);			// 같은 ACGUID 를 가진 계정이 다른 디바이스로 접속 요청시 항상 최신 소켓을 가리키기 위해서..
	m_pThis->m_csAdminClientSession.AddObject(pSession, pSession->ACGUID);

	// 메인 서버에게 접속 알림 .. 서버 그룹에서 최적의 Main 서버를 구해서 클라에게 알려준다..
	UINT32 bestMainSvrID = m_pThis->m_iRecommendMainSvrID[pSession->selectedSvrRegion];
	stMainServerInfo* pMainSvrInfo = (stMainServerInfo*)m_pThis->m_csAdminMainSvrInfo.GetObjectByKey(bestMainSvrID);

	if (pMainSvrInfo)
	{
		PX_BUFFER* pToMainSvr = PxObjectManager::GetSendBufferObj(pMainSvrInfo->pSocket);
		if (pToMainSvr)
		{
			pSession->authKeyForMainServer = m_pThis->GetNextAuthKey();
			pToMainSvr->WriteUI64(pSession->ACGUID);
			pToMainSvr->WriteUI8(pSession->selectedSvrRegion);
			pToMainSvr->WriteUI32(pSession->authKeyForMainServer);
			pToMainSvr->WriteUI8(pSession->platformType);
			pToMainSvr->WriteString(pSession->accessToken);
			pToMainSvr->WriteString(pSession->refreshToken);
			g_pIOCPEngine->SendBuffer(pToMainSvr, PKS_MAIN_SERVER_CONNECT_PREPARE);
		}
	}

	// Global Svr 에게 유일 접속 처리..
	PX_BUFFER* pToGlobalSvr = PxObjectManager::GetSendBufferObj(m_pThis->m_pGlobalSvrSocket);
	if (pToGlobalSvr)
	{
		pToGlobalSvr->WriteUI8(pSession->selectedSvrRegion);
		pToGlobalSvr->WriteUI32(bestMainSvrID);
		pToGlobalSvr->WriteUI64(pSession->ACGUID);
		g_pIOCPEngine->SendBuffer(pToGlobalSvr, PKS_ACCOUNT_CONNECT_UNIQUE);
	}

	// 클라이언트에게 결과 전송
	UINT8 result = 0;
	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
	if (pToClient != nullptr)
	{
		pToClient->WriteUI8(result);
		if (result == 0)	// 성공이라면 메인 서버에 접속하기 위한 추가 정보 전송
		{
			pToClient->WriteUI64(pSession->ACGUID);
		}
		g_pWSEngine->SendBuffer(pToClient, PK_ACCOUNT_LOGIN);
	}

	return TRUE;
}

BOOL	LoginServer_CM::ProcAccountLogout_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{


	return TRUE;
}

