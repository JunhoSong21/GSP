#pragma once
#include "headers.h"
#include "ErrorDisplay.h"
#include "Session.h"
#include "Worker.h"

int main()
{
	WSAData wsa_data{};
	if (int result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
		result != 0) {
		std::cerr << "Fail WSAStartUp : " << result << std::endl;
		return -1;
	}

	SOCKET s_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == s_socket) {
		error_display("listen Socket");
		return -1;
	}

	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (SOCKET_ERROR == ::bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr))) {
		error_display("bind Failed");
		return -1;
	}

	if (SOCKET_ERROR == ::listen(s_socket, SOMAXCONN)) {
		error_display("listen Failed");
		return -1;
	}

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), g_h_iocp, -1, 0);

	Accept_OVEREX* accept_over = new Accept_OVEREX();
	accept_over->s_socket = s_socket;
	accept_over->m_c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == accept_over->m_c_socket) {
		error_display("client Socket");
		return -1;
	}

#pragma warning(suppress : 6387)
	if (FALSE == ::AcceptEx(accept_over->s_socket, accept_over->m_c_socket, accept_over->m_buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over->m_over)) {

		int err = ::WSAGetLastError();
		if (WSA_IO_PENDING != err) {
			std::cout << "AcceptEx Fail : " << err << std::endl;
			closesocket(accept_over->m_c_socket);

			delete accept_over;
		}
	}

	std::vector<std::thread> worker_threads;
	uint32_t num_threads = std::thread::hardware_concurrency();

	for (uint32_t i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(WorkerThread);

	for (std::thread& thread : worker_threads)
		thread.join();

	closesocket(s_socket);
	WSACleanup();
}