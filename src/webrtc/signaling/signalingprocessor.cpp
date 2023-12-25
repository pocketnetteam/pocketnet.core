// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/signaling/signalingprocessor.h"

#include "logging.h"
#include "univalue.h"

static std::string GetConnectionId(webrtc::signaling::Connection& conn)
{
    return conn.remote_endpoint_address() + ":" + std::to_string(conn.remote_endpoint_port());
}

void webrtc::signaling::SignalingProcessor::OnNewConnection(std::shared_ptr<Connection> conn)
{
    // TODO (losty-rtc): use Sec-WebSocket-Key???
    auto id = GetConnectionId(*conn);
    LogPrintf("DEBUG (Signaling): new signaling connection from %s\n", id);
    if (!m_connections.insert(id, std::make_shared<SignalingConnection>(conn))) {
        LogPrintf("DEBUG (Signaling): already existed connection from %s\n", id);
    }
}

void webrtc::signaling::SignalingProcessor::OnClosedConnection(const std::shared_ptr<Connection> &conn)
{
    LogPrintf("DEBUG (Signaling): disconnected signaling with id '%s'\n", GetConnectionId(*conn));
    m_connections.erase(GetConnectionId(*conn));
}

void webrtc::signaling::SignalingProcessor::ProcessMessage(const std::shared_ptr<Connection> &connection, std::shared_ptr<Message> in_message)
{
    auto msg_str = in_message->string();
    auto senderId = GetConnectionId(*connection);
    UniValue msg;
    if (msg_str.empty() || !msg.read(msg_str)) {
        // TODO (losty-signaling): error
        LogPrintf("DEBUG (Signaling): invalid message from %s\n", senderId);
        return;
    }

    if (!msg.exists("type") || !msg["type"].isStr()) {
        // TODO (losty-signaling): error
        LogPrintf("DEBUG (Signaling): invalid message type from %s\n", senderId);
        return;
    } 
    auto type = msg["type"].get_str();

    if (type == "protocol") {
        if (!msg.exists("id") && !msg["id"].isStr()) {
            // TODO (losty-signaling): error
            LogPrintf("DEBUG (Signaling): no id specified in message from %s\n", senderId);
            return;
        }
        auto requestedId = msg["id"].get_str();

        // Replacing ip with sender so receiver will know who sends the message.
        msg.pushKV("id", senderId);
        auto sendFunc = [&msg](const std::shared_ptr<SignalingConnection>& conn) {
            conn->connection->send(msg.write());
        };
        if (!m_connections.exec_for_elem(requestedId, sendFunc)) {
            // TODO (losty-signaling): error - no such connection
            LogPrintf("DEBUG (Signaling): failed to find requested id '%s' from %s\n", requestedId, senderId);
            return; 
        }

    } else if (type == "registerasnode") {
        m_connections.exec_for_elem(senderId, [](const auto& conn) {
            conn->isNode = true;
        });
    } else if (type == "listnodes") {
        UniValue msg (UniValue::VARR);
        m_connections.Iterate([&](std::pair<const std::string&, const std::shared_ptr<SignalingConnection>&> elem) {
            if (elem.second->isNode) {
                msg.push_back(elem.first);
            }
        });
        connection->send(msg.write());
    } else {
        LogPrintf("DEBUG (Signaling): unexpected message type from '%s'\n", senderId);
        // TODO (losty-signaling): error
    }
}

void webrtc::signaling::SignalingProcessor::Stop()
{
    m_connections.clear();
}