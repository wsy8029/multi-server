#include "GlobalServer_CM.h"

BOOL	GlobalServer_CM::InitDBCashes()
{

	return TRUE;
}

UINT16	GlobalServer_CM::DBGetChunkCount(UINT32	timeStamp)
{
	UINT16	chunkCount = 0;

	m_cmdGetChunkCount.SetUInt(1, timeStamp);
	sql::ResultSet* res = m_cmdGetChunkCount.ExecuteAndGetResultSet();

	BOOL bFindChunk = FALSE;
	if(res != NULL)
	{
		while (res->next())
		{
			bFindChunk = TRUE;
			chunkCount = (UINT16)res->getUInt(1);
		}
		delete res;
	}

	if (bFindChunk == FALSE)
	{
		DBInsertChunkCount(timeStamp, chunkCount);
	}

	return chunkCount;
}

void	GlobalServer_CM::DBUpdateChunkCount(UINT32	timeStamp, UINT16	chunkCount)
{
	m_cmdUpdateChunkCount.SetUInt(1, chunkCount);
	m_cmdUpdateChunkCount.SetUInt(2, timeStamp);
	m_cmdUpdateChunkCount.ExecuteUpdate();
}

void	GlobalServer_CM::DBInsertChunkCount(UINT32	timeStamp, UINT16	chunkCount)
{
	m_cmdInsertChunkCount.SetUInt(1, timeStamp);
	m_cmdInsertChunkCount.SetUInt(2, chunkCount);
	m_cmdInsertChunkCount.ExecuteUpdate();
}

