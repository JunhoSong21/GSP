#include "NetworkClient.h"

#include <Windows.h>

#include <algorithm>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

static std::string FixedString(const char* text, size_t maxLength)
{
    size_t length = 0;
    while (length < maxLength && text[length] != '\0')
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

    if (winsockInitialized_)
    {
        ::WSACleanup();
        winsockInitialized_ = false;
    }
}

bool NetworkClient::Connect(const std::string& serverAddress, unsigned short port, const std::string& username)
{
    Disconnect(false);

    username_ = username;
    lastErrorMessage_.clear();
    lastServerMessage_.clear();
    loginAccepted_ = false;
    hasAvatarInfo_ = false;
    objects_.clear();
    receiveBuffer_.clear();
    sendBuffer_.clear();

    if (!InitializeWinsock())
        return false;

    return StartNonBlockingConnect(serverAddress, port);
}

void NetworkClient::Pump()
{
    if (state_ == ConnectionState::Connecting)
        CheckConnectionProgress();

    if (state_ != ConnectionState::Connected)
        return;

    FlushSendQueue();
    if (state_ != ConnectionState::Connected)
        return;

    ReceiveAvailableData();
    if (state_ != ConnectionState::Connected)
        return;

    ParseReceivedPackets();
}

void NetworkClient::Disconnect(bool sendLogout)
{
    if (socket_ != INVALID_SOCKET && state_ == ConnectionState::Connected && sendLogout)
    {
        SendLogout();
        FlushSendQueue();
    }

    CloseSocket();
}

bool NetworkClient::InitializeWinsock()
{
    if (winsockInitialized_)
        return true;

    WSADATA wsaData{};
    const int result = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        SetError("WSAStartup failed", result);
        return false;
    }

    winsockInitialized_ = true;
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
    if (result != 0)
    {
        SetError("getaddrinfo failed", result);
        return false;
    }

    socket_ = ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
    if (socket_ == INVALID_SOCKET)
    {
        SetError("socket failed", ::WSAGetLastError());
        ::freeaddrinfo(addressInfo);
        return false;
    }

    u_long nonBlocking = 1;
    if (::ioctlsocket(socket_, FIONBIO, &nonBlocking) == SOCKET_ERROR)
    {
        SetError("ioctlsocket(FIONBIO) failed", ::WSAGetLastError());
        ::freeaddrinfo(addressInfo);
        CloseSocket();
        return false;
    }

    result = ::connect(socket_, addressInfo->ai_addr, static_cast<int>(addressInfo->ai_addrlen));
    ::freeaddrinfo(addressInfo);

    if (result == 0)
    {
        state_ = ConnectionState::Connected;
        SendLogin();
        DebugLog("Network connected immediately.");
        return true;
    }

    const int errorCode = ::WSAGetLastError();
    if (errorCode == WSAEWOULDBLOCK || errorCode == WSAEINPROGRESS || errorCode == WSAEINVAL)
    {
        state_ = ConnectionState::Connecting;
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
    FD_SET(socket_, &writeSet);
    FD_SET(socket_, &exceptSet);

    timeval timeout{};
    const int result = ::select(0, nullptr, &writeSet, &exceptSet, &timeout);
    if (result == 0)
        return;

    if (result == SOCKET_ERROR)
    {
        SetError("select failed", ::WSAGetLastError());
        CloseSocket();
        return;
    }

    int socketError = 0;
    int socketErrorLength = sizeof(socketError);
    if (::getsockopt(socket_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &socketErrorLength) == SOCKET_ERROR)
    {
        SetError("getsockopt(SO_ERROR) failed", ::WSAGetLastError());
        CloseSocket();
        return;
    }

    if (FD_ISSET(socket_, &exceptSet) || socketError != 0)
    {
        SetError("connect failed", socketError);
        CloseSocket();
        return;
    }

    if (FD_ISSET(socket_, &writeSet))
    {
        state_ = ConnectionState::Connected;
        SendLogin();
        DebugLog("Network connected.");
    }
}

void NetworkClient::ReceiveAvailableData()
{
    char buffer[BUF_SIZE]{};

    while (true)
    {
        const int received = ::recv(socket_, buffer, sizeof(buffer), 0);
        if (received > 0)
        {
            receiveBuffer_.insert(receiveBuffer_.end(), buffer, buffer + received);
            continue;
        }

        if (received == 0)
        {
            SetError("server closed connection", 0);
            CloseSocket();
            return;
        }

        const int errorCode = ::WSAGetLastError();
        if (errorCode == WSAEWOULDBLOCK)
            return;

        SetError("recv failed", errorCode);
        CloseSocket();
        return;
    }
}

