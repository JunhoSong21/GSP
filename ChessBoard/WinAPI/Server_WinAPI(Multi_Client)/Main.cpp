#pragma once
#include "NetworkHeader.h"
#include "ChessPawn.h"
#include "Session.h"

std::vector<Session*> clients(10, nullptr);

void CALLBACK Recv_Callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK Send_Callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED over, DWORD flags);

int main()
{
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::cout << "Failed to WSAStartup : " << result << std::endl;
		return 1;
	}

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (SOCKET_ERROR == s_socket) {
		error_display(L"s_socket", ::WSAGetLastError());
		return -1;
	}

	bool opt = true;
	::setsockopt(s_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));

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

	u_long mode = 1;
	ioctlsocket(s_socket, FIONBIO, &mode);

	for (int i = 0; ;) {
		//std::cout << "Accept Waiting..." << std::endl;
		sockaddr_in c_addr{};
		int addr_len = sizeof(c_addr);
		SOCKET c_socket = ::WSAAccept(s_socket,
			reinterpret_cast<sockaddr*>(&c_addr), &addr_len, NULL, NULL);
		if (INVALID_SOCKET == c_socket) {
			int err = ::WSAGetLastError();
			if (WSAEWOULDBLOCK == err) {
				SleepEx(0, TRUE);
				continue;
			}
			else {
				error_display(L"WSAAccept Failed", ::WSAGetLastError());
				return -1;
			}
		}

		Session* session = new Session(c_socket, i);
		clients[i] = session;

		std::cout << "Client Connected. ID : " << i << std::endl;

		clients[i]->Do_Recv();

		++i;
	}

	closesocket(s_socket);
	WSACleanup();
}

void CALLBACK Recv_Callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED over, DWORD flags)
{
	Session* session = reinterpret_cast<Session*>(over);
	int client_id = session->_id;

	if (bytes_transferred != 0) {
		if (nullptr != clients[client_id]) {
			Direction direction = *reinterpret_cast<Direction*>(&session->_buf[0]);

			switch (direction) {
			case Direction::UP:
				clients[client_id]->chess_pawn.MoveUp();
				break;
			case Direction::DOWN:
				clients[client_id]->chess_pawn.MoveDown();
				break;
			case Direction::LEFT:
				clients[client_id]->chess_pawn.MoveLeft();
				break;
			case Direction::RIGHT:
				clients[client_id]->chess_pawn.MoveRight();
				break;
			default:
				break;
			}
		}

		int x = clients[client_id]->chess_pawn.posX;
		int y = clients[client_id]->chess_pawn.posY;

		std::cout << "Client " << client_id << " " << x << y << std::endl;

		for (auto& cl : clients)
			if (cl)
				cl->Do_Send(client_id, bytes_transferred, x, y);

		clients[client_id]->Do_Recv();
	}
	else if (bytes_transferred == 0) {
		std::cout << "Client Disconnected. ID : " << client_id << std::endl;
		clients[client_id] = nullptr;
		delete session;
		return;
	}
}

void CALLBACK Send_Callback(DWORD error, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	delete reinterpret_cast<Over_Exp*>(over);
}
