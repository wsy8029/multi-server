#include "TemplateManager.h"
#include <crtdbg.h>
#if _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

TemplateManager::TemplateManager()
{
}

TemplateManager::~TemplateManager()
{
}

BOOL	TemplateManager::Initialize()
{
	if (!LoadPlayerCharacterTemplate()) return FALSE;
	if (!LoadNPCTemplate()) return FALSE;
	if (!LoadMapTemplate()) return FALSE;

	return TRUE;
}

void	TemplateManager::Release()
{
	for (HASHMAP_UI32::iterator iter = m_csAdminMapTemplate.m_HashMapUI32.begin(); iter != m_csAdminMapTemplate.m_HashMapUI32.end(); ++iter)
	{
		delete (stMapTemplate*)((*iter).second);
	}

	for (HASHMAP_UI32::iterator iter = m_csAdminPortalTemplate.m_HashMapUI32.begin(); iter != m_csAdminPortalTemplate.m_HashMapUI32.end(); ++iter)
	{
		delete (stPortalTemplate*)((*iter).second);
	}

	for (HASHMAP_UI32::iterator iter = m_csAdminPlayerCharacterTemplate.m_HashMapUI32.begin(); iter != m_csAdminPlayerCharacterTemplate.m_HashMapUI32.end(); ++iter)
	{
		delete (stPlayerCharacterTemplate*)((*iter).second);
	}

	for (HASHMAP_UI32::iterator iter = m_csAdminNPCTemplate.m_HashMapUI32.begin(); iter != m_csAdminNPCTemplate.m_HashMapUI32.end(); ++iter)
	{
		delete (stNPCTemplate*)((*iter).second);
	}
}

