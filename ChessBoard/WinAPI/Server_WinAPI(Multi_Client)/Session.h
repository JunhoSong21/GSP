#pragma once
#include "ChessPawn.h"

// class Over_Exp
class Over_Exp {
public:
	WSAOVERLAPPED	over;
	WSABUF			wsa_buf;
	char			buf[BUF_SIZE];
	int				id;

public:
	Over_Exp(int id, int x, int y);

};

// class Session
class Session {
public:
	WSAOVERLAPPED	_over;
	SOCKET			_socket;
	int				_id;
	char			_buf[BUF_SIZE];
	WSABUF			_wsa_buf;
	
	ChessPawn		chess_pawn;

public:
	Session(SOCKET socket, int id);
	~Session();

	void Do_Recv();
	void Do_Send(int send_id, int num_bytes, int x, int y);
};
