#include "stdafx.h"
#include "AsioNetworkClient.h"
#include "Protect.h"
#include "Utilities/Log/ErrorReport.h"
#include "Utilities/Log/muConsoleDebug.h"

static uint64_t GetCurrentTimeMs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

AsioNetworkClient::AsioNetworkClient()
    : m_socket(m_ioContext)
    , m_resolver(m_ioContext)
{
    std::memset(m_rawRecvBuffer, 0, sizeof(m_rawRecvBuffer));
}

AsioNetworkClient::~AsioNetworkClient()
{
    Disconnect();
}

bool AsioNetworkClient::Connect(const char* host, uint16_t port, uint32_t /*timeoutMs*/)
{
    Disconnect();

    m_serverLost.store(false);
    m_connectedHost = (host ? host : "");
    m_connectedPort.store(port);

    // Re-create socket on this io_context to ensure clean state after Disconnect()
    {
        boost::system::error_code ignored;
        if (m_socket.is_open())
        {
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
            m_socket.close(ignored);
        }
        // Move-assign a fresh socket to replace the closed one
        m_socket = boost::asio::ip::tcp::socket(m_ioContext);
    }

    boost::system::error_code ec;
    auto endpoints = m_resolver.resolve(m_connectedHost, std::to_string(port), ec);
    if (ec)
    {
        g_ErrorReport.Write("[AsioNetwork] DNS resolve failed for %s:%d: %s\r\n",
            m_connectedHost.c_str(), port, ec.message().c_str());
        return false;
    }

    boost::asio::connect(m_socket, endpoints, ec);
    if (ec)
    {
        g_ErrorReport.Write("[AsioNetwork] Connect failed to %s:%d: %s\r\n",
            m_connectedHost.c_str(), port, ec.message().c_str());
        boost::system::error_code closeEc;
        m_socket.close(closeEc);
        return false;
    }

    m_socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);

    m_connected.store(true);
    m_connectedPort.store(port);
    g_ErrorReport.Write("[AsioNetwork] Connected to %s:%d successfully\r\n",
        m_connectedHost.c_str(), port);

    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_lastSampleTimeMs = GetCurrentTimeMs();
        m_lastSentBytes = m_totalSentBytes.load();
        m_lastRecvBytes = m_totalRecvBytes.load();
        m_uploadKBps = 0.0f;
        m_downloadKBps = 0.0f;
    }

    m_ioContext.restart();
    m_workGuard = std::make_unique<WorkGuard>(boost::asio::make_work_guard(m_ioContext));
    m_workerThread = std::make_unique<std::thread>([this]() {
        try
        {
            m_ioContext.run();
        }
        catch (const std::exception& ex)
        {
            g_ErrorReport.Write("[AsioNetwork] io_context exception: %s\r\n", ex.what());
        }
    });

    StartAsyncRead();

    return true;
}

void AsioNetworkClient::Disconnect()
{
    m_connected.store(false);

    boost::system::error_code ec;
    if (m_socket.is_open())
    {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }

    m_resolver.cancel();

    if (m_workGuard)
    {
        m_workGuard.reset();
    }

    m_ioContext.stop();

    if (m_workerThread)
    {
        if (m_workerThread->joinable())
        {
            if (m_workerThread->get_id() != std::this_thread::get_id())
            {
                m_workerThread->join();
            }
            else
            {
                m_workerThread->detach();
            }
        }
        m_workerThread.reset();
    }

    m_ioContext.restart();

    ClearPacketQueue();

    {
        std::lock_guard<std::mutex> lock(m_sendQueueMutex);
        m_sendQueue.clear();
        m_isSending = false;
    }

    m_streamBuffer.clear();
    m_connectedPort.store(0);
}

bool AsioNetworkClient::IsConnected() const
{
    return m_connected.load() && m_socket.is_open();
}

int AsioNetworkClient::Send(const uint8_t* data, size_t size)
{
    if (!IsConnected() || data == nullptr || size == 0)
    {
        return 0;
    }

    std::vector<uint8_t> packet(data, data + size);

#if(ENCRYPT_STATE==1)
    uint16_t port = m_connectedPort.load();
    if (port >= gProtect.m_MainInfo.GSPortMin && port <= gProtect.m_MainInfo.GSPortMax)
    {
        gProtect.EncryptData(packet.data(), static_cast<int>(packet.size()));
    }
#endif

    PostSendPacket(std::move(packet));
    return 1;
}

