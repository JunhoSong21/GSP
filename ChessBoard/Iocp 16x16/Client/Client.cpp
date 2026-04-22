#include "framework.h"
#include "NetworkHeader.h"

#include "ChessPawn.h"

#define MAX_LOADSTRING 100

#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, HWND&);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

ChessPawn avatar;
std::unordered_map<uint64_t, ChessPawn> players;
ULONG_PTR gdiplusToken;

constexpr int TILE_SIZE = 80;

static char packet_buffer[BUF_SIZE];
static int remain_data = 0;

void Network_Loop_Send(SOCKET s_socket, DIRECTION send_data);
void Network_Loop_Recv(SOCKET s_socket);

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

    bool opt = true;
    ::setsockopt(s_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    u_long nonblocking_mode = 1;
    ioctlsocket(s_socket, FIONBIO, &nonblocking_mode);

    MSG msg;

    // 애플리케이션 초기화를 수행합니다:
    HWND hWnd{};
    if (!InitInstance(hInstance, nCmdShow, hWnd))
    {
        return FALSE;
    }

    DIRECTION direction = DIRECTION::NONE;
    Network_Loop_Recv(s_socket);
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    while (true) {
        if (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (WM_QUIT == msg.message)
                return static_cast<int>(msg.wParam);

            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        direction = avatar.MoniteringKey();
        if (direction != DIRECTION::NONE) {
            Network_Loop_Send(s_socket, direction);
            direction = DIRECTION::NONE;
        }
        Network_Loop_Recv(s_socket);
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
    wcex.hIcon          = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, IDI_APPLICATION);

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

            HBRUSH black_brush = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
            HBRUSH white_brush = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));

            int left = (rect.right / 2) - (TILE_SIZE / 2);
            int up = (rect.bottom / 2) - (TILE_SIZE / 2);

            int startX = left - (avatar.Get_PosX() * TILE_SIZE);
            int startY = up - (avatar.Get_PosY() * TILE_SIZE);

            int endX = left + ((WORLD_WIDTH - avatar.Get_PosX() + 1) * TILE_SIZE);
            int endY = up + ((WORLD_HEIGHT - avatar.Get_PosY() + 1) * TILE_SIZE);

            for (int i = startX; i != endX; i += TILE_SIZE) {
                for (int j = startY; j != endY; j += TILE_SIZE) {
                    RECT cell = { i, j, i + TILE_SIZE, j + TILE_SIZE };

                    if (i % 160 == 0)
                        ::FillRect(hdc, &cell, black_brush);
                }
            }

            /*for (auto& player : players) {

            }*/



            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void Send_Login(SOCKET s_socket)
{
    // 유저 닉네임 전송
    C2S_Login packet{};
    packet.size = sizeof(C2S_Login);
    packet.type = C2S_LOGIN;

    std::cout << "Enter Username : ";
    std::cin >> packet.user_name;
    ::FreeConsole();

    WSABUF wsa_buf{};
    wsa_buf.buf = reinterpret_cast<char*>(&packet);
    wsa_buf.len = sizeof(packet);

    DWORD sent_size = 0;
    
    int result = ::WSASend(s_socket, &wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
    if (result == SOCKET_ERROR) {
        int err = ::WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            MessageBoxA(NULL, "Send Login Error", "Login Fail", MB_OK | MB_ICONWARNING);
            return;
        }
    }
}

void Network_Loop_Send(SOCKET s_socket, DIRECTION send_data)
{
    C2S_Move packet{};
    packet.size = sizeof(C2S_Move);
    packet.type = C2S_MOVE;

    packet.direction = send_data;

    WSABUF send_wsa_buf{};
    send_wsa_buf.buf = reinterpret_cast<char*>(&packet);
    send_wsa_buf.len = sizeof(packet);

    DWORD send_size = 0;
    if (SOCKET_ERROR == ::WSASend(s_socket, &send_wsa_buf, 1, &send_size, 0, nullptr, nullptr)) {
        if (WSAEWOULDBLOCK != ::WSAGetLastError())
            return;
    }
}

void Network_Loop_Recv(SOCKET s_socket)
{
    char recv_buf[BUF_SIZE]{};
    DWORD recv_size = 0;
    DWORD recv_flag = 0;
    WSABUF wsa_buf = { sizeof(recv_buf), recv_buf };

    int result = ::WSARecv(s_socket, &wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);
    if (result == SOCKET_ERROR) {
        int err = ::WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            //Disconnect
            return;
        }
    }
    if (recv_size == 0)
        return;

    ::memcpy(packet_buffer + remain_data, recv_buf, recv_size);
    int data_size = recv_size + remain_data;

    unsigned char* p = reinterpret_cast<unsigned char*>(packet_buffer);
    while (data_size > 0) {
        int packet_size = p[0];

        if (packet_size > data_size)
            break;

        unsigned char packet_type = p[1];

        switch (packet_type) {
        case S2C_MOVE_PLAYER: {
            if (packet_size < sizeof(S2C_Move_Player))
                break;

            S2C_Move_Player* packet = reinterpret_cast<S2C_Move_Player*>(p);

            if (packet->player_id == avatar.id)
                avatar.Set_Pos(packet->x, packet->y);

            // 다른 아이디일 경우 탐색해서 수정


            break;
        }
        case S2C_LOGIN_RESULT: {
            if (packet_size < sizeof(S2C_Login_Result))
                break;

            S2C_Login_Result* packet = reinterpret_cast<S2C_Login_Result*>(p);
            if (false == packet->success) {
                MessageBoxW(NULL, L"Error", L"S2C_LOGIN_RESULT", MB_OK | MB_ICONWARNING);
                return;
            }

            Send_Login(s_socket);

            break;
        }
        case S2C_AVATAR_INFO: {
            if (packet_size < sizeof(S2C_Avatar_Info))
                break;

            S2C_Avatar_Info* packet = reinterpret_cast<S2C_Avatar_Info*>(p);
            avatar.id = packet->player_id;
            avatar.Set_Pos(packet->x, packet->y);

            break;
        }
        }
    }
}
