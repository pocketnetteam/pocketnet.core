// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/pocketnet.h"

namespace PocketDb
{
    SQLiteDatabase SQLiteDbInst(false);
    TransactionRepository TransRepoInst(SQLiteDbInst);
    ChainRepository ChainRepoInst(SQLiteDbInst);
    RatingsRepository RatingsRepoInst(SQLiteDbInst);
    ConsensusRepository ConsensusRepoInst(SQLiteDbInst);
    ExplorerRepository ExplorerRepoInst(SQLiteDbInst);
    SystemRepository SystemRepoInst(SQLiteDbInst);
    MigrationRepository MigrationRepoInst(SQLiteDbInst);

    CheckpointRepository CheckpointRepoInst;
} // PocketDb

namespace PocketWeb
{
    PocketFrontend PocketFrontendInst;
} // PocketWeb

namespace PocketServices
{
    WebPostProcessor WebPostProcessorInst;
} // namespace PocketServices