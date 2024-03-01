#ifndef	_GLOBAL_SERVER_CM_H_
#define	_GLOBAL_SERVER_CM_H_

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

#include "../ServerBase/ServerBase_CM.h"

#include "GlobalServer_CM_Define.h"

#include <list>

extern	PxIOCPEngine*		g_pIOCPEngine;
extern	TemplateManager		g_csTemplateManager;

struct stMainServerInfo
{
	stServerInfo*										pServerInfo;
	PX_SOCKET*											pSocket;
	BOOL												isRegionServerRole;

	void Init()
	{
		pServerInfo = NULL;
		pSocket = NULL;
		isRegionServerRole = FALSE;
	}
};

struct stZoneServerInfo
{
	stServerInfo*										pServerInfo;
	PX_SOCKET*											pSocket;

	void Init()
	{
		pServerInfo = NULL;
		pSocket = NULL;
	}
};

class	GlobalServer_CM : public ServerBase_CM
{
public:
	static GlobalServer_CM*								m_pThis;
	
	UINT8												m_iGUIDChunkCountResetSkipCount;
	UINT32												m_iLastGUIDTimeStamp;				// add per 10 min
	UINT32												m_iGUIDTimeStamp;					// add per 10 min
	UINT16												m_iGUIDChunkCount;					// add per request tag (max 4096)

	// DB Commands
	THMySQLPrepareStatement								m_cmdGetChunkCount;
	THMySQLPrepareStatement								m_cmdUpdateChunkCount;
	THMySQLPrepareStatement								m_cmdInsertChunkCount;

	PxAdminUI32											m_csAdminSvrSocket;

	UINT32												m_iLoginSvrIndex;
	PX_SOCKET*											m_pLoginSvrSocket;

	time_t												m_iMainSvrInfoUpdateTime;
	UINT8												m_iMainSvrRegionCount;
	UINT8												m_iMainSvrGroupCount[16];				// [region]
	UINT8												m_iMainSvrIdxCount[16][32];				// [region][group]
	stMainServerInfo									m_stMainSvrInfos[16][32][128];			// [region][group][idx]
	PxAdminUI32											m_csAdminMainSvrInfo;
	
	UINT8												m_iZoneSvrRegionCount;
	UINT8												m_iZoneSvrGroupCount[16];				// [region]
	UINT8												m_iZoneSvrIdxCount[16][32];				// [region][group]
	stZoneServerInfo									m_stZoneSvrInfos[16][32][128];			// [region][group][idx]
	PxAdminUI32											m_csAdminZoneSvrInfo;

	GlobalServer_CM();
	~GlobalServer_CM();

	BOOL												Initialize(UINT32 svrID) override;
	void												Release() override;
	void												Update() override;

	BOOL												InitDBCashes();

	BOOL												PrepareDBCommands(THMySQLConnector* pConn) override;		// DB Command들을 미리 준비하자.
	void												ReleaseDBCommands() override;

	// GUID 관리
	void												ProcessChunkCountAdd();
	void												SendTimeTagInfoAll();
	void												SendTimeTagInfo(PX_SOCKET* pSocket);

	UINT16												DBGetChunkCount(UINT32	timeStamp);
	void												DBUpdateChunkCount(UINT32	timeStamp, UINT16	chunkCount);
	void												DBInsertChunkCount(UINT32	timeStamp, UINT16	chunkCount);

	void												SetServerInfos();
	BOOL												ConnectServers();

	void												UpdateMainSvrInfo();
	
	static	BOOL										OnConnectSocket(PVOID	pData);
	static	BOOL										OnCloseSocket(PVOID		pData);
	static	BOOL										OnDBServerLost(PVOID	pData);

	// Socket Callback Functions
	void												InitPacketServerCBFunction();		// 서버 패킷 CB 을 지정하자
	
	static	BOOL										ProcsServerEnd_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen);
	static	BOOL										ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsAccountConnectUnique_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
};

#endif