void NetworkClient::FlushSendQueue()
{
    while (false == sendBuffer_.empty())
    {
        const int sent = ::send(socket_, sendBuffer_.data(), static_cast<int>(sendBuffer_.size()), 0);
        if (sent > 0)
        {
            sendBuffer_.erase(sendBuffer_.begin(), sendBuffer_.begin() + sent);
            continue;
        }

        const int errorCode = ::WSAGetLastError();
        if (errorCode == WSAEWOULDBLOCK)
            return;

        SetError("send failed", errorCode);
        CloseSocket();
        return;
    }
}

void NetworkClient::ParseReceivedPackets()
{
    constexpr int PACKET_HEADER_SIZE = sizeof(unsigned char) + sizeof(PACKET_TYPE);

    while (static_cast<int>(receiveBuffer_.size()) >= PACKET_HEADER_SIZE)
    {
        const auto packetSize = static_cast<unsigned char>(receiveBuffer_[0]);
        if (packetSize < PACKET_HEADER_SIZE)
        {
            SetError("invalid packet size", packetSize);
            CloseSocket();
            return;
        }

        if (static_cast<int>(receiveBuffer_.size()) < packetSize)
            return;

        ProcessPacket(receiveBuffer_.data(), packetSize);
        receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + packetSize);
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
        if (packetSize < sizeof(S2C_LoginResult))
            return;

        S2C_LoginResult loginResult{};
        std::memcpy(&loginResult, packet, sizeof(loginResult));
        loginAccepted_ = loginResult.success;
        lastServerMessage_ = FixedString(loginResult.message, sizeof(loginResult.message));
        DebugLog("S2C_LOGIN_RESULT: " + lastServerMessage_);
        break;
    }
    case S2C_AVATAR_INFO:
    {
        if (packetSize < sizeof(S2C_AvatarInfo))
            return;

        S2C_AvatarInfo avatar{};
        std::memcpy(&avatar, packet, sizeof(avatar));
        avatarInfo_.playerId = avatar.playerId;
        avatarInfo_.visualId = avatar.visualId;
        avatarInfo_.x = avatar.x;
        avatarInfo_.y = avatar.y;
        avatarInfo_.hp = avatar.hp;
        avatarInfo_.maxHp = avatar.max_hp;
        avatarInfo_.exp = avatar.exp;
        avatarInfo_.level = avatar.level;
        hasAvatarInfo_ = true;
        DebugLog("S2C_AVATAR_INFO received.");
        break;
    }
    case S2C_ADD_OBJECT:
    {
        if (packetSize < sizeof(S2C_AddObject))
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
        objects_[object.objectId] = object;
        break;
    }
    case S2C_REMOVE_OBJECT:
    {
        if (packetSize < sizeof(S2C_RemoveObject))
            return;

        S2C_RemoveObject removeObject{};
        std::memcpy(&removeObject, packet, sizeof(removeObject));
        objects_.erase(removeObject.object_id);
        break;
    }
    case S2C_MOVE_OBJECT:
    {
        if (packetSize < sizeof(S2C_MoveObject))
            return;

        S2C_MoveObject moveObject{};
        std::memcpy(&moveObject, packet, sizeof(moveObject));
        auto& object = objects_[moveObject.object_id];
        object.objectId = moveObject.object_id;
        object.x = moveObject.x;
        object.y = moveObject.y;
        break;
    }
    case S2C_CHAT_MESSAGE:
    {
        if (packetSize < sizeof(S2C_ChatMessage))
            return;

        S2C_ChatMessage chatMessage{};
        std::memcpy(&chatMessage, packet, sizeof(chatMessage));
        lastServerMessage_ = FixedString(chatMessage.message, sizeof(chatMessage.message));
        DebugLog("S2C_CHAT_MESSAGE: " + lastServerMessage_);
        break;
    }
    case S2C_STATUS_CHANGE:
    {
        if (packetSize < sizeof(S2C_StatusChange))
            return;

        S2C_StatusChange statusChange{};
        std::memcpy(&statusChange, packet, sizeof(statusChange));
        auto& object = objects_[statusChange.object_id];
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
    strncpy_s(login.username, username_.c_str(), _TRUNCATE);

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
    sendBuffer_.insert(sendBuffer_.end(), bytes, bytes + packetSize);
}

void NetworkClient::CloseSocket()
{
    if (socket_ != INVALID_SOCKET)
    {
        ::closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }

    state_ = ConnectionState::Disconnected;
}

void NetworkClient::SetError(const std::string& message, int errorCode)
{
    lastErrorMessage_ = message + " (" + std::to_string(errorCode) + ")";
    DebugLog(lastErrorMessage_);
}
