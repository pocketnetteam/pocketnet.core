// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/webrtcfactory.h"

#if USE_WEBRTC
#include "webrtc/webrtc.h"
#endif
#include "webrtc/webrtcpulp.h"

std::unique_ptr<webrtc::IWebRTC> webrtc::WebRTCFactory::InitNewWebRTC(bool enableRTC, std::shared_ptr<IRequestProcessor> requestProcessor, std::shared_ptr<notifications::INotificationProtocol> notificationProtocol, int port) const
{
    #if USE_WEBRTC
    if (enableRTC)
        return std::make_unique<WebRTC>(std::move(requestProcessor), std::move(notificationProtocol), std::move(port));
    else
        return std::make_unique<WebRTCPulp>();
    #else
    return std::make_unique<WebRTCPulp>();
    #endif
}

