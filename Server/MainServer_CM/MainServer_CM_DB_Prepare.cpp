#include "MainServer_CM.h"

void	MainServer_CM::ReleaseDBCommands()
{
	ServerBase_CM::ReleaseDBCommands();

	m_cmdGetCharacter1.Destroy();

	m_cmdUpdateCharacter1.Destroy();

	m_cmdInsertErrorLog1.Destroy();
	m_cmdInsertCharacter1.Destroy();

	m_cmdDeleteCharacter1.Destroy();
}

BOOL	MainServer_CM::PrepareDBCommands(THMySQLConnector* pConn)
{
	ReleaseDBCommands();

	ServerBase_CM::PrepareDBCommands(pConn);

	sql::Connection* pSqlConn = pConn->m_pConn;

	m_cmdGetCharacter1.PrepareStatement(pSqlConn, "SELECT TID,zoneState,mapTID,subZoneTID,mapPosX,mapPosY,mapPosZ,rotY FROM characters WHERE CHGUID=?");

	m_cmdUpdateCharacter1.PrepareStatement(pSqlConn, "UPDATE characters SET recentLoginTime=? WHERE CHGUID=?");

	m_cmdInsertErrorLog1.PrepareStatement(pSqlConn, "INSERT INTO errorLog(type,log) VALUES(?,?)");
	m_cmdInsertCharacter1.PrepareStatement(pSqlConn, "INSERT INTO characters(CHGUID,ACGUID,TID,zoneState,mapTID,subZoneTID,mapPosX,mapPosY,mapPosZ,rotY,recentLoginTime) VALUES(?,?,?,?,?,?,?,?,?,?,?)");

	m_cmdDeleteCharacter1.PrepareStatement(pSqlConn, "DELETE FROM characters WHERE CHGUID=?");
	
	return TRUE;
}
