// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATIONS_NOTIFICATIONSFACTORY_H
#define POCKETNET_CORE_NOTIFICATIONS_NOTIFICATIONSFACTORY_H

#include "notification/INotifications.h"

#include <memory>
namespace notifications {
class NotificationsFactory
{
public:
    std::unique_ptr<INotifications> NewNotifications();
};
} // namespace notifications

#endif // POCKETNET_CORE_NOTIFICATIONS_NOTIFICATIONSFACTORY_H