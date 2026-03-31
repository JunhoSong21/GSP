#pragma once

class ChessPawn {
public:
	ChessPawn();

	void InputProcess(Direction direction);

	void ChangeState(int posX, int posY);

	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();

public:
	std::atomic<int> posX = 0;
	std::atomic<int> posY = 0;

	std::array<std::array<bool, 8>, 8> chessBoard;
};
