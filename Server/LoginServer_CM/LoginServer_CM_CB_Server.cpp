#include "LoginServer_CM.h"

void	LoginServer_CM::InitPacketServerCBFunction()
{
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_END, 0, ProcsServerEnd_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_TYPE_ANNOUNCE, 0, ProcsServerTypeAnnounce_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_REQUEST_TIME_TAG, 0, ProcsRequestTimeTag_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_INFORM_TIME_TAG, 0, ProcsInformTimeTag_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_ALIVE_CHECK, 0, ProcsServerAliveCheck_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_MAIN_SERVER_INFO, 0, ProcsMainServerInfo_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_MAIN_SERVER_CONNECT_PREPARE_RESULT, 0, ProcsMainServerConnectPrepareResult_v0);
}

BOOL	LoginServer_CM::ProcsServerEnd_v0 (PX_SOCKET*	pSocket,BYTE*	pBuffer,int		buffLen)			// 서버 원격 제어등을 이용하여 종료시
{
	g_pIOCPEngine->ReserveEndEngine();
	return TRUE;
}

BOOL	LoginServer_CM::ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLenn)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8 eServerType = PX_RECV_GET_UI8(pBuffer);
	UINT32 svrID = PX_RECV_GET_UI32(pBuffer);

	UINT16 index = m_pThis->GetIndexBySvrID(svrID);

	pSocket->SKUID = index;

	if (eServerType == SVR_MAIN)
	{
		stMainServerInfo* pMainSvrInfo = (stMainServerInfo*)m_pThis->m_csAdminMainSvrInfo.GetObjectByKey(svrID);
		if (pMainSvrInfo != NULL)
		{
			pMainSvrInfo->pSocket = pSocket;
			pSocket->pData = pMainSvrInfo;
		}
	}

	return TRUE;
}

BOOL	LoginServer_CM::ProcsRequestTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32 timeTag = PX_RECV_GET_UI32(pBuffer);
	m_pThis->m_csGUIDFactory.ReserveNextTimeTag(timeTag);

	return TRUE;
}

BOOL	LoginServer_CM::ProcsInformTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT32 timeTag = PX_RECV_GET_UI32(pBuffer);
	m_pThis->m_csGUIDFactory.SetTimeTag(timeTag);

	return TRUE;
}

BOOL	LoginServer_CM::ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	PX_BUFFER* pToSvrReply = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrReply)
	{
		g_pIOCPEngine->SendBuffer(pToSvrReply, PKS_SERVER_ALIVE_CHECK);
	}

	return TRUE;
}

BOOL	LoginServer_CM::ProcsMainServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	stMainServerInfo* pMainSI = (stMainServerInfo*)pSocket->pData;

	pMainSI->iConnectCount = PX_RECV_GET_UI32(pBuffer);

	return TRUE;
}

BOOL	LoginServer_CM::ProcsMainServerConnectPrepareResult_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	stMainServerInfo* pMainSI = (stMainServerInfo*)pSocket->pData;

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
			pToClient->WriteUI32(pSession->authKeyForMainServer);
			pToClient->WriteString(pMainSI->pServerInfo->public_ip);
			pToClient->WriteUI16(pMainSI->pServerInfo->port_client_TCP);
			pToClient->WriteUI16(pMainSI->pServerInfo->endMarker);
		}
		g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_MAIN_SERVER_INFO);
	}

	return	TRUE;
}

