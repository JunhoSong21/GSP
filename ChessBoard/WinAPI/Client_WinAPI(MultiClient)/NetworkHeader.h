#pragma once
#include "framework.h"
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

constexpr short SERVER_PORT = 3000;
constexpr int BUF_SIZE = 4096;

enum class Direction {
	NONE = 0,
	UP = 1,
	DOWN = 2,
	LEFT = 3,
	RIGHT = 4
};

class Over_Exp {
public:
	WSAOVERLAPPED	over;
	WSABUF			wsa_buf;
	char			buf[BUF_SIZE];
	int				id;

public:
	Over_Exp(int id, int x, int y)
	{
		::ZeroMemory(&over, sizeof(over));
		wsa_buf.buf = buf;
		wsa_buf.len = 12;

		int* p = reinterpret_cast<int*>(buf);
		p[0] = id;
		p[1] = x;
		p[2] = y;
	}
};
