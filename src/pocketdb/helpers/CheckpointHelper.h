// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_CHECKPOINTS_HPP
#define POCKETHELPERS_CHECKPOINTS_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketHelpers
{
    using namespace std;
    using namespace PocketConsensus;

    /*
     *
     * Checkpoints for inconsistent social consensus rules 
     * 
     */
    class SocialCheckpoints
    {
    private:
        // TxHash - Consensus check result code
        multimap<string, tuple<PocketTxType, SocialConsensusResult>> _checkpoints;

    public:

        bool IsCheckpoint(const string& txHash, PocketTxType txType, SocialConsensusResult code)
        {
            auto checkpoints = _checkpoints.equal_range(txHash);
            for (auto it = checkpoints.first; it != checkpoints.second; it++)
            {
                auto[_type, _code] = it->second;
                if (_type == txType && _code == code)
                    return true;
            }
            
            return false;
        }

        SocialCheckpoints();
    };


    /*
     *
     * Checkpoints for lottery inconsistent transactions
     *
     */
    class LotteryCheckpoints
    {
    private:
        // Height - Block Hash
        map<int, string> _checkpoints;

    public:

        bool IsCheckpoint(int height, const string& hash)
        {
            if (_checkpoints.find(height) == _checkpoints.end()) return false;
            return _checkpoints[height] == hash;
        }

        LotteryCheckpoints();
    };


    /*
     *
     * Checkpoints for inconsistent OP_RETURN HASH
     *
     */
    class OpReturnCheckpoints
    {
    private:
        // Tx Hash - Payload Hash
        map<string, string> _checkpoints;

    public:

        bool IsCheckpoint(const string& txHash, const string& hash)
        {
            if (_checkpoints.find(txHash) == _checkpoints.end()) return false;
            return _checkpoints[txHash] == hash;
        }

        OpReturnCheckpoints();
    };

} // namespace PocketHelpers

#endif // POCKETHELPERS_CHECKPOINTS_HPP