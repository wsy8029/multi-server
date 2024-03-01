#ifndef	_CM_DEFINE_H_
#define _CM_DEFINE_H_

#include <random>

#include "../PxDefine.h"
#include "../Common/MemoryPool.h"
#include "../PxAdmin/PxAdmin.h"
#include "../WebSocket/PxWebSocketEngine.h"
#include "CMDefine_Enum.h"
#include "MyCommonDefine.h"
#include "../GraphQL/CMDefineGraphQL.h"

struct stChannel;
struct stZone;
struct stSubZone;
struct stAccount;
struct stMapTemplate;

// PlayerCharacterTemplate 나 NPCTemplate json 으로부터 로딩한 캐릭터 템플릿 부모 클래스
struct stCharacterTemplate
{
	UINT32								TID;
	std::wstring						name;
	FLOAT								scale;
	FLOAT 								height;
	FLOAT 								ccRadius;
	FLOAT								mass;
	FLOAT 								moveSpeed;
	FLOAT								jumpPower;
};

// PlayerCharacterTemplate.json 으로부터 로딩한 설정 데이터
struct stPlayerCharacterTemplate : public stCharacterTemplate
{
	std::string							basePrefabName;
	ECharacterSex						sex;
};

// NPCTemplate.json 으로부터 로딩한 설정 데이터
struct stNPCTemplate : public stCharacterTemplate
{
	ECharacterType						characterType;
	std::string							prefabName;
	ECharacterSex						sex;
};

// NPC나 Object가 Interaction을 가질수 있다. 터치시 메뉴 팝업, 문열기, 앉기, 전원 켜기 등등 .. Interaction 공통 클래스
struct stInteraction
{
	EInteractionType					type;

	virtual void Init()
	{
	}

	virtual void Release()
	{
	}
};

struct stItem : public stMemoryPoolItem
{
	stItemGUID							itemGUID;
	UINT64								ACGUID;

	EItemStatus							itemStatus;
	UINT16								count;

	void								Init() {}
};

struct stAccountItem : public stMemoryPoolItem
{
	stItemGUID							itemGUID;
	UINT16								count;

	void								Init() {}
};

struct stCharacterItem : public stMemoryPoolItem
{
	stItemGUID							itemGUID;
	UINT16								count;

	void								Init() {}
};

// stAccountInfo 처럼 캐릭터 프로필,, 메신저나 길드원 목록, 채팅 상대방 프로필등 유지시에 쓰인다.
struct stCharacterInfo : public stMemoryPoolItem
{
	UINT64								CHGUID;
	WCHAR								nickname[NICKNAME_LENGTH];
	CHAR								nickname_UTF8[NICKNAME_LENGTH_UTF8];

	// thumbnail 구성용
	UINT64								customize[ECCT_Count];
	UINT64								item_Accessory;

	void								Init()
	{
		ZeroMemory(nickname, sizeof(nickname));
		ZeroMemory(nickname_UTF8, sizeof(nickname_UTF8));

		ZeroMemory(customize, sizeof(customize));
		item_Accessory = 0;
	}
};

// 캐릭터 부모 클래스 .. 클라이언트와 유사하게 공통된 캐릭터 데이터 가진다.
struct stCharacter
{
	UINT64								CHGUID;
	UINT32								TID;

	FLOAT 								moveSpeed;
	FLOAT 								collisionRadius;
	FLOAT 								collisionHeight;

	ECharacterZoneState					zoneState;
	UINT32								mapTID;
	UINT32								subZoneTID;
	MFVector3							pos;
	MFVector3							velocity;
	MFVector3							rotation;

	INT									animParam_IntType[EAPTI_Count];
	FLOAT								animParam_FloatType[EAPTF_Count];
	UINT32								animParam_BoolType;

	UINT32								zoneSvrID;											// 현재 접속중인 존서버 ID
	stZone*								pZone;
	stChannel*							pChannel;
	stSubZone*							pSubZone;
	
	ECharacterType						chType;

	std::list<stCharacterItem*>			listCharacterItem;
	PxAdminItem							adminCharacterItem;

	stCharacterInfo*					pCharacterInfo;
	stCharacterTemplate*				pCT;

	virtual void	Init()
	{
		mapTID = 0;
		ZeroMemory(&pos, sizeof(pos));
		ZeroMemory(&velocity, sizeof(velocity));
		ZeroMemory(&rotation, sizeof(rotation));

		ZeroMemory(animParam_IntType, sizeof(animParam_IntType));
		ZeroMemory(animParam_FloatType, sizeof(animParam_FloatType));

		zoneSvrID = 0;
		zoneState = ECZS_None;
		pZone = NULL;
		pChannel = NULL;
		pSubZone = NULL;
		pCharacterInfo = NULL;
		pCT = NULL;
	}

	virtual void	Release()
	{
	}

