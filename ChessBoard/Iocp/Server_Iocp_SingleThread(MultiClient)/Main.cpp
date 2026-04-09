#pragma once
#include "NetworkHeader.h"
#include "Protocol.h"
#include "Session.h"

std::array<Session, MAX_PLAYER> clients;

int main()
{
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::cout << "Failed to WSAStartup : " << result << std::endl;
		return 1;
	}

	SOCKET s_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == s_socket) {
		error_display(L"listen Socket", ::WSAGetLastError());
		return -1;
	}
	sockaddr_in server_addr{}; // 기본 초기화
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

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

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), h_iocp, -1, 0);

	Over_Exp* accept_over = new Over_Exp(IO_TYPE::IO_ACCEPT);
	SOCKET c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == c_socket) {
		error_display(L"client Socket", ::WSAGetLastError());
		return -1;
	}

	::AcceptEx(s_socket, c_socket, accept_over->m_buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over->m_over);

	int player_index = 0;
	while (true) {
		DWORD num_bytes = 0;
		ULONG_PTR key = 0;
		LPOVERLAPPED over = nullptr;
		GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		if (nullptr == over) {
			error_display(L"GQCS Error", ::WSAGetLastError());
			continue;
		}

		Over_Exp* over_exp = reinterpret_cast<Over_Exp*>(over);

		switch (over_exp->m_iotype) {
		case IO_ACCEPT:
		{
			// 비어있는 인덱스 탐색
			player_index = -1;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (false == clients[i]._is_connected) {
					player_index = i;
					break;
				}
			}
			// 비어있는 인덱스가 없으면 접속 거부
			if (-1 == player_index) {
				std::cout << "Max_Connect" << std::endl;
				Send_Login_Fail(c_socket);
				closesocket(c_socket);
			}
			else { // 접속 성공
				std::cout << "Client [" << player_index << "] Accept" << std::endl;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), h_iocp, player_index, 0);
				Set_New_Client(clients[player_index], c_socket, player_index);
			}

			// Accept 재등록
			c_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			::AcceptEx(s_socket, c_socket, accept_over->m_buf, 0,
				sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
				NULL, &accept_over->m_over);

			break;
		}
		case IO_RECV:
		{
			int player_index = static_cast<int>(key);
			Session& client = clients[player_index];

			if (0 == num_bytes) {
				client._is_connected = false;
				closesocket(client._c_socket);

				for (auto& cl : clients) { // 접속 종료 패킷 브로드캐스트
					if (cl._is_connected)
						cl.Send_Remove_Player(client._id);
				}

				break;
			}

			std::cout << "Client [" << player_index << "] Move" << std::endl;

			uint8_t* ptr = reinterpret_cast<uint8_t*>(over_exp->m_buf);
			size_t data_size = num_bytes + client._prev_recv;
			while (data_size >= sizeof(uint8_t)) {
				size_t packet_size = ptr[0];

				if (packet_size > data_size) { // 패킷보다 덜 도착하여 버퍼 위치만 저장
					client._prev_recv = data_size;
					break;
				}

				client.Process_Packet(ptr);
				ptr += packet_size;
				data_size -= packet_size;
			}

			if (data_size > 0) {
				::memmove(client._recv_over.m_buf, ptr, data_size);
				client._prev_recv = data_size;
			}
			else
				client._prev_recv = 0;

			client.Do_Recv();

			break;
		}
		case IO_SEND:
		{
			Over_Exp* over_exp = reinterpret_cast<Over_Exp*>(over);
			delete over_exp;

			break;
		}
		default:
			std::cout << "Unknown IO Type" << std::endl;
			break;
		}
	}

	closesocket(s_socket);
	WSACleanup();
}