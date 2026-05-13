#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <chrono>
#include <concurrent_priority_queue.h>
#include <atomic>
#include <memory>
#include <tbb/concurrent_unordered_map.h>
#include "protocol_2026.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;
using namespace std::chrono;

constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;
constexpr int VIEW_RANGE = 8;
constexpr int MOVE_COOL_TIME = 1000; // ms

constexpr int EVENT_MOVE = 1;

struct event_type {
	int obj_id;
	system_clock::time_point wakeup_time;
	int event_id;
	int target_id;

	constexpr bool operator < (const event_type& _Left) const
	{
		return (wakeup_time > _Left.wakeup_time);
	}
};

concurrency::concurrent_priority_queue<event_type> timer_queue;

enum COMP_TYPE {
	OP_ACCEPT,
	OP_RECV,
	OP_SEND,
	OP_NPCMOVE
};
 
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int _ai_target_obj;

	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}

	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	int _id;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	int		_prev_remain;
	unordered_set <int> _view_list;
	mutex	_vl;
	int last_move_time;
	std::atomic<bool> _active_npc;
	system_clock::time_point npc_last_move_time;
	int _sector_key;

public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
		last_move_time = 0;
		_active_npc = false;
		npc_last_move_time = system_clock::now();
		_sector_key = -1;
	}

	~SESSION() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}

	void send_login_info_packet()
	{
		S2C_AvatarInfo p;
		p.playerId = _id;
		p.size = sizeof(S2C_AvatarInfo);
		p.type = S2C_AVATAR_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}

	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_remove_player_packet(int c_id)
	{
		_vl.lock();
		if (_view_list.count(c_id))
			_view_list.erase(c_id);
		else {
			_vl.unlock();
			return;
		}
		_vl.unlock();
		S2C_RemovePlayer p;
		p.playerId = c_id;
		p.size = sizeof(p);
		p.type = S2C_REMOVE_PLAYER;
		do_send(&p);
	}

	void do_random_move();
	void wake_up()
	{
		// _active_npc가 false일 때만 true로 바꾸고 타이머 등록 (중복 등록 방지)
		bool expected = false;
		if (false == _active_npc.compare_exchange_strong(expected, true))
			return;

		event_type ev;
		ev.obj_id = _id;
		ev.event_id = EVENT_MOVE;
		ev.target_id = -1;
		ev.wakeup_time = system_clock::now() + milliseconds(MOVE_COOL_TIME);
		timer_queue.push(ev);
	}
};

struct SECTOR {
	std::mutex lock;
	std::unordered_set<int> objects;
};

HANDLE h_iocp;
tbb::concurrent_unordered_map<int, std::shared_ptr<SESSION>> clients;
tbb::concurrent_unordered_map<int, std::shared_ptr<SECTOR>> sectors;
SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

std::shared_ptr<SESSION> get_session_ptr(int object_id)
{
	auto iter = clients.find(object_id);

	return iter->second;
}

int get_sector_key(short x, short y)
{
	return y * WORLD_WIDTH + x;
}

std::shared_ptr<SECTOR> get_sector_ptr(int sector_key)
{
	auto iter = sectors.find(sector_key);
	if (iter != sectors.end())
		return iter->second;

	std::shared_ptr<SECTOR> new_sector = std::make_shared<SECTOR>();
	auto result = sectors.emplace(sector_key, new_sector);
	if (result.second)
		return new_sector;

	return result.first->second;
}

void enter_sector(int object_id)
{
	std::shared_ptr<SESSION> object = get_session_ptr(object_id);
	int sector_key = get_sector_key(object->x, object->y);
	std::shared_ptr<SECTOR> sector = get_sector_ptr(sector_key);

	{
		std::lock_guard<std::mutex> ll(sector->lock);
		sector->objects.insert(object_id);
	}
	object->_sector_key = sector_key;
}

void leave_sector(int object_id)
{
	std::shared_ptr<SESSION> object = get_session_ptr(object_id);
	int old_sector_key = object->_sector_key;
	if (-1 == old_sector_key)
		return;

	auto iter = sectors.find(old_sector_key);
	if (iter != sectors.end()) {
		std::shared_ptr<SECTOR> sector = iter->second;
		std::lock_guard<std::mutex> ll(sector->lock);
		sector->objects.erase(object_id);
	}
	object->_sector_key = -1;
}

