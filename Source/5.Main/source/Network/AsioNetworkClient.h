#pragma once
#ifndef ASIO_NETWORK_CLIENT_H
#define ASIO_NETWORK_CLIENT_H

// Boost configuration for pure header-only usage
#ifndef BOOST_ALL_NO_LIB
#define BOOST_ALL_NO_LIB 1
#endif
#ifndef BOOST_SYSTEM_NO_LIB
#define BOOST_SYSTEM_NO_LIB 1
#endif
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY 1
#endif
#ifndef BOOST_ASIO_NO_DEPRECATED
#define BOOST_ASIO_NO_DEPRECATED 1
#endif

#if defined(_WIN32) || defined(WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <boost/asio.hpp>

#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include <cstdint>
#include <cstring>

#if !defined(__ANDROID__) && !defined(MU_IOS)
#include <windows.h>
#include <winsock2.h>
#else
#include "Platform/PlatformDefs.h"
#endif

#include "android/AndroidNetwork.h"

class AsioNetworkClient
{
public:
    AsioNetworkClient();
    ~AsioNetworkClient();

    // Lifecycle
    bool Connect(const char* host, uint16_t port, uint32_t timeoutMs = 5000);
    void Disconnect();
    bool IsConnected() const;

    // Send packet
    int Send(const uint8_t* data, size_t size);

    // Receive packets (called by game thread)
    uint8_t* PopReceivedPacket();
    void ClearPacketQueue();

    // Status
    bool HasServerLost() const;
    void ClearServerLost();
    uint16_t GetConnectedPort() const;
    SOCKET GetNativeSocket();

    // Telemetry & Stats (for Android Overlay & debug info)
    AndroidNetworkOverlayStats GetOverlayStats();
    void SetLatencyMs(int latencyMs);

private:
    void StartAsyncRead();
    void OnDataReceived(const boost::system::error_code& ec, size_t bytesTransferred);
    void ProcessStreamBuffer();

    void PostSendPacket(std::vector<uint8_t> packet);
    void StartAsyncWrite();
    void OnDataWritten(const boost::system::error_code& ec, size_t bytesTransferred);

    void HandleDisconnect(const std::string& reason);
    void UpdateTrafficStatsLocked(uint64_t nowMs);

private:
    boost::asio::io_context m_ioContext;
    using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    std::unique_ptr<WorkGuard> m_workGuard;
    std::unique_ptr<std::thread> m_workerThread;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::resolver m_resolver;

    std::atomic<bool> m_connected{ false };
    std::atomic<bool> m_serverLost{ false };
    std::atomic<uint16_t> m_connectedPort{ 0 };
    std::string m_connectedHost;

    // Receiving
    static constexpr size_t RAW_RECV_BUF_SIZE = 8192;
    uint8_t m_rawRecvBuffer[RAW_RECV_BUF_SIZE];
    std::vector<uint8_t> m_streamBuffer;

    std::queue<std::vector<uint8_t>> m_packetQueue;
    mutable std::mutex m_packetQueueMutex;
    std::vector<uint8_t> m_activeReadPacket;

    // Sending
    std::deque<std::vector<uint8_t>> m_sendQueue;
    std::mutex m_sendQueueMutex;
    bool m_isSending{ false };

    // Stats
    std::atomic<uint64_t> m_totalSentBytes{ 0 };
    std::atomic<uint64_t> m_totalRecvBytes{ 0 };
    uint64_t m_lastSampleTimeMs{ 0 };
    uint64_t m_lastSentBytes{ 0 };
    uint64_t m_lastRecvBytes{ 0 };
    float m_uploadKBps{ 0.0f };
    float m_downloadKBps{ 0.0f };
    std::atomic<int> m_latencyMs{ -1 };
    mutable std::mutex m_statsMutex;
};

#endif // ASIO_NETWORK_CLIENT_H
