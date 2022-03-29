// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/signaling/signalingprocessor.h"

#include "univalue.h"


void webrtc::signaling::SignalingProcessor::NewConnection(std::shared_ptr<Connection> conn)
{
    auto ip = conn->remote_endpoint_address();
    m_connections.insert(ip, std::move(conn));
}

void webrtc::signaling::SignalingProcessor::ClosedConnection(const std::shared_ptr<Connection> &conn)
{
    m_connections.erase(conn->remote_endpoint_address());
}

void webrtc::signaling::SignalingProcessor::ProcessMessage(const std::shared_ptr<Connection> &connection, std::shared_ptr<Message> in_message)
{
    auto msg_str = in_message->string();
    auto senderIP = connection->remote_endpoint_address();
    UniValue msg;
    if (msg_str.empty() || !msg.read(msg_str)) {
        // TODO (losty-signaling): error
        return;
    }

    if (!msg.exists("type") || !msg["type"].isStr()) {
        // TODO (losty-signaling): error
        return;
    } 
    auto type = msg["type"].get_str();

    if (type == "protocol") {
        if (!msg.exists("ip") && !msg["ip"].isStr()) {
            // TODO (losty-signaling): error
            return;
        }
        auto requestedIP = msg["ip"].get_str();

        // Replacing ip with sender so receiver will know who sends the message.
        msg.pushKV("ip", senderIP);
        auto sendFunc = [&msg](const std::shared_ptr<Connection>& conn) {
            conn->send(msg.write());
        };
        if (!m_connections.exec_for_elem(requestedIP, sendFunc)) {
            // TODO (losty-signaling): error - no such connection
            return; 
        }

    } else {
        // TODO (losty-signaling): error
    }
}

void webrtc::signaling::SignalingProcessor::Stop()
{
    m_connections.clear();
}