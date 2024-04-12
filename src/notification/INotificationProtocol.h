// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATIONS_INOTIFICATIONPROTOCOL_H
#define POCKETNET_CORE_NOTIFICATIONS_INOTIFICATIONPROTOCOL_H

#include "web/IConnection.h"

#include "notification/NotificationClient.h"

#include <univalue.h>

namespace notifications {
/**
 * Class that should be used to process messages via Notification protocol (Subscribing/Unsubscribing)
 */
class INotificationProtocol
{
public:
    /**
     * Processing sibscribe/unsubscribe requests. Memory is not clearing in case of disconnect and user of this class should control it and call forceDelete() in such cases.
     * @param msg - message containing subscibe/unsubscribe request
     * @param connection - connection to respond with notifications
     * @param id - unique identifier to handle this connection (currently IP address)
     * @return true - if message successfully parsed and requested action was performed. Returns true even if it is resubscription of existed subscribr
     * @return false - if message is in unexpected format
     */
    virtual bool ProcessMessage(const std::string& msg, std::shared_ptr<IConnection> connection, const std::string& id) = 0;

    /**
     * Force deleting the connection from notification without unsubscribe message.
     * Use this in case of disconnections of the client.
     * @param id 
     */
    virtual void forceDelete(const std::string& id) = 0;
};
} // namespace notifications

#endif // POCKETNET_CORE_NOTIFICATIONS_INOTIFICATIONPROTOCOL_H