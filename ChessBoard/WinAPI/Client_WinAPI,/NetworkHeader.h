#pragma once
#include "framework.h"
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

constexpr short SERVER_PORT = 3000;

enum class Direction {
	NONE = 0,
	UP = 1,
	DOWN = 2,
	LEFT = 3,
	RIGHT = 4
};