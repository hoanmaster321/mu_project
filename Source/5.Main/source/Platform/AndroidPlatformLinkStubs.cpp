#if defined(__ANDROID__) || defined(MU_IOS)

#include "stdafx.h"
#include "SimpleModulus.h"
#include "TrayMode.h"
#include "WSctlc.h"
#include "wsctlc_addon.h"
#include "Protect.h"
#include "Utilities/Log/ErrorReport.h"
#include "ZzzBMD.h"
#include "android/SimpleModulusCrypt.h"
#include "ExternalObject/curl/curl.h"
#include <sys/socket.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <mutex>
#include <unordered_map>

// ── Network connection manager externs ────────────────────────────────────
extern "C" int32_t ConnectionManager_Connect(
    const wchar_t* host,
    int32_t port,
    BYTE isEncrypted,
    void(*onPacket)(int32_t, int32_t, uint8_t*),
    void(*onDisconnect)(int32_t));
extern "C" void ConnectionManager_Disconnect(int32_t handle);
extern "C" void ConnectionManager_Send(int32_t handle, const uint8_t* data, int32_t size);
extern "C" void SendServerListRequest(int32_t handle);

extern BOOL g_bGameServerConnected;
extern unsigned short g_ServerPort;

int g_MaxMessagePerCycle = -1;

// ── TrayMode stub ─────────────────────────────────────────────────────────
CTrayMode gTrayMode;
CTrayMode::CTrayMode() : m_TrayIcon(nullptr), m_MainWndProc(nullptr) {}
void CTrayMode::Init(HINSTANCE) {}
void CTrayMode::Toggle() {}
LONG CTrayMode::GetMainWndProc() { return 0; }
void CTrayMode::ShowNotify(bool) {}
LRESULT CALLBACK CTrayMode::TrayModeWndProc(HWND, UINT, WPARAM, LPARAM) { return 0; }

// ── Curl stubs ────────────────────────────────────────────────────────────
extern "C" CURL* curl_easy_init(void) { return nullptr; }
extern "C" CURLcode curl_easy_setopt(CURL*, CURLoption, ...) { return CURLE_FAILED_INIT; }
extern "C" CURLcode curl_easy_perform(CURL*) { return CURLE_FAILED_INIT; }
extern "C" void curl_easy_cleanup(CURL*) {}

// ── Android UI stubs ──────────────────────────────────────────────────────
bool AndroidShowCommandTradePicker() { return false; }
bool AndroidShowCommandPartyPicker() { return false; }
bool AndroidShowCommandGuildPicker() { return false; }
bool AndroidShowCommandDuelPicker() { return false; }
bool AndroidShowCommandFriendPicker() { return false; }
bool AndroidGetMoveMapWindowPosition(int, int, int*, int*) { return false; }
bool AndroidTriggerHotKeySkillTap(int) { return false; }

void StartStackLogging() {}
void CErrorReport::WriteLogBegin() {}
void GetSystemInfo(ER_SystemInfo*) {}
void CErrorReport::WriteSystemInfo(ER_SystemInfo*) {}
void CErrorReport::WriteSoundCardInfo() {}

float g_androidZoomOverride = 0.0f;
bool ShouldThrottleAdaptiveEffectSpawn(int, int, float*, int, float, OBJECT*) { return false; }

// ── SEASON3B stubs ────────────────────────────────────────────────────────
extern int TextNum;
extern int g_iItemInfo[16][17];
extern int ItemHelp;
class JewelHarmonyInfo;
extern JewelHarmonyInfo* g_pUIJewelHarmonyinfo;

namespace SEASON3B
{
    float g_fScreenRate_x = 1.0f;
    float g_fScreenRate_y = 1.0f;
    int MouseY = 0;
    int SelectedCharacter = -1;
    int g_iCancelSkillTarget = 0;
    int g_iLengthAuthorityCode = 20;
    char TextList[60][512] = {};
    int TextListColor[60] = {};
    int TextBold[60] = {};
    int& TextNum = ::TextNum;
    int (&g_iItemInfo)[16][17] = ::g_iItemInfo;
    int& ItemHelp = ::ItemHelp;
    JewelHarmonyInfo*& g_pUIJewelHarmonyinfo = ::g_pUIJewelHarmonyinfo;
}

