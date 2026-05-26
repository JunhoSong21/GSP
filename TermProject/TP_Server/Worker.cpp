#include "headers.h"
#include "Worker.h"

#include "ErrorDisplay.h"
#include "Session.h"

HANDLE g_h_iocp = INVALID_HANDLE_VALUE;
tbb::concurrent_unordered_map<uint64_t, std::shared_ptr<Session>> clients;

std::atomic_uint64_t g_next_player_id = 1;

bool PostAccept(SOCKET listen_socket);
void Disconnect(Session* client);
bool ProcessPacket(Session* client, const char* packet);
void HandleAccept(OVEREX* over_ex, bool success);
void HandleRecv(Session* client, OVEREX* over_ex, DWORD transferred, bool success);
void HandleSend(Session* client, OVEREX* over_ex, bool success);

void WorkerThread()
{
	while (true) {
		DWORD transferred = 0;
		ULONG_PTR completion_key = 0;
		LPOVERLAPPED overlapped = nullptr;

		BOOL result = ::GetQueuedCompletionStatus(g_h_iocp,
			&transferred, &completion_key, &overlapped, INFINITE);

		if (nullptr == overlapped)
			break;

		OVEREX* over_ex = reinterpret_cast<OVEREX*>(overlapped);
		Session* client = reinterpret_cast<Session*>(completion_key);

		bool success;
		if(TRUE == result)
			success = true;
		else {
			int err = ::GetLastError();
			if (ERROR_NETNAME_DELETED == err || ERROR_CONNECTION_ABORTED == err)
				success = false;
			else
				std::cout << "GetQueuedCompletionStatus failed with error: " << err << std::endl;
		}

		switch (over_ex->m_iotype) {
		case IO_TYPE::IO_ACCEPT:
			HandleAccept(over_ex, success);
			break;
		case IO_TYPE::IO_RECV:
			HandleRecv(client, over_ex, transferred, success);
			break;
		case IO_TYPE::IO_SEND:
			HandleSend(client, over_ex, success);
			break;
		default:
			std::cout << "Unknown IO_TYPE" << std::endl;
			break;
		}
	}
}

bool PostAccept(SOCKET listen_socket)
{
	Accept_OVEREX* accept_over = new Accept_OVEREX();
	accept_over->s_socket = listen_socket;
	accept_over->m_c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == accept_over->m_c_socket) {
		delete accept_over;
		error_display("client Socket");
		return false;
	}

#pragma warning(suppress : 6387)
	if (FALSE == ::AcceptEx(accept_over->s_socket, accept_over->m_c_socket, accept_over->m_buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over->m_over)) {

		int err = ::WSAGetLastError();
		if (WSA_IO_PENDING != err) {
			std::cout << "AcceptEx Fail : " << err << std::endl;
			::closesocket(accept_over->m_c_socket);
			delete accept_over;
			return false;
		}
	}

	return true;
}

void Disconnect(Session* client)
{
	if (nullptr == client)
		return;

	if (true == client->_connected.exchange(false))
		::closesocket(client->_c_socket);
}

bool ProcessPacket(Session* client, const char* packet)
{
	PACKET_TYPE packet_type = *reinterpret_cast<const PACKET_TYPE*>(packet + sizeof(unsigned char));

	switch (packet_type) {
	case C2S_LOGIN:
		break;
	case C2S_MOVE:
		break;
	case C2S_CHAT:
		break;
	case C2S_ATTACK:
		break;
	case C2S_TELEPORT:
		break;
	case C2S_LOGOUT:
		Disconnect(client);
		return false;
	default:
		std::cout << "Unknown packet type : " << packet_type << std::endl;
		Disconnect(client);
		return false;
	}

	return true;
}

void HandleAccept(OVEREX* over_ex, bool success)
{
	Accept_OVEREX* accept_over = static_cast<Accept_OVEREX*>(over_ex);
	SOCKET listen_socket = accept_over->s_socket;
	SOCKET c_socket = accept_over->m_c_socket;

	if (false == success) {
		::closesocket(c_socket);
		PostAccept(listen_socket);
		delete accept_over;
		return;
	}

	if (SOCKET_ERROR == ::setsockopt(c_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		reinterpret_cast<const char*>(&listen_socket), sizeof(listen_socket))) {
		error_display("SO_UPDATE_ACCEPT_CONTEXT");
		::closesocket(c_socket);
		PostAccept(listen_socket);
		delete accept_over;
		return;
	}

	std::shared_ptr<Session> client = std::make_shared<Session>();
	client->_c_socket = c_socket;
	client->_connected = true;
	client->_player_id = g_next_player_id.fetch_add(1);

	if (NULL == ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp,
		reinterpret_cast<ULONG_PTR>(client.get()), 0)) {
		error_display("CreateIoCompletionPort client");
		::closesocket(c_socket);
		PostAccept(listen_socket);
		delete accept_over;
		return;
	}

	clients.emplace(client->_player_id, client);

	client->Recv_Process();
	PostAccept(listen_socket);
	delete accept_over;
}

void HandleRecv(Session* client, OVEREX* over_ex, DWORD transferred, bool success)
{
	if (nullptr == client || false == client->_connected.load())
		return;

	if (false == success || 0 == transferred) {
		Disconnect(client);
		return;
	}

	int remain_size = static_cast<int>(transferred) + client->_prev_recv;
	char* packet_start = over_ex->m_buf;

	while (remain_size > 0) {
		unsigned char packet_size = static_cast<unsigned char>(packet_start[0]);
		constexpr int packet_header_size = sizeof(unsigned char) + sizeof(PACKET_TYPE);
		if (packet_size < packet_header_size) {
			Disconnect(client);
			return;
		}

		if (packet_size > remain_size)
			break;

		if (false == ProcessPacket(client, packet_start))
			return;

		packet_start += packet_size;
		remain_size -= packet_size;
	}

	if (remain_size > 0)
		::memmove(over_ex->m_buf, packet_start, remain_size);

	client->_prev_recv = remain_size;
	client->Recv_Process();
}

void HandleSend(Session* client, OVEREX* over_ex, bool success)
{
	if (false == success)
		Disconnect(client);

	delete over_ex;
}
