// USB_Test.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "USB_Test.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
wchar_t GetDriveLetter(unsigned long);
DWORD WINAPI readInput (LPVOID);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
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
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
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

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 0, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

  // ShowWindow(hWnd, nCmdShow);
  // UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
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
						wchar_t szMessage[80];
						wchar_t cDriveLetter = GetDriveLetter(pVol->dbcv_unitmask);
						wsprintf(szMessage, L"Device '%c:' has been inserted.", cDriveLetter);
						MessageBox(hWnd, szMessage, L"USB Notice", MB_OK);
						
						DWORD ThreadId = 0;
						if (!CreateThread (0,0,readInput,(LPVOID)cDriveLetter,0,&ThreadId))
							MessageBox (hWnd,L"Unable to create calculations thread", L"Error",MB_OK);
					}
				}
				break;
			case DBT_DEVICEREMOVECOMPLETE:
				// TODO: Make the server call for logout
				break;
		}
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

wchar_t GetDriveLetter(unsigned long ulUnitMask)
{
	wchar_t c;
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

DWORD WINAPI readInput (LPVOID pparams)
{
	char letter = (char)pparams;
	char line[256];
	std::string path(1,letter);
	path.append(":\\token.txt");
	std::ifstream myfile("F:\\token.txt");//path.c_str());
	if (myfile.is_open())
	{
		//while ( myfile.good() )
		//{
		myfile.getline(line, 256);
		std::string curlpath("http://localhost:8081/user/");
		curlpath.append(line);
		curlpath.append("/login");
			
			CURL *curl;
			CURLcode res;
			curl = curl_easy_init();
			if(curl) {
				curl_easy_setopt(curl, CURLOPT_URL, curlpath.c_str());
				res = curl_easy_perform(curl);
 
				/* always cleanup */ 
				curl_easy_cleanup(curl);
			}
			//std::cout << line << std::endl;
		//}
		myfile.close();
	}
	else std::cout << "Unable to open file";
	return 0;
}
