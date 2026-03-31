#include "NetworkHeader.h"
#include "ChessPawn.h"

ChessPawn::ChessPawn()
{
	chessBoard = {};

	posX.store(3);
	posY.store(3);
	ChangeState(posX, posY);
}

void ChessPawn::InputProcess(Direction direction)
{
	switch (direction) {
	case Direction::UP:
		MoveUp();
		break;
	case Direction::DOWN:
		MoveDown();
		break;
	case Direction::LEFT:
		MoveLeft();
		break;
	case Direction::RIGHT:
		MoveRight();
		break;
	case Direction::NONE:
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

// ÁÂ»ó´Ü ÁÂÇ¥°¡ (0, 0)
// ¿ìÇÏ´Ü ÁÂÇ¥°¡ (7, 7)

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
