#ifndef	_CM_DEFINE_GRAPH_QL_H_
#define _CM_DEFINE_GRAPH_QL_H_

#include "../PxDefine.h"
#include "../Common/MemoryPool.h"
#include "../ServerCommon/MyCommonDefine.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "curl/curl.h"
#include "CMDefineGraphQL_Enum.h"

struct CMGQLBase;

#define		CURL_BUFF_SIZE				4194304
struct stCURLBuffer :public stMemoryPoolItem
{
	UINT16								recvSize;
	char								buff[CURL_BUFF_SIZE];
	stCURLBuffer*						next;
};

struct stGraphQLJob :public stMemoryPoolItem
{
	UINT64								GUID;					// AC or CHGUID
	NestoQL::EGraphQLMethodType			method;
	NestoQL::GraphQLType				type;
	CMGQLBase*							pInput;
	std::string							outputFields;

	NestoQL::GraphQLResult				result;
	WDocument							resultJson;
	CURLcode							resultCURLCode;
	long								resultCURLRespondCode;

	UINT32								__recvSize;
	UINT32								__headerRecvSize;
	stCURLBuffer*						__recvBuff;
	stCURLBuffer*						__recvBuffTail;

	void Init()
	{
		result = NestoQL::GQR_CURL_ERROR;
		__recvSize = 0;
		__headerRecvSize = 0;
		__recvBuff = __recvBuffTail = NULL;
	}
};

struct CMGQLBase
{
	virtual bool Deserialize(const WValue& obj) = 0;
	virtual bool Serialize(std::string& strResult) const = 0;
};

#define GQL_TOKEN_SIZE				512
#define GQL_STR_SIZE				128
#define GQL_STR_UTF8_SIZE			385
#define GQL_ERROR_NUM				4
#define GQL_ERROR_PATH_NUM			4

namespace NestoQL
{
	typedef UINT64	ID;

	BOOL	IsJsonHasValue(const WValue& obj, const wchar_t* name);
	UINT32	GetJsonUInt(const WValue& obj, const wchar_t* name);
	UINT64	GetJsonUInt64(const WValue& obj, const wchar_t* name);
	UINT64	GetJsonUInt64FromStr(const WValue& obj, const wchar_t* name);
	INT32	GetJsonInt(const WValue& obj, const wchar_t* name);
	INT64	GetJsonInt64(const WValue& obj, const wchar_t* name);
	DOUBLE	GetJsonDouble(const WValue& obj, const wchar_t* name);
	BOOL	GetJsonString(const WValue& obj, const wchar_t* name, char* str, int strLen);
	BOOL	GetJsonWString(const WValue& obj, const wchar_t* name, wchar_t* wstr, int wstrLen);
	BOOL	GetJsonWStringAndString(const WValue& obj, const wchar_t* name, wchar_t* wstr, int wstrLen, char* str, int strLen);

	struct ErrorExtension : public CMGQLBase
	{
		INT32			code;

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }

		ErrorExtension()
		{
			code = 0;
		}
	};

	struct Error : public CMGQLBase
	{
		WCHAR			message[GQL_STR_SIZE];
		UINT8			pathCount;
		CHAR			path[GQL_ERROR_PATH_NUM][GQL_STR_SIZE];
		ErrorExtension	extensions;

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }

		Error()
		{
			ZeroMemory(message, sizeof(message));
			pathCount = 0;
			ZeroMemory(path, sizeof(path));
		}
	};

	struct ResultError : public CMGQLBase
	{
		UINT8			errorsCount;
		Error			errors[GQL_ERROR_NUM];

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }

		ResultError()
		{
			errorsCount = 0;
			ZeroMemory(errors, sizeof(errors));
		}
	};

	struct UserProductAvatar : public CMGQLBase
	{
		ID productId;
		CHAR uuid[GQL_STR_SIZE];
		ProductAvatarType	productAvatarType;

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }
		static bool SerializeField(std::string& strResult);

		UserProductAvatar()
		{
			productId = 0;
			ZeroMemory(uuid, sizeof(uuid));
			productAvatarType = ProductAvatarType::FACE_SHAPE;
		}
	};

	struct UserAvatarProducts : public CMGQLBase
	{
		UserProductAvatar faceShape;
		UserProductAvatar eyebrow;
		UserProductAvatar eye;
		UserProductAvatar nose;
		UserProductAvatar mouth;
		UserProductAvatar skinColor;
		UserProductAvatar faceSticker;
		UserProductAvatar hair;
		UserProductAvatar top;
		UserProductAvatar bottom;
		UserProductAvatar onepiece;
		UserProductAvatar shoe;
		UserProductAvatar accessory;

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }
		static bool SerializeField(std::string& strResult);
	};

	struct MyProfileOutput : public CMGQLBase
	{
		ID id;
		ID userNo;
		WCHAR nickname[GQL_STR_SIZE];
		CHAR nickname_UTF8[GQL_STR_UTF8_SIZE];
		CHAR birthdate[GQL_STR_SIZE];
		CHAR thumbnailUrl[GQL_STR_SIZE];
		CHAR mainBadgeUrl[GQL_STR_SIZE];
		CharacterStatusType statusType;
		WCHAR statusDescription[GQL_STR_SIZE];
		CHAR statusDescription_UTF8[GQL_STR_UTF8_SIZE];
		CHAR personalSpaceUrl[GQL_STR_SIZE];
		INT32 level;
		INT32 totalExperienceForNextLevel;
		INT32 point;
		INT32 literacyExp;
		INT32 imaginationExp;
		INT32 narrativeExp;
		INT32 sociabilityExp;
		UserAvatarProducts avatarProducts;

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }
		static bool SerializeField(std::string& strResult);

		MyProfileOutput()
		{
			id = 0;
			userNo = 0;
			ZeroMemory(nickname, sizeof(nickname));
			ZeroMemory(nickname_UTF8, sizeof(nickname_UTF8));
			ZeroMemory(birthdate, sizeof(birthdate));
			ZeroMemory(thumbnailUrl, sizeof(thumbnailUrl));
			ZeroMemory(mainBadgeUrl, sizeof(mainBadgeUrl));
			statusType = CharacterStatusType::DEFAULT;
			ZeroMemory(statusDescription, sizeof(statusDescription));
			ZeroMemory(statusDescription_UTF8, sizeof(statusDescription_UTF8));
			ZeroMemory(personalSpaceUrl, sizeof(personalSpaceUrl));
			level = 0;
			totalExperienceForNextLevel = 0;
			point = 0;
			literacyExp = 0;
			imaginationExp = 0;
			narrativeExp = 0;
			sociabilityExp = 0;
		}
	};

	struct SignInInput : public CMGQLBase, stMemoryPoolItem
	{
		CHAR userId[GQL_STR_SIZE];
		CHAR password[GQL_STR_SIZE];

		virtual bool Deserialize(const WValue& obj) { return true; }
		virtual bool Serialize(std::string& strResult) const;

		SignInInput()
		{
			ZeroMemory(userId, sizeof(userId));
			ZeroMemory(password, sizeof(password));
		}
	};

	struct SignInOutput : public CMGQLBase
	{
		CHAR accessToken[GQL_TOKEN_SIZE];
		CHAR refreshToken[GQL_TOKEN_SIZE];

		virtual bool Deserialize(const WValue& obj);
		virtual bool Serialize(std::string& strResult) const { return true; }
		static bool SerializeField(std::string& strResult);

		SignInOutput()
		{
			ZeroMemory(accessToken, sizeof(accessToken));
			ZeroMemory(refreshToken, sizeof(refreshToken));
		}
	};
}

#endif
