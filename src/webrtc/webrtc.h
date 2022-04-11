// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_H
#define POCKETNET_CORE_WEBRTC_H

#include "protectedmap.h"
#include "rpcapi/rpcapi.h"
#include "rtc/rtc.hpp"
#include "univalue.h"
#include "webrtc/webrtcconnection.h"
#include "webrtc/webrtcprotocol.h"
#include <rtc/datachannel.hpp>
#include <rtc/websocket.hpp>
#include "eventloop.h"
#include "webrtc/wsconnectionhandler.h"

#include <string>
#include <vector>
#include <atomic>
#include <memory>


class WebRTC
{
public:
    WebRTC(std::shared_ptr<IRequestProcessor> requestProcessor, int port);
    /**
     * Initializing internal thread that is required to
     * corretly freeing memory
     */
    void Start();
    void Stop();
    void InitiateNewSignalingConnection(const std::string& ip);
    void DropConnection(const std::string& ip);

private:
    std::atomic_bool m_fRunning = false;
    const int m_port;
    std::shared_ptr<ProtectedMap<std::string, std::shared_ptr<webrtc::WsConnectionHandler>>> m_wsConnections;
    std::shared_ptr<WebRTCProtocol> m_protocol;
    std::shared_ptr<Queue<std::shared_ptr<WebRTCConnection>>> m_queue;
    std::shared_ptr<QueueEventLoopThread<std::shared_ptr<WebRTCConnection>>> m_eventLoop;
};

extern std::shared_ptr<WebRTC> g_webrtc;

#endif // POCKETNET_CORE_WEBRTC_H