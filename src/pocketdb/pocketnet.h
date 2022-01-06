// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_H
#define POCKETNET_H

#include <boost/algorithm/string.hpp>
#include <chainparams.h>
#include <core_io.h>
#include <sstream>
#include <util.h>

#include "logging.h"

#include "pocketdb/repositories/ChainRepository.h"
#include "pocketdb/repositories/RatingsRepository.h"
#include "pocketdb/repositories/TransactionRepository.h"
#include "pocketdb/repositories/ConsensusRepository.h"
#include "pocketdb/repositories/CheckpointRepository.h"
#include "pocketdb/repositories/web/WebRpcRepository.h"
#include "pocketdb/repositories/web/ExplorerRepository.h"
#include "pocketdb/repositories/web/NotifierRepository.h"
#include "pocketdb/web/PocketFrontend.h"
#include "pocketdb/services/WebPostProcessing.h"

namespace PocketDb
{
    extern SQLiteDatabase SQLiteDbInst;
    extern TransactionRepository TransRepoInst;
    extern ChainRepository ChainRepoInst;
    extern RatingsRepository RatingsRepoInst;
    extern ConsensusRepository ConsensusRepoInst;
    extern NotifierRepository NotifierRepoInst;
    extern ExplorerRepository ExplorerRepoInst;

    extern SQLiteDatabase SQLiteDbCheckpointInst;
    extern CheckpointRepository CheckpointRepoInst;
} // namespace PocketDb

namespace PocketServices
{
    extern WebPostProcessor WebPostProcessorInst;
} // namespace PocketServices

namespace PocketWeb
{
    extern PocketFrontend PocketFrontendInst;
} // namespace PocketWeb

#endif // POCKETNET_H