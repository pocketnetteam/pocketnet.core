
#include "webrtc/webrtcconnection.h"

#include <utility>


WebRTCConnection::WebRTCConnection(std::string ip, std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WebRTCConnection>>> container, std::weak_ptr<std::vector<std::shared_ptr<WebRTCConnection>>> clearer, std::shared_ptr<rtc::PeerConnection> pc)
    : m_ip(std::move(ip)),
      m_peerConnection(std::move(pc)),
      m_container(std::move(container)),
      m_clearer(std::move(clearer))
{}

void WebRTCConnection::AddDataChannel(std::shared_ptr<rtc::DataChannel> dc)
{
    m_dataChannels.insert(dc->label(), std::move(dc));
}

void WebRTCConnection::RemoveDataChannel(const std::string& label)
{
    m_dataChannels.erase(label);
}

void WebRTCConnection::OnPeerConnectionClosed(std::shared_ptr<WebRTCConnection> _this)
{
    // TODO (losty-rtc): very ugly clearing
    if (m_closed) {
        return;
    }
    if (auto clearer = m_clearer.lock()) {
        clearer->clear();
        clearer->emplace_back(_this);
    }
    if (auto container = m_container.lock()) {
        container->erase(m_ip);
    }
    m_closed = true;
}

void WebRTCConnection::AddRemoteCandidate(const rtc::Candidate& candidate)
{
    m_peerConnection->addRemoteCandidate(candidate);
}
