#include "ZoneServer_CM.h"

BOOL	ZoneServer_CM::InitDBCashes()
{

	return TRUE;
}

void	ZoneServer_CM::DBGet_Auth(stClientSession* pSession)			// Auth 후 DB 에서 데이터를 가져온다.
{

}

void	ZoneServer_CM::DBGet_Connect(stClientSession* pSession)			// Zone Connect 후 DB 에서 데이터를 가져온다.
{

}

void	ZoneServer_CM::DBProcUpdatePlayerCharacter1(stPlayerCharacter* pPC)
{
	m_cmdUpdateCharacter1.SetUInt(1, pPC->mapTID);
	m_cmdUpdateCharacter1.SetUInt(2, pPC->subZoneTID);
	m_cmdUpdateCharacter1.SetDouble(3, pPC->pos.x);
	m_cmdUpdateCharacter1.SetDouble(4, pPC->pos.y);
	m_cmdUpdateCharacter1.SetDouble(5, pPC->pos.z);
	m_cmdUpdateCharacter1.SetDouble(6, pPC->rotation.y);
	m_cmdUpdateCharacter1.SetUInt64(7, pPC->CHGUID);
	m_cmdUpdateCharacter1.ExecuteUpdate();
}