#ifndef	_SERVER_BASE_CM_H_
#define	_SERVER_BASE_CM_H_

#include "PxIOCPEngine.h"

#include "PxFrameMemory.h"
#include "PxDefine.h"
#include "PxMemory.h"
#include "PxAdmin.h"

#include "time.h"
#include "CrashRpt.h" // Include CrashRpt header

#include "../Common/THGUIDFactory.h"
#include "../Common/MemoryPool.h"

#include "../ServerCommon/MyCommonDefine.h"
#include "../ServerCommon/PxPacketDefine.h"

#include <list>

#include "../Common/THMySQLPrepareStatement.h"

extern	PxIOCPEngine*			g_pIOCPEngine;

class	ServerBase_CM
{
public:
	static	ServerBase_CM*							m_pThisBase;
	BOOL											m_bLoopStop;
	UINT32											m_iServerNum;
	stServerInfo									m_aServerInfo[128];
	
	UINT32											m_iMySvrID;
	UINT8											m_iMySvrRegion;
	UINT8											m_iMySvrGroup;
	UINT16											m_iMySvrIndex;
	UINT16											m_iMySvrPort[2];

	time_t											m_iCurrentTime;
	tm												m_tmCurrentTime;
	CHAR											m_strCurrentTime[128];
	UINT32											m_iCurTick;
	UINT32											m_iCurTickDiff;

	THGUIDFactory									m_csGUIDFactory;

	// DB Objects
	sql::Driver*									m_pDriver;
	THMySQLConnector								m_csDBConn[DB_CONN_COUNT];

	THMySQLPrepareStatement							m_cmdGetServerIP1;

	ServerBase_CM();
	~ServerBase_CM();

	virtual BOOL									Initialize(UINT32 svrID);
	virtual void									Release();
	virtual void									Update();
	void											PostUpdate();

	BOOL											GetServerIPTables();
	
	static BOOL										OnDBServerLost(PVOID	pData);
	static BOOL										FPPrepareDBCommands(THMySQLConnector* pConn);			// DB Command들을 미리 준비하자.
	virtual BOOL									PrepareDBCommands(THMySQLConnector* pConn);				// DB Command들을 미리 준비하자.
	virtual void									ReleaseDBCommands();

	UINT16											GetSvrIndexBySvrID(UINT32 svrID);
	UINT8											GetIndexBySvrID(UINT32 svrID);
};

#endif