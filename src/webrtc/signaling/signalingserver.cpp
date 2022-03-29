// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/signaling/signalingserver.h"


webrtc::signaling::SignalingServer::SignalingServer()
    : m_processor(std::make_shared<SignalingProcessor>())
{}

void webrtc::signaling::SignalingServer::Init(const std::string &ep, const short &port)
{
    m_server = std::make_shared<WsServer>();
    m_server->config.port = port;
    auto& endpoint = m_server->endpoint[ep];

    // Can capture here by shared 
    endpoint.on_open = [proc = m_processor](std::shared_ptr<WsServer::Connection> connection) {
        proc->NewConnection(std::move(connection));
    };

    endpoint.on_close = [proc = m_processor](std::shared_ptr<WsServer::Connection> connection, int, const std::string&) {
        proc->ClosedConnection(std::move(connection));
    };

    // TODO (losty-signaling): ugly capturing
    endpoint.on_message = [proc = m_processor](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> in_message) {
        proc->ProcessMessage(connection, std::move(in_message));
    };
}

void webrtc::signaling::SignalingServer::Start()
{
    m_thread = std::thread([server=m_server]() {
        if (server)
            server->start();
    });

}

void webrtc::signaling::SignalingServer::Stop()
{
    if (m_server)
        m_server->stop();
    if (m_processor)
        m_processor->Stop();
    if (m_thread.joinable())
        m_thread.join();
}