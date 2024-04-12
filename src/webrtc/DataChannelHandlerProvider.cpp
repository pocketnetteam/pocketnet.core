// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/DataChannelHandlerProvider.h"

#include "webrtc/DataChannelConnection.h"
#include "webrtc/webrtcprotocol.h"


std::function<void(rtc::message_variant)> webrtc::DataChannelHandlerProvider::GetRPCHandler(std::shared_ptr<IRequestProcessor> requestProcessor, std::weak_ptr<rtc::DataChannel> dc, const std::string& ip)
{
    return [dc, requestProcessor, ip](rtc::message_variant data) {
                if (!std::holds_alternative<std::string>(data)) {
                    // TOOD (losty-rtc): error
                    return;
                }
                auto datachannel = dc.lock();
                if (!datachannel) {
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
                std::optional<std::string> reqID;
                if (message.exists("id") && message["id"].isStr()) reqID = message["id"].get_str();
                auto replier = std::make_shared<DataChannelReplier>(datachannel, ip, reqID);
                requestProcessor->Process(path, body, replier);
            };
}

std::function<void(rtc::message_variant)> webrtc::DataChannelHandlerProvider::GetNotificationsHandler(std::shared_ptr<notifications::INotificationProtocol> notificationProtocol, std::weak_ptr<rtc::DataChannel> dc, const std::string& ip)
{
    return [notificationProtocol, dc, ip](rtc::message_variant data) {
        if (!std::holds_alternative<std::string>(data)) {
            // TOOD (losty-rtc): error
            return;
        }

        auto datachannel = dc.lock();
        if (!datachannel) {
            // Freeing memory
            return;
        }
        
        if (notificationProtocol)
        {
            auto msg = std::get<std::string>(data);
            notificationProtocol->ProcessMessage(msg, std::make_shared<DataChannelConnection>(datachannel, ip), ip);
        }
    };
}

