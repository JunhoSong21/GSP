#pragma once
#include <stdint.h>

constexpr short SERVER_PORT = 3000;
constexpr int WORLD_WIDTH = 400;
constexpr int WORLD_HEIGHT = 400;
constexpr int MAX_NAME_LEN = 20;
constexpr int MAX_PLAYER = 20000;

enum PACKET_TYPE : uint8_t {
	// C2S
	C2S_LOGIN,
	C2S_MOVE,

	// S2C
	S2C_LOGIN_RESULT,
	S2C_AVATAR_INFO,
	S2C_ADD_PLAYER,
	S2C_REMOVE_PLAYER,
	S2C_MOVE_PLAYER,
};

enum DIRECTION : uint8_t {
	NONE,
	UP,
	DOWN,
	LEFT,
	RIGHT,
};

#pragma pack(push, 1)

// C2S
struct C2S_Login {
	uint16_t	size;
	PACKET_TYPE type;

	char user_name[MAX_NAME_LEN];
};

struct C2S_Move {
	uint16_t	size;
	PACKET_TYPE type;

	DIRECTION direction;

	int move_time;
};

// S2C
struct S2C_Login_Result {
	uint16_t	size;
	PACKET_TYPE type;

	bool success;
	char msg[50];
};

struct S2C_Avatar_Info {
	uint16_t	size;
	PACKET_TYPE type;

	uint64_t player_id;
	short x;
	short y;
};

struct S2C_Add_Player {
	uint16_t	size;
	PACKET_TYPE type;

	uint64_t player_id;
	char username[MAX_NAME_LEN];
	short x;
	short y;
};

struct S2C_Remove_Player {
	uint16_t	size;
	PACKET_TYPE type;

	uint64_t player_id;
};

struct S2C_Move_Player {
	uint16_t	size;
	PACKET_TYPE type;

	uint64_t player_id;
	short x;
	short y;

	int move_time;
};

#pragma pack(pop)
