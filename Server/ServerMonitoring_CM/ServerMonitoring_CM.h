#ifndef	_SERVER_MONITORING_RG_H_
#define	_SERVER_MONITORING_RG_H_

#include "PxIOCPEngine.h"

#include "PxFrameMemory.h"
#include "PxDefine.h"
#include "PxMemory.h"
#include "PxAdmin.h"

#include "time.h"
#include "CrashRpt.h" // Include CrashRpt header

#include "../Common/THGUIDFactory.h"

#include "../ServerCommon/TemplateManager.h"
#include "../ServerCommon/PxPacketDefine.h"

#include "../ServerBase/ServerBase_RG.h"

#include <list>

enum SERVER_MORNITORING_STATUS
{
	SERVER_STATUS_NOT_CONNECTED = 0,
	SERVER_STATUS_CONNECTED = 1,
	SERVER_STATUS_RESET = 2
};

struct stServerMonitoringNode
{
	BOOL							isAvailable;
	UINT16							port;
	std::wstring					processName;
	std::wstring					exePath;

	UINT16							resetCount;

	int								svrindex;
	PX_SOCKET*						pSocket;
	SERVER_MORNITORING_STATUS		mornitoringStatus;
	time_t							lastConnectTryTime;
	time_t							lastServerAliveCheckTime;
	time_t							lastServerAliveCheckResponseTime;
	time_t							lastUIUpdateTime;
};

extern	PxIOCPEngine*		g_pIOCPEngine;

class	ServerMonitoring_RG : public ServerBase_RG
{
public:
	static ServerMonitoring_RG*					m_pThis;

	stServerMonitoringNode							m_stMornitoringNodes[3];

	ServerMonitoring_RG();
	~ServerMonitoring_RG();

	BOOL											Initialize(UINT32 svrID) override;
	void											Release() override;
	void											Update() override;

	static	BOOL									OnConnectSocket(PVOID	pData);
	static	BOOL									OnCloseSocket(PVOID		pData);
	static	BOOL									OnDBServerLost(PVOID	pData);

	std::string										ConvertToString(std::wstring	wstr);
	void											LoadFromJSon();
	void											SaveToJSon();
	void											ResetProcess(stServerMonitoringNode* pServerMonitoringNode);

	// Socket Callback Functions
	void											InitCBFunction();				// CB 을 지정하자
	static	BOOL									ProcServerEnd_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen);
	static	BOOL									ProcSvrServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL									ProcServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
};

#endif