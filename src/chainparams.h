// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_CHAINPARAMS_H
#define POCKETCOIN_CHAINPARAMS_H

#include <chainparamsbase.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <protocol.h>

#include <memory>
#include <vector>

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;

    int GetHeight() const {
        if (mapCheckpoints.empty())
            return 0;
        return mapCheckpoints.rbegin()->first;
    }
};

/**
 * Holds various statistics on transactions within a chain. Used to estimate
 * verification progress during chain sync.
 *
 * See also: CChainParams::TxData, GuessVerificationProgress.
 */
struct ChainTxData {
    int64_t nTime;    //!< UNIX timestamp of last known number of transactions
    int64_t nTxCount; //!< total number of transactions between genesis and that timestamp
    double dTxRate;   //!< estimated number of transactions per second after that timestamp
};

enum NetworkId
{
    NetworkMain,
    NetworkTest,
    NetworkRegTest
};

// TODO (losty): this is kinda duplicate of pocketdb/consensus/Base.h::ConsensusCheckpoint
// consider to use something like generalized fork registry (named forks perhaps?) to use them here as well as in Base.h
class ForkPoint
{
public:
    ForkPoint(int cleanSubversion, int main_height, int test_height, int reg_height)
        : m_cleanSubVersion(cleanSubversion),
          m_main_height(main_height),
          m_test_height(test_height),
          m_reg_height(reg_height)
    {
        assert(m_cleanSubVersion > 0);
    }

    int GetHeight(NetworkId network) const
    {
        switch (network) {
            case NetworkId::NetworkMain: {
                return m_main_height;
            }
            case NetworkId::NetworkTest: {
                return m_test_height;
            }
            case NetworkId::NetworkRegTest: {
                return m_reg_height;
            }
        }
    }

    int GetVersion() const
    {
        return m_cleanSubVersion;
    }

private:
    const int m_cleanSubVersion;
    const int m_main_height;
    const int m_test_height;
    const int m_reg_height;
};

class SocialForks
{
public:
    SocialForks() = default;
    SocialForks(std::vector<ForkPoint> forkPoints)
        : m_forkPoints(std::move(forkPoints))
    {}

    // cleanSubversion = CLIENT_VERSION_MAJOR * 1000000 + CLIENT_VERSION_MINOR * 10000 + CLIENT_VERSION_REVISION * 100 + CLIENT_VERSION_BUILD
    int GetLastAcceptedSubVersion(int height) const;

private:
    std::vector<ForkPoint> m_forkPoints;
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Pocketcoin system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    uint16_t GetDefaultPort() const { return nDefaultPort; }
    uint16_t GetDefaultPort(Network net) const
    {
        return net == NET_I2P ? I2P_SAM31_PORT : GetDefaultPort();
    }
    uint16_t GetDefaultPort(const std::string& addr) const
    {
        CNetAddr a;
        return a.SetSpecial(addr) ? GetDefaultPort(a.GetNetwork()) : GetDefaultPort();
    }

    const CBlock& GenesisBlock() const { return genesis; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Policy: Filter transactions that do not match well-defined patterns */
    bool RequireStandard() const { return fRequireStandard; }
    /** If this chain is exclusively used for testing */
    bool IsTestChain() const { return m_is_test_chain; }
    /** If this chain allows time to be mocked */
    bool IsMockableChain() const { return m_is_mockable_chain; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    /** Minimum free space (in GB) needed for data directory */
    uint64_t AssumedBlockchainSize() const { return m_assumed_blockchain_size; }
    /** Minimum free space (in GB) needed for data directory when pruned; Does not include prune target*/
    uint64_t AssumedChainStateSize() const { return m_assumed_chain_state_size; }
    const SocialForks& Forks() const { return socialForks; }
    /** Whether it is possible to mine blocks on demand (no retargeting) */
    bool MineBlocksOnDemand() const { return consensus.fPowNoRetargeting; }
    /** Return the network string */
    std::string NetworkIDString() const { return strNetworkID; }
    NetworkId NetworkID() const { return networkId; }
    /** Return the list of hostnames to look up for DNS seeds */
    const std::vector<std::string>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::string& Bech32HRP() const { return bech32_hrp; }
    const std::vector<uint8_t>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    const ChainTxData& TxData() const { return chainTxData; }
protected:
    CChainParams() {}

    Consensus::Params consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    int nDefaultPort;
    uint64_t nPruneAfterHeight;
    uint64_t m_assumed_blockchain_size;
    uint64_t m_assumed_chain_state_size;
    std::vector<std::string> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    std::string bech32_hrp;
    std::string strNetworkID;
    NetworkId networkId;
    CBlock genesis;
    std::vector<uint8_t> vFixedSeeds;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool m_is_test_chain;
    bool m_is_mockable_chain;
    CCheckpointData checkpointData;
    ChainTxData chainTxData;
    SocialForks socialForks = {{
        // TODO (release): check (may be fulfill with earlier forks)
        {210300 /* 0.21.3 */, 2360000, 1950500, 0},
        {220000 /* 0.22.0 */, 2583000, 2267333, 0},
        {220300 /* 0.22.3 */, 2794500, 2574300, 0}
    }};
};

/**
 * Creates and returns a std::unique_ptr<CChainParams> of the chosen chain.
 * @returns a CChainParams* of the chosen chain.
 * @throws a std::runtime_error if the chain is not supported.
 */
std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams &Params();

/**
 * Sets the params returned by Params() to those for the given chain name.
 * @throws std::runtime_error when the chain is not supported.
 */
void SelectParams(const std::string& chain);

#endif // POCKETCOIN_CHAINPARAMS_H
