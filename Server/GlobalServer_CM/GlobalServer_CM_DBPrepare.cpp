#include "GlobalServer_CM.h"

void	GlobalServer_CM::ReleaseDBCommands()
{
	ServerBase_CM::ReleaseDBCommands();

	m_cmdGetChunkCount.Destroy();
	m_cmdUpdateChunkCount.Destroy();
	m_cmdInsertChunkCount.Destroy();
}

BOOL	GlobalServer_CM::PrepareDBCommands(THMySQLConnector* pConn)
{
	ReleaseDBCommands();

	ServerBase_CM::PrepareDBCommands(pConn);

	sql::Connection* pSqlConn = pConn->m_pConn;

	m_cmdGetChunkCount.PrepareStatement(pSqlConn, "SELECT chunkCount FROM 2_guidtimechunk WHERE timeStamp=?");
	m_cmdUpdateChunkCount.PrepareStatement(pSqlConn, "UPDATE 2_guidtimechunk SET chunkCount=? WHERE timeStamp=?");
	m_cmdInsertChunkCount.PrepareStatement(pSqlConn, "INSERT INTO 2_guidtimechunk(timeStamp,chunkCount) VALUES(?,?)");

	return TRUE;
}
