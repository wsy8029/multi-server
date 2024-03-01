#include "MyCommonDefine.h"
#include "PxIOCPEngine.h"
#include "PxPacketDefine.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include <sstream>
#include <fstream>
#include <codecvt>

extern	PxIOCPEngine* g_pIOCPEngine;

void	SetStringValueA(CHAR*	pDest1, CHAR*	pUTF8Str)
{
	if (pDest1 != NULL)
	{
		size_t len2 = strlen(pUTF8Str);
		memcpy(pDest1, pUTF8Str, len2);
		pDest1[len2] = '\0';
	}
}

time_t MySQLString2time_t(const std::string& strDateTime)
{
	if (strDateTime.length() == 0) return 0;

	boost::posix_time::ptime t(boost::posix_time::time_from_string(strDateTime));
	tm _tm = to_tm(t);
	return mktime(&_tm);
}

std::string time_t2MySQLString(time_t time)
{
	tm tmTime;
	localtime_s(&tmTime, &time);

	char timeBuf[256];
	strftime(timeBuf, 256, "%Y-%m-%d %H:%M:%S", &tmTime);

	return std::string(timeBuf);
}

bool CMReadFile(const char* filename, std::wstring& rstr)
{
	std::wifstream wif(filename);
	if (wif.fail())
	{
		return false;
	}
	else
	{
		wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
		std::wstringstream wss;
		wss << wif.rdbuf();
		rstr = wss.str();
	}

	return true;
}

std::string ConvertWStrToStr(std::wstring	wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(wstr);
}

std::wstring ConvertStrToWStr(std::string	str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> myconv;
	return myconv.from_bytes(str);
}

void	ConvertCtoWC(const char* str, wchar_t* result, int resultMaxLen)
{
	int strSize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, NULL);
	if (strSize <= resultMaxLen)
	{
		MultiByteToWideChar(CP_UTF8, 0, str, strlen(str) + 1, result, strSize);
	}
	else
	{
		MultiByteToWideChar(CP_UTF8, 0, str, strlen(str) + 1, result, resultMaxLen);
		result[resultMaxLen] = L'\0';
	}
}

void	ConvertWCtoC(const wchar_t* str, char* result, int resultMaxLen)
{
	int strSize = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
	if (strSize <= resultMaxLen)
	{
		WideCharToMultiByte(CP_UTF8, 0, str, -1, result, strSize, 0, 0);
	}
	else
	{
		WideCharToMultiByte(CP_UTF8, 0, str, -1, result, resultMaxLen, 0, 0);
		result[resultMaxLen] = '\0';
	}
}

std::string AES_Encrypt(const std::string& str_in, const std::string& key, const std::string& iv)
{
	std::string str_out;
	CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption encryption((byte*)key.c_str(), key.length(), (byte*)iv.c_str());

	CryptoPP::StringSource encryptor(str_in, true,
		new CryptoPP::StreamTransformationFilter(encryption,
			new CryptoPP::Base64Encoder(
				new CryptoPP::StringSink(str_out),
				false // do not append a newline
			)
		)
	);
	return str_out;
}

std::string AES_Decrypt(const std::string& str_in, const std::string& key, const std::string& iv)
{
	std::string str_out;

	CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption decryption((byte*)key.c_str(), key.length(), (byte*)iv.c_str());

	CryptoPP::StringSource decryptor(str_in, true,
		new CryptoPP::Base64Decoder(
			new CryptoPP::StreamTransformationFilter(decryption,
				new CryptoPP::StringSink(str_out)
			)
		)
	);
	return str_out;
}

UINT8	GetTypeFromGUID(UINT64 GUID)
{
	UINT32 val_low = GUID & 0xFFFFFFFF;
	return (val_low >> 24) & 0xFF;
}

//--------------------------------------------------------------------------------------------------------
PxAdminItem::PxAdminItem()
{
	//InitializeCriticalSection(&m_csCritSec);
	InitializeCriticalSectionAndSpinCount(&m_csCritSec, 0x00000400);
}

PxAdminItem::~PxAdminItem()
{
	DeleteCriticalSection(&m_csCritSec);
}

UINT64	PxAdminItem::GetHashMapItem(stItemGUID nKey)
{
	HASHMAP_ITEM::iterator iter = m_HashMapItemGUID.find(nKey);

	if (iter != m_HashMapItemGUID.end())
		return (*iter).second;
	else return 0;
}

BOOL	PxAdminItem::SetHashMapItem(stItemGUID nKey, UINT64 lIndex)
{
	m_HashMapItemGUID.insert(HASHMAP_ITEM::value_type(nKey, lIndex));

	return TRUE;
}

BOOL	PxAdminItem::RemoveHashMapItem(stItemGUID nKey)
{
	m_HashMapItemGUID.erase(nKey);

	return TRUE;
}

VOID	PxAdminItem::ClearHashMapItem()
{
	m_HashMapItemGUID.clear();
}

