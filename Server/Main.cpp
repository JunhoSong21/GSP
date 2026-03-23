#include "ChessPawn.h"
#include "NetworkHeader.h"

constexpr short SERVER_PORT = 3000;

int main()
{
	std::wcout.imbue(std::locale("korean")); // 오류메시지 한글
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::cout << "Failed to WSAStartup : " << result << std::endl;
		return 1;
	}

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	result = ::bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (SOCKET_ERROR == result) {
		error_display(L"bind Failed", ::WSAGetLastError());
		return 1;
	}

	result = ::listen(s_socket, SOMAXCONN);
	if (SOCKET_ERROR == result) {
		error_display(L"listen Failed", ::WSAGetLastError());
		return 1;
	}
	INT addr_len = sizeof(server_addr);

	SOCKET c_socket = ::WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_len, nullptr, 0);

	ChessPawn chessPawn;

	while (true) {
		int send_data[2] = { chessPawn.posX, chessPawn.posY };
		WSABUF send_wsa_buf = {};
		send_wsa_buf.buf = reinterpret_cast<char*>(send_data);
		send_wsa_buf.len = sizeof(send_data);

		DWORD sent_size = 0;
		result = ::WSASend(c_socket, &send_wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
		if (SOCKET_ERROR == result) {
			error_display(L"Data Send Failed", ::WSAGetLastError());
			return 1;
		}

		std::cout << "Data Send : " << send_data[0] << ", " << send_data[1] << '\n';

		//

		int recv_data = 0;
		WSABUF recv_wsa_buf = {};
		recv_wsa_buf.buf = reinterpret_cast<char*>(&recv_data);
		recv_wsa_buf.len = sizeof(recv_data);

		DWORD recv_size = 0;
		DWORD recv_flag = 0;
		result = ::WSARecv(c_socket, &recv_wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);
		if (SOCKET_ERROR == result) {
			error_display(L"Data Recv Failed", ::WSAGetLastError());
			return 1;
		}

		chessPawn.InputProcess(recv_data);

		std::cout << "Data Recv : " << recv_data << '\n';
	}

	closesocket(s_socket);
	closesocket(c_socket);
	WSACleanup();
}