#ifndef _CM_DEFINE_ENUM_H_
#define _CM_DEFINE_ENUM_H_

enum EServerConnectState
{
	ESCS_WaitForConnectToServer = 0,
	ESCS_ConnectedToServer = 1,
	ESCS_DisconnectByOtherConnect = 2,
	ESCS_Disconnect = 3,
	ESCS_Count
};

enum ECharacterZoneState
{
	ECZS_None = 0,
	ECZS_Zone = 1,
	ECZS_SubZone = 2
};

enum EDevicePlatform
{
	EDP_Android = 0,
	EDP_IOS = 1,
	EDP_WebGL = 2,
	EDP_Windows = 3,
	EDP_Count
};

// 클라이언트 애니메이션 전파시 (PK_ANIMATOR_SYNC) int 값을 갖는 타입들
enum EAnimParamTypeInt
{
	EAPTI_State = 0,
	EAPTI_SubState = 1,
	EAPTI_Count
};

// 클라이언트 애니메이션 전파시 (PK_ANIMATOR_SYNC) float 값을 갖는 타입들
enum EAnimParamTypeFloat
{
	EAPTF_Speed = 0,
	EAPTF_MotionSpeed = 1,
	EAPTF_Count
};

// 클라이언트 애니메이션 전파시 (PK_ANIMATOR_SYNC) bool 값을 갖는 타입들
enum ANIM_PARAM_TYPE_BOOL
{
	EAPTB_Jump = 0,
	EAPTB_Grounded = 1,
	EAPTB_FreeFall = 2,
	EAPTB_Count
};

enum ECharacterType
{
	ECT_PC = 0,
	ECT_NPC = 1,
	ECT_NPC_Equip = 2
};

enum ECharacterSex
{
	ECS_Male = 0,
	ECS_Female = 1,
	ECS_Neutral = 2
};

enum EItemType
{
	EIT_Equip = 0,
	EIT_Attach = 1,
	EIT_Potion = 2,
	EIT_Usable = 3,
	EIT_Other = 4
};

enum EItemEquipSlotType
{
	EIEST_Top = 0,
	EIEST_Bottom = 1,
	EIEST_OnePiece = 2,
	EIEST_Shoes = 3,
	EIEST_Accessory = 4,
	EIEST_Count
};

enum ECharacterCustomizeType
{
	ECCT_FaceShape = 0,
	ECCT_Eye = 1,
	ECCT_Mouth = 2,
	ECCT_SkinColor = 3,
	ECCT_FaceSticker = 4,
	ECCT_Hair = 5,
	ECCT_Count
};

enum EItemStatus
{
	EIS_Equip = 0,
	EIS_Attach = 1,
	EIS_Inventory = 2,
	EIS_Drop = 3
};

// 상호작용 타입.. 문열기, 의자 앉기 등 상호작용이 가능한 NPC나 Object에 부여
enum EInteractionType
{
	EIT_EventReception = 0
};

#endif
