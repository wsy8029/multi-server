#ifndef	_GRAPH_QL_MANAGER_H_
#define _GRAPH_QL_MANAGER_H_

#include "../PxDefine.h"
#include "../Common/MemoryPool.h"
#include "../ServerCommon/PxPacketDefine.h"
#include "../ServerCommon/MyCommonDefine.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "curl/curl.h"

#include "CMDefineGraphQL.h"

struct stAllocByOverflow
{
	void* pAllocedObject;
	stAllocByOverflow* next;
};

typedef BOOL(*PxGraphQLCompleteFunc) (stGraphQLJob* pJob);

class GraphQLManager
{
public:

	CURL*									m_pCURL[NestoQL::EGQLMT_Count];
	char									m_pCURLError[CURL_BUFF_SIZE];
	std::string								m_strUrl;

	// for frame-memory
	size_t									m_iByteAllocated;			// debug 버전에서만 check
	size_t									m_iCurAllocated;
	UINT8* m_pMemoryBlock;
	UINT8* m_pFrame;
	BOOL									m_bBeforeAlloced;
	stAllocByOverflow* m_listOverFlow;

	_MemoryPool<stCURLBuffer>*				m_pPoolCURLBuffer;
	_MemoryPool<stGraphQLJob>*				m_pPoolGraphQLJob;

	// GraphQL Inputs
	_MemoryPool<NestoQL::SignInInput>*		m_pPoolSignInInput;

	std::unordered_map<NestoQL::EGraphQLMethodType, PxGraphQLCompleteFunc>	m_mapGraphQLCompleteFunc;
	std::list<stGraphQLJob*>				m_listGraphQLJobRecycle;
	
	GraphQLManager();
	~GraphQLManager();

	BOOL									Init_NoAlloc();								// 일부 함수 사용 위해서..
	BOOL									Init(std::string url);
	void									Release();
	void									Update();

	BOOL									InitCURL();
	void									ReleaseCURL();

	// for frame-memory
	BOOL									InitFrameMemory(size_t nSize);
	void*									AllocFrameMemory(size_t nBytes);
	void									DeallocFrameMemory(size_t	nBytes);
	void									FrameMemoryClear();
	void									FrameMemoryRelease();

	UINT64									GetUserNoFromJWTToken(CHAR* strToken);

	void									SetGraphQLCompleteCB(NestoQL::EGraphQLMethodType method, PxGraphQLCompleteFunc pFunc);
	BOOL									CallGraphQLCompleteCB(NestoQL::EGraphQLMethodType method, stGraphQLJob* pJob);
	BOOL									QueryGraphQL(UINT64 GUID, NestoQL::EGraphQLMethodType method, NestoQL::GraphQLType type, CMGQLBase* input, std::string outputFields, CHAR* szAccessToken = NULL, stClientSessionBase* pSessionBase = NULL);
};

#endif
