#include "NetworkHeader.h"
#include "Session.h"
#include "ChessPawn.h"

void CALLBACK Recv_Callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK Send_Callback(DWORD error, DWORD bytes_transferred, LPWSAOVERLAPPED over, DWORD flags);

// class Over_Exp
Over_Exp::Over_Exp(int id, int x, int y)
{
	::ZeroMemory(&over, sizeof(over));
	wsa_buf.buf = buf;
	wsa_buf.len = 12;

	int* p = reinterpret_cast<int*>(buf);
	p[0] = id;
	p[1] = x;
	p[2] = y;
}

// class Session
Session::Session(SOCKET socket, int id) : _socket(socket), _id(id)
{
	::ZeroMemory(&_over, sizeof(_over));
	_wsa_buf.buf = _buf;
	_wsa_buf.len = BUF_SIZE;
}

Session::~Session()
{
	closesocket(_socket);
}

void Session::Do_Recv()
{
	::ZeroMemory(&_over, sizeof(_over));
	_wsa_buf.buf = _buf;
	_wsa_buf.len = BUF_SIZE;

	DWORD recv_flag = 0;

	::WSARecv(_socket, &_wsa_buf, 1, 0, &recv_flag, &_over, Recv_Callback);
}

void Session::Do_Send(int send_id, int num_bytes, int x, int y)
{
	Over_Exp* over = new Over_Exp(send_id, x, y);

	::WSASend(_socket, &over->wsa_buf, 1, 0, 0, &over->over, Send_Callback);
}
