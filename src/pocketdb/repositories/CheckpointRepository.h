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


    class CheckpointSocialDb
    {
    private:
        vector<tuple<string, int, int>> _socialCheckpoints;
    public:
        CheckpointSocialDb();
        vector<tuple<string, int, int>>& Checkpoints() { return _socialCheckpoints; }
    }; // CheckpointSocialDb


    class CheckpointLotteryDb
    {
    private:
        vector<tuple<int, string>> _lotteryCheckpoints;
    public:
        CheckpointLotteryDb();
        vector<tuple<int, string>>& Checkpoints() { return _lotteryCheckpoints; }
    }; // CheckpointLotteryDb


    class CheckpointOpReturnDb
    {
    private:
        vector<tuple<string, string>> _opReturnCheckpoints;
    public:
        CheckpointOpReturnDb();
        vector<tuple<string, string>>& Checkpoints() { return _opReturnCheckpoints; }
    }; // CheckpointOpReturnDb


    class CheckpointRepository
    {
    public:
        CheckpointRepository() = default;

        bool IsSocialCheckpoint(const string& txHash, TxType txType, int code);
        bool IsLotteryCheckpoint(int height, const string& hash);
        bool IsOpReturnCheckpoint(const string& txHash, const string& hash);

    }; // namespace PocketDb
}
#endif //SRC_CHECKPOINT_REPOSITORY_H
