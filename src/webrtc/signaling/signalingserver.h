// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_SIGNALING_SERVER_H
#define POCKETNET_CORE_SIGNALING_SERVER_H

#include "protectedmap.h"
#include <set>
#include <string>

#include "webrtc/signaling/signalingprocessor.h"

namespace webrtc::signaling {
    using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

    class SignalingServer
    {
    public:
        SignalingServer();
        void Init(const std::string& ep, const short& port);
        void Start();
        void Stop();
    private:
        std::thread m_thread;
        std::shared_ptr<SignalingProcessor> m_processor;
        std::shared_ptr<WsServer> m_server;
    };
} // webrtc::signaling

#endif // POCKETNET_CORE_SIGNALING_SERVER_H