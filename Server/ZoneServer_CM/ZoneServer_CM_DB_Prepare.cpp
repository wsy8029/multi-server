#include "ZoneServer_CM.h"

void	ZoneServer_CM::ReleaseDBCommands()
{
	ServerBase_CM::ReleaseDBCommands();
	
	m_cmdUpdateCharacter1.Destroy();
	m_cmdInsertErrorLog1.Destroy();
}

BOOL	ZoneServer_CM::PrepareDBCommands(THMySQLConnector* pConn)
{
	ReleaseDBCommands();

	ServerBase_CM::PrepareDBCommands(pConn);

	sql::Connection* pSqlConn = pConn->m_pConn;

	m_cmdUpdateCharacter1.PrepareStatement(pSqlConn, "UPDATE characters SET mapTID=?,subZoneTID=?,mapPosX=?,mapPosY=?,mapPosZ=?,rotY=? WHERE CHGUID=?");

	m_cmdInsertErrorLog1.PrepareStatement(pSqlConn, "INSERT INTO errorLog(type,log) VALUES(?,?)");
	
	return TRUE;
}
