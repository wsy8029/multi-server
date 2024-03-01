#include "THGUIDFactory.h"

THGUIDFactory* THGUIDFactory::m_pThis = NULL;

THGUIDFactory::THGUIDFactory()
{
	m_pThis = this;

	m_iTimeTag = 0;
	m_bNeedNextTimeTag = FALSE;
	m_bSendedNextTimeTagRequest = FALSE;
	m_bHasNextTimeTag = FALSE;
	m_iNextTimeTag = 0;

	m_pSvrSocket = NULL;

	m_iGUIDChunkCountResetSkipCount = 0;
	m_iLastGUIDTimeStamp = 0;
	m_iGUIDTimeStamp = 0;
	m_iGUIDChunkCount = 0;
	m_strGUIDDBTable = "";

	m_cmdGetChunkCount = NULL;
	m_cmdUpdateChunkCount = NULL;
	m_cmdInsertChunkCount = NULL;
}

THGUIDFactory::~THGUIDFactory()
{
}

void		THGUIDFactory::Init(THMySQLConnector* pDBConn)
{
	InitializeCriticalSection(&m_csCritSec);

	ResetInstanceCountArray();

#ifdef GUID_FACTORY_SELF
	pDBConn->m_listFuncDBPrepare.push_back(DBPrepareCommands);
	DBPrepareCommands(pDBConn);
#endif

}

void		THGUIDFactory::Release()
{
	DeleteCriticalSection(&m_csCritSec);
}

BOOL	THGUIDFactory::DBPrepareCommands(THMySQLConnector* pConn)
{
	if (m_pThis->m_cmdGetChunkCount != NULL)
	{
		delete m_pThis->m_cmdGetChunkCount;
		m_pThis->m_cmdGetChunkCount = NULL;
	}

	if (m_pThis->m_cmdUpdateChunkCount != NULL)
	{
		delete m_pThis->m_cmdUpdateChunkCount;
		m_pThis->m_cmdUpdateChunkCount = NULL;
	}

	if (m_pThis->m_cmdInsertChunkCount != NULL)
	{
		delete m_pThis->m_cmdInsertChunkCount;
		m_pThis->m_cmdInsertChunkCount = NULL;
	}

	m_pThis->m_cmdGetChunkCount = pConn->m_pConn->prepareStatement((char*)(std::string("SELECT chunkCount FROM " + m_pThis->m_strGUIDDBTable + " WHERE timeStamp=?").c_str()));
	m_pThis->m_cmdUpdateChunkCount = pConn->m_pConn->prepareStatement((char*)(std::string("UPDATE " + m_pThis->m_strGUIDDBTable + " SET chunkCount=? WHERE timeStamp=?").c_str()));
	m_pThis->m_cmdInsertChunkCount = pConn->m_pConn->prepareStatement((char*)(std::string("INSERT INTO " + m_pThis->m_strGUIDDBTable + "(timeStamp,chunkCount) VALUES(?,?)").c_str()));

	time_t		iCurrentTime;
	time(&iCurrentTime);
	UINT64		elapsedSec = (UINT64)iCurrentTime - GUID_SERVER_START_SEC;
	m_pThis->m_iGUIDTimeStamp = (UINT32)(elapsedSec / GUID_SERVER_TIME_STAMP_ADD_SEC);
	m_pThis->m_iLastGUIDTimeStamp = m_pThis->m_iGUIDTimeStamp;
	m_pThis->m_iGUIDChunkCount = m_pThis->DBGetChunkCount(m_pThis->m_iGUIDTimeStamp);

	m_pThis->ProcessChunkCountAdd();
	UINT32	timeTag = m_pThis->m_iGUIDTimeStamp << 12 | m_pThis->m_iGUIDChunkCount;
	m_pThis->SetTimeTag(timeTag);

	return TRUE;
}

void		THGUIDFactory::Update()
{
	time_t		iCurrentTime;
	time(&iCurrentTime);
	Update(iCurrentTime);
}

void		THGUIDFactory::Update(time_t	currentTime)
{
	EnterCriticalSection(&m_csCritSec);

#ifdef GUID_FACTORY_SELF
	UINT64		elapsedSec = (UINT64)currentTime - GUID_SERVER_START_SEC;
	UINT32		timeStamp = (UINT32)(elapsedSec / GUID_SERVER_TIME_STAMP_ADD_SEC);

	if (timeStamp != m_iLastGUIDTimeStamp)
	{
		if (m_iGUIDChunkCountResetSkipCount <= 0)
		{
			m_iGUIDTimeStamp = timeStamp;
			m_iLastGUIDTimeStamp = timeStamp;
			m_iGUIDChunkCount = DBGetChunkCount(timeStamp);
		}
		else
		{
			m_iLastGUIDTimeStamp = timeStamp;
			--m_iGUIDChunkCountResetSkipCount;
		}
	}

	if (m_bNeedNextTimeTag)
	{
		ProcessChunkCountAdd();
		UINT32	timeTag = m_iGUIDTimeStamp << 12 | m_iGUIDChunkCount;

		ReserveNextTimeTag(timeTag);
		m_bNeedNextTimeTag = FALSE;
	}
#else
	if (m_bNeedNextTimeTag)
	{
		PX_BUFFER* pToGUIDSvr = PxObjectManager::GetSendBufferObj(m_pSvrSocket);
		if (pToGUIDSvr)
		{
			pToGUIDSvr->WriteUI8(0);
			g_pIOCPEngine->SendBuffer(pToGUIDSvr, PKS_REQUEST_TIME_TAG);

			m_bNeedNextTimeTag = FALSE;
			m_bSendedNextTimeTagRequest = TRUE;
		}
	}
#endif

	LeaveCriticalSection(&m_csCritSec);
}

