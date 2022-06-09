// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "notification/NotificationsFactory.h"

#include "notification/Notifications.h"

std::unique_ptr<INotifications> NotificationsFactory::NewNotifications()
{
    return std::make_unique<Notifications>();
}

