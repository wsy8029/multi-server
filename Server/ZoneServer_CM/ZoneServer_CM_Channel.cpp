#include "ZoneServer_CM.h"

stChannel* ZoneServer_CM::GetAvailableChannel(UINT32 mapTID)
{
	stChannel* pChannel = NULL;
	stZone* pZone = (stZone*)m_csAdminZone.GetObjectByKey(mapTID);
	if (pZone == NULL) return NULL;

	if (pZone->listChannel_EmptySpace.size() > 0)
	{
		pChannel = pZone->listChannel_EmptySpace.front();
	}
	else
	{
		stMapTemplate* pMT = (stMapTemplate*)g_csTemplateManager.m_csAdminMapTemplate.GetObjectByKey(mapTID);

		// 채널이 꽉 찼다.. 새로 만들자!
		pChannel = m_pPoolChannel->Allocate();
		pChannel->Init();
		pChannel->CHANGUID = m_csGUIDFactory.RequestGUID(TGO_TYPE_CHANNEL);
		pChannel->pZone = pZone;
		pChannel->pMT = pMT;

		std::list<stMapTemplate*>::iterator itor_MTSZ = pMT->listMapTemplate_SubZone.begin();
		while (itor_MTSZ != pMT->listMapTemplate_SubZone.end())
		{
			stMapTemplate* pMTSZ = *itor_MTSZ;

			stSubZone* pSubZone = m_pPoolSubZone->Allocate();
			pSubZone->Init();
			pSubZone->mapTID = pMTSZ->TID;
			pSubZone->ZONEGUID = m_csGUIDFactory.RequestGUID(TGO_TYPE_SUB_ZONE);

			pChannel->listSubZone.push_back(pSubZone);

			pChannel->csAdminSubZoneByGUID.AddObject(pSubZone, pSubZone->ZONEGUID);
			pChannel->csAdminSubZoneByTID.AddObject(pSubZone, pSubZone->mapTID);

			++itor_MTSZ;
		}

		pZone->listChannel.push_back(pChannel);
		pZone->listChannel_EmptySpace.push_back(pChannel);
		pZone->csAdminChannel.AddObject(pChannel, pChannel->CHANGUID);
		pChannel->isAddedListEmptySpace = TRUE;

		++m_iChannelCount;
	}

	return pChannel;
}

void	ZoneServer_CM::UpdateZone(stZone* pZone)
{
	std::list<stChannel*>::iterator itor_chn = pZone->listChannel.begin();
	while (itor_chn != pZone->listChannel.end())
	{
		stChannel* pChannel = *itor_chn;
		UpdateChannel(pChannel);
		++itor_chn;
	}
}

void	ZoneServer_CM::UpdateChannel(stChannel* pChannel)
{
	std::list<stSubZone*>::iterator itor_SZ = pChannel->listSubZone.begin();
	while (itor_SZ != pChannel->listSubZone.end())
	{
		stSubZone* pSZ = *itor_SZ;
		UpdateSubZone(pSZ);
		++itor_SZ;
	}
}

void	ZoneServer_CM::UpdateSubZone(stSubZone* pSubZone)
{

}

void	ZoneServer_CM::AddCharacterToZone(stZone* pZone, stCharacter* pCha)
{
	if (pCha->chType == ECT_PC)
	{
		stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;
		++pZone->pcCount;
		pZone->listPC.push_back(pPC);
	}
	else if (pCha->chType == ECT_NPC)
	{
		stNPC* pNPC = (stNPC*)pCha;
		++pZone->npcCount;
		pZone->listNPC.push_back(pNPC);
	}
	else if (pCha->chType == ECT_NPC_Equip)
	{
		stNPC_Equip* pNPCEquip = (stNPC_Equip*)pCha;
		++pZone->npcEquipCount;
		pZone->listNPCEquip.push_back(pNPCEquip);
	}

	pZone->csAdminCharacter.AddObject(pCha, pCha->CHGUID);
	pCha->pZone = pZone;
}

