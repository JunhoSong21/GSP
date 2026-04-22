#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <array>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include "protocol.h"
#include <tbb/concurrent_unordered_map.h>
#include <unordered_set>
#include <mutex>

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")

constexpr int BUF_SIZE = 4096;
constexpr int VIEW_RANGE = 5;

inline void error_display(const char* msg)
{
	LPSTR lpMsgBuf = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, ::WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg << " Error " << lpMsgBuf << std::endl;
	while (true);   // µð¹ö±ë ¿ë
	LocalFree(lpMsgBuf);
}