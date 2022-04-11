// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WSCONNECTION_HANDLER_H
#define POCKETNET_CORE_WSCONNECTION_HANDLER_H

#include "rtc/rtc.hpp"

#include "protectedmap.h"

namespace webrtc
{
    class WsConnectionHandler
    {
    public:
        explicit WsConnectionHandler(
                        const std::string& ip,
                        std::shared_ptr<rtc::WebSocket> ws,
                        std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WsConnectionHandler>>> wsConnections);
        void onClosed();
        ~WsConnectionHandler();
    private:
        std::string m_ip;
        std::atomic_bool m_closed = false;
        std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WsConnectionHandler>>> m_wsConnections;
        std::shared_ptr<rtc::WebSocket> m_ws;
    };
}

#endif // POCKETNET_CORE_WSCONNECTION_HANDLER_H