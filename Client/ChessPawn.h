#pragma once
#include <atomic>

class ChessPawn {
public:
	ChessPawn();
	~ChessPawn();

	int		MoniteringKey();
	void	Draw(int posX, int posY);
};