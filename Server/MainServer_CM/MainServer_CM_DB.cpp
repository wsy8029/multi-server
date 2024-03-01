#include "MainServer_CM.h"

BOOL	MainServer_CM::InitDBCashes()
{

	return TRUE;
}

void	MainServer_CM::DBGet_Auth(stClientSession* pSession)			// Auth 후 DB 에서 데이터를 가져온다.
{
	stAccount* pAC = &pSession->account;
	stPlayerCharacter* pPC = pSession->pPC;

	m_cmdGetCharacter1.SetUInt64(1, pPC->CHGUID);
	sql::ResultSet* resGetCharacter = m_cmdGetCharacter1.ExecuteAndGetResultSet();
	if (resGetCharacter != NULL)
	{
		if (resGetCharacter->rowsCount() == 0)		// 캐릭터가 없다..
		{
			pPC->TID = 1;
			pPC->zoneState = ECZS_Zone;
			pPC->mapTID = 1;
			pPC->subZoneTID = 0;
			stMapTemplate* pMT = (stMapTemplate*)g_csTemplateManager.m_csAdminMapTemplate.GetObjectByKey(pPC->mapTID);
			stMapSpawnData* pMSD = pMT->listMapSpawnData.front();
			pPC->pos = pMSD->pos;
			pPC->rotation.x = 0; pPC->rotation.y = pMSD->rotY; pPC->rotation.z = 0;
			pPC->recentLoginTime = m_iCurrentTime;

			m_cmdInsertCharacter1.SetUInt64(1, pPC->CHGUID);
			m_cmdInsertCharacter1.SetUInt64(2, pAC->ACGUID);
			m_cmdInsertCharacter1.SetUInt(3, pPC->TID);
			m_cmdInsertCharacter1.SetUInt(4, pPC->zoneState);
			m_cmdInsertCharacter1.SetUInt(5, pPC->mapTID);
			m_cmdInsertCharacter1.SetUInt(6, pPC->subZoneTID);
			m_cmdInsertCharacter1.SetDouble(7, pPC->pos.x);
			m_cmdInsertCharacter1.SetDouble(8, pPC->pos.y);
			m_cmdInsertCharacter1.SetDouble(9, pPC->pos.z);
			m_cmdInsertCharacter1.SetDouble(10, pPC->rotation.y);
			m_cmdInsertCharacter1.SetDateTime(11, time_t2MySQLString(pPC->recentLoginTime));
			m_cmdInsertCharacter1.ExecuteUpdate();
		}
		else
		{
			resGetCharacter->next();

			pPC->TID = resGetCharacter->getUInt(1);
			pPC->zoneState = (ECharacterZoneState)resGetCharacter->getUInt(2);
			pPC->mapTID = resGetCharacter->getUInt(3);
			pPC->subZoneTID = resGetCharacter->getUInt(4);
			pPC->pos.x = (FLOAT)resGetCharacter->getDouble(5);
			pPC->pos.y = (FLOAT)resGetCharacter->getDouble(6);
			pPC->pos.z = (FLOAT)resGetCharacter->getDouble(7);
			pPC->rotation.y = (FLOAT)resGetCharacter->getDouble(8);

			pPC->recentLoginTime = m_iCurrentTime;
			m_cmdUpdateCharacter1.SetDateTime(1, time_t2MySQLString(pPC->recentLoginTime));
			m_cmdUpdateCharacter1.SetUInt64(2, pPC->CHGUID);
			m_cmdUpdateCharacter1.ExecuteUpdate();
		}
		delete resGetCharacter;
	}
}

void	MainServer_CM::DBGet_Connect(stClientSession* pSession)			// Zone Connect 후 DB 에서 데이터를 가져온다.
{

}