#pragma once

class EXP_OVER {
public:
	WSAOVERLAPPED	_over;
	IOType			_iotype;
	char			_buf[BUF_SIZE];
	WSABUF			_wsa_buf;

public:
	EXP_OVER();
	EXP_OVER(IOType iotype);

};
