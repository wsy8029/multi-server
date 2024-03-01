#include "GraphQLManager.h"
#include <codecvt>
#include <jwt-cpp/jwt.h>

using namespace NestoQL;
GraphQLManager g_csGraphQLManager;

#define SKIP_PEER_VERIFICATION
#define SKIP_HOSTNAME_VERIFICATION

size_t CURLWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	stGraphQLJob* pJob = (stGraphQLJob*)userp;

	stCURLBuffer* pBuff = g_csGraphQLManager.m_pPoolCURLBuffer->Allocate();

	memcpy(pBuff->buff, contents, size * nmemb);
	pBuff->recvSize = size * nmemb;
	pJob->__recvSize += pBuff->recvSize;
	pBuff->next = NULL;

	if (pJob->__recvBuff == NULL)
	{
		pJob->__recvBuff = pBuff;
		pJob->__recvBuffTail = pBuff;
	}
	else
	{
		pJob->__recvBuffTail->next = pBuff;
		pJob->__recvBuffTail = pBuff;
	}
	return size * nmemb;
}

size_t CURLHeaderWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	stGraphQLJob* pJob = (stGraphQLJob*)userp;

	pJob->__headerRecvSize += size * nmemb;

	return size * nmemb;
}

GraphQLManager::GraphQLManager()
{
	ZeroMemory(m_pCURL, sizeof(m_pCURL));
	ZeroMemory(m_pCURLError, sizeof(m_pCURLError));

	m_pPoolCURLBuffer = NULL;
	m_pPoolGraphQLJob = NULL;

	m_pPoolSignInInput = NULL;

	m_iByteAllocated = 0;
	m_iCurAllocated = 0;
	m_pMemoryBlock = NULL;
	m_pFrame = NULL;
	m_bBeforeAlloced = FALSE;
	m_listOverFlow = NULL;
}

GraphQLManager::~GraphQLManager()
{
}

BOOL	GraphQLManager::Init(std::string url)
{
	m_strUrl = url;

	m_pPoolCURLBuffer = new _MemoryPool<stCURLBuffer>(16, "stCURLBuffer");
	m_pPoolGraphQLJob = new _MemoryPool<stGraphQLJob>(8, "stGraphQLJob");
	m_pPoolSignInInput = new _MemoryPool<SignInInput>(4, "stGraphQLJob");

	InitFrameMemory(1024 * 1024 * 4);					//4 MB

	InitCURL();

	return TRUE;
}

BOOL	GraphQLManager::Init_NoAlloc()
{
	return TRUE;
}

void	GraphQLManager::Release()
{
	ReleaseCURL();

	if (m_pPoolCURLBuffer)	{ delete m_pPoolCURLBuffer; m_pPoolCURLBuffer = NULL; }
	if (m_pPoolGraphQLJob)	{ delete m_pPoolGraphQLJob;	m_pPoolGraphQLJob = NULL; }

	if (m_pPoolSignInInput) { delete m_pPoolSignInInput; m_pPoolSignInInput = NULL; }

	FrameMemoryRelease();
}

