#pragma once
#include "NetworkHeader.h"
#include "ChessPawn.h"

int main()
{
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::cout << "Failed to WSAStartup : " << result << std::endl;
		return 1;
	}

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	if (SOCKET_ERROR == s_socket) {
		error_display(L"s_socket", ::WSAGetLastError());
		return -1;
	}

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
}