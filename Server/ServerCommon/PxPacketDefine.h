#ifndef	_PX_PACKET_DEFINE_H_
#define	_PX_PACKET_DEFINE_H_

#include "PxDefine.h"

enum	_PACKET_ID_CLIENT
{
    PK_ERROR_LOG = PK_BASE_COUNT,                           // 클라이언트가 에러 발생시 서버 전송

    PK_TRANSFORM_SYNC,
    PK_ANIMATOR_SYNC,
    PK_DATA_SYNC,

    PK_CONNECT_MAIN_SERVER_INFO,                            // 로그인 성공시 메인 서버에 접속하기 위한 추가 정보를 보낸다
    PK_CONNECT_MAIN_SERVER_AUTH,                            // 메인 서버에 접속 시도한다.
    PK_CONNECT_ZONE_SERVER_INFO,                            // 게임 진입시 존 서버에 접속하기 위한 추가 정보를 보낸다
    PK_CONNECT_ZONE_SERVER_AUTH,                            // 존 서버에 접속 시도한다.

    PK_ACCOUNT_LOGIN,                                       // 유저(계정) 로그인 요청, 최종 접속했던 main svr 로 자동 접속
    PK_ACCOUNT_LOGOUT,
    PK_CHANGE_SERVER_REGION,                                // 서버 지역 변경
    PK_ACCESS_TOKEN,                                        // (C->S):AccessToken이 변경되었을때, (S->C): AccessToken 갱신이 필요할때

    PK_REQUEST_ZONE_ENTER,                                  // 메인 서버에게 존 진입하기를 요청.. 존서버 배정 하는 동안 클라는 신 로딩 시작.
    PK_ZONE_ENTER,                                          // 존에 캐릭터가 진입
    PK_ZONE_CHANGE,                                         // 포털 들을 이용한 지도 이동
    PK_ZONE_CHARACTER_ENTER,                                // (S->C) 존에 캐릭터가 진입 했음을 전파 받는다.
    PK_ZONE_CHARACTER_LEAVE,                                // 존에서 캐릭터가 떠났음을 전파 받는다.

    PK_CHATTING,

    PK_COUNT
};

enum	_PACKET_ID_SERVER
{
    PKS_SERVER_ALIVE_CHECK = PKS_BASE_COUNT,
    PKS_MAIN_SERVER_INFO,                                   // 메인 서버 현재 서버 정보 요청 or 전파 (로그인 서버가 메인 서버 분배 역할을 하므로)
    PKS_MAIN_SERVER_CONNECT_PREPARE,                        // (로그인 -> 메인) 클라이언트 접속 준비를 요청 .. 그에 따른 준비 
    PKS_MAIN_SERVER_CONNECT_PREPARE_RESULT,                 // (메인 -> 로그인) 준비 결과를 알려준다.
    PKS_ZONE_SERVER_INFO,                                   // 존 서버 현재 서버 정보 요청 or 전파 (메인 서버가 존 서버 분배 역할을 하므로)
    PKS_ZONE_SERVER_CONNECT_PREPARE,                        // (메인 -> 존) 클라이언트 접속 준비를 요청 .. 그에 따른 준비 
    PKS_ZONE_SERVER_CONNECT_PREPARE_RESULT,                 // (존 -> 메인) 준비 결과를 알려준다.

    PKS_ACCOUNT_CONNECT_UNIQUE,                             // (로그인 -> 글로벌) 계정 접속 유일성 보장 .. ACGUID 가 접속중인 MainSvr,ZoneSvr에 로그아웃 처리
    PKS_ACCOUNT_LOGOUT,                                     // (글로벌->메인or존) 계정 로그아웃 지시
    PKS_ACCESS_TOKEN,                                       // AccessToken이 변경되었을때 서버간 전파

    PKS_INIT_SERVER_DATA,                                   // 최초 서버간 접속시 초기화 관련 데이터 전송시 (메인(리전)서버에서 관리하는 서버들에게 게임 관련 data 보내기..)
    PKS_SET_REGION_SERVER_ROLE,                             // (글로벌 -> 메인) 지역마다 1개의 메인 서버에게 리전 서버의 역할을 부여한다. 리전 서버는 지역마다 독점적으로 처리해야할 업무를 수행한다.

	PKS_COUNT
};

enum _PACKET_ID_CLIENT_UDP
{
    PKU_DUMMY = PKU_BASE_COUNT,

    PKU_COUNT
};

enum EPacketDataSyncType
{
    EPDST_IsKinematicForce = 0,
    EPDST_PositionAndRotation = 1
};

#endif