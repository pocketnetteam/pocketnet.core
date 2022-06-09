// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_DATACHANNELCONNECTION_H
#define POCKETNET_CORE_WEBRTC_DATACHANNELCONNECTION_H

#include "web/IConnection.h"

#include "rtc/datachannel.hpp"

namespace webrtc {
class DataChannelConnection : public IConnection
{
public:
    explicit DataChannelConnection(std::shared_ptr<rtc::DataChannel> dc, const std::string& ip);
    void Send(const std::string& msg) override;
    void Close(int code) override;
    const std::string& GetRemoteEndpointAddress() const override;
private:
    std::shared_ptr<rtc::DataChannel> m_dc;
    std::string m_ip;
};
} // namespace webrtc

#endif // POCKETNET_CORE_WEBRTC_DATACHANNELCONNECTION_H