#include "Session.h"

// class OVEREX
OVEREX::OVEREX()
{
	::ZeroMemory(&m_over, sizeof(m_over));
	m_iotype = IO_TYPE::IO_SEND;
	m_wsa_buf.buf = m_buf;
	m_wsa_buf.len = BUF_SIZE;
}

OVEREX::OVEREX(IO_TYPE iotype) : m_iotype(iotype)
{
	::ZeroMemory(&m_over, sizeof(m_over));
	m_wsa_buf.buf = m_buf;
	m_wsa_buf.len = BUF_SIZE;
}

// class Accept_OVEREX
Accept_OVEREX::Accept_OVEREX() : OVEREX(IO_TYPE::IO_ACCEPT)
{
	m_c_socket = INVALID_SOCKET;
}

// class Session
Session::Session()
{
	_c_socket = INVALID_SOCKET;
	_connected = false;

	_recv_over.m_iotype = IO_TYPE::IO_RECV;
	_prev_recv = 0;

	_player_id = 0;
}

Session::~Session()
{
	/*if (_state == CLIENT_STATE::LOGOUT)
		closesocket(_c_socket);*/
}

void Session::Recv_Process()
{
	if (false == _connected.load())
		return;

	DWORD recv_flag = 0;

	_recv_over.m_iotype = IO_TYPE::IO_RECV;
	::ZeroMemory(&_recv_over.m_over, sizeof(_recv_over.m_over));
	_recv_over.m_wsa_buf.buf = &_recv_over.m_buf[_prev_recv];
	_recv_over.m_wsa_buf.len = BUF_SIZE - _prev_recv;

	::WSARecv(_c_socket, &_recv_over.m_wsa_buf, 1, 0, &recv_flag, &_recv_over.m_over, nullptr);
}
