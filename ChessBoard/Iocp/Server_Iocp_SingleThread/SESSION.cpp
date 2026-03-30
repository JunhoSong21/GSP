#include "NetworkHeader.h"
#include "SESSION.h"

#include "EXP_OVER.h"

SESSION::SESSION(SOCKET socket, int id) : _c_socket(socket), _c_id(id)
{
	::ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
	_recv_over._iotype = IOType::IO_RECV;
	_recv_over._wsa_buf.buf = _recv_msg;
	_recv_over._wsa_buf.len = BUF_SIZE;
}

SESSION::~SESSION()
{
	closesocket(_c_socket);
}

void SESSION::do_recv()
{
	DWORD recv_flag = 0;
	::ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
	::WSARecv(_c_socket, &_recv_over._wsa_buf, 1, 0, &recv_flag, &_recv_over._over, nullptr);
}

void SESSION::do_send(int send_id, int num_bytes, char* msg)
{
	EXP_OVER* over = new EXP_OVER(IOType::IO_SEND);
	over->_buf[0] = num_bytes + 2;
	over->_buf[1] = send_id;
	::memcpy(over->_buf + 2, msg, num_bytes);
	over->_wsa_buf.len = num_bytes + 2;

	::WSASend(_c_socket, &over->_wsa_buf, 1, 0, 0, &over->_over, nullptr);
}
