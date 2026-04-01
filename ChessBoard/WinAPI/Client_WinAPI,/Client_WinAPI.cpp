#include "framework.h"
#include "NetworkHeader.h"
#include "Client_WinAPI.h"

#include "ChessPawn.h"

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, HWND&);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 체스 프로그램 전역변수
ChessPawn* chess_pawn = new ChessPawn;
ULONG_PTR gdiplusToken;

void Network_Loop(SOCKET s_socket, Direction send_data);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // GDI+
    Gdiplus::GdiplusStartupInput gdiplus_startup_input;
    if (Gdiplus::Ok != Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplus_startup_input, nullptr))
        return -1;

    // Network Init
    int result = 0;
    WSAData wsa_data{};
    result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (0 != result) {
        return FALSE;
    }

    const char* SERVER_IP("127.0.0.1");

    SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
    if (INVALID_SOCKET == s_socket)
        return FALSE;
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ::htons(SERVER_PORT);
    ::inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    result = ::WSAConnect(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), nullptr, nullptr, nullptr, nullptr);
    if (SOCKET_ERROR == result) {
        //error_display(L"서버 연결 실패", ::WSAGetLastError());
        return FALSE;
    }

    u_long mode = 1;
    ioctlsocket(s_socket, FIONBIO, &mode);

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENTWINAPI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    HWND hWnd{};
    if (!InitInstance (hInstance, nCmdShow, hWnd))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENTWINAPI));

    MSG msg;

    while (true) {
        if (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (WM_QUIT == msg.message)
                return static_cast<int>(msg.wParam);

            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        Direction direction = chess_pawn->MoniteringKey();
        Network_Loop(s_socket, direction);
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENTWINAPI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hWnd)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                //DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HDC memDC = CreateCompatibleDC(hdc);

        RECT rect;
        GetClientRect(hWnd, &rect);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

        FillRect(memDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        chess_pawn->DrawChessBoard(memDC, 50, 50, 200, chess_pawn->Get_Pos_X(), chess_pawn->Get_Pos_Y());

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_ERASEBKGND:
        return 1;
        break;
    case WM_DESTROY:
        delete chess_pawn;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void Network_Loop(SOCKET s_socket, Direction send_data)
{
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int result = 0;

    // send
    WSABUF send_wsa_buf = {};
    send_wsa_buf.buf = reinterpret_cast<char*>(&send_data);
    send_wsa_buf.len = sizeof(send_data);

    DWORD sent_size = 0;
    result = ::WSASend(s_socket, &send_wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
    if (SOCKET_ERROR == result) {
        int err = ::WSAGetLastError();
        if (WSAEWOULDBLOCK != err) {
            MessageBoxW(NULL, L"Error", L"WSASend", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    // recv
    int recv_data[2] = {};
    WSABUF recv_wsa_buf = {};
    recv_wsa_buf.buf = reinterpret_cast<char*>(recv_data);
    recv_wsa_buf.len = sizeof(recv_data);

    DWORD recv_size = 0;
    DWORD recv_flag = 0;
    result = ::WSARecv(s_socket, &recv_wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);
    if (SOCKET_ERROR == result) {
        int err = ::WSAGetLastError();
        if (WSAEWOULDBLOCK != err) {
            MessageBoxW(NULL, L"Error", L"WSARecv", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    chess_pawn->Set_Pos(recv_data[1], recv_data[0]);
}