void update_sector(int object_id)
{
	std::shared_ptr<SESSION> object = get_session_ptr(object_id);
	int new_sector_key = get_sector_key(object->x, object->y);
	if (object->_sector_key == new_sector_key)
		return;

	leave_sector(object_id);
	std::shared_ptr<SECTOR> sector = get_sector_ptr(new_sector_key);
	{
		std::lock_guard<std::mutex> ll(sector->lock);
		sector->objects.insert(object_id);
	}
	object->_sector_key = new_sector_key;
}

bool is_pc(int object_id)
{
	return object_id < MAX_PLAYERS;
}

bool is_npc(int object_id)
{
	return !is_pc(object_id);
}

bool can_see(int from, int to)
{
	std::shared_ptr<SESSION> from_session = get_session_ptr(from);
	std::shared_ptr<SESSION> to_session = get_session_ptr(to);

	if (abs(from_session->x - to_session->x) > VIEW_RANGE)
		return false;

	return abs(from_session->y - to_session->y) <= VIEW_RANGE;
}

std::unordered_set<int> get_near_list_by_sector(int object_id, bool include_npc)
{
	std::unordered_set<int> near_list;
	std::shared_ptr<SESSION> object = get_session_ptr(object_id);

	int min_x = max(0, static_cast<int>(object->x) - VIEW_RANGE);
	int max_x = min(WORLD_WIDTH - 1, static_cast<int>(object->x) + VIEW_RANGE);
	int min_y = max(0, static_cast<int>(object->y) - VIEW_RANGE);
	int max_y = min(WORLD_HEIGHT - 1, static_cast<int>(object->y) + VIEW_RANGE);

	for (int y = min_y; y <= max_y; ++y) {
		for (int x = min_x; x <= max_x; ++x) {
			auto sector_iter = sectors.find(get_sector_key(static_cast<short>(x), static_cast<short>(y)));
			if (sector_iter == sectors.end())
				continue;

			std::vector<int> candidates;
			{
				std::shared_ptr<SECTOR> sector = sector_iter->second;
				std::lock_guard<std::mutex> ll(sector->lock);
				candidates.assign(sector->objects.begin(), sector->objects.end());
			}

			for (int other_id : candidates) {
				if (other_id == object_id)
					continue;
				if ((false == include_npc) && is_npc(other_id))
					continue;

				std::shared_ptr<SESSION> other = get_session_ptr(other_id);
				if (ST_INGAME != other->_state)
					continue;
				if (can_see(object_id, other_id))
					near_list.insert(other_id);
			}
		}
	}

	return near_list;
}

void SESSION::do_random_move()
{
	std::unordered_set<int> old_vl = get_near_list_by_sector(_id, false);

	switch (rand() % 4) {
	case 0:
		if (x < (WORLD_WIDTH - 1)) x++;
		break;
	case 1:
		if (x > 0) x--;
		break;
	case 2:
		if (y < (WORLD_HEIGHT - 1)) y++;
		break;
	case 3:
		if (y > 0) y--;
		break;
	}

	update_sector(_id);

	std::unordered_set<int> new_vl = get_near_list_by_sector(_id, false);

	for (auto pl : new_vl) {
		std::shared_ptr<SESSION> player = get_session_ptr(pl);
		if (0 == old_vl.count(pl))
			// 플레이어의 시야에 등장
			player->send_add_player_packet(_id);
		else
			// 플레이어가 계속 보고 있음.
			player->send_move_packet(_id);
	}
	
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			std::shared_ptr<SESSION> player = get_session_ptr(pl);
			player->send_remove_player_packet(_id);
		}
	}

	if (_id == MAX_PLAYERS) {
		auto delay = system_clock::now() - npc_last_move_time;

		//std::cout << "NPC " << _id << " moved. Time since last move: " << duration_cast<milliseconds>(delay).count() << "ms\n";
	}

	npc_last_move_time = system_clock::now();
}

