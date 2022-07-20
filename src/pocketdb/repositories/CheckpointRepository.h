// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_CHECKPOINT_REPOSITORY_H
#define SRC_CHECKPOINT_REPOSITORY_H

#include <util.h>
#include "pocketdb/models/base/PocketTypes.h"

namespace PocketDb
{
    using namespace std;
    using namespace PocketTx;

    class CheckpointRepository
    {
    private:
        map<string, tuple<int, int>> _socialCheckpoints;
        map<int, string> _lotteryCheckpoints;
        map<string, string> _opReturnCheckpoints;
        
    public:
        CheckpointRepository();

        bool IsSocialCheckpoint(const string& txHash, TxType txType, int code);
        bool IsLotteryCheckpoint(int height, const string& hash);
        bool IsOpReturnCheckpoint(const string& txHash, const string& hash);

    }; // namespace PocketDb
}
#endif //SRC_CHECKPOINT_REPOSITORY_H
