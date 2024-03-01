#ifndef	_ZONE_SERVER_CM_H_
#define	_ZONE_SERVER_CM_H_

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

#include "ZoneServer_CM_Define.h"
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
	SPWSSession									pWSSession;
	UINT32										authKeyFromMainServer;

	UINT8										platformType;
	UINT8										connectState;						// SERVER_CONN_STATE

	UINT8										aliveCheckCount;
	UINT32										aliveCheckTick;
	UINT8										isRequestDisconn;

	stAccount									account;
	stPlayerCharacter*							pPC;

	stClientSession*							prev;								// 순회용
	stClientSession*							next;

	void	RefreshAliveCheck()
	{
		aliveCheckTick = 0;
		aliveCheckCount = 0;
	}

	void	Init()
	{
		authKeyFromMainServer = 0;
		aliveCheckTick = 0;
		aliveCheckCount = 0;
		isRequestDisconn = 0;
		pPC = NULL;

		prev = next = NULL;
	}
};

class	ZoneServer_CM : public ServerBase_CM
{
public:
	static ZoneServer_CM*								m_pThis;
	
	PxAdminUI64											m_csAdminClientSession;
	stClientSession*									m_listSession;					// 순회용
	UINT32												m_iSessionCount;
	UINT32												m_iChannelCount;

	// DB Commands
	THMySQLPrepareStatement								m_cmdUpdateCharacter1;
	
	THMySQLPrepareStatement								m_cmdInsertErrorLog1;

	// 로그인, 글로벌 서버와 항상 접속을 유지하려 한다. 일정시간마다 접속 재시도..
	UINT32												m_iGlobalSvrIndex;
	PX_SOCKET*											m_pGlobalSvrSocket;
	UINT32												m_iMainSvrIndex;
	PX_SOCKET*											m_pMainSvrSocket;
	time_t												m_iSvrConnectRetryTime;

	std::list<stCharacterInfo*>							m_listCharacterInfo;
	PxAdminUI64_CharacterInfo							m_csAdminCharacterInfo;
	std::list<stZone*>									m_listZone;
	PxAdminUI32											m_csAdminZone;

	// struct pool
	_MemoryPool<stClientSession>*						m_pPoolClientSession;
	_MemoryPool<stPlayerCharacter>*						m_pPoolPlayerCharacter;
	_MemoryPool<stNPC_Equip>*							m_pPoolNPC_Equip;
	_MemoryPool<stNPC>*									m_pPoolNPC;
	_MemoryPool<stCharacterInfo>*						m_pPoolCharacterInfo;
	_MemoryPool<stAccountItem>*							m_pPoolAccountItem;
	_MemoryPool<stCharacterItem>*						m_pPoolCharacterItem;
	_MemoryPool<stItem>*								m_pPoolItem;
	_MemoryPool<stZone>*								m_pPoolZone;
	_MemoryPool<stChannel>*								m_pPoolChannel;
	_MemoryPool<stSubZone>*								m_pPoolSubZone;

	ZoneServer_CM();
	~ZoneServer_CM();

	BOOL												Initialize(UINT32 svrID) override;
	void												Release() override;
	void												Update() override;

	BOOL												ConnectServers();

	stClientSession*									GetClientSession();
	void												FreeClientSession(stClientSession*	pSession);
	void												FreeClientSessionData(stClientSession* pSession);
	stClientSession*									GetConnectedClientSession(UINT64 SHPGUID);

	BOOL												InitDBCashes();
	void												DBGet_Auth(stClientSession* pSession);				// Auth 후 DB 에서 데이터를 가져온다.
	void												DBGet_Connect(stClientSession* pSession);			// Connect 후 DB 에서 데이터를 가져온다.
	void												DBProcUpdatePlayerCharacter1(stPlayerCharacter* pPC);

	BOOL												PrepareDBCommands(THMySQLConnector* pConn) override;		// DB Command들을 미리 준비하자.
	void												ReleaseDBCommands() override;

	BOOL												InitGameDatas();
	void												ReleaseGameDatas();
	stChannel*											GetAvailableChannel(UINT32 mapTID);

	void												UpdateZone(stZone* pZone);
	void												UpdateChannel(stChannel* pChannel);
	void												UpdateSubZone(stSubZone* pSubZone);

	void												AddCharacterToZone(stZone* pZone, stCharacter* pCha);
	void												RemoveCharacterFromZone(stZone* pZone, stCharacter* pCha);
	void												AddCharacterToChannel(stChannel* pChannel, stCharacter* pCha, BOOL isBroadcast = TRUE);
	void												RemoveCharacterFromChannel(stChannel* pChannel, stCharacter* pCha, BOOL isBroadcast = TRUE);
	void												AddCharacterToSubZone(stSubZone* pSubZone, stCharacter* pCha);
	void												RemoveCharacterFromSubZone(stSubZone* pSubZone, stCharacter* pCha);

	void												ReleasePlayerCharacter(stPlayerCharacter* pPC);
	void												ReleaseNPC_Equip(stNPC_Equip* pNPCEquip);
	void												ReleaseNPC(stNPC* pNPC);
	void												ReleaseAccountItem(stAccountItem* pACItem);
	void												ReleaseCharacterItem(stCharacterItem* pCHItem);
	void												ReleaseItem(stItem* pItem);
	void												ReleaseZone(stZone* pZone);
	void												ReleaseChannel(stChannel* pChannel);
	void												ReleaseSubZone(stSubZone* pSubZone);

	static	BOOL										OnConnectSocket(PVOID	pData);
	static	BOOL										OnCloseSocket(PVOID		pData);
	static	BOOL										OnDBServerLost(PVOID	pData);

	static	BOOL										OnConnectWSSession(SPWSSession pWSSession);
	static	BOOL										OnCloseWSSession(SPWSSession pWSSession);

	// Socket Callback Functions
	void												InitPacketClientCBFunction();		// 클라이언트 패킷 CB 을 지정하자
	void												InitPacketServerCBFunction();		// 서버 패킷 CB 을 지정하자
	void												InitGraphQLCBFunction();

	//server
	static	BOOL										ProcsServerEnd_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int		buffLen);
	static	BOOL										ProcsServerTypeAnnounce_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsRequestTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcsInformTimeTag_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcsServerAliveCheck_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcZoneServerInfo_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcZoneServerConnectPrepare_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcInitServerData_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int buffLen);
	static	BOOL										ProcsAccountLogout_v0(PX_SOCKET* pSocket, BYTE* pBuffer, int	buffLen);

	// graphQL
	static	BOOL										ProcGQLSignIn(stGraphQLJob* pJob);
	
	// client
	static	BOOL										ProcHandShake_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAliveCheck_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcErrorLog_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcTransformSync_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAnimatorSync_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcDataSync_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcConnectZoneServerAuth_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcAccountLogout_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcZoneEnter_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcZoneChange_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcZoneCharacterLeave_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
	static	BOOL										ProcChatting_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen);
};

#endif
