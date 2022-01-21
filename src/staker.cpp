// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chrono>
#include <staker.h>

#include <miner.h>
#include <net.h>
#include <pos.h>
#include <validation.h>
#include <wallet/wallet.h>
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
    lastCoinStakeSearchInterval(0)
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

void Staker::startWorkers(
    boost::thread_group& threadGroup,
    const util::Ref& context,
    CChainParams const& chainparams,
    unsigned int minerSleep
)
{
    if (workersStarted) { return; }
    workersStarted = true;

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

        std::unordered_set<std::string> walletNames;

        for (auto& wallet : wallets)
        {
            std::string name(wallet->GetName());

            if (walletWorkers.find(name) == walletWorkers.end())
            {
                // Create a worker thread for the wallet.
                threadGroup.create_thread(
                    boost::bind(
                        &Staker::worker, this, boost::cref(context), boost::cref(chainparams), boost::cref(name)
                    )
                );
                walletWorkers.insert(name);
            }

            walletNames.insert(name);
        }

        std::vector<std::string> removes;

        for (auto& name : walletWorkers)
        {
            if (GetWallet(name) == nullptr)
            {
                removes.push_back(name);
            }
        }

        UninterruptibleSleep(std::chrono::milliseconds{minerSleep * 10});
    }
}

void Staker::worker(const util::Ref& context, CChainParams const& chainparams, std::string const& walletName)
{
    LogPrintf("Staker thread started for %s\n", walletName);

    util::ThreadRename("coin-staker");

    const auto& node = EnsureNodeContext(context);
    bool running = true;
    int nLastCoinStakeSearchInterval = 0;
    // TODO (losty-critical+): CReverseScript is removed as well as GetScriptForMining().
    // auto coinbaseScript = std::make_shared<CReserveScript>();

    auto wallet = GetWallet(walletName);
    if (!wallet) { return; }
    // wallet->GetScriptForMining(coinbaseScript); // TODO (losty-critical+): this method deleted

    // эта логика похоже должна заменить wallet->GetScriptForMining
    // По сути нам нужен адрес из кошелька взять

    // ---------------------------------------------

    // CScript coinbase_script;
    // std::string error;

    // if (!getScriptFromDescriptor(address_or_descriptor, coinbase_script, error)) {
    //     const auto destination = DecodeDestination(address_or_descriptor);
    //     if (!IsValidDestination(destination)) {
    //         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Error: Invalid address or descriptor");
    //     }

    //     coinbase_script = GetScriptForDestination(destination);
    // }

    // ---------------------------------------------

    // или вот так еще используют

    // CScript coinbase_script = GetScriptForDestination(destination);

    // ---------------------------------------------

    try
    {
        // TODO (losty-critical+): conbasescript removed
        // if (!coinbaseScript || coinbaseScript->reserveScript.empty())
            // throw std::runtime_error("No coinbase script available (staking requires a wallet)");

        while (running && !ShutdownRequested())
        {
            auto wallet = GetWallet(walletName);

            if (!wallet)
            {
                running = false;
                continue;
            }

            while (wallet->IsLocked())
            {
                nLastCoinStakeSearchInterval = 0;
                UninterruptibleSleep(std::chrono::milliseconds{1000});
            }

            if (chainparams.GetConsensus().fPosRequiresPeers)
            {
                do
                {
                    bool fvNodesEmpty;
                    {
                        fvNodesEmpty = !node.connman || node.connman->GetNodeCount(
                            CConnman::CONNECTIONS_ALL
                        ) == 0;
                    }
                    if (!fvNodesEmpty && !::ChainstateActive().IsInitialBlockDownload())
                        break;
                    UninterruptibleSleep(std::chrono::milliseconds{1000});
                } while (true);
            }

            while (!isStaking)
            {
                UninterruptibleSleep(std::chrono::milliseconds{1000});
            }

            while (chainparams.GetConsensus().nPosFirstBlock > ::ChainActive().Tip()->nHeight)
            {
                UninterruptibleSleep(std::chrono::milliseconds{30000});
            }

            uint64_t nFees = 0;
            // TODO (losty-fur): possible null mempool
            auto assembler = BlockAssembler(*node.mempool, chainparams);

            // TODO (losty-critical+): coinbasescript
            auto blocktemplate = assembler.CreateNewBlock(
                Optional<CScript>(), true, &nFees
            );

            auto block = std::make_shared<CBlock>(blocktemplate->block);

            if (signBlock(block, wallet, nFees))
            {
                // Extend pocketBlock with coinStake transaction
                if (auto[ok, ptx] = PocketServices::Serializer::DeserializeTransaction(block->vtx[1]); ok)
                    blocktemplate->pocketBlock->emplace_back(ptx);
                // TODO (losty-fur): possible null chainman
                CheckStake(block, blocktemplate->pocketBlock, wallet, chainparams, *node.chainman, *node.mempool);
                UninterruptibleSleep(std::chrono::milliseconds{500});
            }
            else
            {
                UninterruptibleSleep(std::chrono::milliseconds{minerSleep});
            }
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("Pocketcoin Staker terminated\n");
        throw;
    }
    catch (const std::runtime_error& e)
    {
        LogPrintf("Pocketcoin Staker runtime error: %s\n", e.what());
        return;
    }
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

    static int64_t nLastCoinStakeSearchTime = GetAdjustedTime();

    CKey key;
    CMutableTransaction txCoinStake;
    CTransaction txNew;
    int nBestHeight = ::ChainActive().Tip()->nHeight;

    txCoinStake.nTime = GetAdjustedTime();
    txCoinStake.nTime &= ~STAKE_TIMESTAMP_MASK;

    int64_t nSearchTime = txCoinStake.nTime;

    // TODO (losty-fur): validate this works.
    auto legacyKeyStore = wallet->GetOrCreateLegacyScriptPubKeyMan();
    assert(legacyKeyStore);

    if (nSearchTime > nLastCoinStakeSearchTime)
    {
        int64_t nSearchInterval = nBestHeight + 1 > 0 ? 1 : nSearchTime - nLastCoinStakeSearchTime;
        // TODO (losty-critical+): wallet is no longer a CKeyStore. Research what to use here instead
        // Хороший вопрос, нужен более глубокий анализ
        if (wallet->CreateCoinStake(*legacyKeyStore, block->nBits, nSearchInterval, nFees, txCoinStake, key))
        {
            if (txCoinStake.nTime >= ::ChainActive().Tip()->GetPastTimeLimit() + 1)
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
                    // TODO (losty-critical+): wallet is now not a CKeyStore and SigningProvider. Is GetSolvingProvider suitable here?
                    signSuccess = ProduceSignature(*legacyKeyStore /* *wallet->GetSolvingProvider(scriptPubKey) */,
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
        }
        lastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    }
    else
    {
    }
#endif
    return false;
}