	virtual void	WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer) {}
};

// 커스터마이징과 아이템 장착을 하는 캐릭터 클래스 .. stPlayerCharacter 와 stNPCEquip 이 이 클래스 상속을 받는다. 클라이언트와 같은 구조
struct stCharacter_Equip : public stCharacter
{
	//UINT64								customize[ECCT_Count];					// character info 에 있다..
	stItem*								equipItem[EIEST_Count];

	void	Init()	override
	{
		stCharacter::Init();
		//ZeroMemory(customize, sizeof(customize));
		ZeroMemory(equipItem, sizeof(equipItem));
	}

	void		Release() override
	{
		stCharacter::Release();
	}

	void	WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer) override {};
};

// 플레이어 캐릭터 클래스 .. Zone 이나 Main 서버에는 소켓에 대응한다. (pSession)
struct stPlayerCharacter : public stCharacter_Equip, stMemoryPoolItem
{
	UINT64								ACGUID;

	NestoQL::CharacterStatusType		statusType;
	WCHAR								statusDescription[STATUS_DESCRIPTION_LENGTH];
	CHAR								statusDescription_UTF8[STATUS_DESCRIPTION_LENGTH_UTF8];

	CHAR								birthDate[BIRTHDATE_LENGTH];
	CHAR								thumbnailUrl[THUMBNAIL_URL_LENGTH];
	CHAR								mainBadgeUrl[MAINBADGE_URL_LENGTH];
	CHAR								personalSpaceUrl[PERSONAL_SPACE_URL_LENGTH];

	INT32								level;
	INT32								totalExperienceForNextLevel;
	INT32								point;
	INT32								literacyExp;
	INT32								imaginationExp;
	INT32								narrativeExp;
	INT32								sociabilityExp;

	time_t								recentLoginTime;

	stPlayerCharacterTemplate*			pPCT;
	PVOID								pSession;
	stAccount*							pAccount;

	void	Init()	override
	{
		stCharacter_Equip::Init();
		chType = ECT_PC;
		pPCT = NULL;
		pAccount = NULL;

		ZeroMemory(statusDescription, sizeof(statusDescription));
		ZeroMemory(statusDescription_UTF8, sizeof(statusDescription_UTF8));
		ZeroMemory(birthDate, sizeof(birthDate));
		ZeroMemory(thumbnailUrl, sizeof(thumbnailUrl));
		ZeroMemory(mainBadgeUrl, sizeof(mainBadgeUrl));
		ZeroMemory(personalSpaceUrl, sizeof(personalSpaceUrl));

		level = 0;
		totalExperienceForNextLevel = 0;
		point = 0;
		literacyExp = 0;
		imaginationExp = 0;
		narrativeExp = 0;
		sociabilityExp = 0;
	}

	void		Release() override
	{
		stCharacter_Equip::Release();
	}

	void	WritePacket_Server_v0(PX_BUFFER*& pBuffer);					// 존서버나 인스턴스 서버 이동시 내용 전달
	void	ReadPacket_Server_v0(BYTE*& pBuffer, _MemoryPool<stCharacterItem>* poolCHItem, _MemoryPool<stItem>* poolItem);
	void	WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer) override;
};

// 커스터마이징이나 아이템 장착 안하는 NPC.. 몬스터 등등
struct stNPC : public stCharacter, stMemoryPoolItem
{
	stNPCTemplate*							pNPCT;
	std::list<stInteraction*>				listInteraction;

	void	Init()	override
	{
		stCharacter::Init();
		chType = ECT_NPC;

		pNPCT = NULL;
	}

	void	Release() override
	{
		stCharacter::Release();
	}

	void	WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer) override;
};

// 플레이어 캐릭터 처럼 커스터마이징이나 아이템 장착하는 NPC.. 
struct stNPC_Equip : public stCharacter_Equip, stMemoryPoolItem
{
	stNPCTemplate*					pNPCT;
	std::list<stInteraction*>		listInteraction;

	void	Init()	override
	{
		stCharacter_Equip::Init();
		chType = ECT_NPC_Equip;

		pNPCT = NULL;
	}

	void		Release() override
	{
		stCharacter_Equip::Release();
	}

	void	WritePacket_InGame_Broadcast_v0(PxWSBuffer*& pBuffer) override;
};

struct stAccount
{
	UINT64											ACGUID;
	UINT8											selectedSvrRegion;

	std::list<stAccountItem*>						listAccountItem;
	PxAdminItem										adminAccountItem;

	UINT64											CHGUID;

	void Init() noexcept
	{
		CHGUID = 0;
	}

	void	WritePacket_Server_v0(PX_BUFFER*& pBuffer);
	void	ReadPacket_Server_v0(BYTE*& pBuffer, _MemoryPool<stAccountItem>* poolACItem);
};

