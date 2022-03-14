// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_H
#define POCKETNET_CORE_WEBRTC_H

#include "protectedmap.h"
#include "rpcapi/rpcapi.h"
#include "rtc/rtc.hpp"
#include "univalue.h"
#include "webrtc/webrtcprotocol.h"
#include <rtc/datachannel.hpp>
#include <rtc/websocket.hpp>

#include <string>
#include <vector>
#include <memory>


class WebRTC
{
public:
    WebRTC(std::shared_ptr<IRequestProcessor> requestProcessor, int port);
    void InitiateNewSignalingConnection(const std::string& ip);

private:
    const int m_port;
    ProtectedMap<std::string, std::shared_ptr<rtc::WebSocket>> m_wsConnections;
    std::shared_ptr<WebRTCProtocol> m_protocol;
};

#endif // POCKETNET_CORE_WEBRTC_H