// Copyright (c) 2017 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_POS_H
#define POCKETCOIN_POS_H

#include <validation.h>
#include <streams.h>

static const int STAKE_TIMESTAMP_MASK = 15;

static const int MODIFIER_INTERVAL_RATIO = 3;

class CWallet;

double GetPosDifficulty(const CBlockIndex* blockindex);

double GetPoWMHashPS();

double GetPoSKernelPS();

static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime);

arith_uint256 GetProofOfStakeLimit(int nHeight);

bool TransactionGetCoinAge(CTransactionRef transaction, uint64_t& nCoinAge);

inline unsigned int GetTargetSpacing(int nHeight) {
  return 150;
}

static int64_t GetStakeModifierSelectionIntervalSection(int nSection);

static int64_t GetStakeModifierSelectionInterval();

static bool SelectBlockFromCandidates(std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp, std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockIndex** pindexSelected);

int64_t GetProofOfStakeReward(int nHeight, int64_t nFees, const Consensus::Params & consensusParams);

int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd);

bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime, CWallet* wallet, CDataStream& hashProofOfStakeSource);

bool CheckStakeKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, CBlockIndex& blockFrom, CTransactionRef const & txPrev, COutPoint const & prevout, unsigned int nTimeTx, arith_uint256& hashProofOfStake, CDataStream& hashProofOfStakeSource, arith_uint256& targetProofOfStake, bool fPrintProofOfStake = true);

bool CheckProofOfStake(CBlockIndex* pindexPrev, CTransactionRef const & tx, unsigned int nBits, arith_uint256& hashProofOfStake, CDataStream& hashProofOfStakeSource, arith_uint256& targetProofOfStake, std::vector<CScriptCheck> *pvChecks, bool fCheckSignature = false);

bool CheckStake(const std::shared_ptr<CBlock> pblock, std::shared_ptr<CWallet> wallet, CChainParams const & chainparams);

bool CheckCoinStakeTimestamp(int nHeight, int64_t nTimeBlock, int64_t nTimeTx);

bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);

bool GetRatingRewards(CAmount nCredit, std::vector<CTxOut>& results, CAmount& totalAmount, const CBlockIndex* pindex, CDataStream& hashProofOfStakeSource, const CBlock* block = nullptr);

bool GenerateOuts(CAmount nCredit, std::vector<CTxOut>& results, CAmount& totalAmount, std::vector<std::string> winners, opcodetype op_code_type);

#endif // POCKETCOIN_POS_H
