#pragma once
#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#include <MSWSock.h>
#pragma comment(lib, "MSWSock.lib")
#include <array>

#include "Protocol.h"
#include "Session.h"



inline void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg;
	std::wcout << L" === 에러 " << lpMsgBuf << std::endl;
	while (true);   // 디버깅 용
	LocalFree(lpMsgBuf);
}

inline void Send_Login_Fail(SOCKET socket)
{
	// Packet
	S2C_Login_Result packet;
	packet.packet_size = sizeof(S2C_Login_Result);
	packet.packet_type = S2C_LOGIN_RESULT;
	packet.success = false;
	
	const char* msg = "MAX PLAYER";
	::memcpy(packet.msg, msg, ::strlen(msg));
	packet.msg[::strlen(msg)] = '\0';

	// Send
	Over_Exp* over_exp = new Over_Exp[IO_SEND];
	over_exp->m_wsa_buf.len = sizeof(packet) + 2;

	::memcpy(over_exp->m_buf, reinterpret_cast<char*>(&packet), sizeof(packet));

	if (SOCKET_ERROR == ::WSASend(socket, &over_exp->m_wsa_buf, 1, 0, 0, &over_exp->m_over, nullptr)) {
		if (WSA_IO_PENDING != ::WSAGetLastError())
			exit(-1);
	}
}

inline void Set_New_Client(Session session, SOCKET socket, int id)
{
	session._c_socket = socket;
	session._id = id;
	session._is_connected = true;
	session._x = 0;
	session._y = 0;
	session._prev_recv = 0;

	session.Do_Recv();
}
