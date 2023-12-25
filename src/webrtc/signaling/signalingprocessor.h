// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_SIGNALING_PROCESSOR_H
#define POCKETNET_CORE_SIGNALING_PROCESSOR_H

#include "protectedmap.h"
#include "websocket/ws.h"
#include <string>

namespace webrtc::signaling
{
    using Connection = SimpleWeb::IWSConnection;
    using Message = SimpleWeb::InMessage;

    struct SignalingConnection
    {
        SignalingConnection(std::shared_ptr<Connection> _connection)
            : connection (std::move(_connection))
        {}
        std::shared_ptr<Connection> connection;
        bool isNode = false;
    };

    class SignalingProcessor
    {
    public:
        void OnNewConnection(std::shared_ptr<Connection> conn);
        void OnClosedConnection(const std::shared_ptr<Connection>& conn);
        void ProcessMessage(const std::shared_ptr<Connection>& connection, std::shared_ptr<Message> in_message);
        void Stop();
    private:
        ProtectedMap<std::string, std::shared_ptr<SignalingConnection>> m_connections;
    };
} // webrtc::signaling

#endif // POCKETNET_CORE_SIGNALING_PROCESSOR_H