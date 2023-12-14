// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/signaling/signalingprocessor.h"

#include "logging.h"
#include "univalue.h"


void webrtc::signaling::SignalingProcessor::OnNewConnection(std::shared_ptr<Connection> conn)
{
    auto ip = conn->remote_endpoint_address();
    LogPrintf("DEBUG (Signaling): new signaling connection from %s\n", ip);
    if (!m_connections.insert(ip, std::move(conn))) {
        LogPrintf("DEBUG (Signaling): already existed connection from %s\n", ip);
    }
}

void webrtc::signaling::SignalingProcessor::OnClosedConnection(const std::shared_ptr<Connection> &conn)
{
    LogPrintf("DEBUG (Signaling): disconnected signaling with ip '%s'\n", conn->remote_endpoint_address());
    m_connections.erase(conn->remote_endpoint_address());
}

void webrtc::signaling::SignalingProcessor::ProcessMessage(const std::shared_ptr<Connection> &connection, std::shared_ptr<Message> in_message)
{
    auto msg_str = in_message->string();
    auto senderIP = connection->remote_endpoint_address();
    UniValue msg;
    if (msg_str.empty() || !msg.read(msg_str)) {
        // TODO (losty-signaling): error
        LogPrintf("DEBUG (Signaling): invalid message from %s\n", senderIP);
        return;
    }

    if (!msg.exists("type") || !msg["type"].isStr()) {
        // TODO (losty-signaling): error
        LogPrintf("DEBUG (Signaling): invalid message type from %s\n", senderIP);
        return;
    } 
    auto type = msg["type"].get_str();

    if (type == "protocol") {
        if (!msg.exists("ip") && !msg["ip"].isStr()) {
            // TODO (losty-signaling): error
            LogPrintf("DEBUG (Signaling): no ip specified in message from %s\n", senderIP);
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
            LogPrintf("DEBUG (Signaling): failed to find requested ip '%s' from %s\n", requestedIP, senderIP);
            return; 
        }

    } else {
        LogPrintf("DEBUG (Signaling): unexpected message type from '%s'\n", senderIP);
        // TODO (losty-signaling): error
    }
}

void webrtc::signaling::SignalingProcessor::Stop()
{
    m_connections.clear();
}