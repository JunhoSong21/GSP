#pragma once

constexpr int BUF_SIZE = 200;

enum IO_TYPE : uint8_t {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT,
};

// class Over_Exp
class Over_Exp {
public:
	WSAOVERLAPPED	m_over;
	IO_TYPE			m_iotype;
	WSABUF			m_wsa_buf;
	char			m_buf[BUF_SIZE];

public:
	Over_Exp();
	Over_Exp(IO_TYPE iotype);

};

// class Session
class Session {
public:
	SOCKET		_c_socket;
	int			_id;

public:
	Over_Exp	_recv_over;
	bool		_is_connected;
	short		_x, _y;
	char		_username[MAX_NAME_LEN];
	int			_prev_recv;

public:
	Session();
	Session(SOCKET socket, int id);
	~Session();

	void Do_Recv();
	void Do_Send(int num_bytes, char* packet);
	
	void Send_Login_Result();
	void Send_Avatar_Info();
	void Send_Add_Player(int player_id, char* add_name, int add_x, int add_y);
	void Send_Remove_Player(int player_id);
	void Send_Move_Player(int player_id, int move_x, int move_y);

	void Process_Packet(uint8_t* ptr);

};