// ── SEASON4A stubs ────────────────────────────────────────────────────────
namespace SEASON4A
{
    float IntensityTransform[MAX_MESH][MAX_VERTICES] = {};
    int g_iLimitAttackTime = 15;
}

static BYTE g_wsctlcReadMessage[MAX_RECVBUF] = {};
namespace
{
    constexpr DWORD kSimpleModulusSaveLoadXor[SIZE_ENCRYPTION_KEY] = {
        0x3F08A79B,
        0xE25CC287,
        0x93D27AB9,
        0x20DEA7BF,
    };

    constexpr DWORD kTakumiModulus[SIZE_ENCRYPTION_KEY] = { 95770, 84999, 76807, 71847 };
    constexpr DWORD kTakumiEncryptKey[SIZE_ENCRYPTION_KEY] = { 29511, 17162, 10281, 17092 };
    constexpr DWORD kTakumiDecryptKey[SIZE_ENCRYPTION_KEY] = { 17521, 13625, 21598, 26083 };
    constexpr DWORD kTakumiXorKey[SIZE_ENCRYPTION_KEY] = { 24429, 15164, 17929, 12915 };

    std::mutex g_androidSocketMutex;
    std::unordered_map<int32_t, CWsctlc*> g_androidSocketOwners;

    std::wstring ToWideString(const char* text)
    {
        std::wstring wide;
        if (!text)
        {
            return wide;
        }

        while (*text)
        {
            wide.push_back(static_cast<unsigned char>(*text));
            ++text;
        }

        return wide;
    }

    bool IsConnectServerPort(unsigned short port)
    {
        return (port == g_ServerPort || port == 63000);
    }

    void CopyPacketKey(DWORD* target, const DWORD* source)
    {
        if (target && source)
        {
            std::memcpy(target, source, sizeof(DWORD) * SIZE_ENCRYPTION_KEY);
        }
    }

    void SyncClientKey(const DWORD* modulus, const DWORD* key, const DWORD* xorKey)
    {
        mu::PacketKeySet keySet {};
        for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
        {
            keySet.modulus[static_cast<size_t>(i)] = modulus[i];
            keySet.key[static_cast<size_t>(i)] = key[i];
            keySet.xorKey[static_cast<size_t>(i)] = xorKey[i];
        }
        mu::SetClientToServerKey(keySet);
    }

    void SyncServerKey(const DWORD* modulus, const DWORD* key, const DWORD* xorKey)
    {
        mu::PacketKeySet keySet {};
        for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
        {
            keySet.modulus[static_cast<size_t>(i)] = modulus[i];
            keySet.key[static_cast<size_t>(i)] = key[i];
            keySet.xorKey[static_cast<size_t>(i)] = xorKey[i];
        }
        mu::SetServerToClientKey(keySet);
    }

    bool LoadSimpleModulusKeyFile(const char* path, DWORD* outModulus, DWORD* outKey, DWORD* outXorKey)
    {
        if (!path || !outModulus || !outKey || !outXorKey)
        {
            return false;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }

        ChunkHeader header {};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!file || header.m_sID != CHUNKID_ONEKEY || header.m_iSize != (sizeof(ChunkHeader) + sizeof(DWORD) * SIZE_ENCRYPTION_KEY * 3))
        {
            return false;
        }

        DWORD table[SIZE_ENCRYPTION_KEY] = {};

        for (int section = 0; section < 3; ++section)
        {
            file.read(reinterpret_cast<char*>(table), sizeof(table));
            if (!file)
            {
                return false;
            }

            for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
            {
                table[i] ^= kSimpleModulusSaveLoadXor[i];
            }

            if (section == 0)
            {
                std::memcpy(outModulus, table, sizeof(table));
            }
            else if (section == 1)
            {
                std::memcpy(outKey, table, sizeof(table));
            }
            else
            {
                std::memcpy(outXorKey, table, sizeof(table));
            }
        }

