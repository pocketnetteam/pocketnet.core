
#include "webrtc/webrtcconnection.h"

#include <utility>


webrtc::WebRTCConnection::WebRTCConnection(std::string ip, std::weak_ptr<ProtectedMap<std::string, std::shared_ptr<WebRTCConnection>>> container, std::shared_ptr<Queue<std::shared_ptr<WebRTCConnection>>> clearer, std::shared_ptr<rtc::PeerConnection> pc)
    : m_ip(std::move(ip)),
      m_peerConnection(std::move(pc)),
      m_container(std::move(container)),
      m_clearer(std::move(clearer))
{}

void webrtc::WebRTCConnection::AddDataChannel(std::shared_ptr<rtc::DataChannel> dc)
{
    m_dataChannels.insert(dc->label(), std::move(dc));
}

void webrtc::WebRTCConnection::RemoveDataChannel(const std::string& label)
{
    m_dataChannels.erase(label);
}

void webrtc::WebRTCConnection::OnPeerConnectionClosed(std::shared_ptr<WebRTCConnection> _this)
{
    // TODO (losty-rtc): very ugly clearing
    // The main idea for this dirty-hack:
    // We can know that peer connection is closed only by callback "onStateChange".
    // If we immediatly remove connection from internal container in that callback we will get a leak
    // because PeerConnection destructor is joining it's internal thread that had executed callback.
    // This means the thread can't be joined untill callback is finished. So we can't destruct a peer connection in
    // the callback. For that we delete PeerConnection from container and forward the last shared_ptr of connection ("_this")
    // to another thread that will do nothing but destroy the last shared pointer.
    // This is dirty because requires guarantees that "_this" is the only shared_ptr that handles the connection.
    if (m_closed) {
        return;
    }

    // TODO (losty-rtc): maybe move this to eventloop
    if (auto container = m_container.lock()) {
        container->erase(m_ip);
    }

    // Move is required to guarantee that destruction will occur in different thread.
    m_clearer->Add(std::move(_this));

    m_closed = true;
}

void webrtc::WebRTCConnection::AddRemoteCandidate(const rtc::Candidate& candidate)
{
    m_peerConnection->addRemoteCandidate(candidate);
}
