#include "ChessPawn.h"
#include <iostream>
#include <Windows.h>

#include <io.h>
#include <fcntl.h>

using std::wcout, std::endl;

ChessPawn::ChessPawn()
{
	chessBoard = {};

	posX.store(3);
	posY.store(3);
	ChangeState(posX, posY);

	// 유니코드 출력을 위한 _setmode
	if (-1 == _setmode(_fileno(stdout), _O_U16TEXT)) {
		wcout << L"_setmode() Fail" << endl;
		return;
	}

	// 커서 숨기기 코드
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}

ChessPawn::~ChessPawn()
{
}

void ChessPawn::MoniteringKey()
{
	if (GetAsyncKeyState(VK_UP) & 1) {
		MoveUp();
		Draw();
	}
	if (GetAsyncKeyState(VK_DOWN) & 1) {
		MoveDown();
		Draw();
	}
	if (GetAsyncKeyState(VK_LEFT) & 1) {
		MoveLeft();
		Draw();
	}
	if (GetAsyncKeyState(VK_RIGHT) & 1) {
		MoveRight();
		Draw();
	}
}

void ChessPawn::Draw()
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD(0, 0));

	for (int i = 0; i < chessBoard.size(); ++i) {
		for (int j = 0; j < chessBoard[i].size(); ++j) {
			if (chessBoard[j][i] == false) {
				if ((i + j) & 1)
					wcout << L"\u2592\u2592";
				else
					wcout << L"\u2588\u2588";
			}
			else
				wcout << L"\u2659 ";
		}
	
		wcout << endl;
	}
	
	wcout << "(" << posX << ", " << posY << ")" << endl;
}

void ChessPawn::ChangeState(int posX, int posY)
{
	if (chessBoard[posX][posY] == false)
		chessBoard[posX][posY] = true;
	else
		chessBoard[posX][posY] = false;
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