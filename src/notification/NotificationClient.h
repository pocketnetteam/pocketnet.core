// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATIONS_NOTIFYABLECLIENT_H
#define POCKETNET_CORE_NOTIFICATIONS_NOTIFYABLECLIENT_H

#include "web/IConnection.h"

#include "protectedmap.h"
#include <string>
#include <memory>

// Struct for connecting users
struct NotificationClient
{
    std::shared_ptr<IConnection> Connection;
    std::string Address;
    int Block;
    std::string Ip;
    bool Service;
    int MainPort;
    int WssPort;
};

#endif // POCKETNET_CORE_NOTIFICATIONS_NOTIFYABLECLIENT_H