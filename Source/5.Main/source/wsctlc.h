#ifndef __WSCTLC_H__
#define __WSCTLC_H__

#pragma once

#include "Protect.h"
#include "./Utilities/Log/ErrorReport.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include <memory>

#define MAX_SENDBUF     8192
#define MAX_RECVBUF     8192

typedef struct 
{
    int used;
    int len;
    BYTE buf[MAX_RECVBUF];
} STRUCT_RECVSTACK;

class CPacketQueue;
class AsioNetworkClient;

class CWsctlc
{
private:
    HWND   m_hWnd;
    BOOL   m_bGame;

    int    m_iMaxSockets;
    SOCKET m_socket;

    BYTE   m_SendBuf[MAX_SENDBUF];
    int    m_nSendBufLen;

    BYTE   m_RecvBuf[MAX_RECVBUF];
    int    m_nRecvBufLen;

    int    m_LogPrint;
    FILE*  m_logfp;

    CPacketQueue* m_pPacketQueue;
    std::unique_ptr<AsioNetworkClient> m_pClient;

    BOOL ShutdownConnection(SOCKET sd);

public:
    CWsctlc();
    ~CWsctlc();

    SOCKET GetSocket();

    BOOL Startup();
    void Cleanup();

    int  Create(HWND hWnd, BOOL bGame = FALSE);
    BOOL Close();
    BOOL Close(SOCKET& socket);

    BOOL Connect(char* ip_addr, unsigned short port, DWORD WinMsgNum = 0);

    int  sSend(SOCKET socket, char* buf, int len);
    int  sSend(char* buf, int len);

    int  FDWriteSend();
    int  nRecv();

    BYTE* GetReadMsg();

    AsioNetworkClient* GetAsioClient() const { return m_pClient.get(); }

#if defined(__ANDROID__) || defined(MU_IOS)
    static void AndroidOnPacket(int32_t handle, int32_t size, uint8_t* data);
    static void AndroidOnDisconnect(int32_t handle);
    void AndroidClearPacketQueue();
#endif

    void LogPrint(char* szlog, ...);
    void LogHexPrint(BYTE* buf, int size);
    void LogHexPrintS(BYTE* buf, int size);
    void LogPrintOn();
    void LogPrintOff();
};

#endif // __WSCTLC_H__
