#pragma once

constexpr int SERVER_PORT = 3000;
constexpr int WORLD_WIDTH = 8;
constexpr int WORLD_HEIGHT = 8;
constexpr int MAX_PLAYER = 10;
constexpr int MAX_NAME_LEN = 20;

enum PACKET_TYPE {
	C2S_LOGIN,
	C2S_MOVE,
	S2C_LOGINRESULT,
	S2C_AVATERINFO,
	S2C_ADDPLAYER,
	S2C_REMOVEPLAYER,
	S2C_MOVEPLAYER,
};

enum DIRECTION {
	UP,
	DOWN,
	LEFT,
	RIGHT,
};

#pragma pack(push, 1)

struct C2S_Login {
	unsigned char size;
	PACKET_TYPE type;
	char username[MAX_NAME_LEN];
};

struct C2S_Move {
	unsigned char size;
	PACKET_TYPE type;
	DIRECTION direction;
};

struct S2C_LoginResult {
	unsigned char size;
	PACKET_TYPE type;
	bool success;
	char message[50];
};

struct S2C_AvatarInfo {
	unsigned char size;
	PACKET_TYPE type;
	int playerId;
	short x;
	short y;
};

struct S2C_AddPlayer {
	unsigned char size;
	PACKET_TYPE type;
	int playerId;
	char username[MAX_NAME_LEN];
	short x;
	short y;
};

struct S2C_RemovePlayer {
	unsigned char size;
	PACKET_TYPE type;
	int playerId;
};

struct S2C_MovePlayer {
	unsigned char size;
	PACKET_TYPE type;
	int playerId;
	short x;
	short y;
};

#pragma pack(pop)
