#pragma once
#include <atomic>
#include <array>

class ChessPawn {
public:
	ChessPawn();
	~ChessPawn();

	void MoniteringKey();
	void Draw();

	void ChangeState(int posX, int posY);

	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();

private:
	std::atomic<int> posX = 0;
	std::atomic<int> posY = 0;

	std::array<std::array<bool, 8>, 8> chessBoard;
};