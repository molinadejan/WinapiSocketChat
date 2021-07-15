// WinServer.cpp : 애플리케이션에 대한 진입점을 정의합니다.
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "framework.h"
#include "WinServer.h"

#define MAX_LOADSTRING 100

using namespace std;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK ChatServerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

WSADATA wsadata;
SOCKET s, cs;
SOCKADDR_IN addr = { 0 };
SOCKADDR_IN c_addr;

TCHAR msg[1024];
char buffer[1024];

list<SOCKET> socketList;

int Win_Server_Init(HWND hWnd);
int Win_Server_Close();

SOCKET AcceptSocket(HWND hWnd, SOCKET s, SOCKADDR_IN& c_addr);
void SendMessageToClient(char *buffer);
void ReadMessage(TCHAR* msg, char *buffer, HWND &msgList);

void Win_Server_Close_Client(SOCKET s);
void AddMsgToList(const TCHAR *msg, HWND &msgList);
void MsgToBuffer();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINSERVER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINSERVER));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINSERVER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINSERVER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; 

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 480, 320, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
      return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hDlg = NULL;

    switch (message)
    {
		case WM_CREATE:
		{
			if (!IsWindow(hDlg))
			{
				hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CHAT_SERVER), hWnd, ChatServerProc);
				ShowWindow(hDlg, SW_SHOW);
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);

			switch (wmId)
			{
				case IDM_EXIT:
				{
					DestroyWindow(hWnd);
				}
				break;

				default:
				{

				}
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		default:
		{

		}
		return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int Win_Server_Init(HWND hWnd)
{
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	s = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = 8080;
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	bind(s, (LPSOCKADDR)&addr, sizeof(addr));

	WSAAsyncSelect(s, hWnd, WM_ASYNC, FD_ACCEPT);

	if (listen(s, 5) == -1)
		return 0;

	return 1;
}

int Win_Server_Close()
{
	closesocket(s);
	WSACleanup();

	return 0;
}

SOCKET AcceptSocket(HWND hWnd, SOCKET s, SOCKADDR_IN & c_addr)
{
	SOCKET cs;
	int size;
	size = sizeof(c_addr);

	cs = accept(s, (LPSOCKADDR)&c_addr, &size);

	WSAAsyncSelect(cs, hWnd, WM_ASYNC, FD_READ | FD_CLOSE);

	socketList.push_back(cs);

	return cs;
}

void SendMessageToClient(char *buffer)
{
	for (list<SOCKET>::iterator it = socketList.begin(); it != socketList.end(); ++it)
	{
		SOCKET cs = (*it);
		send(cs, (LPSTR)buffer, strlen(buffer) + 1, 0);
	}
}

void AddMsgToList(const TCHAR *msg, HWND &msgList)
{
	SYSTEMTIME t;
	TCHAR notify[128];

	GetLocalTime(&t);
	_stprintf_s(notify, 127, _T("%02d : %02d : %02d - %s"), t.wHour, t.wMinute, t.wSecond, msg);
	SendMessage(msgList, LB_ADDSTRING, 0, (LPARAM)notify);
}

void ReadMessage(TCHAR *msg, char *buffer, HWND &msgList)
{
	for (list<SOCKET>::iterator it = socketList.begin(); it != socketList.end(); ++it)
	{
		SOCKET cs = (*it);

		int msgLen = recv(cs, buffer, 1024, 0);

		if (msgLen > 0)
		{
			buffer[msgLen] = NULL;

#ifdef _UNICODE
			msgLen = MultiByteToWideChar(CP_ACP, 0, buffer, strlen(buffer), NULL, NULL);
			MultiByteToWideChar(CP_ACP, 0, buffer, strlen(buffer), msg, msgLen);
			msg[msgLen] = NULL;
#else
			strcpy(msg, buffer);
#endif

			AddMsgToList(msg, msgList);
			SendMessageToClient(buffer);
		}
	}
}

void Win_Server_Close_Client(SOCKET socket)
{
	for (list<SOCKET>::iterator it = socketList.begin(); it != socketList.end(); ++it)
	{
		SOCKET cs = (*it);

		if (cs == socket)
		{
			closesocket(cs);
			it = socketList.erase(it);
			break;
		}
	}
}

void MsgToBuffer()
{
	int msgLen;

#ifdef _UNICODE

	msgLen = WideCharToMultiByte(CP_ACP, 0, msg, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, msg, -1, buffer, msgLen, NULL, NULL);

#else
	strcpy(buffer, msg);
	msgLen = strlen(buffer);
#endif

	buffer[msgLen] = NULL;
}

BOOL CALLBACK ChatServerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND msgList;

	static HWND btnOpen;
	static HWND btnClose;
	static HWND btnSend;
	static HWND msgEdit;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			msgList = GetDlgItem(hWnd, IDC_LIST_MSG);

			btnOpen = GetDlgItem(hWnd, ID_BTN_OPEN);
			btnClose = GetDlgItem(hWnd, ID_BTN_CLOSE);
			btnSend = GetDlgItem(hWnd, ID_BTN_SEND);
			msgEdit = GetDlgItem(hWnd, IDC_EDIT_MSG);

			EnableWindow(btnClose, false);
			EnableWindow(btnSend, false);

			return 1;
		}
		break;

		case WM_ASYNC:
		{
			switch (lParam)
			{
				case FD_ACCEPT:
				{
					cs = AcceptSocket(hWnd, s, c_addr);
					AddMsgToList(_T("유저 입장"), msgList);
				}
				break;

				case FD_READ:
				{
					ReadMessage(msg, buffer, msgList);
				}
				break;

				case FD_CLOSE:
				{
					Win_Server_Close_Client(wParam);
					AddMsgToList(_T("유저 퇴장"), msgList);
				}
				break;
			}
		}
		break;

		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);

			switch (wmId)
			{
				case ID_BTN_OPEN:
				{
					if (Win_Server_Init(hWnd))
					{
						AddMsgToList(_T("서버 오픈"), msgList);

						EnableWindow(btnOpen, false);
						EnableWindow(btnClose, true);
						EnableWindow(btnSend, true);

						return 0;
					}
				}
				break;

				case ID_BTN_CLOSE:
				{
					Win_Server_Close();

					AddMsgToList(_T("서버 종료"), msgList);

					EnableWindow(btnOpen, true);
					EnableWindow(btnClose, false);
					EnableWindow(btnSend, false);

					return 0;
				}
				break;

				case ID_BTN_SEND:
				{
					GetWindowText(msgEdit, msg, sizeof(msg) / sizeof(TCHAR));
					SetWindowText(msgEdit, _T(""));
					AddMsgToList(msg, msgList);
					MsgToBuffer();
					SendMessageToClient(buffer);

					return 0;
				}
				break;
			}
		}
		break;
	}

	return 0;
}
