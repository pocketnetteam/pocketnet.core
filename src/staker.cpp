// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chrono>
#include <staker.h>

#include <miner.h>
#include <net.h>
#include <pos.h>
#include <validation.h>
#include <script/sign.h>
#include <consensus/merkle.h>
#include <rpc/blockchain.h>
#include <node/context.h>

#include "logging.h"
#include "pocketdb/services/Serializer.h"
#include "script/signingprovider.h"
#include "shutdown.h"
#include "util/threadnames.h"
#include "util/time.h"

Staker* Staker::getInstance()
{
    static Staker instance;
    return &instance;
}

Staker::Staker() :
    workersStarted(false),
    isStaking(false),
    minerSleep(500),
    lastCoinStakeSearchInterval(0),
    nLastCoinStakeSearchTime(0)
{
}

void Staker::setIsStaking(bool staking)
{
    isStaking = staking;
}

bool Staker::getIsStaking()
{
    return isStaking;
}

uint64_t Staker::getLastCoinStakeSearchInterval()
{
    return lastCoinStakeSearchInterval;
}

uint64_t Staker::getLastCoinStakeSearchTime()
{
    return nLastCoinStakeSearchTime;
}

void Staker::startWorkers(
    boost::thread_group& threadGroup,
    const util::Ref& context,
    CChainParams const& chainparams,
    unsigned int minerSleep
)
{
    if (workersStarted) { return; }
    workersStarted = true;
    m_interrupt.reset();

    this->minerSleep = minerSleep;
    threadGroup.create_thread(
        boost::bind(
            &Staker::run, this, boost::cref(context), boost::cref(chainparams), boost::ref(threadGroup)
        )
    );
}

void Staker::run(const util::Ref& context, CChainParams const& chainparams, boost::thread_group& threadGroup)
{
    while (!ShutdownRequested())
    {
        auto wallets = GetWallets();

        for (auto& wallet : wallets)
        {
            if (walletWorkers.find(wallet->GetName()) == walletWorkers.end())
            {
                // Create a worker thread for the wallet.
                threadGroup.create_thread(
                    boost::bind(
                        &Staker::worker, this, boost::cref(context), boost::cref(chainparams), boost::cref(wallet->GetName())
                    )
                );

                walletWorkers.insert(wallet->GetName());
            }
        }

        for (auto it = walletWorkers.begin(); it != walletWorkers.end(); )
        {
            if (GetWallet(*it) == nullptr)
                it = walletWorkers.erase(it);
            else
                ++it;
        }

        m_interrupt.sleep_for(std::chrono::milliseconds{minerSleep * 10});
    }
}

