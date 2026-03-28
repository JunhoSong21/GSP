#include "ChessPawn.h"
#include <iostream>
#include <Windows.h>

//#include <io.h>
//#include <fcntl.h>
#include <conio.h>

ChessPawn::ChessPawn()
{
	//// 유니코드 출력을 위한 _setmode
	//if (-1 == _setmode(_fileno(stdout), _O_U16TEXT)) {
	//	std::wcout << L"_setmode() Fail" << L"\n";
	//	return;
	//}
	
	// 커서 숨기기 코드
	HANDLE hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	::GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	::SetConsoleCursorInfo(hConsole, &cursorInfo);
}

ChessPawn::~ChessPawn()
{
}

int ChessPawn::MoniteringKey()
{
	//if (GetAsyncKeyState(VK_UP) & 1) {
	//	return Direction::UP;
	//}
	//if (GetAsyncKeyState(VK_DOWN) & 1) {
	//	return Direction::DOWN;
	//}
	//if (GetAsyncKeyState(VK_LEFT) & 1) {
	//	return Direction::LEFT;
	//}
	//if (GetAsyncKeyState(VK_RIGHT) & 1) {
	//	return Direction::RIGHT;
	//}
	
	// 과제 구현을 위해 블로킹 방식으로 전환
	int input = _getch();
	input = _getch();  
	
	return input;
}

void ChessPawn::Draw(int posX, int posY)
{
	::SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD(0, 0));

	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (posX == j && posY == i) {
				std::wcout << L"\u2659 ";
				continue;
			}

			if ((i + j) & 1)
				std::wcout << L"\u2592\u2592";
			else
				std::wcout << L"\u2588\u2588";
		}
	
		std::wcout << L"\n";
	}
	
	std::wcout << L"(" << posX << L", " << posY << L")" << L"\n";
}