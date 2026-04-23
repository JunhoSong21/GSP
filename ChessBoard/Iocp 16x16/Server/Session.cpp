#include "NetworkHeader.h"
#include "Session.h"

extern tbb::concurrent_unordered_map<uint64_t, std::atomic<std::shared_ptr<Session>>> clients;

// class OVEREX
OVEREX::OVEREX()
{
	::ZeroMemory(&m_over, sizeof(m_over));
	m_iotype = IO_TYPE::IO_SEND;
	m_wsa_buf.buf = m_buf;
	m_wsa_buf.len = BUF_SIZE;
}

OVEREX::OVEREX(IO_TYPE iotype) : m_iotype(iotype)
{
	::ZeroMemory(&m_over, sizeof(m_over));
	m_wsa_buf.buf = m_buf;
	m_wsa_buf.len = BUF_SIZE;
}

// class Accept_OVEREX
Accept_OVEREX::Accept_OVEREX() : OVEREX(IO_TYPE::IO_ACCEPT)
{
	m_c_socket = INVALID_SOCKET;
}

// class Session
Session::Session(SOCKET socket, uint64_t id) : _c_socket(socket), _c_id(id)
{
	_recv_over.m_iotype = IO_TYPE::IO_RECV;
	_state = CLIENT_STATE::CONNECT;
	_posX = rand() % 400;
	_posY = rand() % 400;
	::ZeroMemory(_username, sizeof(_username));
	_prev_recv = 0;
	_move_time = 0;
}

Session::~Session()
{
	if (_state == CLIENT_STATE::LOGOUT)
		closesocket(_c_socket);
}

void Session::Send_Process(int num_bytes, char* msg)
{
	OVEREX* over = new OVEREX(IO_TYPE::IO_SEND);
	over->m_wsa_buf.len = num_bytes;
	::memcpy(over->m_buf, msg, num_bytes);

	::WSASend(_c_socket, &over->m_wsa_buf, 1, 0, 0, &over->m_over, nullptr);
}

void Session::Send_Login_Success()
{
	S2C_Login_Result packet{};
	packet.size = sizeof(S2C_Login_Result);
	packet.type = S2C_LOGIN_RESULT;

	packet.success = true;
	::strncpy_s(packet.msg, "Login Success", sizeof(packet.msg));

	Send_Process(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Avatar_Info()
{
	S2C_Avatar_Info packet{};
	packet.size = sizeof(S2C_Avatar_Info);
	packet.type = S2C_AVATAR_INFO;

	packet.player_id = _c_id;
	packet.x = _posX;
	packet.y = _posY;

	Send_Process(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Add_Player(uint64_t player_id)
{
	S2C_Add_Player packet{};
	packet.size = sizeof(S2C_Add_Player);
	packet.type = S2C_ADD_PLAYER;

	packet.player_id = player_id;
	std::shared_ptr<Session> player = clients[player_id].load();
	if (nullptr == player)
		return;

	::memcpy(packet.username, player->_username, sizeof(packet.username));
	packet.x = player->_posX;
	packet.y = player->_posY;

	// 矫具 府胶飘 包访 贸府

	Send_Process(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Move_Player(uint64_t player_id)
{
	S2C_Move_Player packet{};
	packet.size = sizeof(S2C_Move_Player);
	packet.type = S2C_MOVE_PLAYER;

	packet.player_id = player_id;
	std::shared_ptr<Session> player = clients[player_id];
	if (nullptr == player)
		return;
	packet.x = player->_posX;
	packet.y = player->_posY;
	packet.move_time = player->_move_time;

	Send_Process(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::Send_Remove_Player(uint64_t player_id)
{
	S2C_Remove_Player packet{};
	packet.size = sizeof(S2C_Remove_Player);
	packet.type = S2C_REMOVE_PLAYER;

	packet.player_id = player_id;

	_visible_mutex.lock();
	if (0 == _visible_players.count(player_id)) {
		_visible_mutex.unlock();
		return;
	}
	_visible_players.erase(player_id);
	_visible_mutex.unlock();

	Send_Process(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::Recv_Process()
{
	DWORD recv_flag = 0;
	_recv_over.m_iotype = IO_TYPE::IO_RECV;
	::ZeroMemory(&_recv_over.m_over, sizeof(_recv_over.m_over));

	::WSARecv(_c_socket, &_recv_over.m_wsa_buf, 1, 0, &recv_flag, &_recv_over.m_over, nullptr);
}

bool Session::Process_Packet(unsigned char* p)
{
	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(p + sizeof(uint16_t));

	switch (type) {
	case C2S_LOGIN: {
		C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);

		::strncpy_s(_username, packet->user_name, MAX_NAME_LEN);
		std::cout << "Client [" << _c_id << "] login as " << _username << std::endl;

		Send_Avatar_Info();
		_state = CLIENT_STATE::INGAME;

		for (auto& client : clients) {
			std::shared_ptr<Session> player = client.second.load();

			if (nullptr == player)
				continue;
			if (_c_id == player->_c_id)
				continue;
			if (false == Is_Visible(player->_posX, player->_posY))
				continue;
			if (CLIENT_STATE::INGAME != player->_state)
				continue;

			Send_Add_Player(player->_c_id);
			player->Send_Add_Player(_c_id);
		}

		break;
	}
	case C2S_MOVE: {
		C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);

		DIRECTION direction = packet->direction;
		_move_time = packet->move_time;

		switch (direction) {
		case UP:
			if (_posY > 0)
				--_posY;
			break;
		case DOWN:
			if (_posY < WORLD_HEIGHT)
				++_posY;
			break;
		case LEFT:
			if (_posX > 0)
				--_posX;
			break;
		case RIGHT:
			if (_posX < WORLD_WIDTH)
				++_posX;
			break;
		}
		
		std::unordered_set<uint64_t> new_view;
		for (auto& client : clients) {
			std::shared_ptr<Session> player = client.second.load();
			if (nullptr == player)
				continue;
			if (player->_c_id == _c_id)
				continue;
			if (player->_state != CLIENT_STATE::INGAME)
				continue;
			if (Is_Visible(player->_posX, player->_posY))
				new_view.insert(player->_c_id);
		}

		Send_Move_Player(_c_id);

		std::unordered_set<uint64_t> old_view = _visible_players;
		for (uint64_t id : new_view) {
			if (0 == old_view.count(id)) {
				Send_Add_Player(id);
				std::shared_ptr<Session> player = clients[id].load();
				if (nullptr == player)
					continue;
				player->Send_Add_Player(_c_id);
			}
			else {
				std::shared_ptr<Session> player = clients[id].load();
				if (nullptr == player)
					continue;
				player->Send_Move_Player(_c_id);
			}
		}

		for (uint64_t id : old_view) {
			if (0 == new_view.count(id)) {
				Send_Remove_Player(id);
				std::shared_ptr<Session> player = clients[id].load();
				if (nullptr == player)
					continue;
				player->Send_Remove_Player(_c_id);
			}
		}

		break;
	}

	default:
		std::cout << "Unknown Packet Type from Player[" << _c_id << "]" << std::endl;
		return false;
		break;
	}

	return true;
}

bool Session::Is_Visible(short x, short y)
{
	return abs(_posX - x) <= VIEW_RANGE
		&& abs(_posY - y) <= VIEW_RANGE;
}
