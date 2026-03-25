#pragma once
#include "framework.h"

class ChessPawn {
public:
	ChessPawn();
	~ChessPawn();

	int		MoniteringKey();
	void	DrawChessBoard(HDC hDC, int posX, int posY, int board_size, int targetX, int targetY);
};