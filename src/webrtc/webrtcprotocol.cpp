
#include "webrtc/webrtcprotocol.h"
#include "univalue.h"
#include <rtc/peerconnection.hpp>


WebRTCProtocol::WebRTCProtocol(std::shared_ptr<IRequestProcessor> requestHandler) 
    : m_requestHandler(std::move(requestHandler)),
      m_peerConnections(std::make_shared<ProtectedMap<std::string, std::shared_ptr<rtc::PeerConnection>>>()),
      m_dataChannels(std::make_shared<ProtectedMap<std::string, std::shared_ptr<rtc::DataChannel>>>())
{
    // TODO (losty-rtc): maybe another stun servers
    m_config.iceServers = {
            {"stun:stun.l.google.com:19302"},
            {"stun:stun3.l.google.com:19302"},

    };
}


bool WebRTCProtocol::Process(const UniValue& message, const std::string& ip, const std::shared_ptr<rtc::WebSocket>& ws)
{
    if (!message.exists("type"))
        return false;
    auto type = message["type"].get_str();
    // TODO (losty-rtc): move to selector
    std::shared_ptr<rtc::PeerConnection> pc;
    if (type == "offer") {
        if (!message.exists("sdp")) {
            // TODO (losty-rtc): error
            return false;
        }
        pc = std::make_shared<rtc::PeerConnection>(m_config);
        pc->onLocalCandidate([ws, ip](rtc::Candidate candidate) {
            UniValue message(UniValue::VOBJ);
            // TODO (losty-rtc): receiver ip here. // message.pushKV("ip", ip);
            message.pushKV("type", "candidate");
            message.pushKV("candidate", std::string(candidate));
            message.pushKV("mid", candidate.mid());

            ws->send(constructProtocolMessage(message, ip).write());
        });
        pc->onLocalDescription([ws, ip](rtc::Description description) {
            UniValue message(UniValue::VOBJ);
            // TODO (losty-rtc): receiver ip here. // message.pushKV("ip", ip);
            message.pushKV("type", description.typeString());
            message.pushKV("sdp", std::string(description));

            ws->send(constructProtocolMessage(message, ip).write());
        });
        pc->onStateChange([ip, peerConnections = m_peerConnections](rtc::PeerConnection::State state) {
            switch (state) {
                case rtc::PeerConnection::State::Failed:
                case rtc::PeerConnection::State::Disconnected: {
                    // TODO (losty-rtc): dead lock if removing here!
                    peerConnections->erase(ip);
                    break;
                }
                case rtc::PeerConnection::State::Closed: {
                    // The only cased it is called is when peer conenction is destroyed;
                    break;
                }
                default: {
                    int pulp = 1;
                }
            }
        });
        pc->onDataChannel([requestHandler = m_requestHandler, ip, dcMap = m_dataChannels](std::shared_ptr<rtc::DataChannel> dataChannel) {
            // TODO (losty-rtc): MOST INTERESTED THING.
            // Add dataChannel to class that will setup it and provide rpc handlers to it.
            dataChannel->label(); // "notifications" for websocket functional and "rpc" for rpc
            dataChannel->onClosed([&dcMap, ip]() {
                dcMap->erase(ip);
            });
            dataChannel->onMessage([dataChannel, requestHandler](rtc::message_variant data) {
                if (!std::holds_alternative<std::string>(data)) {
                    // TOOD (losty-rtc): error
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
                auto replier = std::make_shared<DataChannelReplier>(dataChannel);
                requestHandler->Process(path, body, replier);
            });
            dcMap->insert(ip, dataChannel);
        });
        auto sdp = message["sdp"].get_str();
        pc->setRemoteDescription(rtc::Description(sdp, type));

        // Just inserting without assigning. This will cause
        // problems for client if he disconnects and instantly tries to reconnect
        // before the connection is cleared. But this prevents from dropping client
        // connections someone else by somehow specifying client's ip in request.
        m_peerConnections->insert(ip, pc);
        return true;
    } else if (type == "answer") {
        // Node is not going to be initiator of connection
        // TODO (losty-rtc): proove no answer will be here even for setRemoteDescription
        return false;
    } else if (type == "candidate") {
        // TODO (losty-rtc): add candidate to PeerConnection specified in id field
        bool res = false;
        m_peerConnections->exec_for_elem(ip, [message, &res](const shared_ptr<rtc::PeerConnection>& pc) {
            if (!message.exists("candidate") || !message.exists("mid")) {
                return;
            }
            auto sdp = message["candidate"].get_str();
            auto mid = message["mid"].get_str();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
            res = true;
        });
        return res;
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