uint8_t* AsioNetworkClient::PopReceivedPacket()
{
    std::lock_guard<std::mutex> lock(m_packetQueueMutex);
    if (m_packetQueue.empty())
    {
        return nullptr;
    }

    m_activeReadPacket = std::move(m_packetQueue.front());
    m_packetQueue.pop();
    return m_activeReadPacket.data();
}

void AsioNetworkClient::ClearPacketQueue()
{
    std::lock_guard<std::mutex> lock(m_packetQueueMutex);
    std::queue<std::vector<uint8_t>> empty;
    std::swap(m_packetQueue, empty);
    m_activeReadPacket.clear();
}

bool AsioNetworkClient::HasServerLost() const
{
    return m_serverLost.load();
}

void AsioNetworkClient::ClearServerLost()
{
    m_serverLost.store(false);
}

uint16_t AsioNetworkClient::GetConnectedPort() const
{
    return m_connectedPort.load();
}

SOCKET AsioNetworkClient::GetNativeSocket()
{
    if (!m_socket.is_open())
    {
        return INVALID_SOCKET;
    }
    return static_cast<SOCKET>(m_socket.native_handle());
}

AndroidNetworkOverlayStats AsioNetworkClient::GetOverlayStats()
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    UpdateTrafficStatsLocked(GetCurrentTimeMs());

    AndroidNetworkOverlayStats stats{};
    stats.connected = IsConnected();
    stats.latencyMs = m_latencyMs.load();
    stats.uploadKBps = m_uploadKBps;
    stats.downloadKBps = m_downloadKBps;
    return stats;
}

void AsioNetworkClient::SetLatencyMs(int latencyMs)
{
    m_latencyMs.store(latencyMs);
}

void AsioNetworkClient::StartAsyncRead()
{
    if (!IsConnected())
    {
        return;
    }

    m_socket.async_read_some(
        boost::asio::buffer(m_rawRecvBuffer, sizeof(m_rawRecvBuffer)),
        [this](const boost::system::error_code& ec, size_t bytesTransferred) {
            OnDataReceived(ec, bytesTransferred);
        });
}

void AsioNetworkClient::OnDataReceived(const boost::system::error_code& ec, size_t bytesTransferred)
{
    if (ec)
    {
        if (ec != boost::asio::error::operation_aborted)
        {
            HandleDisconnect("read error: " + ec.message());
        }
        return;
    }

    if (bytesTransferred > 0)
    {
        m_totalRecvBytes += bytesTransferred;
        {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            UpdateTrafficStatsLocked(GetCurrentTimeMs());
        }

#if(ENCRYPT_STATE==1)
        uint16_t port = m_connectedPort.load();
        if (port >= gProtect.m_MainInfo.GSPortMin && port <= gProtect.m_MainInfo.GSPortMax)
        {
            gProtect.DecryptData(reinterpret_cast<BYTE*>(m_rawRecvBuffer), static_cast<int>(bytesTransferred));
        }
#endif

        m_streamBuffer.insert(m_streamBuffer.end(), m_rawRecvBuffer, m_rawRecvBuffer + bytesTransferred);
        ProcessStreamBuffer();
    }

    StartAsyncRead();
}

