#pragma once
#include <stdint.h>

constexpr int SERVER_PORT	= 3000;
constexpr int WORLD_WIDTH	= 8;
constexpr int WORLD_HEIGHT	= 8;
constexpr int MAX_PLAYER	= 10;
constexpr int MAX_NAME_LEN	= 20;

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

enum DIRECTION {
	UP,
	DOWN,
	LEFT,
	RIGHT,
};

#pragma pack(push, 1)

// C2S
struct C2S_Login {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	char user_name[MAX_NAME_LEN];
};

struct C2S_Move {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	DIRECTION direction;
};

// S2C
struct S2C_Login_Result {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	bool success;
	char msg[50];
};

struct S2C_Avatar_Info {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	int player_id;
	short x;
	short y;
};

struct S2C_Add_Player {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	int player_id;
	char username[MAX_NAME_LEN];
	short x;
	short y;
};

struct S2C_Remove_Player {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	int player_id;
};

struct S2C_Move_Player {
	uint8_t		packet_size;
	PACKET_TYPE packet_type;

	int player_id;
	short x;
	short y;
};

#pragma pack(pop)
