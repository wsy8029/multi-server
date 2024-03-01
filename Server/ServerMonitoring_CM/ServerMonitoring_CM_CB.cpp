#include "ServerMonitoring_RG.h"

void	ServerMonitoring_RG::InitCBFunction()
{
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_END, 0, ProcServerEnd_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_TYPE_ANNOUNCE, 0, ProcSvrServerTypeAnnounce_v0);
	g_pIOCPEngine->SetPacketServerCB(PKS_SERVER_ALIVE_CHECK, 0, ProcServerAliveCheck_v0);
}

BOOL	ServerMonitoring_RG::ProcServerEnd_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)			// 서버 원격 제어등을 이용하여 종료시
{
	g_pIOCPEngine->ReserveEndEngine();
	return TRUE;
}

BOOL	ServerMonitoring_RG::ProcSvrServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	UINT8 eServerType = PX_RECV_GET_UI8(pBuffer);
	UINT8 svrIdx = PX_RECV_GET_UI8(pBuffer);

	if (eServerType == SVR_ZONE)
	{

	}

	return TRUE;
}

BOOL	ServerMonitoring_RG::ProcServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen)
{
	if (pSocket->bInsideConnect == FALSE)	return FALSE;

	stServerMonitoringNode* pMonitoringNode = (stServerMonitoringNode*)pSocket->pData;
	if (pMonitoringNode == NULL)	return FALSE;

	pMonitoringNode->lastServerAliveCheckResponseTime = m_pThis->m_iCurrentTime;

	return TRUE;
}

