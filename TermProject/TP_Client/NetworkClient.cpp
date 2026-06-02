#include "NetworkClient.h"

#include <Windows.h>

#include <algorithm>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

static std::string FixedString(const char* text, size_t maxLength)
{
    size_t length = 0;
    while (maxLength > length && '\0' != text[length])
        ++length;

    return std::string(text, length);
}

static void DebugLog(const std::string& message)
{
    ::OutputDebugStringA((message + "\n").c_str());
}

NetworkClient::~NetworkClient()
{
    Disconnect(false);

    if (_winsockInitialized)
    {
        ::WSACleanup();
        _winsockInitialized = false;
    }
}

bool NetworkClient::Connect(const std::string& serverAddress, unsigned short port, const std::string& username)
{
    Disconnect(false);

    _username = username;
    _lastErrorMessage.clear();
    _lastServerMessage.clear();
    _loginAccepted = false;
    _hasAvatarInfo = false;
    _objects.clear();
    _receiveBuffer.clear();
    _sendBuffer.clear();

    if (!InitializeWinsock())
        return false;

    return StartNonBlockingConnect(serverAddress, port);
}

void NetworkClient::Pump()
{
    if (ConnectionState::Connecting == _state)
        CheckConnectionProgress();

    if (ConnectionState::Connected != _state)
        return;

    FlushSendQueue();
    if (ConnectionState::Connected != _state)
        return;

    ReceiveAvailableData();
    if (ConnectionState::Connected != _state)
        return;

    ParseReceivedPackets();
}

void NetworkClient::Disconnect(bool sendLogout)
{
    if (INVALID_SOCKET != _socket && ConnectionState::Connected == _state && sendLogout)
    {
        SendLogout();
        FlushSendQueue();
    }

    CloseSocket();
}

bool NetworkClient::InitializeWinsock()
{
    if (_winsockInitialized)
        return true;

    WSADATA wsaData{};
    const int result = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (0 != result)
    {
        SetError("WSAStartup failed", result);
        return false;
    }

    _winsockInitialized = true;
    return true;
}

bool NetworkClient::StartNonBlockingConnect(const std::string& serverAddress, unsigned short port)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* addressInfo = nullptr;
    const std::string portText = std::to_string(port);
    int result = ::getaddrinfo(serverAddress.c_str(), portText.c_str(), &hints, &addressInfo);
    if (0 != result)
    {
        SetError("getaddrinfo failed", result);
        return false;
    }

    _socket = ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
    if (INVALID_SOCKET == _socket)
    {
        SetError("socket failed", ::WSAGetLastError());
        ::freeaddrinfo(addressInfo);
        return false;
    }

    u_long nonBlocking = 1;
    if (SOCKET_ERROR == ::ioctlsocket(_socket, FIONBIO, &nonBlocking))
    {
        SetError("ioctlsocket(FIONBIO) failed", ::WSAGetLastError());
        ::freeaddrinfo(addressInfo);
        CloseSocket();
        return false;
    }

    result = ::connect(_socket, addressInfo->ai_addr, static_cast<int>(addressInfo->ai_addrlen));
    ::freeaddrinfo(addressInfo);

    if (0 == result)
    {
        _state = ConnectionState::Connected;
        SendLogin();
        DebugLog("Network connected immediately.");
        return true;
    }

    const int errorCode = ::WSAGetLastError();
    if (WSAEWOULDBLOCK == errorCode || WSAEINPROGRESS == errorCode || WSAEINVAL == errorCode)
    {
        _state = ConnectionState::Connecting;
        DebugLog("Network connecting...");
        return true;
    }

    SetError("connect failed", errorCode);
    CloseSocket();
    return false;
}

