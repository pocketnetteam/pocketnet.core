// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#include "websocket/wshandlerprovider.h"

#include <univalue.h>
#include <validation.h>

std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, std::shared_ptr<SimpleWeb::InMessage>)> WsHandlerProvider::on_message()
{
    return [](std::shared_ptr<SimpleWeb::IWSConnection> connection,
        std::shared_ptr<SimpleWeb::InMessage> in_message)
    {
        UniValue val;
        auto out_message = in_message->string();

        if (out_message == "0") // Ping/Pong
        {
            connection->send("1", [](const SimpleWeb::error_code& ec) {});
        }
        else if (val.read(out_message)) // Messages with data
        {
            try
            {
                std::vector<std::string> keys = val.getKeys();
                if (std::find(keys.begin(), keys.end(), "addr") != keys.end())
                {
                    std::string _addr = val["addr"].get_str();

                    int block = ChainActive().Height();
                    if (std::find(keys.begin(), keys.end(), "block") != keys.end()) block = val["block"].get_int();

                    std::string ip = connection->remote_endpoint_address();
                    bool service = std::find(keys.begin(), keys.end(), "service") != keys.end();

                    int mainPort = 8899;
                    if (std::find(keys.begin(), keys.end(), "mainport") != keys.end())
                        mainPort = val["mainport"].get_int();

                    int wssPort = 8099;
                    if (std::find(keys.begin(), keys.end(), "wssport") != keys.end())
                        wssPort = val["wssport"].get_int();

                    if (std::find(keys.begin(), keys.end(), "nonce") != keys.end())
                    {
                        WSUser wsUser = {connection, _addr, block, ip, service, mainPort, wssPort};
                        WSConnections->insert_or_assign(connection->ID(), wsUser);

                        UniValue m(UniValue::VOBJ);
                        m.pushKV("result", "Registered");
                        connection->send(m.write(), [](const SimpleWeb::error_code& ec) {});

                        return;
                    }
                    else if (std::find(keys.begin(), keys.end(), "msg") != keys.end())
                    {
                        if (val["msg"].get_str() == "unsubscribe")
                        {
                            WSConnections->erase(connection->ID());

                            UniValue m(UniValue::VOBJ);
                            m.pushKV("result", "Unsubscribed");
                            connection->send(m.write(), [](const SimpleWeb::error_code& ec) {});

                            return;
                        }
                    }
                }

                UniValue m(UniValue::VOBJ);
                m.pushKV("result", "Nothing happened");
                connection->send(m.write(), [](const SimpleWeb::error_code& ec) {});
            }
            catch (const std::exception &e)
            {
                UniValue m(UniValue::VOBJ);
                m.pushKV("result", "error");
                m.pushKV("error", e.what());
                connection->send(m.write(), [](const SimpleWeb::error_code& ec) {});
            }
        }
    };
}

std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, int, const std::string &)> WsHandlerProvider::on_close()
{
    return [](std::shared_ptr<SimpleWeb::IWSConnection> connection, int status, const std::string& /*reason*/)
    {
        WSConnections->erase(connection->ID());
    };
}

std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, const SimpleWeb::error_code &)> WsHandlerProvider::on_error()
{
    return [](std::shared_ptr<SimpleWeb::IWSConnection> connection, const SimpleWeb::error_code& ec)
    {
        WSConnections->erase(connection->ID());
    };
}