void	ZoneServer_CM::RemoveCharacterFromZone(stZone* pZone, stCharacter* pCha)
{
	if (pCha->pZone == pZone)
	{
		if (pCha->chType == ECT_PC)
		{
			stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;
			--pZone->pcCount;
			pZone->listPC.remove(pPC);
		}
		else if (pCha->chType == ECT_NPC)
		{
			stNPC* pNPC = (stNPC*)pCha;
			--pZone->npcCount;
			pZone->listNPC.remove(pNPC);
		}
		else if (pCha->chType == ECT_NPC_Equip)
		{
			stNPC_Equip* pNPCEquip = (stNPC_Equip*)pCha;
			--pZone->npcEquipCount;
			pZone->listNPCEquip.remove(pNPCEquip);
		}

		pZone->csAdminCharacter.RemoveObject(pCha->CHGUID);
		pCha->pZone = NULL;
	}
}

void	ZoneServer_CM::AddCharacterToChannel(stChannel* pChannel, stCharacter* pCha, BOOL isBroadcast)
{
	// 기존 플레이어 캐릭터 들에게 새로운 캐릭터가 진입했음을 전파한다.
	if (isBroadcast)
	{
		std::list<stPlayerCharacter*>::iterator itor_pc = pChannel->listPC.begin();
		while (itor_pc != pChannel->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;

			PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
			if (pToClientBroadcast != nullptr)
			{
				pToClientBroadcast->WriteUI8(1);					// 타입 수
				pToClientBroadcast->WriteUI8(pCha->chType);
				pToClientBroadcast->WriteUI16(1);					// 입장 캐릭터 수
				pCha->WritePacket_InGame_Broadcast_v0(pToClientBroadcast);
				g_pWSEngine->SendBuffer(pToClientBroadcast, PK_ZONE_CHARACTER_ENTER);
			}

			++itor_pc;
		}

		if (pCha->chType == ECT_PC)			// PC라면 기존 캐릭터들 리스트 전파
		{
			stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;

			UINT16 pcCount = (UINT16)pChannel->listPC.size();
			UINT16 npcCount = (UINT16)pChannel->listNPC.size();
			UINT16 npcEquipCount = (UINT16)pChannel->listNPCEquip.size();
			stClientSession* pSession = (stClientSession*)pPC->pSession;

			PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
			if (pToClient != nullptr)
			{
				pToClient->WriteUI8(3);								// 타입 수
				pToClient->WriteUI8(ECT_PC);
				pToClient->WriteUI16(pcCount);						// 기존 캐릭터 수

				itor_pc = pChannel->listPC.begin();
				while (itor_pc != pChannel->listPC.end())
				{
					stPlayerCharacter* pExistPC = *itor_pc;
					pExistPC->WritePacket_InGame_Broadcast_v0(pToClient);
					++itor_pc;
				}

				// NPC 전파
				pToClient->WriteUI8(ECT_NPC);
				pToClient->WriteUI16(npcCount);

				std::list<stNPC*>::iterator itor_npc = pChannel->listNPC.begin();
				while (itor_npc != pChannel->listNPC.end())
				{
					stNPC* pNPC = *itor_npc;
					pNPC->WritePacket_InGame_Broadcast_v0(pToClient);
					++itor_npc;
				}

				// NPC Equip 전파
				pToClient->WriteUI8(ECT_NPC_Equip);
				pToClient->WriteUI16(npcEquipCount);

				std::list<stNPC_Equip*>::iterator itor_npc_equip = pChannel->listNPCEquip.begin();
				while (itor_npc_equip != pChannel->listNPCEquip.end())
				{
					stNPC_Equip* pNPCEquip = *itor_npc_equip;
					pNPCEquip->WritePacket_InGame_Broadcast_v0(pToClient);
					++itor_npc_equip;
				}

				g_pWSEngine->SendBuffer(pToClient, PK_ZONE_CHARACTER_ENTER);
			}
		}
	}

	if (pCha->chType == ECT_PC)
	{
		stZone* pZone = pChannel->pZone;
		stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;
		++pChannel->pcCount;
		pChannel->listPC.push_back(pPC);

		if (pChannel->pcCount >= pChannel->pMT->maxChannelPlayer)
		{
			if (pChannel->isAddedListEmptySpace == TRUE)
			{
				pZone->listChannel_EmptySpace.remove(pChannel);
				pChannel->isAddedListEmptySpace = FALSE;
			}
		}
		else
		{
			if (pChannel->isAddedListEmptySpace == FALSE)
			{
				pZone->listChannel_EmptySpace.push_back(pChannel);
				pChannel->isAddedListEmptySpace = TRUE;
			}
		}
	}
	else if (pCha->chType == ECT_NPC)
	{
		stNPC* pNPC = (stNPC*)pCha;
		++pChannel->npcCount;
		pChannel->listNPC.push_back(pNPC);
	}
	else if (pCha->chType == ECT_NPC_Equip)
	{
		stNPC_Equip* pNPCEquip = (stNPC_Equip*)pCha;
		++pChannel->npcEquipCount;
		pChannel->listNPCEquip.push_back(pNPCEquip);
	}

	pCha->zoneState = ECZS_Zone;
	pCha->pChannel = pChannel;
	pChannel->csAdminCharacter.AddObject(pCha, pCha->CHGUID);
}

