// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_WEBRTCFACTORY_H
#define POCKETNET_CORE_WEBRTC_WEBRTCFACTORY_H

#include "rpcapi/rpcapi.h"
#include "notification/INotificationProtocol.h"
#include "webrtc/IWebRTC.h"

namespace webrtc
{
class WebRTCFactory
{
public:
    std::unique_ptr<IWebRTC> InitNewWebRTC(bool enableRTC, std::shared_ptr<IRequestProcessor> requestProcessor, std::shared_ptr<notifications::INotificationProtocol> notificationProtocol, int port) const;
};
} // namespace webrtc

#endif // POCKETNET_CORE_WEBRTC_WEBRTCFACTORY_H