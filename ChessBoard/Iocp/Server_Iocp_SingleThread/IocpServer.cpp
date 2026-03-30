#pragma once
#include "NetworkHeader.h"
#include "SESSION.h"
#include "EXP_OVER.h"

#include <unordered_map>

std::unordered_map<uint64_t, SESSION> session_map;

int main()
{
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::cout << "Failed to WSAStartup : " << result << std::endl;
		return 1;
	}

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == s_socket) {
		error_display(L"listen Socket", ::WSAGetLastError());
		return -1;
	}
	sockaddr_in server_addr{}; // 기본 초기화
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

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), h_iocp, 0, 0);

	EXP_OVER* accept_over = new EXP_OVER(IOType::IO_ACCEPT);
	SOCKET c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == c_socket) {
		error_display(L"client Socket", ::WSAGetLastError());
		return -1;
	}

	::AcceptEx(s_socket, c_socket, accept_over->_buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over->_over);

	for (int i = 1; ; ++i) {
		std::cout << "Client [" << i << "] Accept" << std::endl;
	}
}