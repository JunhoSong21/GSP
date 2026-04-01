#include "framework.h"
#include "NetworkHeader.h"

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

// 체스 프로그램 전역변수
std::vector<ChessPawn*> chess_pawn_vec(10, nullptr);
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
        MessageBoxW(NULL, L"Error", L"WSAConnect", MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    // 전역 문자열을 초기화합니다.
    ::wcscpy_s(szTitle, L"Chess Client");
    ::wcscpy_s(szWindowClass, L"CHESS_WINDOW");
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    HWND hWnd{};
    if (!InitInstance(hInstance, nCmdShow, hWnd))
    {
        return FALSE;
    }

    bool opt = true;
    ::setsockopt(s_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    u_long nonblocking_mode = 1;
    ioctlsocket(s_socket, FIONBIO, &nonblocking_mode);

    ChessPawn* chess_pawn = new ChessPawn();

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

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

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

        for (int i = 0; i < 10; ++i) {
            if (chess_pawn_vec[i])
                if(i < 5)
                    chess_pawn_vec[i]->DrawChessBoard(memDC,
                        50 + (i * 250), 50, 200, chess_pawn_vec[i]->Get_Pos_X(), chess_pawn_vec[i]->Get_Pos_Y());
                else
                    chess_pawn_vec[i]->DrawChessBoard(memDC,
                        50 + (i * 250), 350, 200, chess_pawn_vec[i]->Get_Pos_X(), chess_pawn_vec[i]->Get_Pos_Y());
        }

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
        for (auto& pawn : chess_pawn_vec)
            if (pawn)
                delete pawn;

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void Network_Loop(SOCKET s_socket, Direction send_data)
{
    int result = 0;

    // send
    WSABUF send_wsa_buf = {};
    send_wsa_buf.buf = reinterpret_cast<char*>(&send_data);
    send_wsa_buf.len = sizeof(send_data);

    DWORD sent_size = 0;
    result = ::WSASend(s_socket, &send_wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
    //MessageBoxW(NULL, L"Error", L"WSASend", MB_OK | MB_ICONWARNING);
    // recv
    int recv_data[3] = {};
    WSABUF recv_wsa_buf = {};
    recv_wsa_buf.buf = reinterpret_cast<char*>(recv_data);
    recv_wsa_buf.len = sizeof(recv_data);

    DWORD recv_size = 0;
    DWORD recv_flag = 0;
    result = ::WSARecv(s_socket, &recv_wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);
        
    if (0 == result && sizeof(recv_data) == recv_size) {
        if (chess_pawn_vec[recv_data[0]])
            chess_pawn_vec[recv_data[0]]->Set_Pos(recv_data[2], recv_data[1]);
        else {
            chess_pawn_vec[recv_data[0]] = new ChessPawn();
            chess_pawn_vec[recv_data[0]]->Set_Pos(recv_data[2], recv_data[1]);
        }
    }
}
