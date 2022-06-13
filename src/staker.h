// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_STAKER_H
#define POCKETCOIN_STAKER_H

#include <boost/thread.hpp>
#include <chainparams.h>
#include <util/ref.h>
#include <unordered_set>
#include <memory>

static const bool DEFAULT_STAKINGREQUIRESPEERS = false;

class CWallet;

class Staker
{
public:
    static Staker* getInstance();

    void setIsStaking(bool staking);

    bool getIsStaking();

    uint64_t getLastCoinStakeSearchInterval();

    void startWorkers(
        boost::thread_group& threadGroup,
        const util::Ref& context,
        CChainParams const& chainparams,
        unsigned int minerSleep = 500
    );

    void run(const util::Ref& context, CChainParams const&, boost::thread_group&);

    void worker(const util::Ref& context, CChainParams const&, std::string const& walletName);

    bool signBlock(std::shared_ptr<CBlock>, std::shared_ptr<CWallet>, int64_t);

private:
    Staker();

    Staker(Staker const&);

    void operator=(Staker const&);

    bool workersStarted;
    bool isStaking;
    unsigned int minerSleep;
    uint64_t lastCoinStakeSearchInterval;
    std::unordered_set<std::string> walletWorkers;
};

class StakerWorker
{
public:
    StakerWorker(
        std::string const& walletName,
        unsigned int minerSleep
    );

private:
    std::string walletName;
};

#endif
