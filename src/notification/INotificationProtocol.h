// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATIONS_INOTIFICATIONPROTOCOL_H
#define POCKETNET_CORE_NOTIFICATIONS_INOTIFICATIONPROTOCOL_H

#include "web/IConnection.h"

#include "notification/NotificationClient.h"

#include <univalue.h>


class INotificationProtocol
{
public:
    virtual bool ProcessMessage(const UniValue& msg, std::shared_ptr<IConnection> connection, const std::string& id) = 0;

    virtual void forceDelete(const std::string& id) = 0;
};

#endif // POCKETNET_CORE_NOTIFICATIONS_INOTIFICATIONPROTOCOL_H