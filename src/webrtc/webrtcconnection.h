// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_WEBRTCCONNECTION_H
#define POCKETNET_CORE_WEBRTC_WEBRTCCONNECTION_H

#if defined(HAVE_CONFIG_H)
#include <config/pocketcoin-config.h>
#endif

#include "eventloop.h"
#include "protectedmap.h"

#include <atomic>
#include <rtc/datachannel.hpp>
#include <rtc/peerconnection.hpp>

#include <string>


namespace webrtc {
class WebRTCConnection
{
public:
    WebRTCConnection(std::string ip, std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WebRTCConnection>>> container, std::shared_ptr<Queue<std::shared_ptr<WebRTCConnection>>> clearer, std::shared_ptr<rtc::PeerConnection> pc);
    void AddDataChannel(std::shared_ptr<rtc::DataChannel> dc);
    void RemoveDataChannel(const std::string& label);
    void OnPeerConnectionClosed(std::shared_ptr<WebRTCConnection> _this);
    void AddRemoteCandidate(const rtc::Candidate& candidate);

private:
    std::string m_ip;
    std::atomic_bool m_closed = false;;
    std::shared_ptr<rtc::PeerConnection> m_peerConnection;
    std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WebRTCConnection>>> m_container;
    std::shared_ptr<Queue<std::shared_ptr<WebRTCConnection>>> m_clearer;
    ProtectedMap<std::string, std::shared_ptr<rtc::DataChannel>> m_dataChannels;
};
} // namespace webrtc

#endif // POCKETNET_CORE_WEBRTC_WEBRTCCONNECTION_H