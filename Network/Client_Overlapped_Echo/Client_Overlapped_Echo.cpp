#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char* SERVER_IP = "127.0.0.1";
constexpr short SERVER_PORT = 3000;
constexpr int	BUFFER_SIZE = 4096;

SOCKET g_s_socket;
char g_recv_buf[BUFFER_SIZE];
char g_send_buf[BUFFER_SIZE];
WSABUF g_recv_wsa_buf{ BUFFER_SIZE, g_recv_buf };
WSABUF g_send_wsa_buf{ BUFFER_SIZE, g_send_buf };
WSAOVERLAPPED send_overlapped{};
WSAOVERLAPPED recv_overlapped{};

void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg << L" : " << lpMsgBuf << std::endl;
	while (true);
	// 디버깅 용
	::LocalFree(lpMsgBuf);
}

void CALLBACK send_callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED overlapped, DWORD flags);

void Send_to_Server()
{
	char buffer[BUFFER_SIZE];
	std::cout << "Enter Message to send : ";
	std::cin.getline(buffer, BUFFER_SIZE);

	g_send_wsa_buf.len = static_cast<ULONG>(std::strlen(buffer)) + 1;
	::ZeroMemory(&send_overlapped, sizeof(send_overlapped));
	DWORD sent_size = 0;
	int result = ::WSASend(g_s_socket, &g_send_wsa_buf, 1, &sent_size, 0, &send_overlapped, send_callback);
	if (SOCKET_ERROR == result) {
		int err = ::WSAGetLastError();
		if (err != WSA_IO_PENDING) {
			error_display(L"데이터 수신 실패", ::WSAGetLastError());
			exit(1);
		}
	}
}

void CALLBACK recv_callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	if (error != 0) {
		error_display(L"데이터 수신 실패", WSAGetLastError());
		exit(1);
	}

	Send_to_Server();
	std::cout << "Received from Server : " << g_recv_buf << ", SIZE : " << bytes_transferred << std::endl;
}

void CALLBACK send_callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	if (error != 0) {
		error_display(L"데이터 전송 실패", WSAGetLastError());
		exit(1);
	}
	std::cout << "Sent to Server" << g_send_buf << ", SIZE : " << bytes_transferred << std::endl;

	DWORD recv_flag = 0;
	int result = ::WSARecv(g_s_socket, &g_recv_wsa_buf, 1, nullptr, &recv_flag, &recv_overlapped, recv_callback);
	if (SOCKET_ERROR == result) {
		int err = ::WSAGetLastError();
		if (err != WSA_IO_PENDING) {
			error_display(L"데이터 전송 실패", ::WSAGetLastError());
			exit(1);
		}
	}
}

int main()
{
	std::wcout.imbue(std::locale("korean")); // 오류메시지 한글
	WSAData wsa_data{};
	::WSAStartup(MAKEWORD(2, 2), &wsa_data);

	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(SERVER_PORT);
	::inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

	int result = ::WSAConnect(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), nullptr, nullptr, nullptr, nullptr);
	if (SOCKET_ERROR == result) {
		error_display(L"서버 연결 실패", ::WSAGetLastError());
		return 1;
	}

	Send_to_Server();
	for (;;) {
		SleepEx(0, TRUE);
	}

	WSACleanup();
}