BOOL	GraphQLManager::InitCURL()
{
	for (int i = 0; i < EGQLMT_Count; ++i)
	{
		m_pCURL[i] = curl_easy_init();

		if (m_pCURL[i]) {

			curl_easy_setopt(m_pCURL[i], CURLOPT_BUFFERSIZE, CURL_BUFF_SIZE);

#ifdef SKIP_PEER_VERIFICATION
			/*
			* If you want to connect to a site who isn't using a certificate that is
			* signed by one of the certs in the CA bundle you have, you can skip the
			* verification of the server's certificate. This makes the connection
			* A LOT LESS SECURE.
			*
			* If you have a CA cert for the server stored someplace else than in the
			* default bundle, then the CURLOPT_CAPATH option might come handy for
			* you.
			*/
			curl_easy_setopt(m_pCURL[i], CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
			/*
			* If the site you're connecting to uses a different host name that what
			* they have mentioned in their server certificate's commonName (or
			* subjectAltName) fields, libcurl will refuse to connect. You can skip
			* this check, but this will make the connection less secure.
			*/
			curl_easy_setopt(m_pCURL[i], CURLOPT_SSL_VERIFYHOST, 0L);
#endif
			curl_easy_setopt(m_pCURL[i], CURLOPT_ERRORBUFFER, m_pCURLError);

			curl_easy_setopt(m_pCURL[i], CURLOPT_WRITEFUNCTION, CURLWriteCallback);

			curl_easy_setopt(m_pCURL[i], CURLOPT_HEADER, 1L);
			curl_easy_setopt(m_pCURL[i], CURLOPT_HEADERFUNCTION, CURLHeaderWriteCallback);

			// enable TCP keep-alive for this transfer 
			curl_easy_setopt(m_pCURL[i], CURLOPT_TCP_KEEPALIVE, 1L);

			curl_easy_setopt(m_pCURL[i], CURLOPT_POST, 1L);
			curl_easy_setopt(m_pCURL[i], CURLOPT_URL, m_strUrl.c_str());

			//curl_easy_setopt(m_pCURL[i], CURLOPT_TIMEOUT, 5L);
		}
	}

	return TRUE;
}

void	GraphQLManager::ReleaseCURL()
{
	for (int i = 0; i < EGQLMT_Count; ++i)
	{
		if (m_pCURL[i])
		{
			curl_easy_cleanup(m_pCURL[i]);
			m_pCURL[i] = NULL;
		}
	}
}

void	GraphQLManager::Update()
{

	FrameMemoryClear();
}

BOOL	GraphQLManager::InitFrameMemory(size_t	nSize)
{
	m_pMemoryBlock = new UINT8[nSize];
	m_pFrame = m_pMemoryBlock;
	m_iByteAllocated = nSize;

	return TRUE;
}

void* GraphQLManager::AllocFrameMemory(size_t nBytes)
{
	UINT8* pMem;
	pMem = m_pFrame;

	if (m_iCurAllocated + nBytes > m_iByteAllocated)
	{
		fprintf(stderr, "PxFrameMemory:Memory Buffer초과\n");

		stAllocByOverflow* nw_node = new stAllocByOverflow;
		nw_node->pAllocedObject = new UINT8[nBytes];
		nw_node->next = m_listOverFlow;
		m_listOverFlow = nw_node;

		m_bBeforeAlloced = FALSE;

		return nw_node->pAllocedObject;
	}

	m_iCurAllocated += nBytes;
	m_pFrame += nBytes;
	m_bBeforeAlloced = TRUE;

	return (void*)pMem;
}

void	GraphQLManager::DeallocFrameMemory(size_t	nBytes)
{
	if (m_bBeforeAlloced)
	{
		m_pFrame -= nBytes;
		m_iCurAllocated -= nBytes;
	}
}

void	GraphQLManager::FrameMemoryClear()
{
	m_pFrame = m_pMemoryBlock;					// 시작 위치로
	m_iCurAllocated = 0;

	if (m_listOverFlow)
	{
		stAllocByOverflow* cur_node = m_listOverFlow;
		stAllocByOverflow* remove_node;
		while (cur_node)
		{
			remove_node = cur_node;
			cur_node = cur_node->next;

			delete remove_node->pAllocedObject;
			delete remove_node;
		}

		m_listOverFlow = NULL;
	}
	m_bBeforeAlloced = FALSE;
}

void	GraphQLManager::FrameMemoryRelease()
{
	FrameMemoryClear();
	if (m_pMemoryBlock)	delete[]m_pMemoryBlock;
	m_pMemoryBlock = m_pFrame = NULL;
	m_iByteAllocated = 0;
}

UINT64	GraphQLManager::GetUserNoFromJWTToken(CHAR* strToken)
{
	auto decoded = jwt::decode(strToken);
	auto json = decoded.get_payload_json();

	picojson::value::object::iterator iter = json.find("id");
	if (iter != json.end())
	{
		picojson::value v = iter->second;
		UINT64 id = v.get<INT64>();
		return id;
	}

	return 0;
}

void	GraphQLManager::SetGraphQLCompleteCB(EGraphQLMethodType method, PxGraphQLCompleteFunc pFunc)
{
	m_mapGraphQLCompleteFunc[method] = pFunc;
}

BOOL	GraphQLManager::CallGraphQLCompleteCB(EGraphQLMethodType method, stGraphQLJob* pJob)
{
	std::unordered_map<EGraphQLMethodType, PxGraphQLCompleteFunc>::iterator it = m_mapGraphQLCompleteFunc.find(method);
	if (it != m_mapGraphQLCompleteFunc.end())
	{
		return it->second(pJob);
	}
	return FALSE;
}

BOOL	GraphQLManager::QueryGraphQL(UINT64 GUID, EGraphQLMethodType method, NestoQL::GraphQLType type, CMGQLBase* pInput, std::string outputFields, CHAR* szAccessToken, stClientSessionBase* pSessionBase)
{
	std::string strJson = "{\"query\":\"";
	if (type == NestoQL::MUTATION)
	{
		strJson += "mutation ";
	}
	else if (type == NestoQL::QUERY)
	{
		strJson += "query ";
	}
	std::string strInput = "";
	
	strJson += ToString(method) + std::string("{") + ToString(method);
	if (pInput != NULL)
	{
		pInput->Serialize(strInput);
		strJson += "(input:" + strInput + ")";
	}
	strJson += outputFields + "}\"}";

	stGraphQLJob* pJob = m_pPoolGraphQLJob->Allocate();
	pJob->Init();
	pJob->GUID = GUID;
	pJob->method = method;
	pJob->type = type;
	pJob->pInput = pInput;
	pJob->outputFields = outputFields;

	struct curl_slist* chunk = NULL;
	chunk = curl_slist_append(chunk, std::string("Content-Type:application/json").c_str());
	chunk = curl_slist_append(chunk, std::string("Accept-Charset:application/x-www-form-urlencoded; charset=UTF-8").c_str());
	if (szAccessToken != NULL)
	{
		chunk = curl_slist_append(chunk, std::string("Authorization:" + std::string(szAccessToken)).c_str());
	}

	curl_easy_setopt(m_pCURL[method], CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(m_pCURL[method], CURLOPT_WRITEDATA, pJob);
	curl_easy_setopt(m_pCURL[method], CURLOPT_HEADERDATA, pJob);
	curl_easy_setopt(m_pCURL[method], CURLOPT_POSTFIELDS, strJson.c_str());

	BOOL	bError = FALSE;
	BOOL	bRecycle = FALSE;
	BOOL	bNeedRefreshToken = FALSE;
	BOOL	bDestroy = FALSE;

	pJob->resultCURLCode = curl_easy_perform(m_pCURL[method]);
	if (pJob->resultCURLCode == CURLE_OK)
	{
		char* pBuff = (char*)AllocFrameMemory(pJob->__recvSize + 1);
		int	bodySize = pJob->__recvSize - pJob->__headerRecvSize;
		int	headerSize = pJob->__headerRecvSize;
		char* pBodyBuff = (char*)AllocFrameMemory(bodySize + 1);
		
		char* pHeaderBuff = (char*)AllocFrameMemory(headerSize + 1);

		stCURLBuffer* pRcvBuff = pJob->__recvBuff;
		int index = 0;
		while (pRcvBuff)
		{
			memcpy(&pBuff[index], pRcvBuff->buff, pRcvBuff->recvSize);
			index += pRcvBuff->recvSize;

			pRcvBuff = pRcvBuff->next;
		}
		pBuff[pJob->__recvSize] = '\0';

		memcpy(pHeaderBuff, pBuff, headerSize);
		pHeaderBuff[headerSize] = '\0';

		memcpy(pBodyBuff, &pBuff[pJob->__headerRecvSize], bodySize);
		pBodyBuff[bodySize] = '\0';

		int strSize = MultiByteToWideChar(CP_UTF8, 0, pBodyBuff, bodySize, NULL, NULL);
		wchar_t* pBodyBuffW = (wchar_t*)AllocFrameMemory((strSize + 1) * 2);
		MultiByteToWideChar(CP_UTF8, 0, pBodyBuff, bodySize, pBodyBuffW, strSize);
		pBodyBuffW[strSize] = L'\0';

		curl_easy_getinfo(m_pCURL[method], CURLINFO_HTTP_CODE, &pJob->resultCURLRespondCode);

		if (pJob->resultCURLRespondCode == 400)		// bad request
		{
			bError = TRUE;
		}
		else if (pJob->resultCURLRespondCode == 401)	// Unauthorized
		{
			bError = TRUE;
		}
		else if (pJob->resultCURLRespondCode == 403)	// forbidden
		{
			bError = TRUE;
		}
		else if (pJob->resultCURLRespondCode == 404)	// Not Found
		{
			bError = TRUE;
		}
		else if (pJob->resultCURLRespondCode == 415)	// Unsupported Media Type
		{
			bError = TRUE;
		}
		else if (pJob->resultCURLRespondCode == 500)	// Internal Server Error
		{
			bRecycle = TRUE;
			bError = TRUE;
		}
		else if (pJob->resultCURLRespondCode == 503)	// Service Unavailable
		{
			bRecycle = TRUE;
			bError = TRUE;
		}

		if (bRecycle)
		{
			m_listGraphQLJobRecycle.push_back(pJob);
		}
		else if (bError)
		{
			//m_pPoolGraphQLJob->Free(pJob);
		}
		else
		{
			std::wstring strBody(pBodyBuffW);
			size_t idx = strBody.find(L"\"errors\":");
			if (idx != std::wstring::npos)
			{
				strBody = std::wstring(L"{") + strBody.substr(idx);

				pJob->result = GQR_ERROR;
				pJob->resultJson.Parse<0>(strBody.c_str());
			}
			else
			{
				idx = strBody.find(L"{");
				idx = strBody.find(L"{", idx + 1);
				idx = strBody.find(L"{", idx + 1);
				strBody = strBody.substr(idx, strBody.length() - idx - 2);

				pJob->result = GQR_SUCCESS;
				pJob->resultJson.Parse<0>(strBody.c_str());
			}
		}

		DeallocFrameMemory((strSize + 1) * 2);
		DeallocFrameMemory(pJob->__recvSize + 1);
		DeallocFrameMemory(bodySize + 1);
		DeallocFrameMemory(headerSize + 1);
	}

	stCURLBuffer* pRcvBuff = pJob->__recvBuff;
	while (pRcvBuff)
	{
		stCURLBuffer* pRemoveBuff = pRcvBuff;
		pRcvBuff = pRcvBuff->next;

		m_pPoolCURLBuffer->Free(pRemoveBuff);
	}
	pJob->__recvBuff = NULL;
	
	if (bRecycle == FALSE)
	{
		CallGraphQLCompleteCB(method, pJob);
		m_pPoolGraphQLJob->Free(pJob);
	}
		
	curl_slist_free_all(chunk);

	return TRUE;
}