// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTCPULP_H
#define POCKETNET_CORE_WEBRTCPULP_H

#include "webrtc/IWebRTC.h"

namespace webrtc {
class WebRTCPulp : public IWebRTC
{
public:
    /**
     * Initializing internal thread that is required to
     * corretly freeing memory
     */
    void Start() override {};
    void Stop() override {};
    void InitiateNewSignalingConnection(const std::string& ip, const std::string& myid) override {};
    void DropConnection(const std::string& ip) override {};

};
} // namespace webrtc

#endif // POCKETNET_CORE_WEBRTCPULP_H