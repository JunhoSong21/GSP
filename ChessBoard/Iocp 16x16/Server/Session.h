#pragma once

enum class IO_TYPE : uint8_t {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT,
};

// class OVEREX
class OVEREX {
public:
	WSAOVERLAPPED m_over;
	IO_TYPE m_iotype;
	WSABUF m_wsa_buf;
	char m_buf[BUF_SIZE];

public:
	OVEREX();
	OVEREX(IO_TYPE iotype);

};

// class Accept_OVEREX
class Accept_OVEREX : public OVEREX {
public:
	SOCKET m_c_socket;

public:
	Accept_OVEREX();

};

// enum class CLIENT_STATE
enum class CLIENT_STATE : uint8_t {
	CONNECT,
	INGAME,
	LOGOUT,
};

// class Session
class Session {
public:
	SOCKET			_c_socket;
	uint64_t		_c_id;

public:
	OVEREX			_recv_over;
	CLIENT_STATE	_state;
	short			_posX, _posY;
	char			_username[MAX_NAME_LEN];
	int				_prev_recv;

	int				_move_time;

public:
	Session(SOCKET socket, uint64_t id);
	~Session();
	
public:
	// Send
	void Send_Process(int num_bytes, char* msg);
	
	void Send_Login_Success();
	void Send_Avatar_Info();
	void Send_Add_Player(uint64_t player_id);
	void Send_Move_Player(uint64_t player_id);

	// Recv
	void Recv_Process();

	bool Process_Packet(unsigned char* p);

};
