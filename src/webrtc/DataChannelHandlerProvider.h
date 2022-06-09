// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_DATACHANNELHANDLER_H
#define POCKETNET_CORE_WEBRTC_DATACHANNELHANDLER_H

#include "rpcapi/rpcapi.h"
#include "notification/INotificationProtocol.h"
#include "rtc/datachannel.hpp"

namespace webrtc {
using handlerRet = std::function<void(rtc::message_variant)>;

class DataChannelHandlerProvider
{
public:
    static handlerRet GetRPCHandler(std::shared_ptr<IRequestProcessor> requestProcessor, std::weak_ptr<rtc::DataChannel> dc, const std::string& ip);
    static handlerRet GetNotificationsHandler(std::shared_ptr<notifications::INotificationProtocol> notificationProtocol, std::weak_ptr<rtc::DataChannel> dc, const std::string& ip);
};
} // namespace webrtc

#endif // POCKETNET_CORE_WEBRTC_DATACHANNELHANDLER_H