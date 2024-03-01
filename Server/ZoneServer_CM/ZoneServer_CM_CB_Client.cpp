#include "ZoneServer_CM.h"

void	ZoneServer_CM::InitPacketClientCBFunction()
{
	g_pWSEngine->SetPacketClientCB(PK_HAND_SHAKE, 0, ProcHandShake_v0);
	g_pWSEngine->SetPacketClientCB(PK_ALIVE_CHECK, 0, ProcAliveCheck_v0);
	g_pWSEngine->SetPacketClientCB(PK_ERROR_LOG, 0, ProcErrorLog_v0);
	g_pWSEngine->SetPacketClientCB(PK_TRANSFORM_SYNC, 0, ProcTransformSync_v0);
	g_pWSEngine->SetPacketClientCB(PK_ANIMATOR_SYNC, 0, ProcAnimatorSync_v0);
	g_pWSEngine->SetPacketClientCB(PK_DATA_SYNC, 0, ProcDataSync_v0);
	g_pWSEngine->SetPacketClientCB(PK_CONNECT_ZONE_SERVER_AUTH, 0, ProcConnectZoneServerAuth_v0);
	g_pWSEngine->SetPacketClientCB(PK_ACCOUNT_LOGOUT, 0, ProcAccountLogout_v0);

	g_pWSEngine->SetPacketClientCB(PK_ZONE_ENTER, 0, ProcZoneEnter_v0);
	g_pWSEngine->SetPacketClientCB(PK_ZONE_CHANGE, 0, ProcZoneChange_v0);
	g_pWSEngine->SetPacketClientCB(PK_ZONE_CHARACTER_LEAVE, 0, ProcZoneCharacterLeave_v0);
	g_pWSEngine->SetPacketClientCB(PK_CHATTING, 0, ProcChatting_v0);
}

BOOL	ZoneServer_CM::ProcHandShake_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	UINT8 svrIndex = PX_RECV_GET_UI8(pBuffer);

	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
	if (pToClient != nullptr)
	{
		pToClient->WriteUI8(svrIndex);
		pToClient->WriteUI32(pWSSession->m_iWSSessionUID);
		g_pWSEngine->SendBuffer(pToClient, PK_HAND_SHAKE);
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcAliveCheck_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	return TRUE;
}

BOOL	ZoneServer_CM::ProcAccountLogout_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	pWSSession->Close();

	return TRUE;
}