void	THGUIDFactory::ProcessChunkCountAdd()
{
	if (m_iGUIDChunkCount == GUID_SERVER_CHUNK_COUNT_MAX)
	{
		++m_iGUIDTimeStamp;
		m_iGUIDChunkCount = 0;
		++m_iGUIDChunkCountResetSkipCount;

		DBInsertChunkCount(m_iGUIDTimeStamp, m_iGUIDChunkCount);
	}
	else
	{
		++m_iGUIDChunkCount;
		DBUpdateChunkCount(m_iGUIDTimeStamp, m_iGUIDChunkCount);
	}
}

void		THGUIDFactory::ReserveNextTimeTag(UINT32	timeTag)
{
	EnterCriticalSection(&m_csCritSec);

	m_bNeedNextTimeTag = FALSE;
	m_bSendedNextTimeTagRequest = FALSE;

	if(m_iInstanceCountMax >= 0xFFFF)
	{
		m_iTimeTag = timeTag;

		m_bHasNextTimeTag = FALSE;
		m_iNextTimeTag = 0;

		ResetInstanceCountArray();
	}
	else
	{
		m_bHasNextTimeTag = TRUE;
		m_iNextTimeTag = timeTag;
	}

	LeaveCriticalSection(&m_csCritSec);
}

void		THGUIDFactory::SetTimeTag(UINT32	timeTag)
{
	EnterCriticalSection(&m_csCritSec);

	m_iTimeTag = timeTag;

	m_bNeedNextTimeTag = FALSE;
	m_bHasNextTimeTag = FALSE;
	m_iNextTimeTag = 0;

	ResetInstanceCountArray();

	LeaveCriticalSection(&m_csCritSec);
}

UINT64		THGUIDFactory::RequestGUID(THGUID_OBJ_TYPE		type)
{
	EnterCriticalSection(&m_csCritSec);

	UINT32		count = m_aInstanceCount[type];

	if(count >= 0xFFFFFF || m_iTimeTag == 0)
	{
		LeaveCriticalSection(&m_csCritSec);
		return 0;		// false...
	}

	++m_aInstanceCount[type];
	count = m_aInstanceCount[type];

	if(count > m_iInstanceCountMax)
	{
		m_iInstanceCountMax = count;

		if(count > 0x8000 && m_bSendedNextTimeTagRequest == FALSE && m_bHasNextTimeTag == FALSE)
		{
			m_bNeedNextTimeTag = TRUE;
		}

		if(count > 0xFFFF)
		{
			if(m_bHasNextTimeTag)
			{
				m_iTimeTag = m_iNextTimeTag;
				m_bHasNextTimeTag = FALSE;

				ResetInstanceCountArray();
				count = 0;
			}
		}
	}

	//UINT64	guid = m_iTimeTag << 32 | ((UINT8)type) << 24 | count;

	UINT64	guid = m_iTimeTag;
	guid = guid << 32;
	UINT32	val2 = ((UINT8)type) << 24 | count;
	guid = guid | val2;

	LeaveCriticalSection(&m_csCritSec);

	return guid;
}

void		THGUIDFactory::ResetInstanceCountArray()
{
	m_iInstanceCountMax = 0;
	memset(m_aInstanceCount,0,sizeof(m_aInstanceCount));
}

UINT16		THGUIDFactory::DBGetChunkCount(UINT32	timeStamp)
{
	UINT16	chunkCount = 0;

	m_cmdGetChunkCount->setUInt(1, timeStamp);

	BOOL bFindChunk = FALSE;
	if (m_cmdGetChunkCount->execute())
	{
		std::auto_ptr< sql::ResultSet > res;
		res.reset(m_cmdGetChunkCount->getResultSet());

		while (res->next())
		{
			bFindChunk = TRUE;
			chunkCount = res->getUInt(1);
		}
	}

	if (bFindChunk == FALSE)
	{
		DBInsertChunkCount(timeStamp, chunkCount);
	}

	return chunkCount;
}

void		THGUIDFactory::DBUpdateChunkCount(UINT32	timeStamp, UINT16	chunkCount)
{
	m_cmdUpdateChunkCount->setUInt(1, chunkCount);
	m_cmdUpdateChunkCount->setUInt(2, timeStamp);
	m_cmdUpdateChunkCount->executeUpdate();
}

void		THGUIDFactory::DBInsertChunkCount(UINT32	timeStamp, UINT16	chunkCount)
{
	m_cmdInsertChunkCount->setUInt(1, timeStamp);
	m_cmdInsertChunkCount->setUInt(2, chunkCount);
	m_cmdInsertChunkCount->executeUpdate();
}