#include <iostream>
#include <WS2tcpip.h>
#include <unordered_map>
#pragma comment(lib, "WS2_32.lib")

#include <MSWSock.h>
#pragma comment(lib, "MSWSock.lib")

constexpr int SERVER_PORT = 3000;
constexpr int BUF_SIZE = 4096;

enum IOType {
	IO_SEND,
	IO_RECV,
	IO_ACCEPT
};

class EXP_OVER {
public:
	WSAOVERLAPPED m_over;
	IOType	m_iotype;
	WSABUF	m_wsa_buf;
	char  m_buff[BUF_SIZE];
	EXP_OVER() = default;
	EXP_OVER(IOType iotype) : m_iotype(iotype)
	{
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa_buf.buf = m_buff;
		m_wsa_buf.len = BUF_SIZE;
	}
};

class SESSION {
private:
	SOCKET			_c_socket;
	
	long long		_c_id;

public:
	EXP_OVER		_recv_over;
	CHAR _recv_mess[BUF_SIZE];
	SESSION() { exit(-1); }
	SESSION(int id, SOCKET so) : _c_id(id), _c_socket(so)
	{
		_recv_wsa_buf[0].buf = c_mess;
	}
	~SESSION()
	{
		closesocket(_c_socket);
	}
	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over.m_over, 0, sizeof(_recv_over.m_over));
		WSARecv(_c_socket, &_recv_over.m_wsa_buf, 1, 0, &recv_flag, &_recv_over.m_over, nullptr);
	}
	void do_send(int sender_id, int num_bytes, char* mess)
	{
		EXP_OVER* o = new EXP_OVER(IO_SEND);
		o->m_buff[0] = num_bytes + 2;
		o->m_buff[1] = sender_id;
		::memcpy(o->m_buff + 2, mess, num_bytes);
		WSASend(_c_socket, &o->m_wsa_buf, 1, 0, 0, &o->m_over, nullptr);
	}
};

std::unordered_map<long long, SESSION> clients;

int main()
{
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

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE);
	CreateIoCompletionPort((HANDLE)server, h_iocp, 0, 0);

	SOCKET client_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	EXP_OVER accept_over(IO_ACCEPT);

	AcceptEx(server, client_socket, accept_over.m_buff, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over.m_over);

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
		switch (exp_over->m_iotype) {
		case IO_ACCEPT:
		{
			CreateIoCompletionPort((HANDLE)client_socket, );
			clients.try_emplace(i, i, client_socket);
			clients[i].do_recv();
			SOCKET client_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			AcceptEx(server, client_socket, accept_over.m_buff, 0,
				sizeof(SOCKADDR_IN) + 16, sizeof(sockaddr_in) + 16,
				NULL, &accept_over.m_over);
			break;
		}
		case IO_RECV:
		{
			std::cout << "Received Message" << std::endl;
			int client_id = static_cast<int>(key);
			std::cout << "Client[" << client_id << "] sent: " << clients[client_id].c_mess << endl;

			for (auto& cl : clients)
				cl.second.do_send(client_id, num_bytes, clients[client_id]._recv_over.m_buff);
			clients[client_id].do_recv();
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
			return -1;
		}
	}

	closesocket(server);
	WSACleanup();
}