// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#include "notification/wshandlerprovider.h"
#include "web/WSConnection.h"

#include <univalue.h>
#include <validation.h>

std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, std::shared_ptr<SimpleWeb::InMessage>)> WsHandlerProvider::on_message()
{
    return [](std::shared_ptr<SimpleWeb::IWSConnection> connection,
        std::shared_ptr<SimpleWeb::InMessage> in_message)
    {
        auto out_message = in_message->string();

        try
        {
            notificationProcessor->GetProtocol()->ProcessMessage(out_message, std::make_shared<WSConnection>(connection), connection->ID());
        }
        catch (const std::exception &e)
        {
            LogPrintf("Warning: ws.on_message - %s\n", e.what());
        }
    };
}

std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, int, const std::string &)> WsHandlerProvider::on_close()
{
    return [](std::shared_ptr<SimpleWeb::IWSConnection> connection, int status, const std::string& /*reason*/)
    {
        if (notificationProcessor) {
            notificationProcessor->GetProtocol()->forceDelete(connection->ID());
        }
    };
}

std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, const SimpleWeb::error_code &)> WsHandlerProvider::on_error()
{
    return [](std::shared_ptr<SimpleWeb::IWSConnection> connection, const SimpleWeb::error_code& ec)
    {
        if (notificationProcessor) {
            notificationProcessor->GetProtocol()->forceDelete(connection->ID());
        }
    };
}
