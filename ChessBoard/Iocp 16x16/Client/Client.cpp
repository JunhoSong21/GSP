#include "framework.h"
#include "NetworkHeader.h"

#include "ChessPawn.h"

#define MAX_LOADSTRING 100

#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")

HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, HWND&);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

ChessPawn avatar;
std::unordered_map<uint64_t, ChessPawn> players;
ULONG_PTR gdiplusToken;
SOCKET s_socket;

constexpr int TILE_SIZE = 50;

char recv_buf[BUF_SIZE];
int prev_size = 0;

void Render(HDC hdc);
void Send_Login();
void Send_Move(DIRECTION direction);
void Send_Process(void* packet, uint16_t size);
void Network_Loop_Recv();
void Process_Packet(char* p);

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

    s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
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

    bool opt = true;
    ::setsockopt(s_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
    u_long nonblocking_mode = 1;
    ioctlsocket(s_socket, FIONBIO, &nonblocking_mode);

    // 전역 문자열을 초기화합니다.
    ::wcscpy_s(szTitle, L"Chess Client");
    ::wcscpy_s(szWindowClass, L"CHESS_WINDOW");
    MyRegisterClass(hInstance);

    MSG msg;

    // 애플리케이션 초기화를 수행합니다:
    HWND hWnd{};
    if (!InitInstance(hInstance, nCmdShow, hWnd))
    {
        return FALSE;
    }

    DIRECTION direction = DIRECTION::NONE;

    while (true) {
        if (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (WM_QUIT == msg.message)
                return static_cast<int>(msg.wParam);

            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        Network_Loop_Recv();
        direction = avatar.MoniteringKey();
        if (direction != DIRECTION::NONE) {
            Send_Move(direction);
            direction = DIRECTION::NONE;
        }
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
      CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800, nullptr, nullptr, hInstance, nullptr);

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

        Render(memDC);
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

void Send_Login()
{
    C2S_Login packet{};
    packet.size = sizeof(C2S_Login);
    packet.type = C2S_LOGIN;

    std::cout << "Enter Username : ";
    std::cin >> avatar.username;
    ::memcpy(packet.user_name, avatar.username, MAX_NAME_LEN);
    ::FreeConsole();

    Send_Process(&packet, packet.size);
}

void Send_Move(DIRECTION direction)
{
    C2S_Move packet{};
    packet.size = sizeof(C2S_Move);
    packet.type = C2S_MOVE;

    packet.direction = direction;

    Send_Process(&packet, packet.size);
}

void Send_Process(void* packet, uint16_t size)
{
    int result = ::send(s_socket, reinterpret_cast<char*>(packet), size, 0);
    if (result == SOCKET_ERROR) {
        int err = ::WSAGetLastError();
        if (WSAEWOULDBLOCK != err) {
            return;
        }
    }
}

void Network_Loop_Recv()
{
    int result = ::recv(s_socket, recv_buf + prev_size, BUF_SIZE - prev_size, 0);
    if (result == SOCKET_ERROR) {
        int err = ::WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            exit(-1);
        }

        return;
    }
    if (result == 0)
        return;

    int total = result + prev_size;
    char* p = recv_buf;

    while (total >= sizeof(char)) {
        int packet_size = *reinterpret_cast<uint16_t*>(p);

        if (packet_size > total)
            break;

        Process_Packet(p);

        p += packet_size;
        total -= packet_size;
    }

    prev_size = total;
    if (prev_size > 0)
        ::memmove(recv_buf, p, prev_size);
}

void Process_Packet(char* p)
{
    unsigned char type = static_cast<unsigned char>(p[2]);

    switch (type) {
    case S2C_MOVE_PLAYER: {
        S2C_Move_Player* packet = reinterpret_cast<S2C_Move_Player*>(p);

        if (packet->player_id == avatar.id)
            avatar.Set_Pos(packet->x, packet->y);
        else {
            auto it = players.find(packet->player_id);
            if (it != players.end())
                it->second.Set_Pos(packet->x, packet->y);
        }

        break;
    }
    case S2C_ADD_PLAYER: {
        S2C_Add_Player* packet = reinterpret_cast<S2C_Add_Player*>(p);

        ChessPawn new_player;
        new_player.id = packet->player_id;
        new_player.Set_Pos(packet->x, packet->y);
        ::memcpy(new_player.username, packet->username, MAX_NAME_LEN);
        
        players[packet->player_id] = new_player;

        break;
    }
    case S2C_REMOVE_PLAYER: {
        S2C_Remove_Player* packet = reinterpret_cast<S2C_Remove_Player*>(p);

        players.erase(packet->player_id);

        break;
    }
    case S2C_LOGIN_RESULT: {
        S2C_Login_Result* packet = reinterpret_cast<S2C_Login_Result*>(p);
        if (false == packet->success) {
            MessageBoxW(NULL, L"Error", L"S2C_LOGIN_RESULT", MB_OK | MB_ICONWARNING);
            return;
        }

        Send_Login();

        break;
    }
    case S2C_AVATAR_INFO: {
        S2C_Avatar_Info* packet = reinterpret_cast<S2C_Avatar_Info*>(p);
        avatar.id = packet->player_id;
        avatar.Set_Pos(packet->x, packet->y);

        break;
    }
    }
}

void Render(HDC hdc)
{
    using namespace Gdiplus;

    HBRUSH black_brush = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    HBRUSH white_brush = static_cast<HBRUSH>(::GetStockObject(LTGRAY_BRUSH));
    
    int start_index_x = 0;
    if (avatar.Get_PosX() < 8)
        start_index_x = 8 - avatar.Get_PosX();
    
    int end_index_x = 16;
    if (avatar.Get_PosX() > 392)
        end_index_x = 409 - avatar.Get_PosX();

    int start_index_y = 0;
    if (avatar.Get_PosY() < 8)
        start_index_y = 7 - avatar.Get_PosY();
    
    int end_index_y = 16;
    if (avatar.Get_PosY() > 392)
        end_index_y = 408 - avatar.Get_PosY();
  
    for (int row = start_index_y; row < end_index_y; ++row) {
        for (int col = start_index_x; col < end_index_x; ++col) {
            int x = col * TILE_SIZE;
            int y = row * TILE_SIZE - 25;

            RECT cell = { x, y, x + TILE_SIZE, y + TILE_SIZE };
            
            if ((avatar.Get_PosX() + avatar.Get_PosY()) % 2 == ((row + col) % 2))
                ::FillRect(hdc, &cell, black_brush);
            else
                ::FillRect(hdc, &cell, white_brush);
        }
    }

    // GDI+
    Graphics graphics(hdc);
    graphics.SetCompositingMode(CompositingModeSourceOver);
    Image* image = Image::FromFile(L"pawn-rb.png");

    if (Ok == image->GetLastStatus()) {
        graphics.DrawImage(image,
            400, 325, 50, 50);
    }

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 0));
    RECT rect = { 400, 310, 450, 325 };
    DrawTextA(hdc, avatar.username, -1, &rect, DT_CENTER | DT_SINGLELINE);

    // 미니맵
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HBRUSH brush = CreateSolidBrush(RGB(50, 50, 200));
    HPEN   oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    
    Rectangle(hdc, 800, 0, 1200, 400);

    // 다른 플레이어들 그리기
    for (auto& client : players) {
        ChessPawn& player = client.second;

        int relative_x = player.Get_PosX() - avatar.Get_PosX();
        int relative_y = player.Get_PosY() - avatar.Get_PosY();

        int screen_x = 400 + relative_x * TILE_SIZE;
        int screen_y = 325 + relative_y * TILE_SIZE;

        if (Ok == image->GetLastStatus())
            graphics.DrawImage(image, screen_x, screen_y, 50, 50); 

        SetTextColor(hdc, RGB(0, 255, 0));
        RECT player_name_rect = { screen_x, screen_y - 15, screen_x + 50, screen_y };
        DrawTextA(hdc, player.username, -1, &player_name_rect, DT_CENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);

    RECT rc = { 800 + avatar.Get_PosX(), avatar.Get_PosY(), 800 + avatar.Get_PosX() + 2, avatar.Get_PosY() + 2};
    HBRUSH white_minimap_brush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &rc, white_minimap_brush);
    DeleteObject(white_minimap_brush);
}