BOOL	ZoneServer_CM::ProcErrorLog_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	UINT8 msgLineCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < msgLineCount; ++i)
	{
		UINT8	type = PX_RECV_GET_UI8(pBuffer);
		CHAR log[256];
		PX_RECV_GET_STRINGN(log, pBuffer, 256);

		if (strlen(log) > 0)
		{
			m_pThis->m_cmdInsertErrorLog1.SetUInt(1, type);
			m_pThis->m_cmdInsertErrorLog1.SetString(2, log);
			m_pThis->m_cmdInsertErrorLog1.ExecuteUpdate();
		}
	}

	UINT8 stackTraceLineCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < stackTraceLineCount; ++i)
	{
		UINT8	type = PX_RECV_GET_UI8(pBuffer);
		CHAR log[256];
		PX_RECV_GET_STRINGN(log, pBuffer, 256);

		if (strlen(log) > 0)
		{
			m_pThis->m_cmdInsertErrorLog1.SetUInt(1, type);
			m_pThis->m_cmdInsertErrorLog1.SetString(2, log);
			m_pThis->m_cmdInsertErrorLog1.ExecuteUpdate();
		}
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcTransformSync_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	stPlayerCharacter* pPC = pSession->pPC;
	if (pPC == NULL) return FALSE;
	stZone* pZone = pPC->pZone;
	if (pZone == NULL) return FALSE;

	UINT64 senderCHGUID = PX_RECV_GET_UI64(pBuffer);
	stCharacter* pCH = (stCharacter*)pZone->csAdminCharacter.GetObjectByKey(senderCHGUID);
	if (pCH == NULL)	return FALSE;

	PX_RECV_GET_VECTOR3(pCH->pos, pBuffer);
	PX_RECV_GET_VECTOR3(pCH->velocity, pBuffer);
	PX_RECV_GET_VECTOR3(pCH->rotation, pBuffer);

	if (pPC->zoneState == ECZS_Zone)
	{
		stChannel* pChannel = pCH->pChannel;
		if (pChannel == NULL) return FALSE;

		std::list<stPlayerCharacter*>::iterator itor_pc = pChannel->listPC.begin();
		while (itor_pc != pChannel->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastPC != pPC && pBroadcastSession->connectState == ESCS_ConnectedToServer)
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteVector3(pCH->pos);
					pToClientBroadcast->WriteVector3(pCH->velocity);
					pToClientBroadcast->WriteVector3(pCH->rotation);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_TRANSFORM_SYNC);
				}
			}
			++itor_pc;
		}
	}
	else if (pPC->zoneState == ECZS_SubZone)
	{
		stSubZone* pSubZone = pCH->pSubZone;
		if (pSubZone == NULL) return FALSE;

		std::list<stPlayerCharacter*>::iterator itor_pc = pSubZone->listPC.begin();
		while (itor_pc != pSubZone->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastPC != pPC && pBroadcastSession->connectState == ESCS_ConnectedToServer)
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteVector3(pCH->pos);
					pToClientBroadcast->WriteVector3(pCH->velocity);
					pToClientBroadcast->WriteVector3(pCH->rotation);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_TRANSFORM_SYNC);
				}
			}
			++itor_pc;
		}
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcAnimatorSync_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	stPlayerCharacter* pPC = pSession->pPC;
	if (pPC == NULL) return FALSE;
	stZone* pZone = pPC->pZone;
	if (pZone == NULL) return FALSE;

	UINT64 senderCHGUID = PX_RECV_GET_UI64(pBuffer);
	stCharacter* pCH = (stCharacter*)pZone->csAdminCharacter.GetObjectByKey(senderCHGUID);
	if (pCH == NULL)	return FALSE;

	byte animParamIntCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < animParamIntCount; ++i)
	{
		pCH->animParam_IntType[i] = PX_RECV_GET_I32(pBuffer);
	}

	byte animParamFloatCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < animParamFloatCount; ++i)
	{
		pCH->animParam_FloatType[i] = PX_RECV_GET_FLOAT(pBuffer);
	}

	byte animParamBoolCount = PX_RECV_GET_UI8(pBuffer);
	pCH->animParam_BoolType = PX_RECV_GET_UI32(pBuffer);

	if (pPC->zoneState == ECZS_Zone)
	{
		stChannel* pChannel = pCH->pChannel;
		if (pChannel == NULL) return FALSE;

		std::list<stPlayerCharacter*>::iterator itor_pc = pChannel->listPC.begin();
		while (itor_pc != pChannel->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastPC != pPC && pBroadcastSession->connectState == ESCS_ConnectedToServer)
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteUI8(animParamIntCount);
					pToClientBroadcast->CopyBytes(pCH->animParam_IntType, sizeof(INT32) * animParamIntCount);
					pToClientBroadcast->WriteUI8(animParamFloatCount);
					pToClientBroadcast->CopyBytes(pCH->animParam_FloatType, sizeof(FLOAT) * animParamFloatCount);
					pToClientBroadcast->WriteUI8(animParamBoolCount);
					pToClientBroadcast->WriteUI32(pCH->animParam_BoolType);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_ANIMATOR_SYNC);
				}
			}
			++itor_pc;
		}
	}
	else if (pPC->zoneState == ECZS_SubZone)
	{
		stSubZone* pSubZone = pCH->pSubZone;
		if (pSubZone == NULL) return FALSE;

		std::list<stPlayerCharacter*>::iterator itor_pc = pSubZone->listPC.begin();
		while (itor_pc != pSubZone->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastPC != pPC && pBroadcastSession->connectState == ESCS_ConnectedToServer)
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteUI8(animParamIntCount);
					pToClientBroadcast->CopyBytes(pCH->animParam_IntType, sizeof(INT32) * animParamIntCount);
					pToClientBroadcast->WriteUI8(animParamFloatCount);
					pToClientBroadcast->CopyBytes(pCH->animParam_FloatType, sizeof(FLOAT) * animParamFloatCount);
					pToClientBroadcast->WriteUI8(animParamBoolCount);
					pToClientBroadcast->WriteUI32(pCH->animParam_BoolType);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_ANIMATOR_SYNC);
				}
			}
			++itor_pc;
		}
	}
	return TRUE;
}

