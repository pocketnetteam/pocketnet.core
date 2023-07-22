// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_CHECKPOINT_REPOSITORY_H
#define SRC_CHECKPOINT_REPOSITORY_H

#include <chainparams.h>
#include <util/system.h>
#include "pocketdb/models/base/PocketTypes.h"

namespace PocketDb
{
    using namespace std;
    using namespace PocketTx;

    class CheckpointSocialDb
    {
    private:
        vector<tuple<string, int, int>> _socialCheckpoints;
        void InitMain();
        void InitTest();
    public:
        CheckpointSocialDb(NetworkId network)
        {
            if (network == NetworkId::NetworkMain)
                InitMain();
            if (network == NetworkId::NetworkTest)
                InitTest();
        }
        const vector<tuple<string, int, int>>& Checkpoints() { return _socialCheckpoints; }
    }; // CheckpointSocialDb


    class CheckpointLotteryDb
    {
    private:
        vector<tuple<int, string>> _lotteryCheckpoints;
        void InitMain();
        void InitTest();
    public:
        CheckpointLotteryDb(NetworkId network)
        {
            if (network == NetworkId::NetworkMain)
                InitMain();
            if (network == NetworkId::NetworkTest)
                InitTest();
        }
        const vector<tuple<int, string>>& Checkpoints() { return _lotteryCheckpoints; }
    }; // CheckpointLotteryDb


    class CheckpointOpReturnDb
    {
    private:
        vector<tuple<string, string>> _opReturnCheckpoints;
        void InitMain();
        void InitTest();
    public:
        CheckpointOpReturnDb(NetworkId network)
        {
            if (network == NetworkId::NetworkMain)
                InitMain();
            if (network == NetworkId::NetworkTest)
                InitTest();
        }
        const vector<tuple<string, string>>& Checkpoints() { return _opReturnCheckpoints; }
    }; // CheckpointOpReturnDb


    class CheckpointRepository
    {
    public:
        bool IsSocialCheckpoint(const string& txHash, TxType txType, int code);
        bool IsLotteryCheckpoint(int height, const string& hash);
        bool IsOpReturnCheckpoint(const string& txHash, const string& hash);

    }; // namespace PocketDb
}
#endif //SRC_CHECKPOINT_REPOSITORY_H
