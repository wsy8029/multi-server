#include "LoginServer_CM.h"

void	LoginServer_CM::ReleaseDBCommands()
{
	ServerBase_CM::ReleaseDBCommands();

	m_cmdGetClientInfo1.Destroy();
	m_cmdGetAccount1.Destroy();

	m_cmdInsertErrorLog1.Destroy();
	m_cmdInsertAccount1.Destroy();

	m_cmdUpdateAccount1.Destroy();
}

BOOL	LoginServer_CM::PrepareDBCommands(THMySQLConnector* pConn)
{
	ReleaseDBCommands();

	ServerBase_CM::PrepareDBCommands(pConn);

	sql::Connection* pSqlConn = pConn->m_pConn;

	m_cmdInsertErrorLog1.PrepareStatement(pSqlConn, "INSERT INTO errorLog(type,log) VALUES(?,?)");

	return TRUE;
}
