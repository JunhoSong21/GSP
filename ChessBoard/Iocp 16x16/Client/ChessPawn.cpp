#include "NetworkHeader.h"
#include "ChessPawn.h"

ChessPawn::ChessPawn()
	: _posX(0), _posY(0), id(-1)
{
}

DIRECTION ChessPawn::MoniteringKey()
{
	if (GetAsyncKeyState(VK_UP) & 0x0001)
		return DIRECTION::UP;
	if (GetAsyncKeyState(VK_DOWN) & 0x0001)
		return DIRECTION::DOWN;
	if (GetAsyncKeyState(VK_LEFT) & 0x0001)
		return DIRECTION::LEFT;
	if (GetAsyncKeyState(VK_RIGHT) & 0x0001)
		return DIRECTION::RIGHT;

	return DIRECTION::NONE;
}

int ChessPawn::Get_PosX()
{
	return _posX;
}

int ChessPawn::Get_PosY()
{
	return _posY;
}

void ChessPawn::Set_Pos(short x, short y)
{
	_posX = x;
	_posY = y;
}
