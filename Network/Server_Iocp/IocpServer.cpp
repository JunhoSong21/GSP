#include <iostream>
#include <WS2tcpip.h>
#include <unordered_map>
#include <memory>
#pragma comment(lib, "WS2_32.lib")

#include <MSWSock.h>
#pragma comment(lib, "MSWSock.lib")

constexpr int SERVER_PORT	= 3000;
constexpr int BUF_SIZE		= 4096;

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

enum IOType {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT
};

class EXP_OVER {
public:
	WSAOVERLAPPED	_over;
	IOType			_iotype;
	char			_buff[BUF_SIZE];
	WSABUF			_wsa_buf;

public:
	EXP_OVER()
	{
		::ZeroMemory(&_over, sizeof(_over));
		_iotype = IO_ACCEPT;
		::ZeroMemory(_buff, BUF_SIZE);
		_wsa_buf.buf = _buff;
		_wsa_buf.len = BUF_SIZE;
	}

	EXP_OVER(IOType iotype) : _iotype(iotype)
	{
		::ZeroMemory(&_over, sizeof(_over));
		::ZeroMemory(_buff, BUF_SIZE);
		_wsa_buf.buf = _buff;
		_wsa_buf.len = BUF_SIZE;
	}
};

class SESSION {
private:
	SOCKET			_c_socket;
	long long		_c_id;

public:
	EXP_OVER		_recv_over;
	CHAR			_recv_mess[BUF_SIZE];

	SESSION() { exit(-1); }
	SESSION(int id, SOCKET so) : _c_socket(so), _c_id(id)
	{
        _recv_over._wsa_buf.buf = _recv_mess;
		_recv_over._wsa_buf.len = BUF_SIZE;
		::ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
		_recv_over._iotype = IO_RECV;
	}

	~SESSION()
	{
		closesocket(_c_socket);
	}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		WSARecv(_c_socket, &_recv_over._wsa_buf, 1, 0, &recv_flag, &_recv_over._over, nullptr);
	}

	void do_send(int sender_id, int num_bytes, char* mess)
	{
		EXP_OVER* o = new EXP_OVER(IO_SEND);
		o->_buff[0] = num_bytes + 2;
		o->_buff[1] = sender_id;
		::memcpy(o->_buff + 2, mess, num_bytes);
		// set the actual length to send (header + payload)
		o->_wsa_buf.len = num_bytes + 2;

		WSASend(_c_socket, &o->_wsa_buf, 1, 0, 0, &o->_over, nullptr);
	}
};

std::unordered_map<long long, std::unique_ptr<SESSION>> clients;

int main()
{
	std::wcout.imbue(std::locale("korean"));

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bind(server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(server, SOMAXCONN);

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, 0);
	CreateIoCompletionPort((HANDLE)server, h_iocp, 0, 0);

    EXP_OVER* accept_over = new EXP_OVER(IO_ACCEPT);
	SOCKET client_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	AcceptEx(server, client_socket, accept_over->_buff, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over->_over);

	for (int i = 1; ; ++i) {
		DWORD num_bytes;
		ULONG_PTR key;
		LPOVERLAPPED over;
		GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		if (over == nullptr) {
			//err_display();
			continue;
		}

		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);

		switch (exp_over->_iotype) {
		case IO_ACCEPT:
		{
			std::cout << "Accept Success" << std::endl;

			// the overlapped pointer corresponds to the accept operation
			EXP_OVER* ao = exp_over;

			// associate the accepted socket with IOCP and create session
			CreateIoCompletionPort((HANDLE)client_socket, h_iocp, i, 0);
			clients.emplace(i, std::make_unique<SESSION>(i, client_socket));
			clients[i]->do_recv();

			// free the accept overlapped structure
			delete ao;

			// prepare next accept: allocate new EXP_OVER and new socket
			accept_over = new EXP_OVER(IO_ACCEPT);
			client_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			AcceptEx(server, client_socket, accept_over->_buff, 0,
				sizeof(SOCKADDR_IN) + 16, sizeof(sockaddr_in) + 16,
				NULL, &accept_over->_over);
			break;
		}
		case IO_RECV:
		{
			std::cout << "Received Message" << std::endl;

			int client_id = static_cast<int>(key);
         std::cout << "Client[" << client_id << "] sent: " << clients[client_id]->_recv_over._buff << std::endl;

			for (auto& cl : clients)
				cl.second->do_send(client_id, num_bytes, clients[client_id]->_recv_over._buff);
			clients[client_id]->do_recv();
			break;
		}
		case IO_SEND:
		{
			EXP_OVER* o = reinterpret_cast<EXP_OVER*>(over);
			delete o;
			break;
		}
		default:
			std::cout << "Unkown IOType" << std::endl;
			continue;
		}
	}

	closesocket(server);
	WSACleanup();
}