void SESSION::send_move_packet(int c_id)
{
	std::shared_ptr<SESSION> target = get_session_ptr(c_id);

	S2C_MovePlayer p;
	p.playerId = c_id;
	p.size = sizeof(S2C_MovePlayer);
	p.type = S2C_MOVE_PLAYER;
	p.x = target->x;
	p.y = target->y;
	p.move_time = target->last_move_time;

	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	std::shared_ptr<SESSION> target = get_session_ptr(c_id);

	S2C_AddPlayer add_packet;
	add_packet.playerId = c_id;
	strcpy_s(add_packet.username, target->_name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = S2C_ADD_PLAYER;
	add_packet.x = target->x;
	add_packet.y = target->y;
	_vl.lock();
	_view_list.insert(c_id);
	_vl.unlock();

	do_send(&add_packet);
}

int get_new_client_id()
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		std::shared_ptr<SESSION> client = get_session_ptr(i);
		std::lock_guard <mutex> ll{ client->_s_lock };
		if (client->_state == ST_FREE)
			return i;
	}

	return -1;
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case C2S_LOGIN: {
		std::shared_ptr<SESSION> client = get_session_ptr(c_id);
		C2S_Login* p = reinterpret_cast<C2S_Login*>(packet);
		strcpy_s(client->_name, p->username);
		{
			std::lock_guard<mutex> ll{ client->_s_lock };
			client->x = rand() % WORLD_WIDTH;
			client->y = rand() % WORLD_HEIGHT;
			client->_state = ST_INGAME;
		}
		enter_sector(c_id);
		client->send_login_info_packet();

		std::unordered_set<int> near_list = get_near_list_by_sector(c_id, true);
		for (auto& pl : near_list) {
			std::shared_ptr<SESSION> target = get_session_ptr(pl);
			if (is_pc(pl))
				target->send_add_player_packet(c_id);
			client->send_add_player_packet(pl);

			// 시야에 들어온 NPC를 깨움
			if (is_npc(pl))
				target->wake_up();
		}

		break;
	}
	case C2S_MOVE: {
		std::shared_ptr<SESSION> client = get_session_ptr(c_id);
		C2S_Move* p = reinterpret_cast<C2S_Move*>(packet);
		client->last_move_time = p->move_time;
		short x = client->x;
		short y = client->y;

		switch (p->dir) {
		case 0:
			if (y > 0) y--;
			break;
		case 1:
			if (y < WORLD_HEIGHT - 1) y++;
			break;
		case 2:
			if (x > 0) x--;
			break;
		case 3:
			if (x < WORLD_WIDTH - 1) x++;
			break;
		}
		client->x = x;
		client->y = y;
		update_sector(c_id);

		client->_vl.lock();
		std::unordered_set<int> old_vlist = client->_view_list;
		client->_vl.unlock();

		std::unordered_set<int> near_list = get_near_list_by_sector(c_id, true);

		client->send_move_packet(c_id);

		for (auto& pl : near_list) {
			auto target = get_session_ptr(pl);
			if (is_pc(pl)) {
				target->_vl.lock();
				if (target->_view_list.count(c_id)) {
					target->_vl.unlock();
					target->send_move_packet(c_id);
				}
				else {
					target->_vl.unlock();
					target->send_add_player_packet(c_id);
				}
			}

			if (old_vlist.count(pl) == 0) {
				client->send_add_player_packet(pl);
				// 새로 시야에 들어온 NPC를 깨움
				if (is_npc(pl))
					target->wake_up();
			}
		}

		for (auto& pl : old_vlist) {
			if (0 == near_list.count(pl)) {
				client->send_remove_player_packet(pl);

				if (is_pc(pl)) {
					std::shared_ptr<SESSION> target = get_session_ptr(pl);
					target->send_remove_player_packet(c_id);
				}
			}
		}

		break;
	}
	}
}

void disconnect(int c_id)
{
	std::shared_ptr<SESSION> client = get_session_ptr(c_id);
	SOCKET socket_to_close = INVALID_SOCKET;
	{
		lock_guard<mutex> ll(client->_s_lock);
		if (ST_FREE == client->_state)
			return;

		client->_state = ST_FREE;
		socket_to_close = client->_socket;
		client->_socket = INVALID_SOCKET;
	}

	leave_sector(c_id);

	client->_vl.lock();
	unordered_set <int> vl = client->_view_list;
	client->_view_list.clear();
	client->_vl.unlock();

	for (auto& p_id : vl) {
		if (is_npc(p_id))
			continue;

		std::shared_ptr<SESSION> player = get_session_ptr(p_id);
		{
			lock_guard<mutex> ll(player->_s_lock);
			if (ST_INGAME != player->_state)
				continue;
		}

		if (player->_id == c_id)
			continue;
		player->send_remove_player_packet(c_id);
	}

	if (INVALID_SOCKET != socket_to_close)
		closesocket(socket_to_close);
}

