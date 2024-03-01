#include "ServerMonitoring_RG.h"

extern	PxIOCPEngine*	g_pIOCPEngine;
ServerMonitoring_RG* pServer = NULL;

BOOL onConsoleEvent(DWORD event) {

	switch (event) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		if (pServer)
		{
			pServer->m_bLoopStop = TRUE;
			Sleep(60000);		// 종료 처리가 될수 있도록 대기하자(이 이벤트 호출 시점이 백스레드이다)
		}
		break;
	}

	return TRUE;
}

int __cdecl main(int argc, char **argv)
{
	if (!SetConsoleCtrlHandler(onConsoleEvent, TRUE)) 
	{
		return -1;
	}

#ifdef NDEBUG
	// Install crash reporting
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);             // Size of the structure
	
	info.pszAppVersion = L"1.0.0";              // App version
	info.pszEmailTo = L"a@a.com";      // Email recipient address
	info.pszAppName = L"ServerMonitoring"; // App name
	info.pszEmailSubject = L"ServerMonitoring 1.0.0 Error Report"; // Email subject

	// Install crash handlers
	int nInstResult = crInstall(&info);

	// Check result
	if (nInstResult != 0)
	{
		WCHAR buff[256];
		crGetLastErrorMsg(buff, 256); // Get last error
		fprintf(stderr, "%s\n", buff);	// and output it to the screen
		return -1;
	}
#endif // !_DEBUG

	//FreeConsole();

	// Get console window handle
	HWND wh = GetConsoleWindow();

	int	w = GetSystemMetrics(SM_CXSCREEN);
	int	h = GetSystemMetrics(SM_CYSCREEN);

	int x_idx = 0;
	int y_idx = 1;
	int	wx = (int)((float)w * 0.24f);
	int	wy = (int)((float)h * 0.25f);

	// Move window to required position
	MoveWindow(wh, 10 + x_idx * wx + ((x_idx > 0) ? (x_idx - 1 * 5) : 0), 10 + y_idx * wy + ((y_idx > 0) ? (y_idx - 1 * 5) : 0), wx, wy, TRUE);
	SetConsoleTitleA("ServerMonitoring_RG");

	SERVER_TYPE svrType = ParseServerType(argv[1]);
	int svrRegion = atoi(argv[2]);
	int svrGroup = atoi(argv[3]);
	int svrIndex = atoi(argv[4]);
	UINT32 svrID = GetSvrID(svrType, (UINT8)svrRegion, (UINT8)svrGroup, (UINT8)svrIndex);

	pServer = new ServerMonitoring_RG();
	if(!pServer->Initialize(svrID))
	{
		Sleep(5000);
		pServer->Release();
		delete pServer;
		pServer = NULL;
		return -1;
	}
	
	DWORD	lastUpdateTime = 0;
	bool	bFirstTime = true;
	while(1)
	{
		DWORD	curTime = timeGetTime();
		if (curTime - lastUpdateTime < 100)		// 10 fps
		{
			Sleep(1);
			continue;
		}
		lastUpdateTime = curTime;

		pServer->Update();

		if(pServer->m_bLoopStop)		// 종료 처리!
		{
			break;
		}
	}

	pServer->Release();
	delete pServer;
	pServer = NULL;

#ifdef NDEBUG
	int nUninstRes = crUninstall(); // Uninstall exception handlers
	nUninstRes;
#endif // !_DEBUG
}
