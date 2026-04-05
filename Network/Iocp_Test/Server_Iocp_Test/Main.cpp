#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#include <MSWSock.h>
#pragma comment(lib, "MSWSock.lib")

#include "Protocol.h"

#include <array>

constexpr int BUF_SIZE = 200;

enum IOType {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT,
};

class Over_Exp {
public:
	WSAOVERLAPPED	over;
	IOType			_iotype;
	WSABUF			wsa_buf;
	char			buf[BUF_SIZE];

public:
	Over_Exp()
	{
		::ZeroMemory(&over, sizeof(over));
		wsa_buf.buf = buf;
		wsa_buf.len = BUF_SIZE;
	}

	Over_Exp(IOType iotype) : _iotype(iotype)
	{
		::ZeroMemory(&over, sizeof(over));
		wsa_buf.buf = buf;
		wsa_buf.len = BUF_SIZE;
	}
};

void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg;
	std::wcout << L" === 에러 " << lpMsgBuf << std::endl;
	while (true);   // 디버깅 용
	LocalFree(lpMsgBuf);
}

class Session {
public:
	SOCKET		_c_socket;
	LONGLONG	_id;

public:
	Over_Exp	recv_over;
	bool		_is_connected;
	short		x, y;
	char		username[MAX_NAME_LEN];
	int			prev_recv;

public:
	Session()
	{
		_id = 999;
		_c_socket = INVALID_SOCKET;
		recv_over._iotype = IO_RECV;
		x = 0;
		y = 0;
		prev_recv = 0;
	}

	Session(SOCKET socket, int id) : _c_socket(socket), _id(id)
	{
	}

	~Session()
	{
		if (_is_connected)
			closesocket(_c_socket);
	}

	void Do_Recv()
	{
		DWORD recv_flag = 0;
		recv_over._iotype = IO_RECV;
		::memset(&recv_over.over, 0, sizeof(recv_over.over));
		::WSARecv(_c_socket, &recv_over.wsa_buf, 1, 0, &recv_flag, &recv_over.over, nullptr);
	}

	void Do_Send(int num_bytes, char* msg)
	{
		Over_Exp* over_exp = new Over_Exp(IO_SEND);
		over_exp->wsa_buf.len = num_bytes + 2;

		::memcpy(over_exp->buf, msg, num_bytes);

		::WSASend(_c_socket, &over_exp->wsa_buf, 1, 0, 0, &over_exp->over, nullptr);
	}

	void Send_Avatar_Info()
	{
		S2C_AvatarInfo packet;

		packet.size = sizeof(S2C_AvatarInfo);
		packet.type = S2C_AVATERINFO;
		packet.playerId = _id;
		packet.x = x;
		packet.y = y;
		Do_Send(packet.size, reinterpret_cast<char*>(&packet));
	}

	void Send_Move_Packet()
	{
		S2C_MovePlayer packet;
		packet.size = sizeof(S2C_MovePlayer);
		packet.type = S2C_MOVEPLAYER;
		packet.playerId = _id;
		packet.x = x;
		packet.y = y;
		Do_Send(packet.size, packet);
	}

	void Send_Login_Success()
	{

	}

	void process_packet(unsigned char* p)
	{
		PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);
		switch (type) {
		case C2S_LOGIN: {
			C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);
			strncpy(username, packet->username, MAX_NAME_LEN);
			std::cout << "Player[" << _id << "] logged in as " << username << std::endl;
			Send_Avatar_Info();

			break;
		}
		case C2S_MOVE: {
			C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);
			DIRECTION dir = packet->direction;
			switch (dir) {
			case UP: break;
			}

			Send_Move_Packet();
		}
		}

	}
};

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

	Over_Exp* accept_over = new Over_Exp(IOType::IO_ACCEPT);
	SOCKET c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == c_socket) {
		error_display(L"client Socket", ::WSAGetLastError());
		return -1;
	}

	::AcceptEx(s_socket, c_socket, accept_over->buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over->over);

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
		switch (over_exp->_iotype) {
		case IO_ACCEPT:
		{
			player_index = -1;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (false == clients[player_index]._is_connected) {
					player_index = i;
					break;
				}
			}
			if (-1 == player_index) {
				std::cout << "Max_Connect" << std::endl;
				Send_Login_Fail(c_socket);
				closesocket(c_socket);
			}
			else {
				std::cout << "Client [" << player_index << "] Accept" << std::endl;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), h_iocp, player_index, 0);
				clients[player_index]._is_connected = true;
				clients[player_index]._c_socket = c_socket;
				clients[player_index].x = 0;
				clients[player_index].y = 0;
				clients[player_index]._id = player_index;
				clients[player_index].Send_Login_Success();
				clients[player_index].prev_recv = 0;
				clients[player_index].Do_Recv();
			}
			
			c_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

			::AcceptEx(s_socket, c_socket, accept_over->buf, 0,
				sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
				NULL, &accept_over->over);

			break;
		}
		case IO_RECV:
		{
			int player_index = static_cast<int>(key);

			std::cout << "Client [" << player_index << "] sent a message" << std::endl;

			Session& client = clients[player_index];

			unsigned char* p = reinterpret_cast<unsigned char*>(over_exp->buf);
			int data_size = num_bytes + client.prev_recv;
			while (data_size > 0) {
				int packet_size = p[0];

				if (packet_size > data_size) {
					client.prev_recv = data_size;
					break;
				}

				client.process_packet(p);
				p += packet_size;
				data_size -= packet_size;
			}
			if (data_size > 0) {
				::memmove(client.recv_over.buf, );
				client.prev_recv = data_size;
			}

			client.Do_Recv();

			break;
		}
		case IO_SEND:
		{
			Over_Exp* send_over = reinterpret_cast<Over_Exp*>(over);
			delete send_over;

			break;
		}
		default:
			std::cout << "Unknown IO type" << std::endl;
			exit(-1);
			break;
		}
	}

	closesocket(s_socket);
	WSACleanup();
}
