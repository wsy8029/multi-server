#ifndef	_LOGIN_SERVER_CM_H_
#define	_LOGIN_SERVER_CM_H_

#include "PxIOCPEngine.h"
#include "WebSocket/PxWebSocketEngine.h"

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

#include "LoginServer_CM_Define.h"
#include "../GraphQL/CMDefineGraphQL.h"
#include "../GraphQL/GraphQLManager.h"

#include <list>

extern	PxIOCPEngine*		g_pIOCPEngine;
extern  PxWebSocketEngine*	g_pWSEngine;
extern	TemplateManager		g_csTemplateManager;
extern	GraphQLManager		g_csGraphQLManager;

struct	stClientSession :public stMemoryPoolItem
{
public:
	SPWSSession											pWSSession;

	UINT8												platformType;
	UINT8												selectedSvrRegion;

	UINT8												aliveCheckCount;
	UINT32												aliveCheckTick;
	UINT8												isRequestDisconn;

	UINT64												ACGUID;
	CHAR												accessToken[GQL_TOKEN_SIZE];
	CHAR												refreshToken[GQL_TOKEN_SIZE];

	UINT32												authKeyForMainServer;

	stClientSession*									prev;								// 순회용
	stClientSession*									next;

	void	RefreshAliveCheck()
	{
		aliveCheckTick = 0;
		aliveCheckCount = 0;
	}

	void	Init()
	{
		pWSSession = nullptr;
		aliveCheckTick = 0;
		aliveCheckCount = 0;
		isRequestDisconn = 0;

		prev = next = NULL;
	}
};

struct	stMainServerInfo
{
	stServerInfo*										pServerInfo;
	PX_SOCKET*											pSocket;
	UINT32												iConnectCount;

	void Init()
	{
		pServerInfo = NULL;
		pSocket = NULL;
		iConnectCount = 0;
	}
};

struct stClientInfo
{
	UINT8												platformType;
	UINT16												clientVersion;
	std::string											marketURL;
};

#define		AUTH_KEY_TABLE_COUNT		65536
class	LoginServer_CM : public ServerBase_CM
{
public:
	static LoginServer_CM*								m_pThis;
	
	PxAdminUI64											m_csAdminClientSession;
	stClientSession*									m_listSession;					// 순회용
	UINT32												m_iSessionCount;

	// DB Commands
	THMySQLPrepareStatement								m_cmdGetClientInfo1;
	THMySQLPrepareStatement								m_cmdGetAccount1;
	
	THMySQLPrepareStatement								m_cmdInsertErrorLog1;
	THMySQLPrepareStatement								m_cmdInsertAccount1;
	
	THMySQLPrepareStatement								m_cmdUpdateAccount1;

	UINT32												m_iGlobalSvrIndex;
	PX_SOCKET*											m_pGlobalSvrSocket;

	// 로그인 서버는 서버들의 인원수 및 접속 상태를 관리한다. 메인 서버 분배 역할을 하므로..
	UINT8												m_iMainSvrRegionCount;
	UINT8												m_iMainSvrGroupCount[16];				// [region]
	UINT8												m_iMainSvrIdxCount[16][32];				// [region][group]
	stMainServerInfo									m_stMainSvrInfos[16][32][128];			// [region][group][idx]
	UINT32												m_iRecommendMainSvrID[32];				// [region]
	PxAdminUI32											m_csAdminMainSvrInfo;

	time_t												m_iSvrUpdateTime;
	time_t												m_iSvrConnectRetryTime;					// 모든 서버와 항상 접속을 유지하려 한다. 일정시간마다 접속 재시도..

	UINT32												m_pAuthKeyTable[AUTH_KEY_TABLE_COUNT];
	UINT32												m_iAuthKeyIndex;

	stClientInfo										m_stClientInfo[EDP_Count];

	// struct pool
	_MemoryPool<stClientSession>*						m_pPoolClientSession;

	LoginServer_CM();
	~LoginServer_CM();

	BOOL												Initialize(UINT32 svrID) override;
	void												Release() override;
	void												Update() override;

	void												InitServerInfos();
	BOOL												ConnectServers();

	UINT32												GetNextAuthKey();

	stClientSession*									GetClientSession();
	void												FreeClientSession(stClientSession*	pSession);
	void												FreeClientSessionData(stClientSession* pSession);
	stClientSession*									GetConnectedClientSession(UINT64 ACGUID);

	BOOL												PrepareDBCommands(THMySQLConnector* pConn) override;		// DB Command들을 미리 준비하자.
	void												ReleaseDBCommands() override;
	BOOL												InitDBCashes();
	BOOL												DBGetClientInfo();
	void												DBGet_Auth(stClientSession* pSession);				// Auth 후 DB 에서 데이터를 가져온다.
	void												DBGet_Connect(stClientSession* pSession);			// Connect 후 DB 에서 데이터를 가져온다.

	void												UpdateSvrInfos();
	UINT32												FindBestMainSvrID(UINT8	svrRegion);
	
	static	BOOL										OnConnectSocket(PVOID	pData);
	static	BOOL										OnCloseSocket(PVOID		pData);
	static	BOOL										OnDBServerLost(PVOID	pData);

	static	BOOL										OnConnectWSSession(SPWSSession pWSSession);
	static	BOOL										OnCloseWSSession(SPWSSession pWSSession);

	// Socket Callback Functions
	void												InitPacketClientCBFunction();		// 클라이언트 패킷 CB 을 지정하자
	void												InitPacketServerCBFunction();		// 서버 패킷 CB 을 지정하자

	static	BOOL										ProcsServerEnd_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcsRequestTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcsInformTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcsMainServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsMainServerConnectPrepareResult_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	
	static	BOOL										ProcHandShake_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcInformSend_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAliveCheck_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcErrorLog_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);

	static	BOOL										ProcAccountLogin_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAccountLogout_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
};

#endif