void	ZoneServer_CM::RemoveCharacterFromChannel(stChannel* pChannel, stCharacter* pCha, BOOL isBroadcast)
{
	BOOL bAddedEmptySpace = TRUE;
	if (pCha->chType == ECT_PC)
	{
		stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;

		if (pChannel->pcCount == pChannel->pMT->maxChannelPlayer)		// 이전에 채널이 꽉찼었다.
		{
			bAddedEmptySpace = FALSE;
		}

		--pChannel->pcCount;
		pChannel->listPC.remove(pPC);
	}
	else if (pCha->chType == ECT_NPC)
	{
		stNPC* pNPC = (stNPC*)pCha;

		--pChannel->npcCount;
		pChannel->listNPC.remove(pNPC);
	}
	else if (pCha->chType == ECT_NPC_Equip)
	{
		stNPC_Equip* pNPCEquip = (stNPC_Equip*)pCha;

		--pChannel->npcEquipCount;
		pChannel->listNPCEquip.remove(pNPCEquip);
	}

	pCha->zoneState = ECZS_Zone;
	pCha->pChannel = NULL;
	pChannel->csAdminCharacter.RemoveObject(pCha->CHGUID);

	if (isBroadcast)
	{
		std::list<stPlayerCharacter*>::iterator itor_pc = pChannel->listPC.begin();
		while (itor_pc != pChannel->listPC.end())
		{
			stPlayerCharacter* pBroadcastPC = *itor_pc;
			stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;

			PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
			if (pToClientBroadcast != nullptr)
			{
				pToClientBroadcast->WriteUI64(pCha->CHGUID);
				g_pWSEngine->SendBuffer(pToClientBroadcast, PK_ZONE_CHARACTER_LEAVE);
			}

			++itor_pc;
		}
	}

	if (pCha->chType == ECT_PC)
	{
		stZone* pZone = pChannel->pZone;

		if (pChannel->pcCount == 0)
		{
			pZone->listChannel.remove(pChannel);
			pZone->listChannel_EmptySpace.remove(pChannel);

			ReleaseChannel(pChannel);
		}
		else
		{
			if (bAddedEmptySpace == FALSE)
			{
				pZone->listChannel_EmptySpace.push_back(pChannel);
			}
		}
	}
}

