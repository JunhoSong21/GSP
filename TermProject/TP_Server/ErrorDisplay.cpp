#include "headers.h"
#include "ErrorDisplay.h"

void error_display(const char* msg)
{
	LPSTR lpMsgBuf = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, ::WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg << " Error " << lpMsgBuf << std::endl;
	while (true);   // µð¹ö±ë ¿ë
	LocalFree(lpMsgBuf);
}
