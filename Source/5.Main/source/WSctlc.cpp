#include "stdafx.h"
#include "wsctlc.h"
#include "wsctlc_addon.h"
#include "Protect.h"
#include "Reconnect.h"
#include "Network/AsioNetworkClient.h"
#include "Utilities/Log/ErrorReport.h"
#include "Utilities/Log/muConsoleDebug.h"

extern BOOL g_bGameServerConnected;
extern unsigned short g_ServerPort;

CWsctlc::CWsctlc()
    : m_hWnd(NULL)
    , m_bGame(FALSE)
    , m_iMaxSockets(0)
    , m_socket(INVALID_SOCKET)
    , m_nSendBufLen(0)
    , m_nRecvBufLen(0)
    , m_LogPrint(0)
    , m_logfp(NULL)
    , m_pPacketQueue(new CPacketQueue)
    , m_pClient(std::make_unique<AsioNetworkClient>())
{
    std::memset(m_SendBuf, 0, sizeof(m_SendBuf));
    std::memset(m_RecvBuf, 0, sizeof(m_RecvBuf));
}

CWsctlc::~CWsctlc()
{
    Close();
    if (m_pPacketQueue)
    {
        delete m_pPacketQueue;
        m_pPacketQueue = nullptr;
    }
    LogPrintOff();
}

BOOL CWsctlc::Startup()
{
#if !defined(__ANDROID__) && !defined(MU_IOS)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
#endif
    return TRUE;
}

void CWsctlc::Cleanup()
{
    Close();
#if !defined(__ANDROID__) && !defined(MU_IOS)
    WSACleanup();
#endif
}

int CWsctlc::Create(HWND hWnd, BOOL bGame)
{
    m_hWnd = hWnd;
    m_bGame = bGame;
    if (m_bGame)
    {
        g_bGameServerConnected = FALSE;
    }
    return TRUE;
}

BOOL CWsctlc::Close()
{
    if (m_bGame)
    {
#if(UseReconnect)
        if (SceneFlag == MAIN_SCENE && g_pReconnect && g_pReconnect->CheckSocketPort(m_socket))
        {
            g_pReconnect->ReconnectOnCloseSocket();
        }
#endif
        g_bGameServerConnected = FALSE;
    }

    if (m_pClient)
    {
        m_pClient->Disconnect();
        m_pClient.reset();
    }

    m_socket = INVALID_SOCKET;
    m_nSendBufLen = 0;
    m_nRecvBufLen = 0;

    if (m_pPacketQueue)
    {
        while (!m_pPacketQueue->IsEmpty())
        {
            m_pPacketQueue->PopPacket();
        }
    }

    return TRUE;
}

BOOL CWsctlc::Close(SOCKET& socket)
{
    Close();
    socket = INVALID_SOCKET;
    return TRUE;
}

SOCKET CWsctlc::GetSocket()
{
    if (m_pClient && m_pClient->IsConnected())
    {
        SOCKET native = m_pClient->GetNativeSocket();
        if (native != INVALID_SOCKET)
        {
            m_socket = native;
            return native;
        }
        if (m_socket == INVALID_SOCKET)
        {
            m_socket = static_cast<SOCKET>(1);
        }
        return m_socket;
    }
    m_socket = INVALID_SOCKET;
    return INVALID_SOCKET;
}

BOOL CWsctlc::Connect(char* ip_addr, unsigned short port, DWORD /*WinMsgNum*/)
{
    if (ip_addr == NULL || ip_addr[0] == '\0')
    {
        return FALSE;
    }

    if (m_socket != INVALID_SOCKET)
    {
        Close();
    }

    if (!m_pClient)
    {
        m_pClient = std::make_unique<AsioNetworkClient>();
    }

    bool success = m_pClient->Connect(ip_addr, port);
    if (!success)
    {
        g_ErrorReport.Write("[CWsctlc::Connect] Boost.Asio connect failed to %s:%d\r\n", ip_addr, port);
        m_socket = INVALID_SOCKET;
        return FALSE;
    }

    m_socket = m_pClient->GetNativeSocket();
    if (m_socket == INVALID_SOCKET)
    {
        m_socket = static_cast<SOCKET>(1);
    }

    if (m_bGame)
    {
        g_bGameServerConnected = TRUE;
    }

    g_ErrorReport.Write("[CWsctlc::Connect] Connected successfully to %s:%d (socket=%d)\r\n",
        ip_addr, port, static_cast<int>(m_socket));

    // Automatically request ConnectServer server list if connecting to CS port
    if (!m_bGame && (port == g_ServerPort || port == 63000))
    {
        const uint8_t reqServerList[] = { 0xC1, 0x04, 0xF4, 0x06 };
        m_pClient->Send(reqServerList, sizeof(reqServerList));
        g_ErrorReport.Write("[CWsctlc::Connect] Sent ConnectServer ServerList request\r\n");
    }

    return TRUE;
}

int CWsctlc::sSend(SOCKET socket, char* buf, int len)
{
    if (m_pClient && buf && len > 0)
    {
        return m_pClient->Send(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(len));
    }
    return FALSE;
}

int CWsctlc::sSend(char* buf, int len)
{
    return sSend(m_socket, buf, len);
}

int CWsctlc::FDWriteSend()
{
    return TRUE;
}

int CWsctlc::nRecv()
{
    return 0;
}

BYTE* CWsctlc::GetReadMsg()
{
    if (m_pClient)
    {
        return m_pClient->PopReceivedPacket();
    }
    return NULL;
}

#if defined(__ANDROID__) || defined(MU_IOS)
void CWsctlc::AndroidClearPacketQueue()
{
    if (m_pClient)
    {
        m_pClient->ClearPacketQueue();
    }
}

void CWsctlc::AndroidOnPacket(int32_t /*handle*/, int32_t /*size*/, uint8_t* /*data*/)
{
}

void CWsctlc::AndroidOnDisconnect(int32_t /*handle*/)
{
}
#endif

void CWsctlc::LogPrint(char* /*szlog*/, ...) {}
void CWsctlc::LogHexPrint(BYTE* /*buf*/, int /*size*/) {}
void CWsctlc::LogHexPrintS(BYTE* /*buf*/, int /*size*/) {}
void CWsctlc::LogPrintOn() { m_LogPrint = 1; }
void CWsctlc::LogPrintOff() { m_LogPrint = 0; }
