#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

constexpr short SERVER_PORT = 3000;
constexpr int	BUFSIZE = 4096;

class EXP_OVER {
public:
	WSAOVERLAPPED	_wsa_over;
	size_t			_socket_id;
	WSABUF			_wsa_buf;
	char			_send_msg[BUFSIZE];

public:
	EXP_OVER(size_t socket_id, char num_bytes, char* msg)
		: _socket_id(socket_id)
	{
		::ZeroMemory(&_wsa_over, sizeof(_wsa_over));
	}
};

int main()
{
	
}