void Staker::worker(const util::Ref& context, CChainParams const& chainparams, std::string const& walletName)
{
    LogPrintf("Staker worker thread started for \"%s\"\n", walletName);

    util::ThreadRename("coin-staker");

    const auto& node = EnsureNodeContext(context);
    CHECK_NONFATAL(node.mempool); // Mempool should be always available here
    CHECK_NONFATAL(node.chainman); // Same for this
    bool running = true;

    auto wallet = GetWallet(walletName);
    if (!wallet) return;

    try
    {
        while (running && !ShutdownRequested())
        {
            auto wallet = GetWallet(walletName);

            if (!wallet)
            {
                running = false;
                continue;
            }

            while (wallet->IsLocked() && !ShutdownRequested())
                m_interrupt.sleep_for(std::chrono::milliseconds{1000});

            if (gArgs.GetBoolArg("-stakingrequirespeers", DEFAULT_STAKINGREQUIRESPEERS))
            {
                do
                {
                    if (ShutdownRequested())
                        break;

                    bool fvNodesEmpty = !node.connman || node.connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0;

                    if (!fvNodesEmpty && !::ChainstateActive().IsInitialBlockDownload())
                        break;

                    m_interrupt.sleep_for(std::chrono::milliseconds{1000});
                } while (true);
            }

            while (!isStaking)
            {
                if (ShutdownRequested())
                    break;

                m_interrupt.sleep_for(std::chrono::milliseconds{1000});
            }

            while (chainparams.GetConsensus().nPosFirstBlock > ::ChainActive().Tip()->nHeight)
            {
                if (ShutdownRequested())
                    break;

                m_interrupt.sleep_for(std::chrono::milliseconds{30000});
            }

            if ((GetAdjustedTime() & ~STAKE_TIMESTAMP_MASK) > nLastCoinStakeSearchTime)
            {
                uint64_t nFees = 0;
                auto assembler = BlockAssembler(*node.mempool, chainparams);

                // Nullopt for scriptPubKeyIn because coinbase script is only usefull for mining blocks
                auto blocktemplate = assembler.CreateNewBlock(
                    nullopt, true, &nFees
                );
 
                auto block = std::make_shared<CBlock>(blocktemplate->block);

                if (signBlock(block, wallet, nFees))
                {
                    // Extend pocketBlock with coinStake transaction
                    if (auto[ok, ptx] = PocketServices::Serializer::DeserializeTransaction(block->vtx[1]); ok)
                        blocktemplate->pocketBlock->emplace_back(ptx);

                    CheckStake(block, blocktemplate->pocketBlock, wallet, chainparams, *node.chainman, *node.mempool);
                    UninterruptibleSleep(std::chrono::milliseconds{minerSleep});
                }
                else
                {
                    m_interrupt.sleep_for(std::chrono::milliseconds{minerSleep});
                }
            }
        }
    }
    catch (const std::runtime_error& e)
    {
        LogPrintf("Staker worker thread runtime error: \"%s\"\n", e.what());
        return;
    }

    LogPrintf("Staker worker thread stopped for \"%s\"\n", walletName);
}

bool Staker::stake(const util::Ref& context, CChainParams const& chainparams, unsigned int blocks, const std::string& wallet_name)
{
    const auto& node = EnsureNodeContext(context);
    CHECK_NONFATAL(node.mempool); // Mempool should be always available here
    CHECK_NONFATAL(node.chainman); // Same for this

    auto wallet = GetWallet(wallet_name);
    if (!wallet) return false;

    try
    {
        while (wallet->IsLocked() && !ShutdownRequested())
            m_interrupt.sleep_for(std::chrono::milliseconds{100});

        if (ShutdownRequested())
            return false;

        uint64_t nFees = 0;
        auto assembler = BlockAssembler(*node.mempool, chainparams);

        for (unsigned int i = 0; i < blocks; i++)
        {
            bool blockSigned = false;
            while (!blockSigned)
            {
                // Nullopt for scriptPubKeyIn because coinbase script is only usefull for mining blocks
                auto blocktemplate = assembler.CreateNewBlock(
                    nullopt, true, &nFees
                );

                auto block = std::make_shared<CBlock>(blocktemplate->block);

                if (signBlock(block, wallet, nFees))
                {
                    // Extend pocketBlock with coinStake transaction
                    if (auto[ok, ptx] = PocketServices::Serializer::DeserializeTransaction(block->vtx[1]); ok)
                        blocktemplate->pocketBlock->emplace_back(ptx);

                    blockSigned = CheckStake(block, blocktemplate->pocketBlock, wallet, chainparams, *node.chainman, *node.mempool);
                }

                UninterruptibleSleep(std::chrono::milliseconds{50});
            }
        }

        return true;
    }
    catch (const std::runtime_error& e)
    {
        LogPrintf("Staker runtime error: %s\n", e.what());
        return false;
    }

    return true;
}

