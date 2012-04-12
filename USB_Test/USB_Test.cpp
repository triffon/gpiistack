// USB_Test.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "USB_Test.h"

#define MAX_LOADSTRING 100
#define FLOW_MGR_URL "http://localhost:8081/user/"

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
std::map<char, std::string> tokenMap;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

char GetDriveLetter(unsigned long);
DWORD WINAPI ReadInput(LPVOID);
void CallFlowManager(const char *, char *);
void MakeGetRequest(const char *);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_USB_TEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_USB_TEST));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_USB_TEST));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_USB_TEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 0, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DEVICECHANGE:
		{
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR) lParam;
			switch (wParam)
			{
				case DBT_DEVICEARRIVAL:
					if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
					{
						PDEV_BROADCAST_VOLUME pVol = (PDEV_BROADCAST_VOLUME) pHdr;
						if (!(pVol->dbcv_flags & DBTF_MEDIA)) // Handle USB only, exclude DVD/CDs
						{
							char driveLetter = GetDriveLetter(pVol->dbcv_unitmask);
							DWORD ThreadId = 0;
							CreateThread (0, 0, ReadInput, (LPVOID)driveLetter, 0, &ThreadId);
							// TODO Handle errors upon thread creation.
						}
					}
					break;
				case DBT_DEVICEREMOVECOMPLETE:
					if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
					{
						PDEV_BROADCAST_VOLUME pVol = (PDEV_BROADCAST_VOLUME) pHdr;
						if (!(pVol->dbcv_flags & DBTF_MEDIA)) // Handle USB only, exclude DVD/CDs
						{
							char cDriveLetter = GetDriveLetter(pVol->dbcv_unitmask);
							const char * token = tokenMap[cDriveLetter].c_str();
							//tokenMap.erase(cDriveLetter);
							CallFlowManager(token, "logout");
						}
					}
					break;
			}
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

char GetDriveLetter(unsigned long ulUnitMask)
{
	char c;
	for (c = 0; c < 26; c++)
	{
		if (ulUnitMask & 0x01)
		{
			break;
		}
		ulUnitMask = ulUnitMask >> 1;
	}
	return (c + 'A');
}

DWORD WINAPI ReadInput(LPVOID params)
{
	char driveLetter = (char)params;
	char filepath[] = "X:\\.gpii-user-token.txt";
	filepath[0]=driveLetter;
	std::ifstream myfile(filepath);
	if (myfile.is_open())
	{
		char token[256];
		myfile.getline(token, 256);
		tokenMap.insert(std::pair<char, std::string>(driveLetter, token));
		CallFlowManager(token, "login");
		myfile.close();
	}
	// TODO Handle errors when opening the file.
	return 0;
}

void CallFlowManager(const char * token, char * action)
{
	char url[256];
	strcpy(url, FLOW_MGR_URL);
	strcat(url, token);
	strcat(url, "/");
	strcat(url, action);
	MakeGetRequest(url);
}

void MakeGetRequest(const char *url)
{
	CURL *curl;
	CURLcode responseCode;
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url);
		responseCode = curl_easy_perform(curl);
		// TODO Check the response code and handler errors.
		curl_easy_cleanup(curl);
	}
}
