#pragma once

class EXP_OVER;

class SESSION {
private:
	SOCKET		_c_socket;
	long long	_c_id;

public:
	EXP_OVER	_recv_over;
	char		_recv_msg[BUF_SIZE];

public:
	SESSION(SOCKET socket, int id);
	~SESSION();

	void do_recv();
	void do_send(int send_id, int num_bytes, char* msg);

};
