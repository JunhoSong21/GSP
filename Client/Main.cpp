#include "ChessPawn.h"

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

int main()
{
	SetConsoleOutputCP(CP_UTF8);

	int ret = _setmode(_fileno(stdout), _O_U8TEXT);

	ChessPawn chessPawn;

	chessPawn.Draw();
	while (true) {
		chessPawn.MoniteringKey();
	}
}