// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "notification/NotificationProtocol.h"

#include "validation.h"

notifications::NotificationProtocol::NotificationProtocol(std::shared_ptr<NotifyableStorage> clients)
    : m_clients(std::move(clients))
{}

bool notifications::NotificationProtocol::ProcessMessage(const UniValue& msg, std::shared_ptr<IConnection> connection, const std::string& id)
{
    std::vector<std::string> keys = msg.getKeys();
    if (std::find(keys.begin(), keys.end(), "addr") != keys.end())
    {
        std::string _addr = msg["addr"].get_str();

        int block = ChainActive().Height();
        if (std::find(keys.begin(), keys.end(), "block") != keys.end()) block = msg["block"].get_int();

        std::string ip = connection->GetRemoteEndpointAddress();
        bool service = std::find(keys.begin(), keys.end(), "service") != keys.end();

        int mainPort = 8899;
        if (std::find(keys.begin(), keys.end(), "mainport") != keys.end())
            mainPort = msg["mainport"].get_int();

        int wssPort = 8099;
        if (std::find(keys.begin(), keys.end(), "wssport") != keys.end())
            wssPort = msg["wssport"].get_int();

        if (std::find(keys.begin(), keys.end(), "nonce") != keys.end())
        {
            NotificationClient wsUser = {connection, _addr, block, ip, service, mainPort, wssPort};
            m_clients->insert_or_assign(id, wsUser);
        } else if (std::find(keys.begin(), keys.end(), "msg") != keys.end())
        {
            if (msg["msg"].get_str() == "unsubscribe")
            {
                m_clients->erase(id);
            }
        }

        return true;
    }

    return false;
}

void notifications::NotificationProtocol::forceDelete(const std::string& id)
{
    m_clients->erase(id);
}
