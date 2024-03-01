#include "GlobalServer_CM.h"

void	GlobalServer_CM::InitPacketServerCBFunction()
{
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_END, 0, ProcsServerEnd_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_TYPE_ANNOUNCE, 0, ProcsServerTypeAnnounce_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_ALIVE_CHECK, 0, ProcsServerAliveCheck_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_ACCOUNT_CONNECT_UNIQUE, 0, ProcsAccountConnectUnique_v0);
}

BOOL	GlobalServer_CM::ProcsServerEnd_v0 (PX_SOCKET*	pSocket,BYTE*	pBuffer,int		buffLen)			// 서버 원격 제어등을 이용하여 종료시
{
	g_pIOCPEngine->ReserveEndEngine();
	return TRUE;
}

BOOL	GlobalServer_CM::ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLenn)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8 eServerType = PX_RECV_GET_UI8(pBuffer);
	UINT32 svrID = PX_RECV_GET_UI32(pBuffer);

	UINT16 index = m_pThis->GetIndexBySvrID(svrID);

	pSocket->SKUID = index;

	stServerInfo* pServerInfo = &m_pThis->m_aServerInfo[index];

	if (pServerInfo->svrID != m_pThis->m_iMySvrID &&
		pServerInfo->status == SVR_STATUS_NORMAL)
	{
		PX_SOCKET* pSvrSocket = (PX_SOCKET*)m_pThis->m_csAdminSvrSocket.GetObjectByKey(pServerInfo->svrID);

		if (pSvrSocket == NULL)
		{
			pSocket->pData = pServerInfo;
			m_pThis->m_csAdminSvrSocket.AddObject(pSocket, pServerInfo->svrID);
		}
	}

	if (eServerType == SVR_MAIN)
	{
		stMainServerInfo* pMainSvrInfo = (stMainServerInfo*)m_pThis->m_csAdminMainSvrInfo.GetObjectByKey(svrID);
		if (pMainSvrInfo != NULL)
		{
			pMainSvrInfo->pSocket = pSocket;
		}
	}
	else if (eServerType == SVR_ZONE)
	{
		stZoneServerInfo* pZoneSvrInfo = (stZoneServerInfo*)m_pThis->m_csAdminZoneSvrInfo.GetObjectByKey(svrID);
		if (pZoneSvrInfo != NULL)
		{
			pZoneSvrInfo->pSocket = pSocket;
		}
	}

	m_pThis->SendTimeTagInfo(pSocket);

	return TRUE;
}

BOOL	GlobalServer_CM::ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	PX_BUFFER* pToSvrReply = PxObjectManager::GetSendBufferObj(pSocket);
	if (pToSvrReply)
	{
		g_pIOCPEngine->SendBuffer(pToSvrReply, PKS_SERVER_ALIVE_CHECK);
	}

	return TRUE;
}

BOOL	GlobalServer_CM::ProcsAccountConnectUnique_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8	svrRegion = PX_RECV_GET_UI8(pBuffer);
	UINT32  targetSvrID = PX_RECV_GET_UI32(pBuffer);
	UINT64	ACGUID = PX_RECV_GET_UI64(pBuffer);

	stServerInfo* pServerInfo = &m_pThis->m_aServerInfo[pSocket->SKUID];
	if (pServerInfo->type == SVR_LOGIN)
	{
		for (int svrGrp = 0; svrGrp < m_pThis->m_iMainSvrGroupCount[svrRegion]; ++svrGrp)
		{
			for (int svrIdx = 0; svrIdx < m_pThis->m_iMainSvrIdxCount[svrRegion][svrGrp]; ++svrIdx)
			{
				if (m_pThis->m_stMainSvrInfos[svrRegion][svrGrp][svrIdx].pServerInfo->svrID != targetSvrID)
				{
					PX_SOCKET* pMainSvrSocket = m_pThis->m_stMainSvrInfos[svrRegion][svrGrp][svrIdx].pSocket;
					if (pMainSvrSocket != NULL)
					{
						PX_BUFFER* pToMainSvr = PxObjectManager::GetSendBufferObj(pMainSvrSocket);
						if (pToMainSvr)
						{
							pToMainSvr->WriteUI64(ACGUID);
							g_pIOCPEngine->SendBuffer(pToMainSvr, PKS_ACCOUNT_LOGOUT);
						}
					}
				}
			}
		}
	}
	else if(pServerInfo->type == SVR_MAIN)
	{
		for (int svrGrp = 0; svrGrp < m_pThis->m_iZoneSvrGroupCount[svrRegion]; ++svrGrp)
		{
			for (int svrIdx = 0; svrIdx < m_pThis->m_iZoneSvrIdxCount[svrRegion][svrGrp]; ++svrIdx)
			{
				if (m_pThis->m_stZoneSvrInfos[svrRegion][svrGrp][svrIdx].pServerInfo->svrID != targetSvrID)
				{
					PX_SOCKET* pZoneSvrSocket = m_pThis->m_stZoneSvrInfos[svrRegion][svrGrp][svrIdx].pSocket;
					if (pZoneSvrSocket != NULL)
					{
						PX_BUFFER* pToZoneSvr = PxObjectManager::GetSendBufferObj(pZoneSvrSocket);
						if (pToZoneSvr)
						{
							pToZoneSvr->WriteUI64(ACGUID);
							g_pIOCPEngine->SendBuffer(pToZoneSvr, PKS_ACCOUNT_LOGOUT);
						}
					}
				}
			}
		}
	}

	return TRUE;
}
