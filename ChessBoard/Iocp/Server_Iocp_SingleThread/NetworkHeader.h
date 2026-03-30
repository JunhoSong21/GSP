#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <unordered_map>
#include <memory>
#pragma comment(lib, "WS2_32.lib")

#include <MSWSock.h>
#pragma comment(lib, "MSWSock.lib")

constexpr int SERVER_PORT = 3000;
constexpr int BUF_SIZE = 4096;

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

enum class IOType {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT,
};