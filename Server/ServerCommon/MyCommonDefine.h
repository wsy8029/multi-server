#ifndef	_MY_COMMON_DEFINE_H_
#define _MY_COMMON_DEFINE_H_

#include "PxDefine.h"
#include <limits>

#include "rapidjson/document.h"
#include "rapidjson/reader.h"

#include <iostream>
#include "../Common/MemoryPool.h"

// 12 + 널문자
#define	NICKNAME_LENGTH										25
// (12 * 3) + 널문자
#define	NICKNAME_LENGTH_UTF8								73
#define	CHAT_MSG_LENGTH										256
#define	CHAT_MSG_LENGTH_UTF8								765
#define BIRTHDATE_LENGTH									16
#define THUMBNAIL_URL_LENGTH								128
#define MAINBADGE_URL_LENGTH								128
#define PERSONAL_SPACE_URL_LENGTH							128
#define STATUS_DESCRIPTION_LENGTH							64
#define STATUS_DESCRIPTION_LENGTH_UTF8						190

// 서버간 연결 재시도 시간
#define SERVER_RECONNECT_TRY_TIME							10
#define SERVER_INFO_UPDATE_REQUEST_TIME						5
// DB로부터 주기적으로 데이터를 가져오는 시간
#define DB_SELECT_UPDATE_TIME								10

// 최대 32768명 
#define	SERVER_MAXUSER										0x8000

#define	DB_COMMON											0
#define	DB_CONN_COUNT										1

// Removed Member Info
#define REMOVE_MEMBER_INFO_TRY_TIME_1HOUR					3600
#define	REMOVE_MEMBER_INFO_TIME_14DAY						1209600

struct stGraphQLJob;

// MySQL Blob 저장용 
class MySQL_DataBuf : public std::streambuf
{
public:
	MySQL_DataBuf(char* d, size_t s) {
		setg(d, d, d + s);
	}
};

struct stItemGUID
{
	UINT64								GUID;								// ACGUID or CHGUID
	UINT64								productID;

	stItemGUID() {}
	stItemGUID(UINT64 a, UINT64 b): GUID(a), productID(b) {}

	bool operator < (const stItemGUID& other) const {
		if (GUID == GUID)
			return productID < productID;
		return GUID < GUID;
	}
};

struct	stClientSessionBase
{
	std::list<PVOID>			m_listGraphQLWaitTokenRefresh;				// 형변환해서 쓰자..
};

typedef std::map<stItemGUID, UINT64>	HASHMAP_ITEM;
class PxAdminItem
{
private:
	UINT64			GetHashMapItem(stItemGUID nKey);
	BOOL			SetHashMapItem(stItemGUID nKey, UINT64 lIndex);
	BOOL			RemoveHashMapItem(stItemGUID nKey);
	VOID			ClearHashMapItem();		// RemoveAll과 같은 기능

	CRITICAL_SECTION	m_csCritSec;

public:
	HASHMAP_ITEM	m_HashMapItemGUID;

	PxAdminItem();
	virtual ~PxAdminItem();

	// add object...
	BOOL	AddObject(PVOID pObject, stItemGUID nKey);

	// remove object... (m_csObject, B+tree)
	BOOL	RemoveObject(stItemGUID nKey);
	BOOL	RemoveObjectAll();

	// get object...
	PVOID	GetObjectByKey(stItemGUID nKey);

	// get container's element count
	INT32	GetObjectCount();
};

// JSon 억세스용 데이터 타입
typedef rapidjson::GenericDocument<rapidjson::UTF16<> > WDocument;
typedef rapidjson::GenericValue<rapidjson::UTF16<> > WValue;
typedef rapidjson::Document SDocument;
typedef rapidjson::Value SValue;

void			SetStringValueA(CHAR*	pDest1, CHAR*	pUTF8Str);

time_t			MySQLString2time_t(const std::string& strDateTime);
std::string		time_t2MySQLString(time_t time);
std::string		AES_Encrypt(const std::string& str_in, const std::string& key, const std::string& iv);
std::string		AES_Decrypt(const std::string& str_in, const std::string& key, const std::string& iv);

// 패킷 send시 타입별 버전 세팅.. 패킷 데이터 포맷이 변경될 시에 버전을 올리고 클라이언트 receive 함수도 버전을 올려서 코딩 .. 클라도 같은 구조
void			SetPacketVersion_Client(UINT16* pArrVersion);
void			SetPacketVersion_Server(UINT16* pArrVersion);
void			SetPacketVersion_Client_UDP(UINT16* pArrVersion);
bool			CMReadFile(const char* filename, std::wstring& rstr);
std::string		ConvertWStrToStr(std::wstring	wstr);
std::wstring	ConvertStrToWStr(std::string	str);
void			ConvertCtoWC(const char* str, wchar_t* result, int resultMaxLan);
void			ConvertWCtoC(const wchar_t* str, char* result, int resultMaxLen);

UINT8			GetTypeFromGUID(UINT64 GUID);

#endif
