
#include "webrtc/webrtcprotocol.h"

#include <memory>
#include <rtc/peerconnection.hpp>

#include "univalue.h"
#include "webrtc/webrtcconnection.h"


WebRTCProtocol::WebRTCProtocol(std::shared_ptr<IRequestProcessor> requestHandler) 
    : m_requestHandler(std::move(requestHandler)),
      m_connections(std::make_shared<ProtectedMap<std::string, std::shared_ptr<WebRTCConnection>>>())
{
    // TODO (losty-rtc): maybe another stun servers
    m_config.iceServers = {
            {"stun:stun.l.google.com:19302"},
//            {"stun:stun3.l.google.com:19302"},
    };
}


bool WebRTCProtocol::Process(const UniValue& message, const std::string& ip, const std::shared_ptr<rtc::WebSocket>& ws)
{
    if (!message.exists("type"))
        return false;
    auto type = message["type"].get_str();
    // TODO (losty-rtc): move to selector
    if (type == "offer") {
        if (!message.exists("sdp")) {
            // TODO (losty-rtc): error
            return false;
        }
        auto pc = std::make_shared<rtc::PeerConnection>(m_config);
        auto webrtcConnectionnn = std::make_shared<WebRTCConnection>(ip, std::weak_ptr(m_connections), pc);

        pc->onLocalCandidate([ws = std::weak_ptr(ws), ip](rtc::Candidate candidate) {
            UniValue message(UniValue::VOBJ);
            // TODO (losty-rtc): receiver ip here. // message.pushKV("ip", ip);
            message.pushKV("type", "candidate");
            message.pushKV("candidate", std::string(candidate));
            message.pushKV("mid", candidate.mid());
            if (auto lock = ws.lock()) {
                lock->send(constructProtocolMessage(message, ip).write());
            }
        });
        pc->onLocalDescription([ws = std::weak_ptr(ws), ip](rtc::Description description) {
            UniValue message(UniValue::VOBJ);
            // TODO (losty-rtc): receiver ip here. // message.pushKV("ip", ip);
            message.pushKV("type", description.typeString());
            message.pushKV("sdp", std::string(description));

            if (auto lock = ws.lock()) {
                lock->send(constructProtocolMessage(message, ip).write());
            }
        });
        pc->onStateChange([webrtcConnection = std::weak_ptr(webrtcConnectionnn)](rtc::PeerConnection::State state) {
            switch (state) {
                case rtc::PeerConnection::State::Failed:
                case rtc::PeerConnection::State::Disconnected:
                case rtc::PeerConnection::State::Closed: {
                    if (auto lock = webrtcConnection.lock()) {
                        lock->OnPeerConnectionClosed();
                    }
                    break;
                }
                default: {
                    int pulp = 1;
                }
            }
        });
        pc->onDataChannel([requestHandler = m_requestHandler, webrtcConnection = std::weak_ptr(webrtcConnectionnn)](std::shared_ptr<rtc::DataChannel> dataChannel) {
            // TODO (losty-rtc): MOST INTERESTED THING.
            // Add dataChannel to class that will setup it and provide rpc handlers to it.
            const auto label = dataChannel->label(); // "notifications" for websocket functional and "rpc" for rpc
            dataChannel->onClosed([webrtcConnection, label]() {
                if (auto lock = webrtcConnection.lock()) {
                    lock->RemoveDataChannel(label);
                }
            });
            dataChannel->onMessage([/*TODO (losty-rtc): a bit dirty hack to allow free memory for datachannel*/dataChannel = std::weak_ptr(dataChannel), requestHandler](rtc::message_variant data) {
                if (!std::holds_alternative<std::string>(data)) {
                    // TOOD (losty-rtc): error
                    return;
                }
                auto dc = dataChannel.lock();
                if (!dc) {
                    // Freeing memory
                    return;
                }
                // auto message = std::get<std::string>(data);
                UniValue message(UniValue::VOBJ);
                message.read(std::get<std::string>(data));
                if (!message.exists("path") || !message.exists("requestData")) {
                    // TODO (losty-rtc): error
                    return;
                }
                auto path = message["path"].get_str();
                auto body = message["requestData"].write();
                // TODO (losty-rtc): probably unique replier for all
                auto replier = std::make_shared<DataChannelReplier>(dc);
                requestHandler->Process(path, body, replier);

            });
            if (auto lock = webrtcConnection.lock()) {
                lock->AddDataChannel(dataChannel);
            }
        });
        auto sdp = message["sdp"].get_str();
        pc->setRemoteDescription(rtc::Description(sdp, type));

        // insert_or_assign is called to allow client quickly reconnect to us because there is a possible delay
        // between real disconnection and its handling.
        // This can be possibly evaluated by malevolent signaling server to force client disconnection
        // by providing new connection offer with same IP.
        m_connections->insert_or_assign(ip, webrtcConnectionnn);
        return true;
    } else if (type == "candidate") {
        if (!message.exists("candidate") || !message.exists("mid")) {
            return false;
        }
        auto sdp = message["candidate"].get_str();
        auto mid = message["mid"].get_str();
        rtc::Candidate candidate(sdp, mid);
        return m_connections->exec_for_elem(ip, [&candidate](const shared_ptr<WebRTCConnection>& webrtcConnection) {
            webrtcConnection->AddRemoteCandidate(candidate);
        });
    } else {
        return false;
    }
}

inline UniValue WebRTCProtocol::constructProtocolMessage(const UniValue& message, const std::string& ip)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("type", "protocol");
    result.pushKV("ip", ip);
    result.pushKV("message", message);
    return result;
}
