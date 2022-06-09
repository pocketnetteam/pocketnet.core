// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATIONS_H
#define POCKETNET_CORE_NOTIFICATIONS_H

#include "notification/INotifications.h"

#include "validation.h"

class Notifications : public INotifications
{
public:
    Notifications();
    std::shared_ptr<INotificationProtocol> GetProtocol() const override;
    std::shared_ptr<NotificationQueue> GetNotificationQueue() const override;
    UniValue CollectStats() const override;
    void Start(int threads) override;
    void Stop() override;
private:
    std::shared_ptr<INotificationProtocol> m_protocol;
    std::shared_ptr<Queue<std::pair<CBlock, CBlockIndex*>>> m_queue;
    std::vector<std::shared_ptr<QueueEventLoopThread<std::pair<CBlock, CBlockIndex*>>>> m_workers;
    std::shared_ptr<NotifyableStorage> m_clients;
};

#endif // POCKETNET_CORE_NOTIFICATIONS_H