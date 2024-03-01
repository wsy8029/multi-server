#include "CMDefine.h"
#include "PxIOCPEngine.h"
#include "PxPacketDefine.h"

// 여러 부분에서 공통적으로 쓰이는 패킷 전송 부분을 함수로 모아서 수정 및 버그 대처 용이하게 함
void	stAccount::WritePacket_Server_v0(PX_BUFFER*& pBuffer)			// 존서버나 인스턴스 서버 이동시 내용 전달
{
	pBuffer->WriteUI8(selectedSvrRegion);

	UINT16 accountItemCount = (UINT16)listAccountItem.size();
	pBuffer->WriteUI16(accountItemCount);
	std::list<stAccountItem*>::iterator itor_acitem = listAccountItem.begin();
	while (itor_acitem != listAccountItem.end())
	{
		stAccountItem* pACItem = *itor_acitem;

		pBuffer->WriteUI64(pACItem->itemGUID.productID);
		pBuffer->WriteUI16(pACItem->count);

		++itor_acitem;
	}
}

void	stAccount::ReadPacket_Server_v0(BYTE*& pBuffer, _MemoryPool<stAccountItem>* poolACItem)
{
	selectedSvrRegion = PX_RECV_GET_UI8(pBuffer);

	UINT16 accountItemCount = PX_RECV_GET_UI16(pBuffer);
	for (int i = 0; i < accountItemCount; ++i)
	{
		stAccountItem* pACItem = poolACItem->Allocate();
		pACItem->Init();
		pACItem->itemGUID.GUID = ACGUID;
		pACItem->itemGUID.productID = PX_RECV_GET_UI64(pBuffer);
		pACItem->count = PX_RECV_GET_UI16(pBuffer);

		listAccountItem.push_back(pACItem);
		adminAccountItem.AddObject(pACItem, pACItem->itemGUID);
	}
}

void	stPlayerCharacter::WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer)	// 게임내 입장 전파 .. chType은 미리 전파하는등 조치 필요..
{
	stCharacterInfo* pCHInfo = pCharacterInfo;

	pBuffer->WriteUI32(TID);
	pBuffer->WriteUI64(CHGUID);
	pBuffer->WriteVector3(pos);
	pBuffer->WriteVector3(rotation);

	pBuffer->WriteUI8(EAPTI_Count);
	pBuffer->CopyBytes(animParam_IntType, sizeof(animParam_IntType));
	pBuffer->WriteUI8(EAPTF_Count);
	pBuffer->CopyBytes(animParam_FloatType, sizeof(animParam_FloatType));
	pBuffer->WriteUI8(EAPTB_Count);
	pBuffer->WriteUI32(animParam_BoolType);

	pBuffer->WriteUI64(ACGUID);
	
	pBuffer->WriteString(pCHInfo->nickname_UTF8);
	pBuffer->WriteUI8(ECCT_Count);
	for (int i = 0; i < ECCT_Count; ++i)
	{
		pBuffer->WriteUI64(pCHInfo->customize[i]);
	}

	pBuffer->WriteUI8(EIEST_Count);
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (equipItem[i] == NULL)
		{
			pBuffer->WriteUI8(0);
		}
		else
		{
			pBuffer->WriteUI8(1);

			stItem* pItem = equipItem[i];
			pBuffer->WriteUI64(pItem->itemGUID.productID);
		}
	}

	pBuffer->WriteString(birthDate);
	pBuffer->WriteString(thumbnailUrl);
	pBuffer->WriteString(mainBadgeUrl);
	pBuffer->WriteUI8(statusType);
	pBuffer->WriteString(statusDescription_UTF8);
	pBuffer->WriteString(personalSpaceUrl);

	pBuffer->WriteI32(level);
	pBuffer->WriteI32(totalExperienceForNextLevel);
	pBuffer->WriteI32(point);
	pBuffer->WriteI32(literacyExp);
	pBuffer->WriteI32(imaginationExp);
	pBuffer->WriteI32(narrativeExp);
	pBuffer->WriteI32(sociabilityExp);
}