        return true;
    }

}

CWsctlc::CWsctlc()
{
    m_hWnd = NULL;
    m_bGame = FALSE;
    m_iMaxSockets = 0;
    m_socket = INVALID_SOCKET;
    std::memset(m_SendBuf, 0, sizeof(m_SendBuf));
    m_nSendBufLen = 0;
    std::memset(m_RecvBuf, 0, sizeof(m_RecvBuf));
    m_nRecvBufLen = 0;
    m_LogPrint = 0;
    m_logfp = NULL;
    m_pPacketQueue = new CPacketQueue;
}

CWsctlc::~CWsctlc()
{
    Close();
    delete m_pPacketQueue;
    m_pPacketQueue = nullptr;
}

void CWsctlc::AndroidClearPacketQueue()
{
    if (m_pPacketQueue == nullptr)
    {
        return;
    }

    while (!m_pPacketQueue->IsEmpty())
    {
        m_pPacketQueue->PopPacket();
    }

    m_pPacketQueue->ClearGarbage();
}

void CWsctlc::AndroidOnPacket(int32_t handle, int32_t size, uint8_t* data)
{
    if (handle <= 0 || size <= 0 || data == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(g_androidSocketMutex);
    const auto it = g_androidSocketOwners.find(handle);
    if (it == g_androidSocketOwners.end() || it->second == nullptr || it->second->m_pPacketQueue == nullptr)
    {
        return;
    }

    CWsctlc* client = it->second;

    if (size > MAX_RECVBUF || client->m_nRecvBufLen < 0 || client->m_nRecvBufLen >= MAX_RECVBUF)
    {
        g_ErrorReport.Write("[Android Socket] packet too large size=%d handle=%d\r\n", size, handle);
        return;
    }

    if ((client->m_nRecvBufLen + size) > MAX_RECVBUF)
    {
        g_ErrorReport.Write(
            "[Android Socket] recv buffer overflow size=%d current=%d handle=%d\r\n",
            size,
            client->m_nRecvBufLen,
            handle);
        client->m_nRecvBufLen = 0;
        return;
    }

    std::memcpy(client->m_RecvBuf + client->m_nRecvBufLen, data, static_cast<size_t>(size));

#if(ENCRYPT_STATE==1)
    if (gProtect.CheckSocketPort(client->m_socket) != 0)
    {
        gProtect.DecryptData(client->m_RecvBuf + client->m_nRecvBufLen, size);
    }
#endif

    client->m_nRecvBufLen += size;

    if (client->m_nRecvBufLen < 3)
    {
        return;
    }

    int offset = 0;
    int packetSize = 0;

    while (1)
    {
        if (client->m_RecvBuf[offset] == 0xC1 || client->m_RecvBuf[offset] == 0xC3)
        {
            packetSize = static_cast<int>(client->m_RecvBuf[offset + 1]);
        }
        else if (client->m_RecvBuf[offset] == 0xC2 || client->m_RecvBuf[offset] == 0xC4)
        {
            packetSize = (static_cast<int>(client->m_RecvBuf[offset + 1]) << 8) + client->m_RecvBuf[offset + 2];
        }
        else
        {
            g_ErrorReport.Write(
                "[Android Socket] packet sync lost handle=%d first=0x%02X len=%d\r\n",
                handle,
                client->m_RecvBuf[offset],
                client->m_nRecvBufLen);
            client->m_nRecvBufLen = 0;
            return;
        }

        if (packetSize <= 0)
        {
            g_ErrorReport.Write("[Android Socket] invalid packet size=%d handle=%d\r\n", packetSize, handle);
            client->m_nRecvBufLen = 0;
            return;
        }
        else if (packetSize <= client->m_nRecvBufLen)
        {
#if defined(MU_ANDROID_VERBOSE_NET_LOG)
            g_ErrorReport.Write(
                "[Android Socket] parsed packet handle=%d type=0x%02X size=%d head=0x%02X sub=0x%02X\r\n",
                handle,
                client->m_RecvBuf[offset],
                packetSize,
                (packetSize > 2) ? client->m_RecvBuf[offset + 2] : 0,
                (packetSize > 3) ? client->m_RecvBuf[offset + 3] : 0);
#endif
            client->m_pPacketQueue->PushPacket(client->m_RecvBuf + offset, packetSize);
            offset += packetSize;
            client->m_nRecvBufLen -= packetSize;

            if (client->m_nRecvBufLen <= 0)
            {
                break;
            }
        }
        else
        {
            if (offset > 0 && client->m_nRecvBufLen > 0)
            {
                std::memmove(client->m_RecvBuf, client->m_RecvBuf + offset, static_cast<size_t>(client->m_nRecvBufLen));
            }
            break;
        }
    }

    client->m_pPacketQueue->ClearGarbage();
}

void CWsctlc::AndroidOnDisconnect(int32_t handle)
{
    std::lock_guard<std::mutex> lock(g_androidSocketMutex);
    const auto it = g_androidSocketOwners.find(handle);
    if (it == g_androidSocketOwners.end())
    {
        return;
    }

    CWsctlc* client = it->second;
    g_androidSocketOwners.erase(it);

    if (client != nullptr)
    {
        client->m_socket = INVALID_SOCKET;
        client->m_nRecvBufLen = 0;
        client->m_nSendBufLen = 0;

        if (client->m_bGame)
        {
            g_bGameServerConnected = FALSE;
        }
    }
}

SOCKET CWsctlc::GetSocket()
{
    return m_socket;
}

BOOL CWsctlc::Startup()
{
    m_socket = INVALID_SOCKET;
    m_iMaxSockets = 1;
    return TRUE;
}

void CWsctlc::Cleanup()
{
}

int CWsctlc::Create(HWND hWnd, BOOL bGame)
{
    m_hWnd = hWnd;
    m_bGame = bGame;
    m_socket = INVALID_SOCKET;
    m_nSendBufLen = 0;
    m_nRecvBufLen = 0;

    if (m_bGame)
    {
        g_bGameServerConnected = FALSE;
    }

    return TRUE;
}

BOOL CWsctlc::Close()
{
    const int32_t handle = (m_socket == INVALID_SOCKET) ? 0 : static_cast<int32_t>(m_socket);

    {
        std::lock_guard<std::mutex> lock(g_androidSocketMutex);
        if (handle > 0)
        {
            g_androidSocketOwners.erase(handle);
        }

        AndroidClearPacketQueue();
    }

    std::memset(m_SendBuf, 0, sizeof(m_SendBuf));
    std::memset(m_RecvBuf, 0, sizeof(m_RecvBuf));
    m_nSendBufLen = 0;
    m_nRecvBufLen = 0;
    m_socket = INVALID_SOCKET;

    if (m_bGame)
    {
        g_bGameServerConnected = FALSE;
    }

    if (handle > 0)
    {
        ConnectionManager_Disconnect(handle);
    }

    return TRUE;
}

BOOL CWsctlc::Close(SOCKET& socket)
{
    if (socket == m_socket)
    {
        Close();
    }
    else if (socket != INVALID_SOCKET)
    {
        const int32_t handle = static_cast<int32_t>(socket);
        {
            std::lock_guard<std::mutex> lock(g_androidSocketMutex);
            g_androidSocketOwners.erase(handle);
        }
        ConnectionManager_Disconnect(handle);
        socket = INVALID_SOCKET;
    }

    socket = INVALID_SOCKET;
    return TRUE;
}

BOOL CWsctlc::Connect(char* ipAddr, unsigned short port, DWORD)
{
    if (m_hWnd == NULL || ipAddr == nullptr || ipAddr[0] == '\0')
    {
        return FALSE;
    }

    if (m_socket != INVALID_SOCKET)
    {
        Close();
    }

    const std::wstring hostWide = ToWideString(ipAddr);
    const int32_t handle = ConnectionManager_Connect(
        hostWide.c_str(),
        static_cast<int32_t>(port),
        0,
        CWsctlc::AndroidOnPacket,
        CWsctlc::AndroidOnDisconnect);

    if (handle <= 0)
    {
        g_ErrorReport.Write("[Android Socket] connect failed ip=%s port=%d\r\n", ipAddr, port);
        return FALSE;
    }

    {
        std::lock_guard<std::mutex> lock(g_androidSocketMutex);
        g_androidSocketOwners[handle] = this;
        AndroidClearPacketQueue();
    }

    m_socket = static_cast<SOCKET>(handle);
    g_ErrorReport.Write("[Android Socket] connected ip=%s port=%d handle=%d\r\n", ipAddr, port, handle);

    if (IsConnectServerPort(port))
    {
        SendServerListRequest(handle);
        g_ErrorReport.Write("[Android Socket] server list requested handle=%d\r\n", handle);
    }

    return TRUE;
}

int CWsctlc::sSend(SOCKET socket, char* buf, int len)
{
    if (socket == INVALID_SOCKET || buf == nullptr || len <= 0)
    {
        return FALSE;
    }

#if defined(MU_ANDROID_VERBOSE_NET_LOG)
    g_ErrorReport.Write(
        "[Android Socket] send packet handle=%d type=0x%02X size=%d head=0x%02X sub=0x%02X\r\n",
        static_cast<int>(socket),
        static_cast<unsigned char>(buf[0]),
        len,
        (len > 2) ? static_cast<unsigned char>(buf[2]) : 0,
        (len > 3) ? static_cast<unsigned char>(buf[3]) : 0);
#endif

    std::vector<uint8_t> sendBuffer(reinterpret_cast<uint8_t*>(buf), reinterpret_cast<uint8_t*>(buf) + len);

#if(ENCRYPT_STATE==1)
    if (gProtect.CheckSocketPort(socket) != 0)
    {
        gProtect.EncryptData(sendBuffer.data(), len);
    }
#endif

    ConnectionManager_Send(static_cast<int32_t>(socket), sendBuffer.data(), len);
    return TRUE;
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
    std::lock_guard<std::mutex> lock(g_androidSocketMutex);

    if (m_pPacketQueue != nullptr && !m_pPacketQueue->IsEmpty())
    {
        CPacket* packet = m_pPacketQueue->FrontPacket();
        m_pPacketQueue->PopPacket();
        return packet->GetBuffer();
    }

    return g_wsctlcReadMessage[0] ? g_wsctlcReadMessage : NULL;
}

void CWsctlc::LogPrint(char*, ...) {}
void CWsctlc::LogHexPrint(BYTE*, int) {}
void CWsctlc::LogHexPrintS(BYTE*, int) {}
void CWsctlc::LogPrintOn() {}
void CWsctlc::LogPrintOff() {}

CSimpleModulus::CSimpleModulus()
{
    Init();
}
CSimpleModulus::~CSimpleModulus() {}
void CSimpleModulus::Init(void)
{
    std::memset(m_dwModulus, 0, sizeof(m_dwModulus));
    std::memset(m_dwEncryptionKey, 0, sizeof(m_dwEncryptionKey));
    std::memset(m_dwDecryptionKey, 0, sizeof(m_dwDecryptionKey));
    std::memset(m_dwXORKey, 0, sizeof(m_dwXORKey));
}
int CSimpleModulus::Encrypt(void* lpTarget, void* lpSource, int iSize)
{
    if (lpSource == nullptr || iSize <= 0)
    {
        return 0;
    }

    if (lpTarget == nullptr)
    {
        return ((iSize + 7) / 8) * 11;
    }

    const auto* source = reinterpret_cast<const uint8_t*>(lpSource);
    const uint8_t counter = source[0];

    std::vector<uint8_t> plainPacket(static_cast<size_t>(iSize + 1), 0);
    plainPacket[0] = 0xC3;
    plainPacket[1] = static_cast<uint8_t>(plainPacket.size() & 0xFF);

    if (iSize > 1)
    {
        std::memcpy(plainPacket.data() + 2, source + 1, static_cast<size_t>(iSize - 1));
    }

    mu::PacketKeySet keySet {};
    for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
    {
        keySet.modulus[static_cast<size_t>(i)] = m_dwModulus[i];
        keySet.key[static_cast<size_t>(i)] = m_dwEncryptionKey[i];
        keySet.xorKey[static_cast<size_t>(i)] = m_dwXORKey[i];
    }

    const std::vector<uint8_t> encryptedPacket = mu::SM_EncryptPacketWithKey(plainPacket, counter, keySet);
    const int encryptedSize = static_cast<int>(encryptedPacket.size()) - 2;

    if (encryptedSize > 0)
    {
        std::memcpy(lpTarget, encryptedPacket.data() + 2, static_cast<size_t>(encryptedSize));
    }

    return encryptedSize;
}
int CSimpleModulus::Decrypt(void* lpTarget, void* lpSource, int iSize)
{
    if (lpSource == nullptr || iSize <= 0)
    {
        return 0;
    }

    std::vector<uint8_t> encryptedPacket;

    if ((iSize + 2) <= 0xFF)
    {
        encryptedPacket.resize(static_cast<size_t>(iSize + 2), 0);
        encryptedPacket[0] = 0xC3;
        encryptedPacket[1] = static_cast<uint8_t>(encryptedPacket.size() & 0xFF);
        std::memcpy(encryptedPacket.data() + 2, lpSource, static_cast<size_t>(iSize));
    }
    else
    {
        encryptedPacket.resize(static_cast<size_t>(iSize + 3), 0);
        encryptedPacket[0] = 0xC4;
        encryptedPacket[1] = static_cast<uint8_t>((encryptedPacket.size() >> 8) & 0xFF);
        encryptedPacket[2] = static_cast<uint8_t>(encryptedPacket.size() & 0xFF);
        std::memcpy(encryptedPacket.data() + 3, lpSource, static_cast<size_t>(iSize));
    }

    bool success = false;
    uint8_t packetCounter = 0;
    mu::PacketKeySet keySet {};
    for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
    {
        keySet.modulus[static_cast<size_t>(i)] = m_dwModulus[i];
        keySet.key[static_cast<size_t>(i)] = m_dwDecryptionKey[i];
        keySet.xorKey[static_cast<size_t>(i)] = m_dwXORKey[i];
    }

    const std::vector<uint8_t> plainPacket = mu::SM_DecryptPacketWithKey(encryptedPacket, keySet, &success, &packetCounter);

    if (!success || plainPacket.size() < 3)
    {
        return -1;
    }

    const int headerSize = (plainPacket[0] == 0xC4) ? 3 : 2;
    const int payloadSize = static_cast<int>(plainPacket.size()) - headerSize;
    const int resultSize = payloadSize + 1;

    if (lpTarget != nullptr)
    {
        auto* target = reinterpret_cast<uint8_t*>(lpTarget);
        target[0] = packetCounter;
        if (payloadSize > 0)
        {
            std::memcpy(target + 1, plainPacket.data() + headerSize, static_cast<size_t>(payloadSize));
        }
    }

    return resultSize;
}
void CSimpleModulus::EncryptBlock(void*, void*, int) {}
int CSimpleModulus::DecryptBlock(void*, void*) { return 0; }
int CSimpleModulus::AddBits(void*, int, void*, int, int) { return 0; }
void CSimpleModulus::Shift(void*, int, int) {}
int CSimpleModulus::GetByteOfBit(int nBit) { return nBit / 8; }
BOOL CSimpleModulus::SaveAllKey(char*) { return TRUE; }
BOOL CSimpleModulus::LoadAllKey(char* lpszFileName) { return (LoadEncryptionKey(lpszFileName) && LoadDecryptionKey(lpszFileName)); }
BOOL CSimpleModulus::SaveEncryptionKey(char*) { return TRUE; }
BOOL CSimpleModulus::LoadEncryptionKey(char* lpszFileName) { return LoadKey(lpszFileName, CHUNKID_ONEKEY, TRUE, TRUE, FALSE, TRUE); }
BOOL CSimpleModulus::SaveDecryptionKey(char*) { return TRUE; }
BOOL CSimpleModulus::LoadDecryptionKey(char* lpszFileName) { return LoadKey(lpszFileName, CHUNKID_ONEKEY, TRUE, FALSE, TRUE, TRUE); }
BOOL CSimpleModulus::SaveKey(char*, unsigned short, BOOL, BOOL, BOOL, BOOL) { return TRUE; }
BOOL CSimpleModulus::LoadKey(char* lpszFileName, unsigned short sID, BOOL bMod, BOOL bEnc, BOOL bDec, BOOL bXOR)
{
    if (sID != CHUNKID_ONEKEY)
    {
        return FALSE;
    }

    DWORD modulus[SIZE_ENCRYPTION_KEY] = {};
    DWORD key[SIZE_ENCRYPTION_KEY] = {};
    DWORD xorKey[SIZE_ENCRYPTION_KEY] = {};

    bool loaded = LoadSimpleModulusKeyFile(lpszFileName, modulus, key, xorKey);

    if (!loaded)
    {
        CopyPacketKey(modulus, kTakumiModulus);
        CopyPacketKey(xorKey, kTakumiXorKey);

        if (bEnc)
        {
            CopyPacketKey(key, kTakumiEncryptKey);
            g_ErrorReport.Write("[AndroidLogin] SimpleModulus encrypt key fallback active path=%s\r\n", lpszFileName ? lpszFileName : "(null)");
        }
        else if (bDec)
        {
            CopyPacketKey(key, kTakumiDecryptKey);
            g_ErrorReport.Write("[AndroidLogin] SimpleModulus decrypt key fallback active path=%s\r\n", lpszFileName ? lpszFileName : "(null)");
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        g_ErrorReport.Write("[AndroidLogin] SimpleModulus key loaded path=%s\r\n", lpszFileName ? lpszFileName : "(null)");
    }

    if (bMod)
    {
        std::memcpy(m_dwModulus, modulus, sizeof(m_dwModulus));
    }

    if (bEnc)
    {
        std::memcpy(m_dwEncryptionKey, key, sizeof(m_dwEncryptionKey));
    }

    if (bDec)
    {
        std::memcpy(m_dwDecryptionKey, key, sizeof(m_dwDecryptionKey));
    }

    if (bXOR)
    {
        std::memcpy(m_dwXORKey, xorKey, sizeof(m_dwXORKey));
    }

    if (bEnc)
    {
        SyncClientKey(m_dwModulus, m_dwEncryptionKey, m_dwXORKey);
    }

    if (bDec)
    {
        SyncServerKey(m_dwModulus, m_dwDecryptionKey, m_dwXORKey);
    }

    return TRUE;
}
BOOL CSimpleModulus::LoadKeyFromBuffer(BYTE* pbyBuffer, BOOL bMod, BOOL bEnc, BOOL bDec, BOOL bXOR)
{
    if (pbyBuffer == nullptr)
    {
        return FALSE;
    }

    DWORD temp[SIZE_ENCRYPTION_KEY] = {};
    std::memcpy(temp, pbyBuffer, sizeof(temp));

    if (bMod)
    {
        std::memcpy(m_dwModulus, temp, sizeof(m_dwModulus));
    }

    if (bEnc)
    {
        std::memcpy(m_dwEncryptionKey, temp, sizeof(m_dwEncryptionKey));
    }

    if (bDec)
    {
        std::memcpy(m_dwDecryptionKey, temp, sizeof(m_dwDecryptionKey));
    }

    if (bXOR)
    {
        std::memcpy(m_dwXORKey, temp, sizeof(m_dwXORKey));
    }

    if (bEnc)
    {
        SyncClientKey(m_dwModulus, m_dwEncryptionKey, m_dwXORKey);
    }

    if (bDec)
    {
        SyncServerKey(m_dwModulus, m_dwDecryptionKey, m_dwXORKey);
    }

    return TRUE;
}

DWORD CSimpleModulus::s_dwSaveLoadXOR[SIZE_ENCRYPTION_KEY] = {
    0x3F08A79B,
    0xE25CC287,
    0x93D27AB9,
    0x20DEA7BF,
};

#endif // defined(__ANDROID__) || defined(MU_IOS)
