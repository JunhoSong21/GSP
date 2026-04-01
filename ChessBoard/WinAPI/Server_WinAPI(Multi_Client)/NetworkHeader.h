#pragma once

#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <atomic>
#include <vector>
#include <array>

constexpr short SERVER_PORT = 3000;
constexpr int BUF_SIZE		= 4096;

inline void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg << L" : " << lpMsgBuf << std::endl;
	while (true);
	// µð¹ö±ë ¿ë
	::LocalFree(lpMsgBuf);
}

enum class Direction {
	NONE = 0,
	UP = 1,
	DOWN = 2,
	LEFT = 3,
	RIGHT = 4,
	LOGIN = 99,
};
