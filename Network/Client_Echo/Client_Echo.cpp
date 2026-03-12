#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char*		SERVER_IP = "127.0.0.1";
constexpr short SERVER_PORT = 169740;
constexpr int	BUFFER_SIZE = 4096;

int main()
{
	std::wcout.imbue(std::locale("korean")); // 오류메시지 한글
	WSAStartup(MAKEWORD(2, 2), nullptr);

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

	WSAConnect(s_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr), NULL, NULL, NULL);
	for (;;) {
		std::string input;
		char buffer[BUFFER_SIZE];
		std::cout << "Enter Message to send : ";
		std::cin.getline(buffer, BUFFER_SIZE);

		WSABUF wsaBuf{std::strlen(buffer) + 1, buffer};
		DWORD sentSize = 0;
		WSASend(s_socket, &wsaBuf, 1, &sentSize, 0, nullptr, nullptr);

		//

		char recvBuffer[BUFFER_SIZE]{};
		WSABUF recvWsaBuf{ BUFFER_SIZE, recvBuffer };
		DWORD recvSize = 0;
		DWORD recvFlag = 0;
		WSARecv(s_socket, &recvWsaBuf, 1, &recvSize, &recvFlag, nullptr, nullptr);
	
		std::cout << "Rec from Server : " << recvBuffer;
		std::cout << ", Recv Size : " << recvSize << std::endl;
	}
}