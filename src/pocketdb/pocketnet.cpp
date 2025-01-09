// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/pocketnet.h"

namespace PocketDb
{
    SQLiteDatabase SQLiteDbInst(false);
    TransactionRepository TransRepoInst(SQLiteDbInst, false);
    ChainRepository ChainRepoInst(SQLiteDbInst, false);
    RatingsRepository RatingsRepoInst(SQLiteDbInst, false);
    ConsensusRepository ConsensusRepoInst(SQLiteDbInst, false);
    ExplorerRepository ExplorerRepoInst(SQLiteDbInst, false);
    SystemRepository SystemRepoInst(SQLiteDbInst, false);
    MigrationRepository MigrationRepoInst(SQLiteDbInst, false);

    CheckpointRepository CheckpointRepoInst;
} // PocketDb

namespace PocketWeb
{
    PocketFrontend PocketFrontendInst;
} // PocketWeb

namespace PocketServices
{
    WebPostProcessor WebPostProcessorInst;
    // WalController WalControllerInst;
} // namespace PocketServices