// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/DataChannelConnection.h"

webrtc::DataChannelConnection::DataChannelConnection(std::shared_ptr<rtc::DataChannel> dc, const std::string& ip)
    : m_dc(std::move(dc)),
      m_ip(std::move(ip))
{}

void webrtc::DataChannelConnection::Send(const std::string& msg) {
    m_dc->send(msg);
}

void webrtc::DataChannelConnection::Close(int code) {
    m_dc->close();
}

const std::string& webrtc::DataChannelConnection::GetRemoteEndpointAddress() const
{
    return m_ip;
}