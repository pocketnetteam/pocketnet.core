// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "webrtc/DataChannelHandler.h"

#include "webrtc/DataChannelConnection.h"
#include "webrtc/webrtcprotocol.h"


std::function<void(rtc::message_variant)> DataChannelHandler::GetRPCHandler(std::shared_ptr<IRequestProcessor> requestProcessor, std::weak_ptr<rtc::DataChannel> dc, const std::string& ip)
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

std::function<void(rtc::message_variant)> DataChannelHandler::GetNotificationsHandler(std::shared_ptr<INotificationProtocol> notificationProtocol, std::weak_ptr<rtc::DataChannel> dc, const std::string& ip)
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
                UniValue message(UniValue::VOBJ);
                if (!message.read(std::get<std::string>(data))) {
                    // TODO (losty-rtc): error
                    return;
                }
                if (notificationProtocol)
                    notificationProtocol->ProcessMessage(message, std::make_shared<DataChannelConnection>(datachannel, ip), ip);
    };
}