void AsioNetworkClient::ProcessStreamBuffer()
{
    while (m_streamBuffer.size() >= 3)
    {
        uint8_t head = m_streamBuffer[0];
        size_t packetSize = 0;

        if (head == 0xC1 || head == 0xC3)
        {
            packetSize = static_cast<size_t>(m_streamBuffer[1]);
        }
        else if (head == 0xC2 || head == 0xC4)
        {
            packetSize = (static_cast<size_t>(m_streamBuffer[1]) << 8) | static_cast<size_t>(m_streamBuffer[2]);
        }
        else
        {
            // Out of sync: search for next packet head
            size_t nextHeader = 1;
            while (nextHeader < m_streamBuffer.size())
            {
                uint8_t b = m_streamBuffer[nextHeader];
                if (b == 0xC1 || b == 0xC2 || b == 0xC3 || b == 0xC4)
                {
                    break;
                }
                ++nextHeader;
            }
            m_streamBuffer.erase(m_streamBuffer.begin(), m_streamBuffer.begin() + nextHeader);
            continue;
        }

        if (packetSize < 3 || packetSize > 65535)
        {
            m_streamBuffer.clear();
            break;
        }

        if (m_streamBuffer.size() < packetSize)
        {
            // Incomplete packet; wait for next read
            break;
        }

        std::vector<uint8_t> packet(m_streamBuffer.begin(), m_streamBuffer.begin() + packetSize);
        m_streamBuffer.erase(m_streamBuffer.begin(), m_streamBuffer.begin() + packetSize);

        uint8_t pktHead = packet[0];
        size_t pktLen = packet.size();
        uint8_t pktSub = (pktLen > 2 ? packet[2] : 0);

        {
            std::lock_guard<std::mutex> lock(m_packetQueueMutex);
            m_packetQueue.push(std::move(packet));
        }
    }
}

void AsioNetworkClient::PostSendPacket(std::vector<uint8_t> packet)
{
    boost::asio::post(m_ioContext, [this, pkt = std::move(packet)]() mutable {
        bool writeInProgress = false;
        {
            std::lock_guard<std::mutex> lock(m_sendQueueMutex);
            writeInProgress = !m_sendQueue.empty() || m_isSending;
            m_sendQueue.push_back(std::move(pkt));
        }

        if (!writeInProgress)
        {
            StartAsyncWrite();
        }
    });
}

void AsioNetworkClient::StartAsyncWrite()
{
    std::vector<uint8_t>* currentPkt = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_sendQueueMutex);
        if (m_sendQueue.empty())
        {
            m_isSending = false;
            return;
        }
        m_isSending = true;
        currentPkt = &m_sendQueue.front();
    }

    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(currentPkt->data(), currentPkt->size()),
        [this](const boost::system::error_code& ec, size_t bytesWritten) {
            OnDataWritten(ec, bytesWritten);
        });
}

void AsioNetworkClient::OnDataWritten(const boost::system::error_code& ec, size_t bytesWritten)
{
    if (ec)
    {
        if (ec != boost::asio::error::operation_aborted)
        {
            HandleDisconnect("write error: " + ec.message());
        }
        return;
    }

    m_totalSentBytes += bytesWritten;
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        UpdateTrafficStatsLocked(GetCurrentTimeMs());
    }

    {
        std::lock_guard<std::mutex> lock(m_sendQueueMutex);
        if (!m_sendQueue.empty())
        {
            m_sendQueue.pop_front();
        }
        if (m_sendQueue.empty())
        {
            m_isSending = false;
            return;
        }
    }

    StartAsyncWrite();
}

void AsioNetworkClient::HandleDisconnect(const std::string& reason)
{
    bool wasConnected = m_connected.exchange(false);
    if (wasConnected)
    {
        m_serverLost.store(true);
        g_ErrorReport.Write("[AsioNetwork] Disconnected: %s\r\n", reason.c_str());
        boost::system::error_code ec;
        m_socket.close(ec);
    }
}

void AsioNetworkClient::UpdateTrafficStatsLocked(uint64_t nowMs)
{
    if (m_lastSampleTimeMs == 0)
    {
        m_lastSampleTimeMs = nowMs;
        m_lastSentBytes = m_totalSentBytes.load();
        m_lastRecvBytes = m_totalRecvBytes.load();
        return;
    }

    const uint64_t elapsedMs = nowMs - m_lastSampleTimeMs;
    if (elapsedMs < 500)
    {
        return;
    }

    const uint64_t curSent = m_totalSentBytes.load();
    const uint64_t curRecv = m_totalRecvBytes.load();

    const uint64_t sentDelta = curSent - m_lastSentBytes;
    const uint64_t recvDelta = curRecv - m_lastRecvBytes;

    m_uploadKBps = static_cast<float>(sentDelta) * 1000.0f / static_cast<float>(elapsedMs) / 1024.0f;
    m_downloadKBps = static_cast<float>(recvDelta) * 1000.0f / static_cast<float>(elapsedMs) / 1024.0f;

    m_lastSampleTimeMs = nowMs;
    m_lastSentBytes = curSent;
    m_lastRecvBytes = curRecv;
}