BOOL	ZoneServer_CM::ProcDataSync_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	stPlayerCharacter* pPC = pSession->pPC;
	if (pPC == NULL) return FALSE;
	stZone* pZone = pPC->pZone;
	if (pZone == NULL) return FALSE;

	UINT64 senderCHGUID = PX_RECV_GET_UI64(pBuffer);
	stCharacter* pCH = (stCharacter*)pZone->csAdminCharacter.GetObjectByKey(senderCHGUID);
	if (pCH == NULL)	return FALSE;

	UINT8	dataType = PX_RECV_GET_UI8(pBuffer);
	UINT16	dataSize = PX_RECV_GET_UI16(pBuffer);
	BYTE* pData = (BYTE*)PxFrameMemory::AllocFrameMemory(dataSize);
	PX_RECV_GET_COPY(pData, pBuffer, dataSize);
	BYTE* pBuff = pData;

	if (dataType == EPDST_IsKinematicForce)
	{
		//pPC->isKinematicForce = PX_RECV_GET_UI8(pBuff);
	}
	else if (dataType == EPDST_PositionAndRotation)
	{
		PX_RECV_GET_VECTOR3(pPC->pos, pBuff);
		PX_RECV_GET_VECTOR3(pPC->rotation, pBuff);
	}

	if (pPC->zoneState == ECZS_Zone)
	{
		stChannel* pChannel = pCH->pChannel;
		if (pChannel == NULL)
		{
			PxFrameMemory::DeallocFrameMemory(dataSize);
			return FALSE;
		}

		std::list<stPlayerCharacter*>::iterator itor_pc = pChannel->listPC.begin();
		while (itor_pc != pChannel->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastPC != pPC && pBroadcastSession->connectState == ESCS_ConnectedToServer)
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteUI8(dataType);
					pToClientBroadcast->WriteUI16(dataSize);
					pToClientBroadcast->CopyBytes(pData, dataSize);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_DATA_SYNC);
				}
			}
			++itor_pc;
		}
	}
	else if (pPC->zoneState == ECZS_SubZone)
	{
		stSubZone* pSubZone = pCH->pSubZone;
		if (pSubZone == NULL)
		{
			PxFrameMemory::DeallocFrameMemory(dataSize);
			return FALSE;
		}

		std::list<stPlayerCharacter*>::iterator itor_pc = pSubZone->listPC.begin();
		while (itor_pc != pSubZone->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastPC != pPC && pBroadcastSession->connectState == ESCS_ConnectedToServer)
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteUI8(dataType);
					pToClientBroadcast->WriteUI16(dataSize);
					pToClientBroadcast->CopyBytes(pData, dataSize);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_DATA_SYNC);
				}
			}
			++itor_pc;
		}
	}
	PxFrameMemory::DeallocFrameMemory(dataSize);

	return TRUE;
}