BOOL	PxAdminItem::AddObject(PVOID pObject, stItemGUID nKey)
{
	if (!pObject)	return FALSE;

	EnterCriticalSection(&m_csCritSec);
	if (GetHashMapItem(nKey))
	{
		std::ofstream	fout;
		fout.open("admin_log.txt", std::ios::out | std::ios::app);
		fout << "Item>> CHGUID:" << nKey.GUID << "," << nKey.productID << std::endl;
		fout.close();

		LeaveCriticalSection(&m_csCritSec);
		return FALSE;
	}

	if (!SetHashMapItem(nKey, (UINT64)pObject))
	{
		LeaveCriticalSection(&m_csCritSec);
		return FALSE;
	}
	LeaveCriticalSection(&m_csCritSec);

	return TRUE;
}

// remove object... 
BOOL	PxAdminItem::RemoveObject(stItemGUID nKey)
{
	EnterCriticalSection(&m_csCritSec);
	PVOID	pObject = GetObjectByKey(nKey);
	if (!pObject)
	{
		LeaveCriticalSection(&m_csCritSec);
		return FALSE;
	}

	if (!RemoveHashMapItem(nKey))
	{
		LeaveCriticalSection(&m_csCritSec);
		return FALSE;
	}
	LeaveCriticalSection(&m_csCritSec);

	return TRUE;
}

BOOL	PxAdminItem::RemoveObjectAll()
{
	EnterCriticalSection(&m_csCritSec);
	ClearHashMapItem();
	LeaveCriticalSection(&m_csCritSec);

	return TRUE;
}

// get object...
PVOID	PxAdminItem::GetObjectByKey(stItemGUID nKey)
{
	EnterCriticalSection(&m_csCritSec);
	PVOID pvObject = (PVOID)GetHashMapItem(nKey);
	LeaveCriticalSection(&m_csCritSec);

	return pvObject;
}

INT32	PxAdminItem::GetObjectCount()
{
	return	(INT32)m_HashMapItemGUID.size();
}
//-------------------------------------------------------------------------------------------

void	SetPacketVersion_Server(UINT16* pArrVersion)
{
	// Svr Base
	pArrVersion[PKS_SERVER_END] = 0;
	pArrVersion[PKS_SERVER_TYPE_ANNOUNCE] = 0;
	pArrVersion[PKS_REQUEST_TIME_TAG] = 0;
	pArrVersion[PKS_INFORM_TIME_TAG] = 0;

	// Svr Specific
	pArrVersion[PKS_SERVER_ALIVE_CHECK] = 0;
	pArrVersion[PKS_MAIN_SERVER_INFO] = 0;
	pArrVersion[PKS_MAIN_SERVER_CONNECT_PREPARE] = 0;
	pArrVersion[PKS_MAIN_SERVER_CONNECT_PREPARE_RESULT] = 0;
	pArrVersion[PKS_ZONE_SERVER_INFO] = 0;
	pArrVersion[PKS_ZONE_SERVER_CONNECT_PREPARE] = 0;
	pArrVersion[PKS_ZONE_SERVER_CONNECT_PREPARE_RESULT] = 0;

	pArrVersion[PKS_ACCOUNT_CONNECT_UNIQUE] = 0;
	pArrVersion[PKS_ACCOUNT_LOGOUT] = 0;

	pArrVersion[PKS_INIT_SERVER_DATA] = 0;
	pArrVersion[PKS_SET_REGION_SERVER_ROLE] = 0;
}

void	SetPacketVersion_Client(UINT16* pArrVersion)
{
	// Client Base
	pArrVersion[PK_HAND_SHAKE] = 0;
	pArrVersion[PK_INFORM_SEND] = 0;
	pArrVersion[PK_ALIVE_CHECK] = 0;

	// Client Specific
	pArrVersion[PK_ERROR_LOG] = 0;

	pArrVersion[PK_TRANSFORM_SYNC] = 0;
	pArrVersion[PK_ANIMATOR_SYNC] = 0;
	pArrVersion[PK_DATA_SYNC] = 0;

	pArrVersion[PK_CONNECT_MAIN_SERVER_INFO] = 0;
	pArrVersion[PK_CONNECT_MAIN_SERVER_AUTH] = 0;
	pArrVersion[PK_CONNECT_ZONE_SERVER_INFO] = 0;
	pArrVersion[PK_CONNECT_ZONE_SERVER_AUTH] = 0;

	pArrVersion[PK_ACCOUNT_LOGIN] = 0;
	pArrVersion[PK_ACCOUNT_LOGOUT] = 0;
	pArrVersion[PK_CHANGE_SERVER_REGION] = 0;

	pArrVersion[PK_REQUEST_ZONE_ENTER] = 0;
	pArrVersion[PK_ZONE_ENTER] = 0;
	pArrVersion[PK_ZONE_CHANGE] = 0;
	pArrVersion[PK_ZONE_CHARACTER_ENTER] = 0;
	pArrVersion[PK_ZONE_CHARACTER_LEAVE] = 0;

	pArrVersion[PK_CHATTING] = 0;
}

void	SetPacketVersion_Client_UDP(UINT16* pArrVersion)
{
	// UDP Base
	pArrVersion[PKU_HAND_SHAKE] = 0;
	pArrVersion[PKU_COMPUTE_LATENCY] = 0;
	pArrVersion[PKU_ACK_RANGE] = 0;
	pArrVersion[PKU_DISCONNECT] = 0;

	// UDP Specific
	pArrVersion[PKU_DUMMY] = 0;
}

