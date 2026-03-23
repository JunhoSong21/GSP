#include "NetworkHeader.h"
#include <string>
#include "ChessPawn.h"

#include <io.h>
#include <fcntl.h>

constexpr short SERVER_PORT = 3000;

constexpr int KEY_UP = 72;
constexpr int KEY_DOWN = 80;
constexpr int KEY_LEFT = 75;
constexpr int KEY_RIGHT = 77;

int main()
{
	// 유니코드 출력을 위한 _setmode
	if (-1 == _setmode(_fileno(stdout), _O_U16TEXT)) {
		std::wcout << L"_setmode() Fail" << L"\n";
	}
	//std::wcout.imbue(std::locale("korean"));
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::wcout << L"Failed to WSAStartup : " << result << L"\n";
		return 1;
	}

	std::string SERVER_IP;
	std::wcout << L"Input Server IP want to Connect : ";
	std::cin >> SERVER_IP;

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(SERVER_PORT);
	::inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr);

	result = ::WSAConnect(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), nullptr, nullptr, nullptr, nullptr);
	if (SOCKET_ERROR == result) {
		error_display(L"서버 연결 실패", ::WSAGetLastError());
		return 1;
	}

	ChessPawn* chessPawn = new ChessPawn;
	system("cls");

	while(true) {
		int recv_data[2] = {};
		WSABUF recv_wsa_buf = {};
		recv_wsa_buf.buf = reinterpret_cast<char*>(recv_data);
		recv_wsa_buf.len = sizeof(recv_data);

		DWORD recv_size = 0;
		DWORD recv_flag = 0;
		result = ::WSARecv(s_socket, &recv_wsa_buf, 1, &recv_size, &recv_flag, nullptr, nullptr);
		if (SOCKET_ERROR == result) {
			error_display(L"데이터 수신 실패", ::WSAGetLastError());
			return 1;
		}

		chessPawn->Draw(recv_data[0], recv_data[1]);
		//std::wcout << L"Recv Data : " << recv_data[0] << L", " << recv_data[1] << L"\n";

		//-------------------------------------------------------------

		int send_data = chessPawn->MoniteringKey();

		WSABUF send_wsa_buf = {};
		send_wsa_buf.buf = reinterpret_cast<char*>(&send_data);
		send_wsa_buf.len = sizeof(send_data);
		
		DWORD sent_size = 0;
		result = ::WSASend(s_socket, &send_wsa_buf, 1, &sent_size, 0, nullptr, nullptr);
		if (SOCKET_ERROR == result) {
			error_display(L"데이터 전송 실패", ::WSAGetLastError());
			return 1;
		}
	}

	delete chessPawn;
	closesocket(s_socket);
	WSACleanup();
}