void	stPlayerCharacter::WritePacket_Server_v0(PX_BUFFER*& pBuffer)
{
	stCharacterInfo* pCHInfo = pCharacterInfo;

	pBuffer->WriteUI32(TID);
	pBuffer->WriteUI32(mapTID);
	pBuffer->WriteUI32(subZoneTID);
	
	pBuffer->WriteString(pCHInfo->nickname_UTF8);
	pBuffer->WriteUI8(ECCT_Count);
	for (int i = 0; i < ECCT_Count; ++i)
	{
		pBuffer->WriteUI64(pCHInfo->customize[i]);
	}

	UINT16 characterItemCount = (UINT16)listCharacterItem.size();
	pBuffer->WriteUI16(characterItemCount);
	std::list<stCharacterItem*>::iterator itor_chitem = listCharacterItem.begin();
	while (itor_chitem != listCharacterItem.end())
	{
		stCharacterItem* pCHItem = *itor_chitem;

		pBuffer->WriteUI64(pCHItem->itemGUID.productID);
		pBuffer->WriteUI16(pCHItem->count);

		++itor_chitem;
	}

	pBuffer->WriteUI8(EIEST_Count);
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (equipItem[i] == NULL)
		{
			pBuffer->WriteUI8(0);
		}
		else
		{
			pBuffer->WriteUI8(1);

			stItem* pItem = equipItem[i];
			pBuffer->WriteUI64(pItem->itemGUID.productID);
		}
	}

	pBuffer->WriteString(birthDate);
	pBuffer->WriteString(thumbnailUrl);
	pBuffer->WriteString(mainBadgeUrl);
	pBuffer->WriteUI8(statusType);
	pBuffer->WriteString(statusDescription_UTF8);
	pBuffer->WriteString(personalSpaceUrl);

	pBuffer->WriteI32(level);
	pBuffer->WriteI32(totalExperienceForNextLevel);
	pBuffer->WriteI32(point);
	pBuffer->WriteI32(literacyExp);
	pBuffer->WriteI32(imaginationExp);
	pBuffer->WriteI32(narrativeExp);
	pBuffer->WriteI32(sociabilityExp);

	pBuffer->WriteVector3(pos);
	pBuffer->WriteVector3(rotation);
}

