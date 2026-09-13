#if defined(__ANDROID__) || defined(MU_IOS)

#include "stdafx.h"
#include "AndroidNetwork.h"
#include "wsctlc.h"
#include "Network/AsioNetworkClient.h"

extern CWsctlc SocketClient;

void AndroidPostPacket(void* /*packetInfoPtr*/) {}
void AndroidDrainPackets() {}

AndroidNetworkOverlayStats AndroidQueryNetworkOverlayStats(int32_t /*handle*/)
{
    if (SocketClient.GetAsioClient())
    {
        return SocketClient.GetAsioClient()->GetOverlayStats();
    }
    return {};
}

#endif
