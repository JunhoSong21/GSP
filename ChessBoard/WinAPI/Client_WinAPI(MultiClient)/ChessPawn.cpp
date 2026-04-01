#include "NetworkHeader.h"
#include "ChessPawn.h"

#include <conio.h> // _kbhit(), _getch()

ChessPawn::ChessPawn() : posX(0), posY(0), is_alive(false), id(-1)
{
}

Direction ChessPawn::MoniteringKey()
{
	if (GetAsyncKeyState(VK_UP) & 0x0001)
		return Direction::UP;
	if (GetAsyncKeyState(VK_DOWN) & 0x0001)
		return Direction::DOWN;
	if (GetAsyncKeyState(VK_LEFT) & 0x0001)
		return Direction::LEFT;
	if (GetAsyncKeyState(VK_RIGHT) & 0x0001)
		return Direction::RIGHT;

	return Direction::NONE;
}

void ChessPawn::DrawChessBoard(
	HDC hDC, int posX, int posY, int board_size, int targetX, int targetY)
{
	using namespace Gdiplus;

	// 테두리
	HPEN hPen = ::CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
	HPEN hOldPen = static_cast<HPEN>(::SelectObject(hDC, hPen));

	::Rectangle(hDC, posX, posY, posX + board_size + 1, posY + board_size + 1);

	::SelectObject(hDC, hOldPen);
	::DeleteObject(hPen);

	// 체스 판
	int cell_size = board_size / 8;

	HBRUSH black_brush = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	HBRUSH white_brush = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));

	// GDI+
	Graphics graphics(hDC);
	graphics.SetCompositingMode(CompositingModeSourceOver);
	Image* image = Image::FromFile(L"pawn-rb.png");

	int width_downsize = board_size / 50;

	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			int x = (col * cell_size) + posX;
			int y = (row * cell_size) + posY;

			RECT cell_rect = { x, y, x + cell_size, y + cell_size };

			if (1 == ((row + col) % 2))
				::FillRect(hDC, &cell_rect, black_brush);
			else
				::FillRect(hDC, &cell_rect, white_brush);

			if (row == targetX && col == targetY) {
				if (Ok == image->GetLastStatus()) {
					graphics.DrawImage(image,
						x + width_downsize, y, cell_size - (width_downsize * 2), cell_size);
				}
			}
		}
	}

	SetTextColor(hDC, RGB(0, 0, 0));
	const wchar_t* text = L"Client 1";
	TextOutW(hDC, 50, 20, text, lstrlenW(text));
}

int	ChessPawn::Get_Pos_X()
{
	return posX;
}

int	ChessPawn::Get_Pos_Y()
{
	return posY;
}

void ChessPawn::Set_Pos(int targetX, int targetY)
{
	posX = targetX;
	posY = targetY;
}
