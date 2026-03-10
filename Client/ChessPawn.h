#pragma once
#include <atomic>
#include <array>

class ChessPawn {
public:
	ChessPawn();
	~ChessPawn();

	void MoniteringKey();
	void Draw();

	void ChangeState(char posX, char posY);

	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();

private:
	std::atomic<char> posX = 0;
	std::atomic<char> posY = 0;

	std::array<std::array<bool, 8>, 8> chessBoard;
};