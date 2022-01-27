// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <validation.h>

arith_uint256 bnProofOfStakeLimit(~arith_uint256() >> 14);
arith_uint256 GetProofOfStakeLimit(int nHeight)
{
  return (bnProofOfStakeLimit);
}

// ppcoin: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(
  const CBlockIndex* pindex,
  bool fProofOfStake
)
{
  while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake)) {
    pindex = pindex->pprev;
  }
  return pindex;
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    bool isProofOfStake (params.nPosFirstBlock <= (pindexLast->nHeight));
    assert(pindexLast != nullptr);

    if (!isProofOfStake) {
      unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

      // Only change once per difficulty adjustment interval
      if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
      {
        if (params.fPowAllowMinDifficultyBlocks)
        {
          // Special difficulty rule for testnet:
          // If the new block's timestamp is more than 2* 10 minutes
          // then allow mining of a min-difficulty block.
          if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
            return nProofOfWorkLimit;
          else
          {
            // Return the last non-special-min-difficulty-rules-block
            const CBlockIndex* pindex = pindexLast;
            while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
              pindex = pindex->pprev;
            return pindex->nBits;
          }
        }
        return pindexLast->nBits;
      }

      // Go back by what we want to be 14 days worth of blocks
      int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
      assert(nHeightFirst >= 0);
      const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
      assert(pindexFirst);

      return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
    } else {

      arith_uint256 nProofOfWorkLimit = isProofOfStake ? GetProofOfStakeLimit(pindexLast->nHeight) : UintToArith256(params.powLimit);

      if (pindexLast == NULL) {
        return nProofOfWorkLimit.GetCompact(); // genesis block
      }

      const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, isProofOfStake);
      if (pindexPrev->pprev == NULL) {
        return nProofOfWorkLimit.GetCompact(); // first block
      }

      const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, isProofOfStake);
      if (pindexPrevPrev->pprev == NULL) {
        return nProofOfWorkLimit.GetCompact(); // second block
      }

      int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();

      if (nActualSpacing < 0) {
        nActualSpacing = params.nPosTargetSpacing;
      }

      arith_uint256 arithNew;
      arithNew.SetCompact(pindexPrev->nBits);

      arith_uint256 nInterval = (params.nPosTargetTimespan) / (params.nPosTargetSpacing);
      arithNew *= (((nInterval - 1)) * (params.nPosTargetSpacing) + (nActualSpacing) + (nActualSpacing));
      arithNew /= (((nInterval + 1)) * (params.nPosTargetSpacing));

      if (arithNew <= 0 || arithNew > nProofOfWorkLimit) {
        arithNew = nProofOfWorkLimit;
      }

      return arithNew.GetCompact();
    }
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params, int height)
{

  if (height >= params.nPosFirstBlock) {
      return true;
    }

    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
