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

    class CheckpointDb
    {
    protected:
        virtual void InitMain() {}
        virtual void InitTest() {}
    public:
        CheckpointDb(NetworkId network)
        {
            if (network == NetworkId::NetworkMain)
                InitMain();
            if (network == NetworkId::NetworkTest)
                InitTest();
        }
    }; // CheckpointDb

    class CheckpointSocialDb : public CheckpointDb
    {
    private:
        vector<tuple<string, int, int>> _socialCheckpoints;
    protected:
        void InitMain() override;
        void InitTest() override;
    public:
        CheckpointSocialDb(NetworkId network) : CheckpointDb(network) {}
        const vector<tuple<string, int, int>>& Checkpoints() { return _socialCheckpoints; }
    }; // CheckpointSocialDb


    class CheckpointLotteryDb : public CheckpointDb
    {
    private:
        vector<tuple<int, string>> _lotteryCheckpoints;
    protected:
        void InitMain() override;
        void InitTest() override;
    public:
        CheckpointLotteryDb(NetworkId network) : CheckpointDb(network) {}
        const vector<tuple<int, string>>& Checkpoints() { return _lotteryCheckpoints; }
    }; // CheckpointLotteryDb


    class CheckpointOpReturnDb : public CheckpointDb
    {
    private:
        vector<tuple<string, string>> _opReturnCheckpoints;
    protected:
        void InitMain() override;
        void InitTest() override;
    public:
        CheckpointOpReturnDb(NetworkId network) : CheckpointDb(network) {}
        const vector<tuple<string, string>>& Checkpoints() { return _opReturnCheckpoints; }
    }; // CheckpointOpReturnDb


    class CheckpointRepository
    {
    private:
        NetworkId _network;
    public:
        CheckpointRepository(NetworkId network);

        bool IsSocialCheckpoint(const string& txHash, TxType txType, int code);
        bool IsLotteryCheckpoint(int height, const string& hash);
        bool IsOpReturnCheckpoint(const string& txHash, const string& hash);

    }; // namespace PocketDb
}
#endif //SRC_CHECKPOINT_REPOSITORY_H