void NetworkClient::CheckConnectionProgress()
{
    fd_set writeSet;
    fd_set exceptSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&exceptSet);
    FD_SET(_socket, &writeSet);
    FD_SET(_socket, &exceptSet);

    timeval timeout{};
    const int result = ::select(0, nullptr, &writeSet, &exceptSet, &timeout);
    if (0 == result)
        return;

    if (SOCKET_ERROR == result)
    {
        SetError("select failed", ::WSAGetLastError());
        CloseSocket();
        return;
    }

    int socketError = 0;
    int socketErrorLength = sizeof(socketError);
    if (SOCKET_ERROR == ::getsockopt(_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &socketErrorLength))
    {
        SetError("getsockopt(SO_ERROR) failed", ::WSAGetLastError());
        CloseSocket();
        return;
    }

    if (FD_ISSET(_socket, &exceptSet) || 0 != socketError)
    {
        SetError("connect failed", socketError);
        CloseSocket();
        return;
    }

    if (FD_ISSET(_socket, &writeSet))
    {
        _state = ConnectionState::Connected;
        SendLogin();
        DebugLog("Network connected.");
    }
}

void NetworkClient::ReceiveAvailableData()
{
    char buffer[BUF_SIZE]{};

    while (true)
    {
        const int received = ::recv(_socket, buffer, sizeof(buffer), 0);
        if (0 < received)
        {
            _receiveBuffer.insert(_receiveBuffer.end(), buffer, buffer + received);
            continue;
        }

        if (0 == received)
        {
            SetError("server closed connection", 0);
            CloseSocket();
            return;
        }

        const int errorCode = ::WSAGetLastError();
        if (WSAEWOULDBLOCK == errorCode)
            return;

        SetError("recv failed", errorCode);
        CloseSocket();
        return;
    }
}

void NetworkClient::FlushSendQueue()
{
    while (false == _sendBuffer.empty())
    {
        const int sent = ::send(_socket, _sendBuffer.data(), static_cast<int>(_sendBuffer.size()), 0);
        if (0 < sent)
        {
            _sendBuffer.erase(_sendBuffer.begin(), _sendBuffer.begin() + sent);
            continue;
        }

        const int errorCode = ::WSAGetLastError();
        if (WSAEWOULDBLOCK == errorCode)
            return;

        SetError("send failed", errorCode);
        CloseSocket();
        return;
    }
}

void NetworkClient::ParseReceivedPackets()
{
    constexpr int PACKET_HEADER_SIZE = sizeof(unsigned char) + sizeof(PACKET_TYPE);

    while (PACKET_HEADER_SIZE <= static_cast<int>(_receiveBuffer.size()))
    {
        const auto packetSize = static_cast<unsigned char>(_receiveBuffer[0]);
        if (PACKET_HEADER_SIZE > packetSize)
        {
            SetError("invalid packet size", packetSize);
            CloseSocket();
            return;
        }

        if (packetSize > static_cast<int>(_receiveBuffer.size()))
            return;

        ProcessPacket(_receiveBuffer.data(), packetSize);
        _receiveBuffer.erase(_receiveBuffer.begin(), _receiveBuffer.begin() + packetSize);
    }
}

