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
        // TODO (losty-rtc): dirty hack.
        // `manualId` is something that is used to identificate connections.
        // This equeals to ip:port pair by default and is being changed during registerasnode request.
        // However, the logic of identifying peers is too complicated because we want clients to know
        // what node they are exactly going to connect too (because there might be e.x. 2 or more nodes behind the
        // same NAT) and at the same time nodes do not care about who is going to connect so there is no need for clients to be
        // registered as signaling. This result in different logic for clients and nodes manipuldating with signaling.
        // Consider generalized solution after determine the way how nodes will be identified by the clients.
        SignalingConnection(std::shared_ptr<Connection> _connection, std::string _manualId)
            : connection (std::move(_connection)),
              manualId(std::move(_manualId))
        {}
        std::shared_ptr<Connection> connection;
        std::string manualId;
        bool fIsNode = false;
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