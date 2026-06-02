#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "../TP_Server/protocol_2026.h"

#include <string>
#include <unordered_map>
#include <vector>

class NetworkClient {
public:
    enum class ConnectionState
    {
        Disconnected,
        Connecting,
        Connected
    };

    struct AvatarInfo
    {
        int playerId{};
        int visualId{};
        short x{};
        short y{};
        int hp{};
        int maxHp{};
        unsigned long long exp{};
        unsigned char level{};
    };

    struct NetworkObject
    {
        int objectId{};
        int visualId{};
        std::string name;
        short x{};
        short y{};
        int hp{};
        int maxHp{};
        unsigned long long exp{};
        unsigned char level{};
    };

    NetworkClient() = default;
    ~NetworkClient();

    bool Connect(const std::string& serverAddress, unsigned short port, const std::string& username);
    void Pump();
    void Disconnect(bool sendLogout = true);

    ConnectionState GetState() const { return _state; }
    const std::string& GetLastErrorMessage() const { return _lastErrorMessage; }
    const std::string& GetLastServerMessage() const { return _lastServerMessage; }
    bool IsLoginAccepted() const { return _loginAccepted; }
    bool HasAvatarInfo() const { return _hasAvatarInfo; }
    const AvatarInfo& GetAvatarInfo() const { return _avatarInfo; }
    const std::unordered_map<int, NetworkObject>& GetObjects() const { return _objects; }

private:
    bool InitializeWinsock();
    bool StartNonBlockingConnect(const std::string& serverAddress, unsigned short port);
    void CheckConnectionProgress();
    void ReceiveAvailableData();
    void FlushSendQueue();
    void ParseReceivedPackets();
    void ProcessPacket(const char* packet, int packetSize);
    void SendLogin();
    void SendLogout();
    void QueuePacket(const void* packet, int packetSize);
    void CloseSocket();
    void SetError(const std::string& message, int errorCode);

private:
    SOCKET          _socket = INVALID_SOCKET;
    ConnectionState _state{ ConnectionState::Disconnected };

    bool _winsockInitialized{};
    bool _loginAccepted{};
    bool _hasAvatarInfo{};
    std::string _username;
    std::string _lastErrorMessage;
    std::string _lastServerMessage;
    AvatarInfo _avatarInfo;
    std::unordered_map<int, NetworkObject> _objects;
    std::vector<char> _receiveBuffer;
    std::vector<char> _sendBuffer;
};