BOOL	TemplateManager::LoadMapTemplate()
{
	std::string fpath = std::string("../Data/MapTemplate.json");
	std::wstring	rbuff;
	rapidjson::GenericDocument<rapidjson::UTF16<> > json;
	bool bSuccess = CMReadFile(fpath.c_str(), rbuff);

	if (bSuccess == false)
	{
		fprintf(stderr, "LoadMapTemplate File Open Error\n");
		return FALSE;
	}

	json.Parse<0>(rbuff.c_str());

	if (json.IsObject())
	{
		if (json.HasMember(L"listMapTemplate"))
		{
			WValue& json_LMT = json[L"listMapTemplate"];

			if (json_LMT.IsArray())
			{
				for (rapidjson::SizeType i = 0; i < json_LMT.Size(); ++i)
				{
					if (json_LMT[i].IsObject())
					{
						stMapTemplate* pMT = new stMapTemplate;
						if (json_LMT[i].HasMember(L"TID"))	pMT->TID = json_LMT[i][L"TID"].GetUint();
						if (json_LMT[i].HasMember(L"name"))	pMT->name = json_LMT[i][L"name"].GetString();
						if (json_LMT[i].HasMember(L"mapPath"))	pMT->mapPath = ConvertWStrToStr(json_LMT[i][L"mapPath"].GetString());
						if (json_LMT[i].HasMember(L"maxConnectPlayer"))	pMT->maxConnectPlayer = (UINT32)json_LMT[i][L"maxConnectPlayer"].GetUint();
						if (json_LMT[i].HasMember(L"maxChannelPlayer"))	pMT->maxChannelPlayer = (UINT32)json_LMT[i][L"maxChannelPlayer"].GetUint();
						if (json_LMT[i].HasMember(L"isSubZone"))	pMT->isSubZone = json_LMT[i][L"isSubZone"].GetBool() ? TRUE : FALSE;
						if (json_LMT[i].HasMember(L"parentMapTID"))	pMT->parentMapTID = json_LMT[i][L"parentMapTID"].GetUint();

						if (json_LMT[i].HasMember(L"listSpawnData"))
						{
							WValue& json_LMSD = json_LMT[i][L"listSpawnData"];
							if (json_LMSD.IsArray())
							{
								for (rapidjson::SizeType j = 0; j < json_LMSD.Size(); ++j)
								{
									if (json_LMSD[j].IsObject())
									{
										stMapSpawnData* pMSD = new stMapSpawnData;
										if (json_LMSD[j].HasMember(L"pos"))
										{
											WValue& json_V3 = json_LMSD[j][L"pos"];
											if (json_V3.HasMember(L"x"))	pMSD->pos.x = (FLOAT)json_V3[L"x"].GetDouble();
											if (json_V3.HasMember(L"y"))	pMSD->pos.y = (FLOAT)json_V3[L"y"].GetDouble();
											if (json_V3.HasMember(L"z"))	pMSD->pos.z = (FLOAT)json_V3[L"z"].GetDouble();
										}
										if (json_LMSD[j].HasMember(L"rotY"))	pMSD->rotY = (FLOAT)json_LMSD[j][L"rotY"].GetDouble();
										pMT->listMapSpawnData.push_back(pMSD);
									}
								}
							}
						}
						m_csAdminMapTemplate.AddObject(pMT, pMT->TID);
					}
				}
			}
		}

		if (json.HasMember(L"listPortalTemplate"))
		{
			WValue& json_LPT = json[L"listPortalTemplate"];

			if (json_LPT.IsArray())
			{
				for (rapidjson::SizeType i = 0; i < json_LPT.Size(); ++i)
				{
					if (json_LPT[i].IsObject())
					{
						stPortalTemplate* pPT = new stPortalTemplate;
						if (json_LPT[i].HasMember(L"TID"))	pPT->TID = json_LPT[i][L"TID"].GetUint();
						if (json_LPT[i].HasMember(L"toData"))
						{
							WValue& json_TD = json_LPT[i][L"toData"];
							if (json_TD.IsObject())
							{
								if (json_TD.HasMember(L"mapTID"))	pPT->toData.mapTID = json_TD[L"mapTID"].GetUint();
								if (json_TD.HasMember(L"pos"))
								{
									WValue& json_V3 = json_TD[L"pos"];
									if (json_V3.HasMember(L"x"))	pPT->toData.pos.x = (FLOAT)json_V3[L"x"].GetDouble();
									if (json_V3.HasMember(L"y"))	pPT->toData.pos.y = (FLOAT)json_V3[L"y"].GetDouble();
									if (json_V3.HasMember(L"z"))	pPT->toData.pos.z = (FLOAT)json_V3[L"z"].GetDouble();
								}
								if (json_TD.HasMember(L"rotY"))		pPT->toData.rotY = (FLOAT)json_TD[L"rotY"].GetDouble();
							}
						}
						m_csAdminPortalTemplate.AddObject(pPT, pPT->TID);
					}
				}
			}
		}
	}

	HASHMAP_UI32::iterator	itor_MT = m_csAdminMapTemplate.m_HashMapUI32.begin();
	while (itor_MT != m_csAdminMapTemplate.m_HashMapUI32.end())
	{
		stMapTemplate* pMT = (stMapTemplate*)((*itor_MT).second);

		if (pMT->isSubZone)
		{
			stMapTemplate* pParentMT = (stMapTemplate*)m_csAdminMapTemplate.GetObjectByKey(pMT->parentMapTID);
			pParentMT->listMapTemplate_SubZone.push_back(pMT);
		}

		++itor_MT;
	}

	return TRUE;
}

BOOL	TemplateManager::LoadNPCTemplate()
{
	return TRUE;
}

BOOL	TemplateManager::LoadPlayerCharacterTemplate()
{

	return TRUE;
}

std::vector<std::string>	TemplateManager::ParseStringVector(const WValue& json)
{
	std::vector<std::string> stringVector;
	if (json.IsArray())
	{
		for (rapidjson::SizeType i = 0; i < json.Size(); i++)
		{
			std::string		destStr = "";
			if (json[i].IsNull() == false)	destStr = ConvertWStrToStr(json[i].GetString());
			stringVector.push_back(destStr);
		}
	}
	else if (json.IsString())
	{
		std::string		destStr = "";
		if (json.IsNull() == false)	destStr = ConvertWStrToStr(json.GetString());
		stringVector.push_back(destStr);
	}
	return stringVector;
}