class	PxAdminUI64_Zone_Character : public PxAdminUI64
{
public:
	PxAdminUI64_Zone_Character()
	{
		m_strDebugInfo = "Zone_Character";
	}
};

class	PxAdminUI64_Channel_Character : public PxAdminUI64
{
public:
	PxAdminUI64_Channel_Character()
	{
		m_strDebugInfo = "Channel_Character";
	}
};

class	PxAdminUI64_SubZone_Character : public PxAdminUI64
{
public:
	PxAdminUI64_SubZone_Character()
	{
		m_strDebugInfo = "SubZone_Character";
	}
};

class	PxAdminUI64_CharacterInfo : public PxAdminUI64
{
public:
	PxAdminUI64_CharacterInfo()
	{
		m_strDebugInfo = "CharacterInfo";
	}
};

// 존은 채널이나 subZone 이 존재하기 위해 항상 있어야 하는 최상위 단위이다. 아래 예를 든 서울대공원도 서울대공원 전체는 존에 해당한다. 서브존들이 있는 맵에도 항상 존은 존재..
struct stZone : public stMemoryPoolItem
{
	UINT32									mapTID;
	UINT64									ZONEGUID;								// vivox 음성 채팅용 등

	UINT16									pcCount;
	std::list<stPlayerCharacter*>			listPC;
	UINT16									npcCount;
	std::list<stNPC*>						listNPC;
	UINT16									npcEquipCount;
	std::list<stNPC_Equip*>					listNPCEquip;
	PxAdminUI64_Zone_Character				csAdminCharacter;

	std::list<stChannel*>					listChannel;
	std::list<stChannel*>					listChannel_EmptySpace;
	PxAdminUI64								csAdminChannel;

	void Init()
	{
		pcCount = 0;
		listPC.clear();
		npcCount = 0;
		listNPC.clear();
		npcEquipCount = 0;
		listNPCEquip.clear();
		csAdminCharacter.RemoveObjectAll();

		listChannel.clear();
		listChannel_EmptySpace.clear();
		csAdminChannel.RemoveObjectAll();
	}
};

// 홍대(존) - 홍대에 접속한 사람들을 일정 수 단위로 그룹핑 하기 위해 채널이 존재.. 클라이언트 최적화, 서버 전파 대역폭 이슈 등
struct stChannel : public stMemoryPoolItem
{
	UINT64									CHANGUID;
	stZone*									pZone;									// 소속 존

	UINT16									pcCount;
	std::list<stPlayerCharacter*>			listPC;
	UINT16									npcCount;
	std::list<stNPC*>						listNPC;
	UINT16									npcEquipCount;
	std::list<stNPC_Equip*>					listNPCEquip;
	PxAdminUI64_Channel_Character			csAdminCharacter;

	std::list<stSubZone*>					listSubZone;
	PxAdminUI64								csAdminSubZoneByGUID;
	PxAdminUI32								csAdminSubZoneByTID;

	stMapTemplate*							pMT;

	BOOL									isAddedListEmptySpace;

	void Init()
	{
		pZone = NULL;

		pcCount = 0;
		listPC.clear();
		npcCount = 0;
		listNPC.clear();
		npcEquipCount = 0;
		listNPCEquip.clear();
		csAdminCharacter.RemoveObjectAll();

		listSubZone.clear();
		csAdminSubZoneByGUID.RemoveObjectAll();
		csAdminSubZoneByTID.RemoveObjectAll();
		pMT = NULL;
		isAddedListEmptySpace = FALSE;
	}
};

// 채널에 속한 사람들이 맵을 이동해도 서로 계속 공유가 가능하기 위해.. ex) 서울대공원(존) - 채널1(150명) - 조류관(서브존),식물원(서브존),파충류관(서브존) 등 .. 채널1에 속한 사람은 맵로딩등으로 전환이 일어나도 따라다니면 계속 볼 수 있다.
// 서브 존으로 이동한 캐릭터는 원래 채널(=존)에서 삭제된다. 
// 현재 서브존엔 EventGame 클래스가 없다. 추후 필요시 추가..
struct stSubZone : public stMemoryPoolItem
{
	UINT32									mapTID;
	UINT64									ZONEGUID;								// vivox 음성 채팅용 등
	stChannel*								pChannel;								// 소속 채널

	UINT16									pcCount;
	std::list<stPlayerCharacter*>			listPC;
	UINT16									npcCount;
	std::list<stNPC*>						listNPC;
	UINT16									npcEquipCount;
	std::list<stNPC_Equip*>					listNPCEquip;
	PxAdminUI64_SubZone_Character			csAdminCharacter;

	void Init()
	{
		pChannel = NULL;

		pcCount = 0;
		listPC.clear();
		npcCount = 0;
		listNPC.clear();
		npcEquipCount = 0;
		listNPCEquip.clear();
		csAdminCharacter.RemoveObjectAll();
	}
};

#endif
