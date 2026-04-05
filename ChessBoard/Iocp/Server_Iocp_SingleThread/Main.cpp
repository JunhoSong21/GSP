#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#include <MSWSock.h>
#pragma comment(lib, "MSWSock.lib")

#include <unordered_map>

constexpr int SERVER_PORT	= 3000;
constexpr int BUF_SIZE		= 200;

enum IOType {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT,
};

class Over_Exp {
public:
	WSAOVERLAPPED	over;
	IOType			_iotype;
	WSABUF			wsa_buf;
	char			buf[BUF_SIZE];

public:
	Over_Exp()
	{
		::ZeroMemory(&over, sizeof(over));
		wsa_buf.buf = buf;
		wsa_buf.len = BUF_SIZE;
	}

	Over_Exp(IOType iotype) : _iotype(iotype)
	{
		::ZeroMemory(&over, sizeof(over));
		wsa_buf.buf = buf;
		wsa_buf.len = BUF_SIZE;
	}
};

void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg;
	std::wcout << L" === 에러 " << lpMsgBuf << std::endl;
	while (true);   // 디버깅 용
	LocalFree(lpMsgBuf);
}

class Session {
private:
	SOCKET		_c_socket;
	LONGLONG	_id;

public:
	Over_Exp	recv_over;

public:
	Session()
	{
		
	}

	Session(SOCKET socket, int id) : _c_socket(socket), _id(id)
	{
	}

	~Session()
	{
		closesocket(_c_socket);
	}

	void Do_Recv()
	{
		DWORD recv_flag = 0;
		recv_over._iotype = IO_RECV;
		::memset(&recv_over.over, 0, sizeof(recv_over.over));
		::WSARecv(_c_socket, &recv_over.wsa_buf, 1, 0, &recv_flag, &recv_over.over, nullptr);
	}

	void Do_Send(int send_id, int num_bytes, char* msg)
	{
		Over_Exp* over_exp = new Over_Exp(IO_SEND);
		over_exp->buf[0] = num_bytes + 2;
		over_exp->buf[1] = send_id;
		::memcpy(over_exp->buf + 2, msg, num_bytes);

		over_exp->wsa_buf.len = num_bytes + 2;

		::WSASend(_c_socket, &over_exp->wsa_buf, 1, 0, 0, &over_exp->over, nullptr);
	}
};

std::unordered_map<LONGLONG, Session> clients;

int main()
{
	int result = 0;

	WSAData wsa_data{};
	result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (0 != result) {
		std::cout << "Failed to WSAStartup : " << result << std::endl;
		return 1;
	}

	SOCKET s_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == s_socket) {
		error_display(L"listen Socket", ::WSAGetLastError());
		return -1;
	}
	sockaddr_in server_addr{}; // 기본 초기화
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	result = ::bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (SOCKET_ERROR == result) {
		error_display(L"bind Failed", ::WSAGetLastError());
		return 1;
	}

	result = ::listen(s_socket, SOMAXCONN);
	if (SOCKET_ERROR == result) {
		error_display(L"listen Failed", ::WSAGetLastError());
		return 1;
	}

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), h_iocp, 0, 0);

	Over_Exp* accept_over = new Over_Exp(IOType::IO_ACCEPT);
	SOCKET c_socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == c_socket) {
		error_display(L"client Socket", ::WSAGetLastError());
		return -1;
	}

	::AcceptEx(s_socket, c_socket, accept_over->buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over->over);

	int i = 1; 
	while(true) {
		DWORD num_bytes = 0;
		ULONG_PTR key = 0;
		LPOVERLAPPED over = nullptr;
		GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		if (nullptr == over) {
			error_display(L"GQCS Error", ::WSAGetLastError());
			continue;
		}

		Over_Exp* over_exp = reinterpret_cast<Over_Exp*>(over);
		switch (over_exp->_iotype) {
		case IO_ACCEPT:
		{
			std::cout << "Client [" << i << "] Accept" << std::endl;

			CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), h_iocp, i, 0);
			clients.try_emplace(i, c_socket, i);
			clients[i].Do_Recv();

			c_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

			::AcceptEx(s_socket, c_socket, accept_over->buf, 0,
				sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
				NULL, &accept_over->over);

			++i;
			break;
		}
		case IO_RECV:
		{
			int client_id = static_cast<int>(key);

			if (num_bytes == 0) {
				std::cout << "Client [" << client_id << "] Disconnect" << std::endl;
				clients.erase(client_id);
				break;
			}

			std::cout << "Client [" << client_id << "] : "
				<< clients[client_id].recv_over.buf << std::endl;

			for (auto& cl : clients)
				cl.second.Do_Send(client_id, num_bytes, clients[client_id].recv_over.buf);
			clients[client_id].Do_Recv();

			break;
		}
		case IO_SEND:
		{
			Over_Exp* send_over = reinterpret_cast<Over_Exp*>(over);
			delete send_over;

			break;
		}
		default:
			std::cout << "Unknown IO type" << std::endl;
			break;
		}
	}

	closesocket(s_socket);
	WSACleanup();
}
