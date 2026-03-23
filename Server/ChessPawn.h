#pragma once
#include <atomic>
#include <array>

constexpr int KEY_UP = 72;
constexpr int KEY_DOWN = 80;
constexpr int KEY_LEFT = 75;
constexpr int KEY_RIGHT = 77;

class ChessPawn {
public:
	ChessPawn();
	~ChessPawn();

	void InputProcess(int direction);

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