void	stPlayerCharacter::ReadPacket_Server_v0(BYTE*& pBuffer, _MemoryPool<stCharacterItem>* poolCHItem, _MemoryPool<stItem>* poolItem)
{
	stCharacterInfo* pCHInfo = pCharacterInfo;

	TID = PX_RECV_GET_UI32(pBuffer);
	mapTID = PX_RECV_GET_UI32(pBuffer);
	subZoneTID = PX_RECV_GET_UI32(pBuffer);
	PX_RECV_GET_STRING(pCHInfo->nickname_UTF8, pBuffer);
	UINT8 custom_count = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < custom_count; ++i)
	{
		pCHInfo->customize[i] = PX_RECV_GET_UI64(pBuffer);
	}

	UINT16 characterItemCount = PX_RECV_GET_UI16(pBuffer);
	for (int i = 0; i < characterItemCount; ++i)
	{
		stCharacterItem* pCHItem = poolCHItem->Allocate();
		pCHItem->Init();
		pCHItem->itemGUID.GUID = CHGUID;
		pCHItem->itemGUID.productID = PX_RECV_GET_UI64(pBuffer);
		pCHItem->count = PX_RECV_GET_UI16(pBuffer);

		listCharacterItem.push_back(pCHItem);
		adminCharacterItem.AddObject(pCHItem, pCHItem->itemGUID);
	}

	UINT8 itemSlotCount = PX_RECV_GET_UI8(pBuffer);
	for (int i = 0; i < itemSlotCount; ++i)
	{
		UINT8	isEquip = PX_RECV_GET_UI8(pBuffer);
		if (isEquip == 1)
		{
			stItem* pItem = equipItem[i] = poolItem->Allocate();
			pItem->Init();

			pItem->itemGUID.GUID = CHGUID;
			pItem->itemGUID.productID = PX_RECV_GET_UI64(pBuffer);
			pItem->itemStatus = EIS_Equip;
			pItem->count = 1;
		}
		else
		{
			equipItem[i] = NULL;
		}
	}

	PX_RECV_GET_STRINGN(birthDate, pBuffer, BIRTHDATE_LENGTH);
	PX_RECV_GET_STRINGN(thumbnailUrl, pBuffer, THUMBNAIL_URL_LENGTH);
	PX_RECV_GET_STRINGN(mainBadgeUrl, pBuffer, MAINBADGE_URL_LENGTH);
	statusType = (NestoQL::CharacterStatusType)PX_RECV_GET_UI8(pBuffer);
	PX_RECV_GET_STRINGN(statusDescription_UTF8, pBuffer, STATUS_DESCRIPTION_LENGTH_UTF8);
	PX_RECV_GET_STRINGN(personalSpaceUrl, pBuffer, PERSONAL_SPACE_URL_LENGTH);

	level = PX_RECV_GET_I32(pBuffer);
	totalExperienceForNextLevel = PX_RECV_GET_I32(pBuffer);
	point = PX_RECV_GET_I32(pBuffer);
	literacyExp = PX_RECV_GET_I32(pBuffer);
	imaginationExp = PX_RECV_GET_I32(pBuffer);
	narrativeExp = PX_RECV_GET_I32(pBuffer);
	sociabilityExp = PX_RECV_GET_I32(pBuffer);

	PX_RECV_GET_VECTOR3(pos, pBuffer);
	PX_RECV_GET_VECTOR3(rotation, pBuffer);
}

void	stNPC::WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer)
{
	pBuffer->WriteUI32(TID);
	pBuffer->WriteUI64(CHGUID);
	pBuffer->WriteVector3(pos);
	pBuffer->WriteVector3(rotation);

	pBuffer->WriteUI8(EAPTI_Count);
	pBuffer->CopyBytes(animParam_IntType, sizeof(animParam_IntType));
	pBuffer->WriteUI8(EAPTF_Count);
	pBuffer->CopyBytes(animParam_FloatType, sizeof(animParam_FloatType));
	pBuffer->WriteUI8(EAPTB_Count);
	pBuffer->WriteUI32(animParam_BoolType);
}

void	stNPC_Equip::WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer)
{
	stCharacterInfo* pCHInfo = pCharacterInfo;

	pBuffer->WriteUI32(TID);
	pBuffer->WriteUI64(CHGUID);
	pBuffer->WriteVector3(pos);
	pBuffer->WriteVector3(rotation);

	pBuffer->WriteUI8(EAPTI_Count);
	pBuffer->CopyBytes(animParam_IntType, sizeof(animParam_IntType));
	pBuffer->WriteUI8(EAPTF_Count);
	pBuffer->CopyBytes(animParam_FloatType, sizeof(animParam_FloatType));
	pBuffer->WriteUI8(EAPTB_Count);
	pBuffer->WriteUI32(animParam_BoolType);

	pBuffer->WriteUI8(ECCT_Count);
	for (int i = 0; i < ECCT_Count; ++i)
	{
		pBuffer->WriteUI64(pCHInfo->customize[i]);
	}

	pBuffer->WriteUI8(EIEST_Count);
	for (int i = 0; i < EIEST_Count; ++i)
	{
		if (equipItem[i] == NULL)
		{
			pBuffer->WriteUI8(0);
		}
		else
		{
			pBuffer->WriteUI8(1);

			stItem* pItem = equipItem[i];
			pBuffer->WriteUI64(pItem->itemGUID.productID);
		}
	}
}

