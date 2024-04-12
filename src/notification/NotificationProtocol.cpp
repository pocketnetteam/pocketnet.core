// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "notification/NotificationProtocol.h"

#include "validation.h"

notifications::NotificationProtocol::NotificationProtocol(std::shared_ptr<NotifyableStorage> clients)
    : m_clients(std::move(clients))
{}

// TODO (losty-nat) : split ProcessMessage for string/UniValue msg
bool notifications::NotificationProtocol::ProcessMessage(const std::string& msg, std::shared_ptr<IConnection> connection, const std::string& id)
{
    UniValue val;

    if (msg == "0") // Ping/Pong
    {
        connection->Send("1");
    }
    else if (val.read(msg)) // Messages with data
    {
        std::vector<std::string> keys = val.getKeys();
        if (std::find(keys.begin(), keys.end(), "addr") != keys.end())
        {
            std::string _addr = val["addr"].get_str();

            int block = ChainActive().Height();
            if (std::find(keys.begin(), keys.end(), "block") != keys.end()) block = val["block"].get_int();

            std::string ip = connection->GetRemoteEndpointAddress();
            bool service = std::find(keys.begin(), keys.end(), "service") != keys.end();

            int mainPort = 8899;
            if (std::find(keys.begin(), keys.end(), "mainport") != keys.end())
                mainPort = val["mainport"].get_int();

            int wssPort = 8099;
            if (std::find(keys.begin(), keys.end(), "wssport") != keys.end())
                wssPort = val["wssport"].get_int();

            if (std::find(keys.begin(), keys.end(), "nonce") != keys.end())
            {
                NotificationClient wsUser = {std::weak_ptr(connection), _addr, block, ip, service, mainPort, wssPort};
                m_clients->insert_or_assign(id, wsUser);
            } else if (std::find(keys.begin(), keys.end(), "msg") != keys.end())
            {
                if (val["msg"].get_str() == "unsubscribe")
                {
                    m_clients->erase(id);
                }
            }

            return true;
        }
    }

    return false;
}

void notifications::NotificationProtocol::forceDelete(const std::string& id)
{
    m_clients->erase(id);
}
