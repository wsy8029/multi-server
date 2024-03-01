#ifndef	_TH_GUID_FACTORY_H_
#define _TH_GUID_FACTORY_H_

#include "PxDefine.h"
#include "PxObjectManager.h"
#include "PxIOCPEngine.h"

#include "THGUIDFactoryDefine.h"
#include "THMySQLConnector.h"

extern	PxIOCPEngine*		g_pIOCPEngine;

#define	GUID_SERVER_START_SEC				1658194544
#define	GUID_SERVER_TIME_STAMP_ADD_SEC		600
#define	GUID_SERVER_CHUNK_COUNT_MAX			0xFFF

// 20 bit | 12 bit | 8 bit | 24 bit
// timestamp | chunk | type | instanceID

class	THGUIDFactory
{
public:
	static THGUIDFactory*		m_pThis;
	UINT32				m_iTimeTag;
	BOOL				m_bNeedNextTimeTag;
	BOOL				m_bSendedNextTimeTagRequest;
	BOOL				m_bHasNextTimeTag;
	UINT32				m_iNextTimeTag;

	UINT32				m_aInstanceCount[TGO_TYPE_MAX];
	UINT32				m_iInstanceCountMax;

	CRITICAL_SECTION	m_csCritSec;

	//@Self
	sql::PreparedStatement* m_cmdGetChunkCount;
	sql::PreparedStatement* m_cmdUpdateChunkCount;
	sql::PreparedStatement* m_cmdInsertChunkCount;
	std::string			m_strGUIDDBTable;
	UINT8				m_iGUIDChunkCountResetSkipCount;
	UINT32				m_iLastGUIDTimeStamp;				// add per 10 min
	UINT32				m_iGUIDTimeStamp;					// add per 10 min
	UINT16				m_iGUIDChunkCount;					// add per request tag (max 4096)
	//@Self

	_PX_SOCKET*			m_pSvrSocket;

	THGUIDFactory();
	~THGUIDFactory();

	void				Init(THMySQLConnector*	pDBConn);
	void				Release();
	void				Update();
	void				Update(time_t	currentTime);

	void				ResetInstanceCountArray();
	void				ReserveNextTimeTag(UINT32	timeTag);
	void				SetTimeTag(UINT32			timeTag);

	UINT64				RequestGUID(THGUID_OBJ_TYPE		type);

	void				ProcessChunkCountAdd();

	static	BOOL		DBPrepareCommands(THMySQLConnector* pConn);				// DB Command들을 미리 준비하자.

	UINT16				DBGetChunkCount(UINT32	timeStamp);
	void				DBUpdateChunkCount(UINT32	timeStamp, UINT16	chunkCount);
	void				DBInsertChunkCount(UINT32	timeStamp, UINT16	chunkCount);
};

#endif