void	ZoneServer_CM::AddCharacterToSubZone(stSubZone* pSubZone, stCharacter* pCha)
{
	// 기존 플레이어 캐릭터 들에게 새로운 캐릭터가 진입했음을 전파한다.
	std::list<stPlayerCharacter*>::iterator itor_pc = pSubZone->listPC.begin();
	while (itor_pc != pSubZone->listPC.end())
	{
		stPlayerCharacter* pBroadcastPC = *itor_pc;
		stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;

		PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
		if (pToClientBroadcast != nullptr)
		{
			pToClientBroadcast->WriteUI8(1);					// 타입 수
			pToClientBroadcast->WriteUI8(pCha->chType);
			pToClientBroadcast->WriteUI16(1);					// 입장 캐릭터 수

			pCha->WritePacket_InGame_Broadcast_v0(pToClientBroadcast);

			g_pWSEngine->SendBuffer(pToClientBroadcast, PK_ZONE_CHARACTER_ENTER);
		}
		++itor_pc;
	}

	if (pCha->chType == ECT_PC)			// PC라면 기존 캐릭터들 리스트 전파
	{
		stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;

		UINT16 pcCount = (UINT16)pSubZone->listPC.size();
		UINT16 npcCount = (UINT16)pSubZone->listNPC.size();
		UINT16 npcEquipCount = (UINT16)pSubZone->listNPCEquip.size();
		stClientSession* pSession = (stClientSession*)pPC->pSession;

		PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
		if (pToClient != nullptr)
		{
			pToClient->WriteUI8(3);								// 타입 수
			pToClient->WriteUI8(ECT_PC);
			pToClient->WriteUI16(pcCount);						// 기존 캐릭터 수

			itor_pc = pSubZone->listPC.begin();
			while (itor_pc != pSubZone->listPC.end())
			{
				stPlayerCharacter* pExistPC = *itor_pc;
				pExistPC->WritePacket_InGame_Broadcast_v0(pToClient);
				++itor_pc;
			}

			// NPC 전파
			pToClient->WriteUI8(ECT_NPC);
			pToClient->WriteUI16(npcCount);

			std::list<stNPC*>::iterator itor_npc = pSubZone->listNPC.begin();
			while (itor_npc != pSubZone->listNPC.end())
			{
				stNPC* pNPC = *itor_npc;
				pNPC->WritePacket_InGame_Broadcast_v0(pToClient);
				++itor_npc;
			}

			// NPC Equip 전파
			pToClient->WriteUI8(ECT_NPC_Equip);
			pToClient->WriteUI16(npcEquipCount);

			std::list<stNPC_Equip*>::iterator itor_npc_equip = pSubZone->listNPCEquip.begin();
			while (itor_npc_equip != pSubZone->listNPCEquip.end())
			{
				stNPC_Equip* pNPCEquip = *itor_npc_equip;
				pNPCEquip->WritePacket_InGame_Broadcast_v0(pToClient);
				++itor_npc_equip;
			}

			g_pWSEngine->SendBuffer(pToClient, PK_ZONE_CHARACTER_ENTER);
		}

		++pSubZone->pcCount;
		pSubZone->listPC.push_back(pPC);
	}
	else if (pCha->chType == ECT_NPC)
	{
		stNPC* pNPC = (stNPC*)pCha;
		++pSubZone->npcCount;
		pSubZone->listNPC.push_back(pNPC);
	}
	else if (pCha->chType == ECT_NPC_Equip)
	{
		stNPC_Equip* pNPCEquip = (stNPC_Equip*)pCha;
		++pSubZone->npcEquipCount;
		pSubZone->listNPCEquip.push_back(pNPCEquip);
	}

	pCha->zoneState = ECZS_SubZone;
	pCha->pSubZone = pSubZone;
	pSubZone->csAdminCharacter.AddObject(pCha, pCha->CHGUID);
}

void	ZoneServer_CM::RemoveCharacterFromSubZone(stSubZone* pSubZone, stCharacter* pCha)
{
	if (pCha->chType == ECT_PC)
	{
		stPlayerCharacter* pPC = (stPlayerCharacter*)pCha;

		--pSubZone->pcCount;
		pSubZone->listPC.remove(pPC);
	}
	else if (pCha->chType == ECT_NPC)
	{
		stNPC* pNPC = (stNPC*)pCha;

		--pSubZone->npcCount;
		pSubZone->listNPC.remove(pNPC);
	}
	else if (pCha->chType == ECT_NPC_Equip)
	{
		stNPC_Equip* pNPCEquip = (stNPC_Equip*)pCha;

		--pSubZone->npcEquipCount;
		pSubZone->listNPCEquip.remove(pNPCEquip);
	}

	pCha->zoneState = ECZS_Zone;
	pCha->pSubZone = NULL;
	pSubZone->csAdminCharacter.RemoveObject(pCha->CHGUID);

	std::list<stPlayerCharacter*>::iterator itor_pc = pSubZone->listPC.begin();
	while (itor_pc != pSubZone->listPC.end())
	{
		stPlayerCharacter* pBroadcastPC = *itor_pc;
		stClientSession* pBroadcastSession = (stClientSession*)pBroadcastPC->pSession;

		PxWSBuffer* pToClientBroadcast = g_pWSEngine->GetSendBufferObj(pBroadcastSession->pWSSession);
		if (pToClientBroadcast != nullptr)
		{
			pToClientBroadcast->WriteUI64(pCha->CHGUID);
			g_pWSEngine->SendBuffer(pToClientBroadcast, PK_ZONE_CHARACTER_LEAVE);
		}

		++itor_pc;
	}
}