void NetworkClient::ProcessPacket(const char* packet, int packetSize)
{
    PACKET_TYPE packetType{};
    std::memcpy(&packetType, packet + sizeof(unsigned char), sizeof(packetType));

    switch (packetType)
    {
    case S2C_LOGIN_RESULT:
    {
        if (static_cast<int>(sizeof(S2C_LoginResult)) > packetSize)
            return;

        S2C_LoginResult loginResult{};
        std::memcpy(&loginResult, packet, sizeof(loginResult));
        _loginAccepted = loginResult.success;
        _lastServerMessage = FixedString(loginResult.message, sizeof(loginResult.message));
        DebugLog("S2C_LOGIN_RESULT: " + _lastServerMessage);
        break;
    }
    case S2C_AVATAR_INFO:
    {
        if (static_cast<int>(sizeof(S2C_AvatarInfo)) > packetSize)
            return;

        S2C_AvatarInfo avatar{};
        std::memcpy(&avatar, packet, sizeof(avatar));
        _avatarInfo.playerId = avatar.playerId;
        _avatarInfo.visualId = avatar.visualId;
        _avatarInfo.x = avatar.x;
        _avatarInfo.y = avatar.y;
        _avatarInfo.hp = avatar.hp;
        _avatarInfo.maxHp = avatar.max_hp;
        _avatarInfo.exp = avatar.exp;
        _avatarInfo.level = avatar.level;
        _hasAvatarInfo = true;
        DebugLog("S2C_AVATAR_INFO received.");
        break;
    }
    case S2C_ADD_OBJECT:
    {
        if (static_cast<int>(sizeof(S2C_AddObject)) > packetSize)
            return;

        S2C_AddObject addObject{};
        std::memcpy(&addObject, packet, sizeof(addObject));

        NetworkObject object{};
        object.objectId = addObject.object_id;
        object.visualId = addObject.visual_id;
        object.name = FixedString(addObject.obj_name, sizeof(addObject.obj_name));
        object.x = addObject.x;
        object.y = addObject.y;
        object.hp = addObject.hp;
        object.maxHp = addObject.max_hp;
        object.exp = addObject.exp;
        object.level = addObject.level;
        _objects[object.objectId] = object;
        break;
    }
    case S2C_REMOVE_OBJECT:
    {
        if (static_cast<int>(sizeof(S2C_RemoveObject)) > packetSize)
            return;

        S2C_RemoveObject removeObject{};
        std::memcpy(&removeObject, packet, sizeof(removeObject));
        _objects.erase(removeObject.object_id);
        break;
    }
    case S2C_MOVE_OBJECT:
    {
        if (static_cast<int>(sizeof(S2C_MoveObject)) > packetSize)
            return;

        S2C_MoveObject moveObject{};
        std::memcpy(&moveObject, packet, sizeof(moveObject));
        auto& object = _objects[moveObject.object_id];
        object.objectId = moveObject.object_id;
        object.x = moveObject.x;
        object.y = moveObject.y;
        break;
    }
    case S2C_CHAT_MESSAGE:
    {
        if (static_cast<int>(sizeof(S2C_ChatMessage)) > packetSize)
            return;

        S2C_ChatMessage chatMessage{};
        std::memcpy(&chatMessage, packet, sizeof(chatMessage));
        _lastServerMessage = FixedString(chatMessage.message, sizeof(chatMessage.message));
        DebugLog("S2C_CHAT_MESSAGE: " + _lastServerMessage);
        break;
    }
    case S2C_STATUS_CHANGE:
    {
        if (static_cast<int>(sizeof(S2C_StatusChange)) > packetSize)
            return;

        S2C_StatusChange statusChange{};
        std::memcpy(&statusChange, packet, sizeof(statusChange));
        auto& object = _objects[statusChange.object_id];
        object.objectId = statusChange.object_id;
        object.hp = statusChange.hp;
        object.maxHp = statusChange.max_hp;
        object.exp = statusChange.exp;
        object.level = statusChange.level;
        break;
    }
    default:
        SetError("unknown packet type", static_cast<int>(packetType));
        CloseSocket();
        break;
    }
}

void NetworkClient::SendLogin()
{
    C2S_Login login{};
    login.size = static_cast<unsigned char>(sizeof(login));
    login.type = C2S_LOGIN;
    strncpy_s(login.username, _username.c_str(), _TRUNCATE);

    QueuePacket(&login, sizeof(login));
}

void NetworkClient::SendLogout()
{
    C2S_Logout logout{};
    logout.size = static_cast<unsigned char>(sizeof(logout));
    logout.type = C2S_LOGOUT;

    QueuePacket(&logout, sizeof(logout));
}

void NetworkClient::QueuePacket(const void* packet, int packetSize)
{
    const auto* bytes = static_cast<const char*>(packet);
    _sendBuffer.insert(_sendBuffer.end(), bytes, bytes + packetSize);
}

void NetworkClient::CloseSocket()
{
    if (INVALID_SOCKET != _socket)
    {
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
    }

    _state = ConnectionState::Disconnected;
}

void NetworkClient::SetError(const std::string& message, int errorCode)
{
    _lastErrorMessage = message + " (" + std::to_string(errorCode) + ")";
    DebugLog(_lastErrorMessage);
}