void do_npc_random_move(int npc_id)
{
	// NPC의 랜덤 이동을 처리하는 함수
	// npc_id에 해당하는 NPC를 랜덤한 방향으로 이동시키고, 주변 플레이어들에게 이동 정보를 알려준다.
	std::shared_ptr<SESSION> npc = get_session_ptr(npc_id);
	npc->do_random_move();
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			DWORD error_code = WSAGetLastError();
			if (nullptr == ex_over) {
				std::cout << "GQCS Error: " << error_code << "\n";
				continue;
			}

			if (ex_over->_comp_type == OP_ACCEPT) {
				std::cout << "Accept Error: " << error_code << "\n";
				continue;
			}
			
			if(::WSAGetLastError() != ERROR_NETNAME_DELETED)
				std::cout << "GQCS Error on client[" << key << "], error=" << error_code << "\n";

			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND)
				delete ex_over;

			continue;
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));

			if (ex_over->_comp_type == OP_SEND)
				delete ex_over;

			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				auto client = get_session_ptr(client_id);
				{
					lock_guard<mutex> ll(client->_s_lock);
					client->_state = ST_ALLOC;
				}
				client->x = 0;
				client->y = 0;
				client->_id = client_id;
				client->_name[0] = 0;
				client->_prev_remain = 0;
				client->_socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				client->do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else
				cout << "Max user exceeded.\n";

			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int client_id = static_cast<int>(key);
			auto client = get_session_ptr(client_id);
			int remain_data = num_bytes + client->_prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(client_id, p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			client->_prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			client->do_recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		case OP_NPCMOVE: {
			delete ex_over;
			int npc_id = static_cast<int>(key);
			do_npc_random_move(npc_id);

			// 시야 내에 플레이어가 있는지 확인
			bool has_nearby_player = false == get_near_list_by_sector(npc_id, false).empty();

			if (has_nearby_player) {
				// 다음 이동 이벤트 재등록
				event_type ev;
				ev.event_id = EVENT_MOVE;
				ev.obj_id = npc_id;
				ev.target_id = -1;
				ev.wakeup_time = system_clock::now() + milliseconds(MOVE_COOL_TIME);
				timer_queue.push(ev);
			}
			else {
				// 시야 내 플레이어 없음 → AI 비활성화 (다음 wake_up() 호출까지 대기)
				auto npc = get_session_ptr(npc_id);
				npc->_active_npc = false;
			}
			break;
		}
		}
	}
}

void InitializeNPC()
{
	cout << "NPC intialize begin.\n";
	for (int i = 0; i < MAX_PLAYERS + MAX_NPCS; ++i)
		clients.emplace(i, std::make_shared<SESSION>());

	for (int i = MAX_PLAYERS; i < MAX_PLAYERS + MAX_NPCS; ++i) {
		auto npc = get_session_ptr(i);
		npc->x = rand() % WORLD_WIDTH;
		npc->y = rand() % WORLD_HEIGHT;
		npc->_id = i;
		sprintf_s(npc->_name, "NPC%d", i);
		npc->_state = ST_INGAME;
		npc->last_move_time = 0;
		npc->npc_last_move_time = system_clock::now();
		enter_sector(i);
	}
	cout << "NPC initialize end.\n";
}

void timer_thread()
{
	while (true) {
		event_type ev;

		if (timer_queue.try_pop(ev)) {
			auto now = system_clock::now();
			if (ev.wakeup_time <= now) {
				switch (ev.event_id) {
				case EVENT_MOVE:
					OVER_EXP* move_over = new OVER_EXP;
					move_over->_comp_type = OP_NPCMOVE; // 이동 이벤트는 OP_SEND로 처리
					PostQueuedCompletionStatus(h_iocp, -1, ev.obj_id, &move_over->_over); // 이동 이벤트를 워커 스레드로 전달
					break;
				}
			}
			else {
				// 아직 시간이 안 됐으면 다시 큐에 넣음
				timer_queue.push(ev);
				this_thread::sleep_for(chrono::milliseconds(1));
			}
		}
		else
			this_thread::sleep_for(chrono::milliseconds(1));
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (SOCKET_ERROR == ::bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr))) {
		return 1;
	}
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);

	InitializeNPC();

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	std::vector<thread> worker_threads;
	std::thread timer_th(timer_thread);   // HB_thread 대신 timer_thread 사용
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();
	timer_th.join();
	closesocket(g_s_socket);
	WSACleanup();
}
