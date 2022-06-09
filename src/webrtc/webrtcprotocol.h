// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEBRTC_WEBRTCPROTOCOL_H
#define POCKETNET_CORE_WEBRTC_WEBRTCPROTOCOL_H


#include "protectedmap.h"
#include "rpcapi/rpcapi.h"
#include "univalue.h"
#include <map>
#include <memory>
#include <string>


#include <rtc/peerconnection.hpp>
#include "rtc/rtc.hpp"
#include "webrtc/webrtcconnection.h"

namespace webrtc {
class DataChannelReplier : public IReplier
{
public:
    DataChannelReplier(std::shared_ptr<rtc::DataChannel> dataChannel, const std::string& ip, const std::optional<std::string>& reqID)
        : m_dataChannel(std::move(dataChannel)),
          m_ip(ip),
          m_reqID(reqID)
    {}
    void WriteReply(int nStatus, const std::string& reply = "") override
    {
        // TODO (losty-rtc): formatting message here
        m_dataChannel->send(ConstructMessage(reply));
    }
    void WriteHeader(const std::string& hdr, const std::string& value) override
    {
        // Ignore
    }
    const std::string& GetPeerStr() const override
    {
        return m_ip;
    }
    bool GetAuthCredentials(std::string& out) override
    {
        // No auth creadentials for datachannel connections
        return false;
    }
    Statistic::RequestTime GetCreated() const override
    {
        return m_created;
    }
protected:
    std::string ConstructMessage(const std::string& reply)
    {
        UniValue data;
        if (!data.read(reply)) {
            // TODO (losty): reply should be always valid json but what if not?
            return reply;
        }
        UniValue msg(UniValue::VOBJ);
        if (m_reqID) {
            msg.pushKV("id", m_reqID.value());
        }
        msg.pushKV("data", data);
        return msg.write();
    }
private:
    std::shared_ptr<rtc::DataChannel> m_dataChannel;
    std::string m_ip;
    std::optional<std::string> m_reqID;
    Statistic::RequestTime m_created = gStatEngineInstance.GetCurrentSystemTime();
};

class WebRTCProtocol
{
public:
    WebRTCProtocol(std::shared_ptr<IRequestProcessor> requestHandler, std::shared_ptr<notifications::INotificationProtocol> notificationProtocol, std::shared_ptr<Queue<std::shared_ptr<WebRTCConnection>>> clearQueue);
    bool Process(const UniValue& message, const std::string& ip, const std::shared_ptr<rtc::WebSocket>& ws);
    void StopAll();
protected:
    static inline UniValue constructProtocolMessage(const UniValue& message, const std::string& ip);

private:
    // TODO (losty-rtc): move out from protocol
    std::shared_ptr<ProtectedMap<std::string, std::shared_ptr<WebRTCConnection>>> m_connections;
    std::shared_ptr<IRequestProcessor> m_requestHandler;
    std::shared_ptr<notifications::INotificationProtocol> m_notificationProtocol;
    rtc::Configuration m_config;
    std::shared_ptr<Queue<std::shared_ptr<WebRTCConnection>>> m_clearQueue;
};
} // namespace webrtc

#endif // POCKETNET_CORE_WEBRTC_WEBRTCPROTOCOL_H