BOOL	ZoneServer_CM::ProcConnectZoneServerAuth_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	UINT64	ACGUID = PX_RECV_GET_UI64(pBuffer);
	UINT32  authKey = PX_RECV_GET_UI32(pBuffer);

	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
	if (pToClient != nullptr)
	{
		stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(ACGUID);
		if (!pSession)
		{
			pToClient->WriteUI8(1);						// 해당 guid가 없다
			g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_ZONE_SERVER_AUTH);

			pWSSession->Close();
			return FALSE;
		}

		pSession->RefreshAliveCheck();

		if (pSession->authKeyFromMainServer != authKey)
		{
			pToClient->WriteUI8(2);						// 인증키가 틀리다.
			g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_ZONE_SERVER_AUTH);

			pWSSession->Close();
			return FALSE;
		}

		if (pWSSession->m_pData != NULL)
		{
			stClientSession* pRemoveSession = (stClientSession*)pWSSession->m_pData;
			m_pThis->FreeClientSession(pRemoveSession);
		}

		pWSSession->m_pData = (PVOID)pSession;													// per Socket Data Set!!
		pSession->pWSSession = pWSSession;
		pSession->connectState = ESCS_ConnectedToServer;

		m_pThis->DBGet_Auth(pSession);

		pToClient->WriteUI8(0);

		g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_ZONE_SERVER_AUTH);
	}
}

BOOL	ZoneServer_CM::ProcZoneEnter_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pWSSession);
	if (pToClient != nullptr)
	{
		stAccount* pAccount = &pSession->account;
		stPlayerCharacter* pPC = pSession->pPC;

		stChannel* pChannel = m_pThis->GetAvailableChannel(pPC->mapTID);
		if (pChannel == NULL)
		{
			pToClient->WriteUI8(1);										// Channel 이 없다.
			g_pWSEngine->SendBuffer(pToClient, PK_ZONE_ENTER);
			return FALSE;
		}

		stZone* pZone = pChannel->pZone;
		stSubZone* pSubZone = NULL;

		if (pPC->subZoneTID != 0)
		{
			pSubZone = (stSubZone*)pChannel->csAdminSubZoneByTID.GetObjectByKey(pPC->subZoneTID);

			if (pSubZone == NULL)
			{
				pToClient->WriteUI8(2);									// SubZone 이 없다.
				g_pWSEngine->SendBuffer(pToClient, PK_ZONE_ENTER);
				return FALSE;
			}
		}

		m_pThis->DBGet_Connect(pSession);								// PC 상세정보 얻어오기 .. 없을때만..

		pToClient->WriteUI8(0);
		pToClient->WriteUI64(pZone->ZONEGUID);
		pToClient->WriteUI64(pChannel->CHANGUID);
		pToClient->WriteUI32(pPC->subZoneTID);

		if (pPC->subZoneTID != 0)
		{
			pSubZone = (stSubZone*)pChannel->csAdminSubZoneByTID.GetObjectByKey(pPC->subZoneTID);
			pToClient->WriteUI64(pSubZone->ZONEGUID);
		}
		else
		{
			pToClient->WriteUI64(0);
		}

		pToClient->WriteVector3(pPC->pos);
		pToClient->WriteFloat(pPC->rotation.y);

		pToClient->WriteString(pPC->birthDate);
		pToClient->WriteString(pPC->thumbnailUrl);
		pToClient->WriteString(pPC->mainBadgeUrl);
		pToClient->WriteUI8(pPC->statusType);
		pToClient->WriteString(pPC->statusDescription_UTF8);
		pToClient->WriteString(pPC->personalSpaceUrl);

		pToClient->WriteI32(pPC->level);
		pToClient->WriteI32(pPC->totalExperienceForNextLevel);
		pToClient->WriteI32(pPC->point);
		pToClient->WriteI32(pPC->literacyExp);
		pToClient->WriteI32(pPC->imaginationExp);
		pToClient->WriteI32(pPC->narrativeExp);
		pToClient->WriteI32(pPC->sociabilityExp);

		g_pWSEngine->SendBuffer(pToClient, PK_ZONE_ENTER);

		m_pThis->AddCharacterToZone(pZone, pPC);

		if (pPC->subZoneTID != 0)
		{
			m_pThis->AddCharacterToSubZone(pSubZone, pPC);
			m_pThis->AddCharacterToChannel(pChannel, pPC, FALSE);
		}
		else
		{
			m_pThis->AddCharacterToChannel(pChannel, pPC);
		}
	}

	return TRUE;
}

