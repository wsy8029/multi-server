#include "ServerMonitoring_RG.h"

#include <limits>

#include <io.h>
#include <string>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <TlHelp32.h> 

ServerMonitoring_RG* ServerMonitoring_RG::m_pThis = NULL;

ServerMonitoring_RG::ServerMonitoring_RG()
{
	m_pThis = this;

	ZeroMemory(m_stMornitoringNodes, sizeof(m_stMornitoringNodes));
}

ServerMonitoring_RG::~ServerMonitoring_RG()
{
}

BOOL	ServerMonitoring_RG::Initialize(UINT32 svrID)
{
	if (ServerBase_RG::Initialize(svrID) == FALSE)
	{
		return FALSE;
	}

	g_pIOCPEngine->m_strDebugFileNameHeader = "ServerMonitoring_RG";

	LoadFromJSon();

	// CB setting
	g_pIOCPEngine->m_pSocketConnCB = OnConnectSocket;
	g_pIOCPEngine->m_pSocketDisconnCB = OnCloseSocket;
	g_pIOCPEngine->m_pDBServerLostCB = OnDBServerLost;

	InitCBFunction();

	UINT16	portArray[1];
	portArray[0] = 0;

	g_pIOCPEngine->StartEngine(portArray, 1, 0, 0, 1);
	g_pIOCPEngine->SetPacketTypeCount(PK_COUNT, PKS_COUNT, PKU_COUNT);
	g_pIOCPEngine->m_bPrintStatics = FALSE;

	return TRUE;
}

void	ServerMonitoring_RG::Release()
{
	ServerBase_RG::Release();

}

void	ServerMonitoring_RG::Update()
{
	ServerBase_RG::Update();

	static bool isFirstTime = true;

	for (int i = 0; i < 3; ++i)
	{
		if (m_stMornitoringNodes[i].isAvailable)
		{
			if (m_stMornitoringNodes[i].mornitoringStatus == SERVER_STATUS_NOT_CONNECTED || m_stMornitoringNodes[i].mornitoringStatus == SERVER_STATUS_RESET)
			{
				if (m_iCurrentTime - m_stMornitoringNodes[i].lastConnectTryTime > 10)
				{
					m_stMornitoringNodes[i].lastConnectTryTime = m_iCurrentTime;
					m_stMornitoringNodes[i].pSocket = g_pIOCPEngine->ConnectTo("127.0.0.1", m_stMornitoringNodes[i].port, SVR_MONITORING, 0);
						
					if (m_stMornitoringNodes[i].pSocket != NULL)
					{
						m_stMornitoringNodes[i].mornitoringStatus = SERVER_STATUS_CONNECTED;

						m_stMornitoringNodes[i].pSocket->pData = &m_stMornitoringNodes[i];
						m_stMornitoringNodes[i].lastServerAliveCheckResponseTime = m_iCurrentTime;
					}
				}
			}
			else if (m_stMornitoringNodes[i].mornitoringStatus == SERVER_STATUS_CONNECTED)
			{
				if (m_iCurrentTime - m_stMornitoringNodes[i].lastServerAliveCheckResponseTime > 5 || m_stMornitoringNodes[i].pSocket == NULL)
				{
					// 서버 재실행 하자
					ResetProcess(&m_stMornitoringNodes[i]);

					m_stMornitoringNodes[i].mornitoringStatus = SERVER_STATUS_RESET;
				}
				else
				{
					if (m_iCurrentTime - m_stMornitoringNodes[i].lastServerAliveCheckTime > 1)
					{
						m_stMornitoringNodes[i].lastServerAliveCheckTime = m_iCurrentTime;

						PX_BUFFER* pToSvr = PxObjectManager::GetSendBufferObj(m_stMornitoringNodes[i].pSocket);
						if (pToSvr)
						{
							pToSvr->WriteUI32(1);
							g_pIOCPEngine->SendBuffer(pToSvr, PKS_SERVER_ALIVE_CHECK);
						}
					}
				}
			}

			if (m_iCurrentTime - m_stMornitoringNodes[i].lastUIUpdateTime > 10)
			{
				m_stMornitoringNodes[i].lastUIUpdateTime = m_iCurrentTime;

				if (m_stMornitoringNodes[i].mornitoringStatus == SERVER_STATUS_NOT_CONNECTED)
				{
					printf("%d] NotConn / Restart: %d\n", i, m_stMornitoringNodes[i].resetCount);
				}
				else if (m_stMornitoringNodes[i].mornitoringStatus == SERVER_STATUS_CONNECTED)
				{
					printf("%d] Conn / Restart: %d\n", i, m_stMornitoringNodes[i].resetCount);
				}
				else if (m_stMornitoringNodes[i].mornitoringStatus == SERVER_STATUS_RESET)
				{
					printf("%d] Reset / Restart: %d\n", i, m_stMornitoringNodes[i].resetCount);
				}
			}

			// for test
			//if (GetAsyncKeyState(0x30 + i) & 0x7FFF)
			//{
			//	ResetProcess(&m_stMornitoringNodes[i]);
			//}
		}
	}

	//g_pIOCPEngine->ProcRcvBufUDP_Client();

	g_pIOCPEngine->ProcRcvBuf_Client();
	g_pIOCPEngine->ProcRcvBuf_Server();
	g_pIOCPEngine->ProcSocketDisconnect();

	ServerBase_RG::PostUpdate();
}

