#include "ChessPawn.h"
#include <iostream>
#include <Windows.h>

ChessPawn::ChessPawn()
{
	chessBoard = {};

	posX.store(3);
	posY.store(3);
	ChangeState(posX, posY);
}

ChessPawn::~ChessPawn()
{
}

void ChessPawn::InputProcess(int direction)
{
	switch (direction)
	{
	case KEY_UP: // VK_UP
		MoveUp();
		break;
	case KEY_DOWN: // VK_DOWN
		MoveDown();
		break;
	case KEY_LEFT: // VK_LEFT
		MoveLeft();
		break;
	case KEY_RIGHT: // VK_RIGHT
		MoveRight();
		break;
	default:
		break;
	}
}

void ChessPawn::ChangeState(int posX, int posY)
{
	if (false == chessBoard[posY][posX])
		chessBoard[posY][posX] = true;
	else
		chessBoard[posY][posX] = false;
}

// 좌상단 좌표가 (0, 0)
// 우하단 좌표가 (7, 7)

void ChessPawn::MoveUp()
{
	if (posY > 0) {
		ChangeState(posX, posY);
		posY.fetch_add(-1);
		ChangeState(posX, posY);
	}
}

void ChessPawn::MoveDown()
{
	if (posY < 7) {
		ChangeState(posX, posY);
		posY.fetch_add(1);
		ChangeState(posX, posY);
	}
}

void ChessPawn::MoveLeft()
{
	if (posX > 0) {
		ChangeState(posX, posY);
		posX.fetch_add(-1);
		ChangeState(posX, posY);
	}
}

void ChessPawn::MoveRight()
{
	if (posX < 7) {
		ChangeState(posX, posY);
		posX.fetch_add(1);
		ChangeState(posX, posY);
	}
}