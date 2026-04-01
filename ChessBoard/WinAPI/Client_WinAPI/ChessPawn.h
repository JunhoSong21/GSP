#pragma once

class ChessPawn {
public:
	ChessPawn();

	Direction	MoniteringKey();
	void		DrawChessBoard(HDC hDC, int posX, int posY, int board_size, int targetX, int targetY);

	int			Get_Pos_X();
	int			Get_Pos_Y();

	void		Set_Pos(int targetX, int targetY);

private:
	int posX;
	int posY;
};