#pragma once
#include "headers.h"
#include "enum.h"

// class OVEREX
class OVEREX {
public:
	WSAOVERLAPPED	m_over;
	IO_TYPE			m_iotype;
	WSABUF			m_wsa_buf;
	char			m_buf[BUF_SIZE];

public:
	OVEREX();
	OVEREX(IO_TYPE iotype);

};

// class Accept_OVEREX
class Accept_OVEREX : public OVEREX {
public:
	SOCKET m_c_socket;
	SOCKET s_socket;

public:
	Accept_OVEREX();

};

// class Session
class Session {
public:
	SOCKET			_c_socket;
	std::atomic_bool _connected;

	OVEREX			_recv_over;
	int				_prev_recv;

	uint64_t		_player_id;

public:
	Session();
	~Session();

public:
	void Recv_Process();

};
