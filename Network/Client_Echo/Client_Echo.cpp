#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char*		SERVER_IP = "127.0.0.1";
constexpr short SERVER_PORT = 3000;
constexpr int	BUFFER_SIZE = 4096;

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

int main()
{
	std::wcout.imbue(std::locale("korean")); // 오류메시지 한글
	WSAData wsa_data{};
	::WSAStartup(MAKEWORD(2, 2), &wsa_data);

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(SERVER_PORT);
	::inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

	int result = ::WSAConnect(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), nullptr, nullptr, nullptr, nullptr);
	if (SOCKET_ERROR == result) {
		error_display(L"서버 연결 실패", ::WSAGetLastError());
		return 1;
	}

	for (;;) {
		std::string input;
		char buffer[BUFFER_SIZE];
		std::cout << "Enter Message to send : ";
		std::cin.getline(buffer, BUFFER_SIZE);

		WSABUF wsa_buf{ static_cast<ULONG>(std::strlen(buffer)) + 1, buffer };
		DWORD sent_size = 0;
		result = ::WSASend(s_socket, &wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
		if (SOCKET_ERROR == result) {
			error_display(L"데이터 전송 실패", ::WSAGetLastError());
			return 1;
		}

		//

		char recv_buf[BUFFER_SIZE]{};
		WSABUF recv_wsa_buf{ BUFFER_SIZE, recv_buf };
		DWORD recv_size = 0;
		DWORD recv_flag = 0;
		::WSARecv(s_socket, &recv_wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);
	
		std::cout << "Recv from Server : " << recv_buf;
		std::cout << ", Recv Size : " << recv_size << std::endl;
	}

	WSACleanup();
}