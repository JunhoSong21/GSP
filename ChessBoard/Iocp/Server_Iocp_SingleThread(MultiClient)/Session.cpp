#include "NetworkHeader.h"
#include "Session.h"
#include "Protocol.h"

extern std::array<Session, MAX_PLAYER> clients;

// class Over_Exp
Over_Exp::Over_Exp()
{
	::ZeroMemory(&m_over, sizeof(m_over));
	m_iotype = IO_RECV;
	::ZeroMemory(&m_buf, sizeof(m_buf));
	m_wsa_buf.buf = m_buf;
	m_wsa_buf.len = BUF_SIZE;
}

Over_Exp::Over_Exp(IO_TYPE iotype) : m_iotype(iotype)
{
	::ZeroMemory(&m_over, sizeof(m_over));
	::ZeroMemory(&m_buf, sizeof(m_buf));
	m_wsa_buf.buf = m_buf;
	m_wsa_buf.len = BUF_SIZE;
}

// class Session
Session::Session()
{
	_c_socket = INVALID_SOCKET;
	_id = 999;
	_recv_over.m_iotype = IO_RECV;
	_is_connected = false;
	_x = 0;
	_y = 0;
	::ZeroMemory(&_username, sizeof(_username));
	_prev_recv = 0;
}

Session::Session(SOCKET socket, int id) : _c_socket(socket), _id(id)
{
	_recv_over.m_iotype = IO_RECV;
	_is_connected = false;
	_x = 0;
	_y = 0;
	::ZeroMemory(&_username, sizeof(_username));
	_prev_recv = 0;
}

Session::~Session()
{
	if (_is_connected)
		closesocket(_c_socket);
}

void Session::Do_Recv()
{
	DWORD recv_flag = 0;
	_recv_over.m_iotype = IO_RECV;
	::ZeroMemory(&_recv_over.m_over, sizeof(_recv_over.m_over));

	if (SOCKET_ERROR == ::WSARecv(_c_socket, &_recv_over.m_wsa_buf, 1, 0, &recv_flag, &_recv_over.m_over, nullptr)) {
		if (WSA_IO_PENDING != ::WSAGetLastError())
			exit(-1);
	}
}

void Session::Do_Send(int num_bytes, char* packet)
{
	Over_Exp* over_exp = new Over_Exp(IO_SEND);
	over_exp->m_wsa_buf.len = num_bytes + 2;

	::memcpy(over_exp->m_buf, packet, num_bytes);

	if (SOCKET_ERROR == ::WSASend(_c_socket, &over_exp->m_wsa_buf, 1, 0, 0, &over_exp->m_over, nullptr)) {
		if (WSA_IO_PENDING != ::WSAGetLastError())
			exit(-1);
	}
}

void Session::Send_Login_Result()
{
	S2C_Login_Result packet;
	packet.packet_size = sizeof(S2C_Login_Result);
	packet.packet_type = S2C_LOGIN_RESULT;

	packet.success = true;

	const char* msg = "Login Success";
	::memcpy(packet.msg, msg, ::strlen(msg));
	packet.msg[::strlen(msg)] = '\0';

	Do_Send(packet.packet_size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Avatar_Info()
{
	S2C_Avatar_Info packet;
	packet.packet_size = sizeof(S2C_Avatar_Info);
	packet.packet_type = S2C_AVATAR_INFO;

	packet.player_id = _id;
	packet.x = _x;
	packet.y = _y;

	Do_Send(packet.packet_size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Add_Player(int player_id, char* add_name ,int add_x, int add_y)
{
	S2C_Add_Player packet;
	packet.packet_size = sizeof(S2C_Add_Player);
	packet.packet_type = S2C_ADD_PLAYER;

	packet.player_id = player_id;
	::memcpy(packet.username, add_name, sizeof(add_name));
	packet.x = add_x;
	packet.y = add_y;

	Do_Send(packet.packet_size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Remove_Player(int player_id)
{
	S2C_Remove_Player packet;
	packet.packet_size = sizeof(S2C_Remove_Player);
	packet.packet_type = S2C_REMOVE_PLAYER;

	packet.player_id = player_id;

	Do_Send(packet.packet_size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Move_Player(int player_id, int move_x, int move_y)
{
	S2C_Move_Player packet;
	packet.packet_size = sizeof(S2C_Move_Player);
	packet.packet_type = S2C_MOVE_PLAYER;

	packet.player_id = player_id;
	packet.x = move_x;
	packet.y = move_y;

	Do_Send(packet.packet_size, reinterpret_cast<char*>(&packet));
}

void Session::Process_Packet(uint8_t* ptr)
{
	PACKET_TYPE packet_type = *reinterpret_cast<PACKET_TYPE*>(&ptr[1]);
	switch (packet_type) {
	case C2S_LOGIN:
	{
		C2S_Login* packet = reinterpret_cast<C2S_Login*>(ptr);
		::strncpy_s(_username, packet->user_name, MAX_NAME_LEN);
		std::cout << "Player [" << _id << "] login as " << _username << std::endl;

		Send_Login_Result();
		Send_Avatar_Info();
		for (auto& cl : clients) {
			if (_id == cl._id) // 본인에게는 전달하지 않는다
				continue;

			if (cl._is_connected)
				Send_Add_Player(_id, _username, _x, _y);
		}

		break;
	}
	case C2S_MOVE:
	{
		C2S_Move* packet = reinterpret_cast<C2S_Move*>(ptr);
		DIRECTION direction = packet->direction;

		switch (direction) {
		case UP:
			if (_y < 7) ++_y; break;
		case DOWN:
			if (_y > 0) --_y; break;
		case LEFT:
			if (_x > 0) --_x; break;
		case RIGHT:
			if (_x < 7) ++_x; break;
		default:
			std::cout << "Unknown Direction" << std::endl;
		}

		for (auto& cl : clients) {
			if (cl._is_connected)
				cl.Send_Move_Player(_id, _x, _y);
		}

		break;
	}
	default:
		std::cout << "Unknown Packet Type Recv" << std::endl;
		break;
	}
}
