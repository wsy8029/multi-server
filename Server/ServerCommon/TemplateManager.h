#ifndef	_TEMPLATE_MANAGER_H_
#define	_TEMPLATE_MANAGER_H_

#include "PxAdmin.h"
#include "PxExcelTxtLib.h"

#include "MyCommonDefine.h"
#include "CMDefine.h"

// MapTemplate 에서 로딩하는 스폰 지역 정보
struct stMapSpawnData
{
	MFVector3								pos;
	FLOAT									rotY;
};

// MapTemplate
struct stMapTemplate
{
	UINT32									TID;
	std::wstring							name;
	std::string								mapPath;
	UINT32									maxConnectPlayer;
	UINT32									maxChannelPlayer;
	BOOL									isSubZone;
	UINT32									parentMapTID;

	std::list<stMapSpawnData*>				listMapSpawnData;
	std::list<stMapTemplate*>				listMapTemplate_SubZone;
};

// MapTemplate.json 에 포함된 PortalTemplate 에서 쓰이는 포탈 이용시 이동하는 위치 정보
struct stPortalToData
{
	UINT32									mapTID;
	MFVector3								pos;
	FLOAT									rotY;
};

// MapTemplate.json 에 포함된 PortalTemplate
struct stPortalTemplate
{
	UINT32									TID;
	stPortalToData							toData;
};



// 템플릿 매니저 .. json , txt 등 설정 파일 로딩 담당
class TemplateManager
{
public:
	TemplateManager();
	~TemplateManager();

	PxAdminUI32						m_csAdminMapTemplate;
	PxAdminUI32						m_csAdminPortalTemplate;
	PxAdminUI32						m_csAdminPlayerCharacterTemplate;
	PxAdminUI32						m_csAdminNPCTemplate;

	BOOL							Initialize();
	void							Release();

	BOOL							LoadMapTemplate();
	BOOL							LoadNPCTemplate();
	BOOL							LoadPlayerCharacterTemplate();

	std::vector<std::string>		ParseStringVector(const WValue& json);
};

#endif