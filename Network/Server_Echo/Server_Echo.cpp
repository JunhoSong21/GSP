#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

constexpr short SERVER_PORT = 3000;
constexpr int	BUFFER_SIZE = 4096;

int main()
{
	std::wcout.imbue(std::locale("korean")); // 오류메시지 한글
	WSAData wsa_data{};
	::WSAStartup(MAKEWORD(2, 2), &wsa_data);

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	::bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

	::listen(s_socket, SOMAXCONN);
	INT addr_len = sizeof(server_addr);

	SOCKET c_socket = ::WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_len, nullptr, 0);

	for (;;) {
		char recv_buf[BUFFER_SIZE]{};
		WSABUF recv_wsa_buf{ BUFFER_SIZE, recv_buf };
		DWORD recv_size = 0;
		DWORD recv_flag = 0;
		WSARecv(c_socket, &recv_wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);

		std::cout << "Recv from Client : " << recv_buf;
		std::cout << ", Recv Size : " << recv_size << std::endl;

		//

		DWORD sent_size = 0;
		WSABUF send_wsa_buf = { recv_size, recv_buf };
		WSASend(c_socket, &send_wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
		
		std::cout << recv_size << "Send to Client\n";
	}

	WSACleanup();
}