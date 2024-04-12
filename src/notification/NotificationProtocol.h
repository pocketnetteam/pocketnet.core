// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATIONS_NOTIFICATIONPROTOCOL_H
#define POCKETNET_CORE_NOTIFICATIONS_NOTIFICATIONPROTOCOL_H


#include "notification/INotificationProtocol.h"

#include "notification/NotificationTypedefs.h"

#include "protectedmap.h"
#include <memory>
#include <algorithm>

namespace notifications {
class NotificationProtocol : public INotificationProtocol
{
public:
    explicit NotificationProtocol(std::shared_ptr<NotifyableStorage> clients);
    bool ProcessMessage(const std::string& msg, std::shared_ptr<IConnection> connection, const std::string& id) override;

    void forceDelete(const std::string& id) override;

private:
    std::shared_ptr<NotifyableStorage> m_clients;
};
} // namespace notifications

#endif // POCKETNET_CORE_NOTIFICATIONS_NOTIFICATIONPROTOCOL_H