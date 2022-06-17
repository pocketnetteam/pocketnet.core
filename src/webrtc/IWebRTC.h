// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_IWEBRTC_H
#define POCKETNET_CORE_IWEBRTC_H

#include <string>
#include <memory>

namespace webrtc {
class IWebRTC
{
public:
    /**
     * Initializing internal thread that is required to
     * corretly freeing memory
     */
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void InitiateNewSignalingConnection(const std::string& ip) = 0;
    virtual void DropConnection(const std::string& ip) = 0;
};
} // namespace webrtc

extern std::unique_ptr<webrtc::IWebRTC> g_webrtc;

#endif // POCKETNET_CORE_IWEBRTC_H