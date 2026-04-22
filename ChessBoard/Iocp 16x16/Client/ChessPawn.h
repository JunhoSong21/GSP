#pragma once

class ChessPawn {
public:
	ChessPawn();

	DIRECTION MoniteringKey();
	void DrawChessBoard();

	int Get_PosX();
	int Get_PosY();

	void Set_Pos(short x, short y);

private:
	short _posX;
	short _posY;

public:
	uint64_t id;

};
