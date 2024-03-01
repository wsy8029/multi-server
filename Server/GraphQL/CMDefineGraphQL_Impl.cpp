#include "CMDefineGraphQL.h"
#include <codecvt>

using namespace NestoQL;

BOOL	NestoQL::IsJsonHasValue(const WValue& obj, const wchar_t* name)
{
	if (obj.HasMember(name))
	{
		if (obj[name].IsNull() == false) return TRUE;
	}
	return FALSE;
}
UINT32	NestoQL::GetJsonUInt(const WValue& obj, const wchar_t* name)
{
	return obj[name].GetUint();
}
UINT64	NestoQL::GetJsonUInt64(const WValue& obj, const wchar_t* name)
{
	return obj[name].GetUint64();
}
UINT64	NestoQL::GetJsonUInt64FromStr(const WValue& obj, const wchar_t* name)
{
	return std::stoull(obj[name].GetString());
}
INT32	NestoQL::GetJsonInt(const WValue& obj, const wchar_t* name)
{
	return obj[name].GetInt();
}
INT64	NestoQL::GetJsonInt64(const WValue& obj, const wchar_t* name)
{
	return obj[name].GetInt64();
}
DOUBLE	NestoQL::GetJsonDouble(const WValue& obj, const wchar_t* name)
{
	return obj[name].GetDouble();
}
BOOL	NestoQL::GetJsonString(const WValue& obj, const wchar_t* name, char* str, int strLen)
{
	ConvertWCtoC(obj[name].GetString(), str, strLen);
	return TRUE;
}
BOOL	NestoQL::GetJsonWString(const WValue& obj, const wchar_t* name, wchar_t* wstr, int wstrLen)
{
	wcsncpy_s(wstr, wstrLen, obj[name].GetString(), _TRUNCATE);
	return TRUE;
}
BOOL	NestoQL::GetJsonWStringAndString(const WValue& obj, const wchar_t* name, wchar_t* wstr, int wstrLen, char* str, int strLen)
{
	wcsncpy_s(wstr, wstrLen, obj[name].GetString(), _TRUNCATE);
	ConvertWCtoC(wstr, str, strLen);
	return TRUE;
}

bool ErrorExtension::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"code"))	code = GetJsonInt(obj, L"code");
	return true;
}

bool Error::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"message")) { GetJsonWString(obj, L"message", message, GQL_STR_SIZE); }
	if (IsJsonHasValue(obj, L"path"))
	{
		const WValue& pathDto = obj[L"path"];
		pathCount = 0;
		for (rapidjson::SizeType i = 0; i < pathDto.Size(); i++)
		{
			if (i == 4) break;
			ConvertWCtoC(pathDto[i].GetString(), path[pathCount++], GQL_STR_SIZE);
		}
	}
	if (IsJsonHasValue(obj, L"extensions")) { if (obj.IsObject()) { extensions.Deserialize(obj[L"extensions"]); }}
	return true;
}

bool ResultError::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"errors"))
	{
		const WValue& errorsDto = obj[L"errors"];
		errorsCount = 0;
		for (rapidjson::SizeType i = 0; i < errorsDto.Size(); i++)
		{
			if (i == 4) break;
			errors[errorsCount++].Deserialize(errorsDto[i]);
		}
	}
	return true;
}

bool UserProductAvatar::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"productId")) productId = GetJsonUInt64FromStr(obj, L"productId");
	if (IsJsonHasValue(obj, L"uuid")) GetJsonString(obj, L"uuid", uuid, GQL_STR_SIZE);
	if (IsJsonHasValue(obj, L"productAvatarType")) { char cv[GQL_STR_SIZE]; ConvertWCtoC(obj[L"productAvatarType"].GetString(), cv, GQL_STR_SIZE); productAvatarType = ConvertProductAvatarType(cv); }
	return true;
}

