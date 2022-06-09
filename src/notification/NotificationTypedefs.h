// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFICATION_TYPEDEFS_H
#define POCKETNET_CORE_NOTIFICATION_TYPEDEFS_H

#include "eventloop.h"
#include "protectedmap.h"
#include <string>


class CBlock;
class CBlockIndex;

using NotificationQueue = IQueueAdjuster<std::pair<CBlock, CBlockIndex*>>;

class NotificationClient;
using NotifyableStorage = ProtectedMap<std::string, NotificationClient>;

#endif // POCKETNET_CORE_NOTIFICATION_TYPEDEFS_H