BOOL	ZoneServer_CM::ProcZoneChange_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();


	return TRUE;
}

BOOL	ZoneServer_CM::ProcZoneCharacterLeave_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();


	return TRUE;
}

BOOL	ZoneServer_CM::ProcChatting_v0(SPWSSession pWSSession, BYTE* pBuffer, int	buffLen)
{
	stClientSession* pSession = (stClientSession*)pWSSession->m_pData;
	if (pSession == NULL) return FALSE;
	pSession->RefreshAliveCheck();

	stPlayerCharacter* pPC = pSession->pPC;
	if (pPC == NULL) return FALSE;
	stZone* pZone = pPC->pZone;
	if (pZone == NULL) return FALSE;

	UINT64 senderCHGUID = PX_RECV_GET_UI64(pBuffer);
	stCharacter* pCH = (stCharacter*)pZone->csAdminCharacter.GetObjectByKey(senderCHGUID);
	if (pCH == NULL)	return FALSE;

	CHAR	chatMsg_UTF8[CHAT_MSG_LENGTH_UTF8];
	PX_RECV_GET_STRINGN(chatMsg_UTF8, pBuffer, CHAT_MSG_LENGTH_UTF8);

	WCHAR chatMsg[CHAT_MSG_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, chatMsg_UTF8, -1, chatMsg, CHAT_MSG_LENGTH);

	std::wstring strChat = std::wstring(chatMsg);
	//for (int i = 0; i < m_pThis->m_vecChattingFilter.size(); ++i)
	//{
	//	std::size_t found = strChat.find(m_pThis->m_vecChattingFilter[i]);
	//	if (found != std::wstring::npos)
	//	{
	//		strChat.replace(found, m_pThis->m_vecChattingFilter[i].length(), L"**");
	//	}
	//}

	CHAR	chatMsg_Filtered_UTF8[CHAT_MSG_LENGTH_UTF8];
	int lenUTF8 = WideCharToMultiByte(CP_UTF8, 0, strChat.c_str(), -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, strChat.c_str(), -1, chatMsg_Filtered_UTF8, lenUTF8, NULL, NULL);

	if (pPC->zoneState == ECZS_Zone)
	{
		stChannel* pChannel = pCH->pChannel;
		if (pChannel == NULL) return FALSE;

		std::list<stPlayerCharacter*>::iterator itor_pc = pChannel->listPC.begin();
		while (itor_pc != pChannel->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastSession->connectState == ESCS_ConnectedToServer)			// 자기 자신에게도 보내자..
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteString(pPC->pCharacterInfo->nickname_UTF8);
					pToClientBroadcast->WriteString(chatMsg_Filtered_UTF8);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_CHATTING);
				}
			}
			++itor_pc;
		}
	}
	else if (pPC->zoneState == ECZS_SubZone)
	{
		stSubZone* pSubZone = pCH->pSubZone;
		if (pSubZone == NULL) return FALSE;

		std::list<stPlayerCharacter*>::iterator itor_pc = pSubZone->listPC.begin();
		while (itor_pc != pSubZone->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;
			if (pBroadcastSession->connectState == ESCS_ConnectedToServer)		// 자기 자신에게도 보내자..
			{
				PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
				if (pToClientBroadcast != nullptr)
				{
					pToClientBroadcast->WriteUI64(pCH->CHGUID);
					pToClientBroadcast->WriteString(pPC->pCharacterInfo->nickname_UTF8);
					pToClientBroadcast->WriteString(chatMsg_Filtered_UTF8);
					g_pWSEngine->SendBuffer(pToClientBroadcast, PK_CHATTING);
				}
			}
			++itor_pc;
		}
	}

	return TRUE;
}