bool UserProductAvatar::SerializeField(std::string& strResult)
{
	strResult += "{";
	strResult += "productId ";
	strResult += "uuid ";
	strResult += "productAvatarType ";
	strResult += "}";
	return true;
}

bool UserAvatarProducts::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"faceShape")) { if (obj.IsObject()) { faceShape.Deserialize(obj[L"faceShape"]);	}}
	if (IsJsonHasValue(obj, L"eyebrow")) { if (obj.IsObject()) { eyebrow.Deserialize(obj[L"eyebrow"]); } }
	if (IsJsonHasValue(obj, L"eye")) { if (obj.IsObject()) { eye.Deserialize(obj[L"eye"]); } }
	if (IsJsonHasValue(obj, L"nose")) { if (obj.IsObject()) { nose.Deserialize(obj[L"nose"]); } }
	if (IsJsonHasValue(obj, L"mouth")) { if (obj.IsObject()) { mouth.Deserialize(obj[L"mouth"]); } }
	if (IsJsonHasValue(obj, L"skinColor")) { if (obj.IsObject()) { skinColor.Deserialize(obj[L"skinColor"]); } }
	if (IsJsonHasValue(obj, L"faceSticker")) { if (obj.IsObject()) { faceSticker.Deserialize(obj[L"faceSticker"]); } }
	if (IsJsonHasValue(obj, L"hair")) { if (obj.IsObject()) { hair.Deserialize(obj[L"hair"]); } }
	if (IsJsonHasValue(obj, L"top")) { if (obj.IsObject()) { top.Deserialize(obj[L"top"]); } }
	if (IsJsonHasValue(obj, L"bottom")) { if (obj.IsObject()) { bottom.Deserialize(obj[L"bottom"]); } }
	if (IsJsonHasValue(obj, L"onepiece")) { if (obj.IsObject()) { onepiece.Deserialize(obj[L"onepiece"]); } }
	if (IsJsonHasValue(obj, L"shoe")) { if (obj.IsObject()) { shoe.Deserialize(obj[L"shoe"]); } }
	if (IsJsonHasValue(obj, L"accessory")) { if (obj.IsObject()) { accessory.Deserialize(obj[L"accessory"]); } }
	return true;
}

bool UserAvatarProducts::SerializeField(std::string& strResult)
{
	strResult += "{";
	strResult += "faceShape";
	UserProductAvatar::SerializeField(strResult);
	strResult += "eyebrow";
	UserProductAvatar::SerializeField(strResult);
	strResult += "eye";
	UserProductAvatar::SerializeField(strResult);
	strResult += "nose";
	UserProductAvatar::SerializeField(strResult);
	strResult += "mouth";
	UserProductAvatar::SerializeField(strResult);
	strResult += "skinColor";
	UserProductAvatar::SerializeField(strResult);
	strResult += "faceSticker";
	UserProductAvatar::SerializeField(strResult);
	strResult += "hair";
	UserProductAvatar::SerializeField(strResult);
	strResult += "top";
	UserProductAvatar::SerializeField(strResult);
	strResult += "bottom";
	UserProductAvatar::SerializeField(strResult);
	strResult += "onepiece";
	UserProductAvatar::SerializeField(strResult);
	strResult += "shoe";
	UserProductAvatar::SerializeField(strResult);
	strResult += "accessory";
	UserProductAvatar::SerializeField(strResult);
	strResult += "}";
	return true;
}

