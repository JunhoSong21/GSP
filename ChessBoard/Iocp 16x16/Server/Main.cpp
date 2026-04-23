#pragma once
#include "Protocol.h"
#include "NetworkHeader.h"
#include "Session.h"

std::atomic<uint64_t> player_index = 1;
tbb::concurrent_unordered_map<uint64_t, std::atomic<std::shared_ptr<Session>>> clients;
std::vector<std::thread> worker_threads;

SOCKET s_socket;
HANDLE h_iocp;

void worker_thread();
void Disconnect(uint64_t key);

int main()
{
	WSAData wsa_data{};
	if (int result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
		result != 0) {
		std::cerr << "Fail WSAStartUp : " << result << std::endl;
		return -1;
	}

	s_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == s_socket) {
		error_display("listen Socket");
		return -1;
	}

	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (SOCKET_ERROR == ::bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr))) {
		error_display("bind Failed");
		return -1;
	}

	if (SOCKET_ERROR == ::listen(s_socket, SOMAXCONN)) {
		error_display("listen Failed");
		return -1;
	}

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), h_iocp, -1, 0);

	Accept_OVEREX* accept_over = new Accept_OVEREX();
	accept_over->m_c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == accept_over->m_c_socket) {
		error_display("client Socket");
		return -1;
	}

	::AcceptEx(s_socket, accept_over->m_c_socket, accept_over->m_buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over->m_over);

	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread);

	for (auto& thread : worker_threads)
		thread.join();

	closesocket(s_socket);
	WSACleanup();
}

void worker_thread()
{
	while (true) {
		DWORD num_bytes = 0;
		ULONG_PTR key = 0;
		LPOVERLAPPED over_ptr = nullptr;

		if (FALSE == GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over_ptr, INFINITE)) {
			if (over_ptr) {
				uint64_t index = static_cast<uint64_t>(key);

				std::cout << "Client [" << index << "] Disconnect" << std::endl;
				Disconnect(key);
				continue;
			}
			else {
				error_display("GQCS");
				continue;
			}
		}

		OVEREX* over = reinterpret_cast<OVEREX*>(over_ptr);
		switch (over->m_iotype) {
		case IO_TYPE::IO_ACCEPT:
		{
			Accept_OVEREX* accept_over = reinterpret_cast<Accept_OVEREX*>(over);
			SOCKET c_socket = accept_over->m_c_socket;

			uint64_t client_id = player_index;

			std::cout << "Client [" << client_id << "] Accept" << std::endl;
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), h_iocp, player_index, 0);

			if (MAX_PLAYER <= clients.size()) {
				std::cout << "No more player can be accepted" << std::endl;
				//Send_Login_Fail(c_socket, "Login Fail");
				closesocket(accept_over->m_c_socket);
			}
			else {
				uint64_t id = player_index;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(accept_over->m_c_socket), h_iocp, id, 0);
				std::shared_ptr<Session> new_player = std::make_shared<Session>(accept_over->m_c_socket, id);
				clients[id] = new_player;
				new_player->Send_Login_Success();
				new_player->Recv_Process();

				player_index.fetch_add(1);
			}

			accept_over->m_c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == accept_over->m_c_socket) {
				error_display("client Socket");
				continue;
			}

			::ZeroMemory(&accept_over->m_over, sizeof(accept_over->m_over));

			::AcceptEx(s_socket, accept_over->m_c_socket, accept_over->m_buf, 0,
				sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
				NULL, &accept_over->m_over);

			break;
		}
		case IO_TYPE::IO_RECV:
		{
			uint64_t index = static_cast<uint64_t>(key);

			if (0 == num_bytes) {
				Disconnect(key);
				break;
			}

			std::shared_ptr<Session> client = clients[key];
			if (nullptr == client) {
				std::cout << "Session not found Client [" << index << "]" << std::endl;
				break;
			}

			unsigned char* p = reinterpret_cast<unsigned char*>(over->m_buf);
			int data_size = num_bytes + client->_prev_recv;
			while (data_size > 0) {
				if (data_size < sizeof(uint16_t))
					break;

				uint16_t packet_size = *reinterpret_cast<uint16_t*>(p);
				if (packet_size > data_size)
					break;

				if (false == client->Process_Packet(p)) {
					Disconnect(key);
					break;
				}

				p += packet_size;
				data_size -= packet_size;
			}
			if (data_size > 0)
				::memmove(client->_recv_over.m_buf, p, data_size);

			client->_prev_recv = data_size;
			client->Recv_Process();

			break;
		}
		case IO_TYPE::IO_SEND:
		{
			OVEREX* send_over = reinterpret_cast<OVEREX*>(over);
			delete send_over;

			break;
		}
		default:
			std::cout << "Unknown IO type" << std::endl;
			exit(-1);
			break;
		}
	}
}

void Send_Login_Fail(SOCKET c_socket, const char* msg)
{
	S2C_Login_Result packet{};
	packet.size = sizeof(S2C_Login_Result);
	packet.type = S2C_LOGIN_RESULT;

	packet.success = false;
	strncpy_s(packet.msg, msg, sizeof(packet.msg));

	WSABUF wsa_buf{};
	wsa_buf.buf = reinterpret_cast<char*>(&packet);
	wsa_buf.len = packet.size;

	::WSASend(c_socket, &wsa_buf, 1, 0, 0, nullptr, nullptr);
}

void Disconnect(uint64_t key)
{
	std::cout << "Client [" << key << "] Disconnect" << std::endl;

	std::shared_ptr<Session> client = clients[key].load();
	if (client) {
		client->_state = CLIENT_STATE::LOGOUT;
		closesocket(client->_c_socket);
		client->_c_socket = INVALID_SOCKET;
	}

	clients[key].store(nullptr);
}