bool Staker::signBlock(std::shared_ptr<CBlock> block, std::shared_ptr<CWallet> wallet, int64_t nFees)
{
#ifdef ENABLE_WALLET
    std::vector<CTransactionRef> vtx = block->vtx;
    // if we are trying to sign
    // something other than a proof-of-stake block template
    if (!vtx[0]->vout[0].IsEmptyOrWinners())
    {
        LogPrintf("SignBlock() : Trying to sign malformed proof-of-stake block template\n");
        return false;
    }

    // if we are trying to sign a complete proof-of-stake block
    if (block->IsProofOfStake())
    {
        LogPrintf("Block is already proof of stake. No need to sign");
        return true;
    }

    CKey key;
    CMutableTransaction txCoinStake;
    CTransaction txNew;
    int nBestHeight = ::ChainActive().Tip()->nHeight;

    txCoinStake.nTime = GetAdjustedTime();
    txCoinStake.nTime &= ~STAKE_TIMESTAMP_MASK;

    int64_t nSearchTime = txCoinStake.nTime;

    auto legacyKeyStore = wallet->GetOrCreateLegacyScriptPubKeyMan();
    assert(legacyKeyStore);

    if (nSearchTime > nLastCoinStakeSearchTime ||              // For main network algorithm in full-time mode
       Params().NetworkID() == NetworkId::NetworkRegTest)      // For regtest we can skip time checks
    {
        int64_t nSearchInterval = nBestHeight + 1 > 0 ? 1 : nSearchTime - nLastCoinStakeSearchTime;
        if (wallet->CreateCoinStake(*legacyKeyStore, block->nBits, nSearchInterval, nFees, txCoinStake, key))
        {
            if (txCoinStake.nTime > ::ChainActive().Tip()->GetMedianTimePast())
            {
                return sign(block, txCoinStake, vtx, *legacyKeyStore, key, wallet);
            }
        }

        lastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    }
#endif

    return false;
}

bool Staker::sign(std::shared_ptr<CBlock> block, CMutableTransaction& txCoinStake, std::vector<CTransactionRef>& vtx,
    LegacyScriptPubKeyMan& legacyKeyStore, CKey& key, std::shared_ptr<CWallet> wallet)
{
    // make sure coinstake would meet timestamp protocol
    // as it would be the same as the block timestamp
    CMutableTransaction txn(*block->vtx[0].get());
    txn.nTime = block->nTime = txCoinStake.nTime;
    block->vtx[0] = MakeTransactionRef(std::move(txn));

    // We have to make sure that we have no future timestamps in
    // our transactions set
    for (auto it = vtx.begin(); it != vtx.end();)
    {
        auto tx = *it;
        if (tx->nTime > block->nTime)
        {
            it = vtx.erase(it);
        }
        else
        {
            ++it;
        }
    }

    txCoinStake.nVersion = CTransaction::CURRENT_VERSION;

    // After the changes, we need to resign inputs.
    CMutableTransaction txNewConst(txCoinStake);

    for (unsigned int i = 0; i < txCoinStake.vin.size(); i++)
    {
        bool signSuccess;
        uint256 prevHash = txCoinStake.vin[i].prevout.hash;
        uint32_t n = txCoinStake.vin[i].prevout.n;
        assert(wallet->mapWallet.count(prevHash));
        auto prevTx = wallet->GetWalletTx(prevHash);
        const CScript& scriptPubKey = prevTx->tx->vout[n].scriptPubKey;
        SignatureData sigdata;
        signSuccess = ProduceSignature(legacyKeyStore,
            MutableTransactionSignatureCreator(&txNewConst, i, prevTx->tx->vout[n].nValue, SIGHASH_ALL),
            scriptPubKey, sigdata);

        if (!signSuccess)
        {
            return false;
        }
        else
        {
            UpdateInput(txCoinStake.vin[i], sigdata);
        }
    }

    CTransactionRef txNew = MakeTransactionRef(std::move(txCoinStake));
    block->vtx.insert(block->vtx.begin() + 1, txNew);
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return key.Sign(block->GetHash(), block->vchBlockSig);
}

void Staker::stop()
{
    // TODO (losty): this should be called only after shutdown requested
    //               because otherwise sleeps will be just skipped but thread will
    //               not exited from it's loop that can result in very big cpu consumption. Probably exit
    //               loop by checking "if (m_interrupt)" instead of checking for ShutdownRequested?
    m_interrupt();
}