bool MyProfileOutput::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"id")) id = GetJsonUInt64FromStr(obj, L"id");
	if (IsJsonHasValue(obj, L"userNo")) userNo = GetJsonUInt64FromStr(obj, L"userNo");
	if (IsJsonHasValue(obj, L"nickname")) GetJsonWStringAndString(obj, L"nickname", nickname, GQL_STR_SIZE, nickname_UTF8, GQL_STR_UTF8_SIZE);
	if (IsJsonHasValue(obj, L"birthdate")) GetJsonString(obj, L"birthdate", birthdate, GQL_STR_SIZE);
	if (IsJsonHasValue(obj, L"thumbnailUrl")) GetJsonString(obj, L"thumbnailUrl", thumbnailUrl, GQL_STR_SIZE);
	if (IsJsonHasValue(obj, L"mainBadgeUrl")) GetJsonString(obj, L"mainBadgeUrl", mainBadgeUrl, GQL_STR_SIZE);
	if (IsJsonHasValue(obj, L"statusType")) { char cv[GQL_STR_SIZE]; ConvertWCtoC(obj[L"statusType"].GetString(), cv, GQL_STR_SIZE); statusType = ConvertCharacterStatusType(cv); }
	if (IsJsonHasValue(obj, L"statusDescription")) GetJsonWStringAndString(obj, L"statusDescription", statusDescription, GQL_STR_SIZE, statusDescription_UTF8, GQL_STR_UTF8_SIZE);
	if (IsJsonHasValue(obj, L"personalSpaceUrl")) GetJsonString(obj, L"personalSpaceUrl", personalSpaceUrl, GQL_STR_SIZE);
	if (IsJsonHasValue(obj, L"level")) level = GetJsonInt(obj, L"level");
	if (IsJsonHasValue(obj, L"totalExperienceForNextLevel")) totalExperienceForNextLevel = GetJsonInt(obj, L"totalExperienceForNextLevel");
	if (IsJsonHasValue(obj, L"point")) point = GetJsonInt(obj, L"point");
	if (IsJsonHasValue(obj, L"literacyExp")) literacyExp = GetJsonInt(obj, L"literacyExp");
	if (IsJsonHasValue(obj, L"imaginationExp")) imaginationExp = GetJsonInt(obj, L"imaginationExp");
	if (IsJsonHasValue(obj, L"narrativeExp")) narrativeExp = GetJsonInt(obj, L"narrativeExp");
	if (IsJsonHasValue(obj, L"sociabilityExp")) sociabilityExp = GetJsonInt(obj, L"sociabilityExp");
	if (IsJsonHasValue(obj, L"avatarProducts")) { if (obj.IsObject()) { avatarProducts.Deserialize(obj[L"avatarProducts"]); } }
	return true;
}

bool MyProfileOutput::SerializeField(std::string& strResult)
{
	strResult += "{";
	strResult += "id ";
	strResult += "userNo ";
	strResult += "nickname ";
	strResult += "birthdate ";
	strResult += "thumbnailUrl ";
	strResult += "mainBadgeUrl ";
	strResult += "statusType ";
	strResult += "statusDescription ";
	strResult += "personalSpaceUrl ";
	strResult += "level ";
	strResult += "totalExperienceForNextLevel ";
	strResult += "point ";
	strResult += "literacyExp ";
	strResult += "imaginationExp ";
	strResult += "narrativeExp ";
	strResult += "sociabilityExp ";
	strResult += "avatarProducts";
	UserAvatarProducts::SerializeField(strResult);
	strResult += "}";
	return true;
}

bool SignInInput::Serialize(std::string& strResult) const
{
	strResult += "{";
	strResult += "userId:";
	strResult += "\\\"" + std::string(userId) + "\\\"";
	strResult += ",password:";
	strResult += "\\\"" + std::string(password) + "\\\"";
	strResult += "}";
	return true;
}

bool SignInOutput::Deserialize(const WValue& obj)
{
	if (IsJsonHasValue(obj, L"accessToken")) GetJsonString(obj, L"accessToken", accessToken, GQL_STR_SIZE);
	if (IsJsonHasValue(obj, L"refreshToken")) GetJsonString(obj, L"refreshToken", refreshToken, GQL_STR_SIZE);
	return true;
}

bool SignInOutput::SerializeField(std::string& strResult)
{
	strResult += "{";
	strResult += "accessToken ";
	strResult += "refreshToken";
	strResult += "}";
	return true;
}