BOOL	ServerMonitoring_RG::OnConnectSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	return TRUE;
}

BOOL	ServerMonitoring_RG::OnCloseSocket(PVOID	pData)
{
	PX_SOCKET* pSocket = (PX_SOCKET*)pData;

	stServerMonitoringNode* pMonitoringNode = (stServerMonitoringNode*)pSocket->pData;
	if (pMonitoringNode)
	{
		pMonitoringNode->pSocket = NULL;
		pSocket->pData = NULL;
	}

	return TRUE;
}

BOOL	ServerMonitoring_RG::OnDBServerLost(PVOID	pData)
{
	g_pIOCPEngine->dbgprint("OnDBServerLost\n");

	return TRUE;
}

std::string ServerMonitoring_RG::ConvertToString(std::wstring	wstr)
{
	std::string		destStr = "";
	destStr.assign(wstr.begin(), wstr.end());
	return	destStr;
}

std::wstring ReadJsonFile(const char* filename)
{
	std::wifstream wif(filename);
	wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	std::wstringstream wss;
	wss << wif.rdbuf();
	return wss.str();
}

void	ServerMonitoring_RG::LoadFromJSon()
{
	std::string fpath = std::string("../Data_Setting/MonitoringSetting.json");
	std::wstring	rbuff;
	rapidjson::GenericDocument<rapidjson::UTF16<> > json;
	if (_access(fpath.c_str(), 00) != -1)
	{
		rbuff = ReadJsonFile(fpath.c_str());
	}
	else
	{
		fprintf(stderr, "LoadFromJSon File Open Error\n");
		return;
	}
	json.Parse<0>(rbuff.c_str());

	if (json.IsArray())
	{
		for (rapidjson::SizeType i = 0; i < json.Size(); i++)
		{
			if (json[i].IsObject())
			{
				int index = -1;
				if (json[i].HasMember(L"isAvailable"))	m_stMornitoringNodes[i].isAvailable = json[i][L"isAvailable"].GetBool();
				if (json[i].HasMember(L"port"))	m_stMornitoringNodes[i].port = json[i][L"port"].GetUint();
				if (json[i].HasMember(L"processName"))	m_stMornitoringNodes[i].processName = json[i][L"processName"].GetString();
				if (json[i].HasMember(L"exePath"))	m_stMornitoringNodes[i].exePath = json[i][L"exePath"].GetString();
			}
		}
	}
}

void	ServerMonitoring_RG::SaveToJSon()
{
	std::string fpath = std::string("../Data_Setting/MonitoringSetting.json");
	rapidjson::StringBuffer s;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

	writer.StartArray();
	for (int i = 0; i < 3; ++i)
	{
		writer.StartObject();
		writer.Key("isAvailable");
		writer.Bool(m_stMornitoringNodes[i].isAvailable);
		writer.Key("port");
		writer.Uint(m_stMornitoringNodes[i].port);
		writer.Key("processName");
		writer.String(ConvertToString(m_stMornitoringNodes[i].processName).c_str());
		writer.Key("exePath");
		writer.String(ConvertToString(m_stMornitoringNodes[i].exePath).c_str());
		writer.EndObject();
	}
	writer.EndArray();
	std::ofstream of(fpath);
	of << s.GetString();
	if (!of.good())
		throw std::runtime_error("Can't write the JSON string to the file!");
}

void	ServerMonitoring_RG::ResetProcess(stServerMonitoringNode* pServerMonitoringNode)
{
	HANDLE         hProcessSnap = NULL;
	PROCESSENTRY32 pe32 = { 0 };

	std::wstring procName1 = pServerMonitoringNode->processName;
	std::transform(procName1.begin(), procName1.end(), procName1.begin(),
		[](unsigned char c) { return std::tolower(c); });

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return;

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hProcessSnap, &pe32))
	{
		DWORD Code = 0;
		DWORD dwPriorityClass;

		do {
			HANDLE hProcess;
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			dwPriorityClass = GetPriorityClass(hProcess);

			WCHAR* Temp = pe32.szExeFile;
			_wcslwr(Temp);

			if (Temp == procName1)
			{
				if (TerminateProcess(hProcess, 0))
					GetExitCodeProcess(hProcess, &Code);
				else
					return;
			}
			CloseHandle(hProcess);
		} while (Process32Next(hProcessSnap, &pe32));
	}
	CloseHandle(hProcessSnap);

	// Create the process
	//STARTUPINFO startupInfo;
	//PROCESS_INFORMATION processInfo;
	//memset(&startupInfo, 0, sizeof(startupInfo));
	//CreateProcess(pServerMonitoringNode->exePath.c_str(), L"", NULL, NULL, NULL, NULL, NULL, NULL, &startupInfo, &processInfo);

	//std::string cmd = "start " + ConvertToString(pServerMonitoringNode->exePath);
	//system(cmd.c_str());

	ShellExecute(NULL, NULL, pServerMonitoringNode->exePath.c_str(), NULL, NULL, SW_SHOW);

	++pServerMonitoringNode->resetCount;
	pServerMonitoringNode->mornitoringStatus = SERVER_STATUS_RESET;
}

