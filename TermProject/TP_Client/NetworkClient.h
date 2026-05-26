#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "../TP_Server/protocol_2026.h"

#include <string>
#include <unordered_map>
#include <vector>

class NetworkClient
{
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

    ConnectionState GetState() const { return state_; }
    const std::string& GetLastErrorMessage() const { return lastErrorMessage_; }
    const std::string& GetLastServerMessage() const { return lastServerMessage_; }
    bool IsLoginAccepted() const { return loginAccepted_; }
    bool HasAvatarInfo() const { return hasAvatarInfo_; }
    const AvatarInfo& GetAvatarInfo() const { return avatarInfo_; }
    const std::unordered_map<int, NetworkObject>& GetObjects() const { return objects_; }

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

    SOCKET socket_{ INVALID_SOCKET };
    ConnectionState state_{ ConnectionState::Disconnected };
    bool winsockInitialized_{};
    bool loginAccepted_{};
    bool hasAvatarInfo_{};
    std::string username_;
    std::string lastErrorMessage_;
    std::string lastServerMessage_;
    AvatarInfo avatarInfo_;
    std::unordered_map<int, NetworkObject> objects_;
    std::vector<char> receiveBuffer_;
    std::vector<char> sendBuffer_;
};
