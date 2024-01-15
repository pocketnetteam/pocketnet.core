// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "notification/Notifications.h"

#include "notification/NotificationProtocol.h"
#include "notification/notifyprocessor.h"

notifications::Notifications::Notifications()
{
    m_clients = std::make_shared<NotifyableStorage>();
    m_protocol = std::make_shared<NotificationProtocol>(m_clients);
    m_queue = std::make_shared<Queue<std::pair<CBlock, CBlockIndex*>>>();
}

std::shared_ptr<notifications::INotificationProtocol> notifications::Notifications::GetProtocol() const
{
    return m_protocol;
}

std::shared_ptr<notifications::NotificationQueue> notifications::Notifications::GetNotificationQueue() const
{
    return m_queue;
}

UniValue notifications::Notifications::CollectStats() const
{
    UniValue proxies(UniValue::VARR);
    auto fillProxy = [&proxies](const std::pair<const std::string, NotificationClient>& it) {
                if (it.second.Service) {
                    UniValue proxy(UniValue::VOBJ);
                    proxy.pushKV("address", it.second.Address);
                    proxy.pushKV("ip", it.second.Ip);
                    proxy.pushKV("port", it.second.MainPort);
                    proxy.pushKV("portWss", it.second.WssPort);
                    proxies.push_back(proxy);
                }
            };
    m_clients->Iterate(fillProxy);

    return proxies;
}

void notifications::Notifications::Start(int threads)
{
    
    for (int i = 0; i < threads; i++) {
        auto worker = std::make_shared<QueueEventLoopThread<std::pair<CBlock, CBlockIndex*>>>(m_queue, std::make_shared<NotifyBlockProcessor>(m_clients));
        worker->Start();
        m_workers.emplace_back(std::move(worker));
    }
}

void notifications::Notifications::Stop()
{
    for (auto& worker : m_workers) {
        worker->Stop();
    }
    m_workers.clear();
}
