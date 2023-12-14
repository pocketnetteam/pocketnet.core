// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/wsconnectionhandler.h"


void webrtc::WsConnectionHandler::onClosed()
{
    // This is used to determine if connection was dropped by remote
    // or by us. If connection is closed by us, memory is already freed and no
    // need to try delete again that will result in deadlock.
    if (m_closed)
        return;

    m_closed = true;
    if (auto lock = m_wsConnections.lock()) {
        lock->erase(m_ip);
    }
}

webrtc::WsConnectionHandler::WsConnectionHandler(
                    const std::string& ip,
                    std::shared_ptr<rtc::WebSocket> ws,
                    std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WsConnectionHandler>>> wsConnections)
{
    m_ip = ip;
    m_ws = std::move(ws);
    m_wsConnections = wsConnections;
}

webrtc::WsConnectionHandler::~WsConnectionHandler()
{
    m_closed = true;
}
