#ifndef	_MAIN_SERVER_CM_H_
#define	_MAIN_SERVER_CM_H_

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

#include "MainServer_CM_Define.h"
#include "../GraphQL/CMDefineGraphQL.h"
#include "../GraphQL/GraphQLManager.h"

#include <list>

extern	PxIOCPEngine*		g_pIOCPEngine;
extern  PxWebSocketEngine*	g_pWSEngine;
extern	TemplateManager		g_csTemplateManager;
extern	GraphQLManager		g_csGraphQLManager;

struct	stClientSession :public stMemoryPoolItem, stClientSessionBase
{
	SPWSSession									pWSSession;

	UINT8										platformType;

	UINT8										connectState;						// EServerConnectState
	UINT8										aliveCheckCount;
	UINT32										aliveCheckTick;
	UINT8										isRequestDisconn;

	UINT32										authKeyFromLoginServer;
	UINT32										authKeyForZoneServer;

	stAccount									account;
	stPlayerCharacter*							pPC;
	CHAR										accessToken[GQL_TOKEN_SIZE];
	CHAR										refreshToken[GQL_TOKEN_SIZE];

	stClientSession*							prev;								// 순회용
	stClientSession*							next;

	void	RefreshAliveCheck()
	{
		aliveCheckTick = 0;
		aliveCheckCount = 0;
	}

	void	Init()
	{
		authKeyFromLoginServer = 0;
		authKeyForZoneServer = 0;

		aliveCheckTick = 0;
		aliveCheckCount = 0;
		isRequestDisconn = 0;

		pPC = NULL;
		prev = next = NULL;
	}
};

struct	stZoneServerInfo
{
	stServerInfo*				pServerInfo;
	PX_SOCKET*					pSocket;
	UINT32						iAccountCount;
	UINT32						iChannelCount;
};

#define		AUTH_KEY_TABLE_COUNT		65536
class	MainServer_CM : public ServerBase_CM
{
public:
	static MainServer_CM*								m_pThis;
	
	PxAdminUI64											m_csAdminClientSession;
	stClientSession*									m_listSession;					// 순회용
	UINT32												m_iSessionCount;

	// 로그인, 글로벌 서버와 항상 접속을 유지하려 한다. 일정시간마다 접속 재시도..
	UINT32												m_iGlobalSvrIndex;
	PX_SOCKET*											m_pGlobalSvrSocket;
	UINT32												m_iLoginSvrIndex;
	PX_SOCKET*											m_pLoginSvrSocket;
	time_t												m_iSvrConnectRetryTime;

	// 같은 서버 지역 및 그룹 에 속하는 존서버들을 관리.. 존 서버 분배 역할을 하므로..
	UINT8												m_iZoneSvrCount;
	stZoneServerInfo									m_stZoneSvrInfos[32];
	UINT32												m_iRecommendZoneSvrID;
	PxAdminUI32											m_csAdminZoneSvrInfo;

	time_t												m_iSvrInfoUpdateTime;

	UINT32												m_pAuthKeyTableForZoneSvr[AUTH_KEY_TABLE_COUNT];
	UINT32												m_iAuthKeyIndexForZoneSvr;

	std::list<stCharacterInfo*>							m_listCharacterInfo;
	PxAdminUI64_CharacterInfo							m_csAdminCharacterInfo;

	BOOL												m_bRegionServerRole;

	// struct pool
	_MemoryPool<stClientSession>*						m_pPoolClientSession;
	_MemoryPool<stPlayerCharacter>*						m_pPoolPlayerCharacter;
	_MemoryPool<stCharacterInfo>*						m_pPoolCharacterInfo;
	_MemoryPool<stAccountItem>*							m_pPoolAccountItem;
	_MemoryPool<stCharacterItem>*						m_pPoolCharacterItem;
	_MemoryPool<stItem>*								m_pPoolItem;

	// DB Commands
	THMySQLPrepareStatement								m_cmdGetCharacter1;

	THMySQLPrepareStatement								m_cmdUpdateCharacter1;

	THMySQLPrepareStatement								m_cmdInsertErrorLog1;
	THMySQLPrepareStatement								m_cmdInsertCharacter1;

	THMySQLPrepareStatement								m_cmdDeleteCharacter1;

	MainServer_CM();
	~MainServer_CM();

	BOOL												Initialize(UINT32 svrID) override;
	void												Release() override;
	void												Update() override;

	void												InitServerInfos();
	BOOL												ConnectServers();

	UINT32												GetNextAuthKeyForZoneSvr();

	stClientSession*									GetClientSession();
	void												FreeClientSession(stClientSession*	pSession);
	void												FreeClientSessionData(stClientSession* pSession);
	stClientSession*									GetConnectedClientSession(UINT64 SHPGUID);

	void												ReleasePlayerCharacter(stPlayerCharacter* pPC);
	void												ReleaseAccountItem(stAccountItem* pACItem);
	void												ReleaseCharacterItem(stCharacterItem* pCHItem);
	void												ReleaseItem(stItem* pItem);

	void												InitPlayerCharacterByMyProfile(stPlayerCharacter* pPC, NestoQL::MyProfileOutput& myProfile);

	BOOL												InitDBCashes();
	void												DBGet_Auth(stClientSession* pSession);				// Auth 후 DB 에서 데이터를 가져온다.
	void												DBGet_Connect(stClientSession* pSession);			// Connect 후 DB 에서 데이터를 가져온다.

	BOOL												PrepareDBCommands(THMySQLConnector* pConn) override;		// DB Command들을 미리 준비하자.
	void												ReleaseDBCommands() override;

	void												UpdateSvrInfos();
	UINT32												FindBestZoneSvrID();

	static	BOOL										OnConnectSocket(PVOID	pData);
	static	BOOL										OnCloseSocket(PVOID		pData);
	static	BOOL										OnDBServerLost(PVOID	pData);

	static	BOOL										OnConnectWSSession(SPWSSession pWSSession);
	static	BOOL										OnCloseWSSession(SPWSSession pWSSession);

	// Socket Callback Functions
	void												InitPacketClientCBFunction();		// 클라이언트 패킷 CB 을 지정하자
	void												InitPacketServerCBFunction();		// 서버 패킷 CB 을 지정하자
	void												InitGraphQLCBFunction();

	// server
	static	BOOL										ProcsServerEnd_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen);
	static	BOOL										ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsRequestTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcsInformTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcMainServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcZoneServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsMainServerConnectPrepare_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcZoneServerConnectPrepareResult_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsAccountLogout_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsSetRegionServerRole_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);

	// graphQL
	static	BOOL										ProcGQLSignIn(stGraphQLJob* pJob);
	static	BOOL										ProcGQLMyProfile(stGraphQLJob* pJob);
	
	// client
	static	BOOL										ProcHandShake_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAliveCheck_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcErrorLog_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcConnectMainServerAuth_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAccountLogout_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);

	static	BOOL										ProcRequestZoneEnter_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
};

#endif
