// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Pocketcoin Core developers
// Copyright (c) 2018 The Pocketcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <validation.h>
#include <pos.h>
#include <pow.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <wallet/wallet.h>
#include <utiltime.h>
#include <consensus/validation.h>
#include <index/addrindex.h>
#include <antibot/antibot.h>
#include <boost/range/adaptor/reversed.hpp>

double GetPosDifficulty(const CBlockIndex* blockindex) {
	// Floating point number that is a multiple of the minimum difficulty,
	// minimum difficulty = 1.0.
	if (blockindex == nullptr) {
		if (chainActive.Tip() == nullptr) {
			return 1.0;
		}
		else {
			blockindex = GetLastBlockIndex(chainActive.Tip(), false);
		}
	}

	int nShift = (blockindex->nBits >> 24) & 0xff;

	double dDiff = (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

	while (nShift < 29) {
		dDiff *= 256.0;
		nShift++;
	}
	while (nShift > 29) {
		dDiff /= 256.0;
		nShift--;
	}

	return dDiff;
}

double GetPoWMHashPS() {
	return 0;
}

double GetPoSKernelPS() {
	int nPoSInterval = 72;
	double dStakeKernelsTriedAvg = 0;
	int nStakesHandled = 0, nStakesTime = 0;

	CBlockIndex* pindex = chainActive.Tip();
	CBlockIndex* pindexPrevStake = NULL;

	while (pindex && nStakesHandled < nPoSInterval) {
		if (pindex->IsProofOfStake()) {
			if (pindexPrevStake) {
				dStakeKernelsTriedAvg += GetPosDifficulty(pindexPrevStake) * 4294967296.0;
				nStakesTime += pindexPrevStake->nTime - pindex->nTime;
				nStakesHandled++;
			}
			pindexPrevStake = pindex;
		}

		pindex = pindex->pprev;
	}

	double result = 0;

	if (nStakesTime) {
		result = dStakeKernelsTriedAvg / nStakesTime;
	}

	result *= STAKE_TIMESTAMP_MASK + 1;

	return result;
}

int64_t GetProofOfStakeReward(int nHeight, int64_t nFees, const Consensus::Params & consensusParams) {
	int halvings = nHeight / consensusParams.nSubsidyHalvingInterval;
	// Force block reward to zero when right shift is undefined.
	if (halvings >= 64) {
		return nFees;
	}

	CAmount nSubsidy = 5 * COIN;

	// Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
	nSubsidy >>= halvings;
	return nSubsidy + nFees;
}

int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd) {
	// Kernel hash weight starts from 0 at the min age
	// this change increases active coins participating the hash and helps
	// to secure the network when proof-of-stake difficulty is low
	return nIntervalEnd - nIntervalBeginning - Params().GetConsensus().nStakeMinAge;
}

bool CheckStake(const std::shared_ptr<CBlock> pblock, std::shared_ptr<CWallet> wallet, CChainParams const & chainparams) {
	arith_uint256 proofHash = arith_uint256(0), hashTarget = arith_uint256(0);
	uint256 hashBlock = pblock->GetHash();

	if (!pblock->IsProofOfStake()) {
		return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex());
	}

	// verify hash target and signature of coinstake tx
	CDataStream hashProofOfStakeSource(SER_GETHASH, 0);
	if (!CheckProofOfStake(mapBlockIndex[pblock->hashPrevBlock], pblock->vtx[1], pblock->nBits, proofHash, hashProofOfStakeSource, hashTarget, NULL)) {
		return error("CheckStake() : proof-of-stake checking failed (%s)", pblock->hashPrevBlock.GetHex());
	}

	//// debug print
	LogPrintf("=== CheckStake() : new PoS block found hash: %s\n", hashBlock.GetHex());

	// Found a solution
	{
		LOCK(cs_main);
		auto hashBestChain = chainActive.Tip()->GetBlockHash();
		if (pblock->hashPrevBlock != hashBestChain) {
			return error("CheckStake() : generated block is stale");
		}

		GetMainSignals().BlockFound(pblock->GetHash());

	}

	// Process this block the same as if we had received it from another node
	CValidationState state;
	if (!ProcessNewBlock(state, chainparams, pblock, true, /* fReceived */ false, NULL))
	{
		return error("CoinStaker: ProcessNewBlock, block not accepted %s", state.GetRejectReason());
	}

	return true;
}

bool CheckProofOfStake(CBlockIndex* pindexPrev, CTransactionRef const & tx, unsigned int nBits, arith_uint256& hashProofOfStake, CDataStream& hashProofOfStakeSource, arith_uint256& targetProofOfStake, std::vector<CScriptCheck> *pvChecks, bool fCheckSignature) {
	if (!tx->IsCoinStake()) {
		return error("CheckProofOfStake() : called on non-coinstake %s", tx->GetHash().ToString());
	}

	// Kernel (input 0) must match the stake hash target per coin age (nBits)
	const CTxIn& txin = tx->vin[0];

	CTransactionRef txPrev;
	uint256 hashBlock = uint256();
	if (!GetTransaction(txin.prevout.hash, txPrev, Params().GetConsensus(), hashBlock, true)) {
		return error("CheckProofOfStake() : INFO: read txPrev failed %s", txin.prevout.hash.GetHex());  // previous transaction not in main chain, may occur during initial download
	}

	if (pvChecks) {
		pvChecks->reserve(tx->vin.size());
	}

	if (fCheckSignature) {
		const CTransaction & txn = *tx;
		PrecomputedTransactionData txdata(txn);
		const COutPoint &prevout = tx->vin[0].prevout;
		const Coin* coins = &pcoinsTip->AccessCoin(prevout);
		assert(coins);

		// Verify signature
		CScriptCheck check(coins->out, *tx, 0, SCRIPT_VERIFY_NONE, false, &txdata);
		if (pvChecks) {
			pvChecks->push_back(CScriptCheck());
			check.swap(pvChecks->back());
		}
		else if (!check()) {
			return error("CheckProofOfStake() : script-verify-failed %s", ScriptErrorString(check.GetScriptError()));
		}
	}

	if (mapBlockIndex.count(hashBlock) == 0) {
		return error("CheckProofOfStake() : read block failed"); // unable to read block of previous transaction
	}

	CBlockIndex* pblockindex = mapBlockIndex[hashBlock];

	if (txin.prevout.hash != txPrev->GetHash()) {
		return error("CheckProofOfStake(): Coinstake input does not match previous output %s", txin.prevout.hash.GetHex());
	}

	if (!CheckStakeKernelHash(pindexPrev, nBits, *pblockindex, txPrev, txin.prevout, tx->nTime, hashProofOfStake, hashProofOfStakeSource, targetProofOfStake)) {
		return error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx->GetHash().ToString(), hashProofOfStake.ToString()); // may occur during initial download or if behind on block chain sync
	}

	return true;
}

bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime, CWallet* wallet, CDataStream& hashProofOfStakeSource) {
	arith_uint256 hashProofOfStake, targetProofOfStake;

	CTransactionRef txPrev;
	uint256 hashBlock = uint256();
	if (!GetTransaction(prevout.hash, txPrev, Params().GetConsensus(), hashBlock, true)) {
		LogPrintf("CheckKernel : Could not find previous transaction %s\n", prevout.hash.ToString());
		return false;
	}

	if (mapBlockIndex.count(hashBlock) == 0) {
		LogPrintf("CheckKernel : Could not find block of previous transaction %s\n", hashBlock.ToString());
		return false;
	}

	CBlockIndex* pblockindex = mapBlockIndex[hashBlock];

	if (pblockindex->GetBlockTime() + Params().GetConsensus().nStakeMinAge > nTime) {
		return false;
	}

	if (pBlockTime) {
		*pBlockTime = pblockindex->GetBlockTime();
	}

	if (!wallet->mapWallet.count(prevout.hash)) {
		return("CheckProofOfStake(): Couldn't get Tx Index");
	}

	return CheckStakeKernelHash(pindexPrev, nBits, *pblockindex, txPrev,
		prevout, nTime, hashProofOfStake, hashProofOfStakeSource, targetProofOfStake);
}

bool CheckStakeKernelHash(CBlockIndex * pindexPrev, unsigned int nBits, CBlockIndex & blockFrom, CTransactionRef const & txPrev, COutPoint const & prevout, unsigned int nTimeTx, arith_uint256& hashProofOfStake, CDataStream& hashProofOfStakeSource, arith_uint256& targetProofOfStake, bool fPrintProofOfStake) {

	unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();

	if (nTimeTx < txPrev->nTime) {
		LogPrintf(" === ERROR: CheckStakeKernelHash() : nTime violation");
		return error("CheckStakeKernelHash() : nTime violation");
	}


	if (nTimeBlockFrom + Params().GetConsensus().nStakeMinAge > nTimeTx) {
		LogPrintf(" === ERROR: CheckStakeKernelHash() : min age violation");
		return error("CheckStakeKernelHash() : min age violation");
	}

	// Base target
	arith_uint256 bnTarget;
	bnTarget.SetCompact(nBits);

	// Weighted target
	int64_t nValueIn = txPrev->vout[prevout.n].nValue;
	arith_uint256 bnWeight = std::min(
		nValueIn, Params().GetConsensus().nStakeMaximumThreshold
	);
	bnTarget *= bnWeight;

	targetProofOfStake = bnTarget;

	uint64_t nStakeModifier = pindexPrev->nStakeModifier;
	int nStakeModifierHeight = pindexPrev->nHeight;
	int64_t nStakeModifierTime = pindexPrev->nTime;

	// Calculate hash
	CDataStream ss(SER_GETHASH, 0);
	ss << nStakeModifier << nTimeBlockFrom << txPrev->nTime << prevout.hash << prevout.n << nTimeTx;
	hashProofOfStakeSource = ss;
	hashProofOfStake = UintToArith256(Hash(ss.begin(), ss.end()));

	if (fPrintProofOfStake)
	{
		//LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from timestamp=%s\n",
		//    nStakeModifier, nStakeModifierHeight,
		//    FormatISO8601DateTime(nStakeModifierTime),
		//    FormatISO8601DateTime(nTimeBlockFrom));
		//LogPrintf("CheckStakeKernelHash() : check modifier=0x%016x nTimeBlockFrom=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s bnTarget=%s nBits=%08x nValueIn=%d bnWeight=%s\n",
		//    nStakeModifier,
		//    nTimeBlockFrom, txPrev->nTime, prevout.n, nTimeTx,
		//    hashProofOfStake.ToString(),bnTarget.ToString(), nBits, nValueIn,bnWeight.ToString());
	}

	// Now check if proof-of-stake hash meets target protocol
	if (hashProofOfStake > bnTarget) {
		return false;
	}

	if (!fPrintProofOfStake)
	{
		LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from timestamp=%s\n",
			nStakeModifier, nStakeModifierHeight,
			FormatISO8601DateTime(nStakeModifierTime),
			FormatISO8601DateTime(nTimeBlockFrom));
		LogPrintf("CheckStakeKernelHash() : pass modifier=0x%016x nTimeBlockFrom=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
			nStakeModifier,
			nTimeBlockFrom, txPrev->nTime, prevout.n, nTimeTx,
			hashProofOfStake.ToString());
	}

	return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int nHeight, int64_t nTimeBlock, int64_t nTimeTx)
{
	if (nHeight > 0) {
		return (nTimeBlock == nTimeTx) && ((nTimeTx & STAKE_TIMESTAMP_MASK) == 0);
	}
	else {
		return (nTimeBlock == nTimeTx);
	}
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
	nStakeModifier = 0;
	fGeneratedStakeModifier = false;
	if (!pindexPrev)
	{
		fGeneratedStakeModifier = true;
		return error("ComputeNextStakeModifier(): Could not find pindexPrev");  // genesis block's modifier is 0
	}
	// First find current stake modifier and its generation block time
	// if it's not old enough, return the same stake modifier
	int64_t nModifierTime = 0;
	if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime)) {
		return error("ComputeNextStakeModifier: unable to get last modifier");
	}
	//LogPrintf("ComputeNextStakeModifier: prev modifier=0x%016x time=%s\n", nStakeModifier, FormatISO8601DateTime(nModifierTime));
	if (nModifierTime / Params().GetConsensus().nModifierInterval >= pindexPrev->GetBlockTime() / Params().GetConsensus().nModifierInterval) {
		return true;
	}

	// Sort candidate blocks by timestamp
	std::vector<std::pair<int64_t, uint256> > vSortedByTimestamp;
	vSortedByTimestamp.reserve(64 * Params().GetConsensus().nModifierInterval / GetTargetSpacing(pindexPrev->nHeight));
	int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
	int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / Params().GetConsensus().nModifierInterval)
		* Params().GetConsensus().nModifierInterval - nSelectionInterval;
	//LogPrintf("nSelectionInterval = %d nSelectionIntervalStart = %d\n",nSelectionInterval,nSelectionIntervalStart);

	const CBlockIndex* pindex = pindexPrev;
	while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
	{
		vSortedByTimestamp.push_back(std::make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
		pindex = pindex->pprev;
	}
	int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
	std::reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
	std::sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

	// Select 64 blocks from candidate blocks to generate stake modifier
	uint64_t nStakeModifierNew = 0;
	int64_t nSelectionIntervalStop = nSelectionIntervalStart;
	std::map<uint256, const CBlockIndex*> mapSelectedBlocks;
	for (int nRound = 0; nRound < std::min(64, (int)vSortedByTimestamp.size()); nRound++)
	{
		// add an interval section to the current selection round
		nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
		// select a block from the candidates of current round
		if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex)) {
			return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
		}
		// write the entropy bit of the selected block
		nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
		// add the selected block from candidates to selected list
		mapSelectedBlocks.insert(std::make_pair(pindex->GetBlockHash(), pindex));
		//        LogPrint("stakemodifier", "ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n", nRound, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nSelectionIntervalStop), pindex->nHeight, pindex->GetStakeEntropyBit());
	}

	// Print selection map for visualization of the selected blocks
	if (LogAcceptCategory(BCLog::STAKEMODIF)) {
		std::string strSelectionMap = "";
		// '-' indicates proof-of-work blocks not selected
		strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
		pindex = pindexPrev;
		while (pindex && pindex->nHeight >= nHeightFirstCandidate)
		{
			// '=' indicates proof-of-stake blocks not selected
			if (pindex->IsProofOfStake()) {
				strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
			}
			pindex = pindex->pprev;
		}
		for (auto & item : mapSelectedBlocks) {
			// 'S' indicates selected proof-of-stake blocks
			// 'W' indicates selected proof-of-work blocks
			strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake() ? "S" : "W");
		}
	}

	nStakeModifier = nStakeModifierNew;
	fGeneratedStakeModifier = true;
	return true;
}

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
	if (!pindex)
		return error("GetLastStakeModifier: null pindex");
	while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
		pindex = pindex->pprev;
	if (!pindex->GeneratedStakeModifier()) {
		nStakeModifier = 1;
		nModifierTime = pindex->GetBlockTime();
		return true;
	}
	nStakeModifier = pindex->nStakeModifier;
	nModifierTime = pindex->GetBlockTime();
	return true;
}

// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
	assert(nSection >= 0 && nSection < 64);
	return (Params().GetConsensus().nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
	int64_t nSelectionInterval = 0;
	for (int nSection = 0; nSection < 64; nSection++)
		nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
	return nSelectionInterval;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp, std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
	int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockIndex** pindexSelected)
{
	bool fSelected = false;
	uint256 hashBest = uint256();
	*pindexSelected = (const CBlockIndex*)0;
	for (auto & item : vSortedByTimestamp) {
		if (!mapBlockIndex.count(item.second))
			return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString());
		const CBlockIndex* pindex = mapBlockIndex[item.second];
		if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop) {
			break;

		}

		if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
			continue;
		// compute the selection hash by hashing its proof-hash and the
		// previous proof-of-stake modifier
		CDataStream ss(SER_GETHASH, 0);
		ss << ArithToUint256(pindex->hashProof) << nStakeModifierPrev;
		uint256 hashSelection = Hash(ss.begin(), ss.end());


		// the selection hash is divided by 2**32 so that proof-of-stake block
		// is always favored over proof-of-work block. this is to preserve
		// the energy efficiency property
		if (pindex->IsProofOfStake()) {
			hashSelection = ArithToUint256(UintToArith256(hashSelection) >> 32);
		}

		if (fSelected && hashSelection < hashBest) {
			hashBest = hashSelection;
			*pindexSelected = (const CBlockIndex*)pindex;
		}
		else if (!fSelected) {
			fSelected = true;
			hashBest = hashSelection;
			*pindexSelected = (const CBlockIndex*)pindex;
		}
	}
	return fSelected;
}

bool TransactionGetCoinAge(CTransactionRef transaction, uint64_t& nCoinAge) {
	CAmount bnCentSecond = 0;  // coin age in the unit of cent-seconds
	nCoinAge = 0;

	if (transaction->IsCoinBase()) {
		return true;
	}

	for (auto txin : transaction->vin) {
		// First try finding the previous transaction in database
		CTransactionRef txPrev;
		uint256 hashBlock = uint256();

		if (!GetTransaction(txin.prevout.hash, txPrev, Params().GetConsensus(), hashBlock, true))
			continue;  // previous transaction not in main chain

		if (transaction->nTime < txPrev->nTime)
			return false;  // Transaction timestamp violation

		if (mapBlockIndex.count(hashBlock) == 0)
			return false; //Block not found

		CBlockIndex* pblockindex = mapBlockIndex[hashBlock];

		if (pblockindex->nTime + Params().GetConsensus().nStakeMinAge > transaction->nTime)
			continue; // only count coins meeting min age requirement

		int64_t nValueIn = txPrev->vout[txin.prevout.n].nValue;
		bnCentSecond += CAmount(nValueIn) * (transaction->nTime - txPrev->nTime) / CENT;
	}


	CAmount bnCoinDay = ((bnCentSecond * CENT) / COIN) / (24 * 60 * 60);
	nCoinAge = bnCoinDay;

	return true;
}

bool GetRatingRewards(CAmount nCredit, std::vector<CTxOut>& results, CAmount& totalAmount, const CBlockIndex* pindexPrev, CDataStream& hashProofOfStakeSource, const CBlock* block)
{
    const int RATINGS_PAYOUT_MAX = 25;

	std::vector<std::string> vLotteryPost;
	std::vector<std::string> vLotteryComment;
    
    std::map<std::string, int> allPostRatings;
    std::map<std::string, int> allCommentRatings;

    CBlock blockPrev;
    ReadBlockFromDisk(blockPrev, pindexPrev, Params().GetConsensus());

    // Get all users from prev block for current lottery
    for (const auto& tx : blockPrev.vtx) {
        std::vector<std::string> vasm;
        if (!g_addrindex->FindPocketNetAsmString(tx, vasm)) continue;
        if ((vasm[1] == OR_SCORE || vasm[1] == OR_COMMENT_SCORE) && vasm.size() >= 4) {
            std::vector<unsigned char> _data_hex = ParseHex(vasm[3]);
            std::string _data_str(_data_hex.begin(), _data_hex.end());
            std::vector<std::string> _data;
            boost::split(_data, _data_str, boost::is_any_of("\t "));
            if (_data.size() >= 2) {
                std::string _address = _data[0];
                int _value = std::stoi(_data[1]);

                // For lottery use scores as 4=1 and 5=2 - Scores to posts
                if (vasm[1] == OR_SCORE && (_value == 4 || _value == 5)) {
                    // Get address of score initiator
                    reindexer::Item _score_itm;
                    if (!g_pocketdb->SelectOne( reindexer::Query("Scores").Where("txid", CondEq, tx->GetHash().GetHex()), _score_itm ).ok()) {
                        LogPrintf("--- GetRatingRewards error get score: %s\n", tx->GetHash().GetHex());
                        continue;
                    }

                    reindexer::Item _post_itm;
                    if (!g_pocketdb->SelectOne( reindexer::Query("Posts").Where("txid", CondEq, _score_itm["posttxid"].As<string>()), _post_itm ).ok()) {
                        LogPrintf("--- GetRatingRewards error get post: %s\n", _score_itm["posttxid"].As<string>());
                        continue;
                    }
                    
                    if (g_antibot->AllowModifyReputationOverPost(_score_itm["address"].As<string>(), _post_itm["address"].As<string>(), pindexPrev->nHeight, tx, true)) {
                        if (allPostRatings.find(_address) == allPostRatings.end()) allPostRatings.insert(std::make_pair(_address, 0));
                        allPostRatings[_address] += (_value - 3);
                    }
                }

                // For lottery use scores as 1 and -1 - Scores to comments
                if (vasm[1] == OR_COMMENT_SCORE && (_value == 1)) {
                    // Get address of score initiator
                    reindexer::Item _score_itm;
                    if (!g_pocketdb->SelectOne( reindexer::Query("CommentScores").Where("txid", CondEq, tx->GetHash().GetHex()), _score_itm ).ok()) {
                        LogPrintf("--- GetRatingRewards error get comment score: %s\n", tx->GetHash().GetHex());
                        continue;
                    }

                    reindexer::Item _comment_itm;
                    if (!g_pocketdb->SelectOne( reindexer::Query("Comment").Where("otxid", CondEq, _score_itm["commentid"].As<string>()), _comment_itm ).ok()) {
                        LogPrintf("--- GetRatingRewards error get comment: %s\n", _score_itm["posttxid"].As<string>());
                        continue;
                    }
                    
                    if (g_antibot->AllowModifyReputationOverComment(_score_itm["address"].As<string>(), _comment_itm["address"].As<string>(), pindexPrev->nHeight, tx, true)) {
                        if (allCommentRatings.find(_address) == allCommentRatings.end()) allCommentRatings.insert(std::make_pair(_address, 0));
                        allCommentRatings[_address] += _value;
                    }
                }
            }
        }
    }

    // Sort founded users
    {
        std::vector<std::pair<std::string, std::pair<int, arith_uint256>>> allPostSorted;
        std::vector<std::pair<std::string, std::pair<int, arith_uint256>>> allCommentSorted;

        {
            // Users with scores by post
            for (auto& it : allPostRatings) {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
                allPostSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }

            // Users with scores by comment
            for (auto& it : allCommentRatings) {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
                allCommentSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }
        }

        // Shrink founded users
        {
            // Users with scores by post
            if (allPostSorted.size() > 0) {
                std::sort(allPostSorted.begin(), allPostSorted.end(), [](auto & a, auto & b) {
                    return a.second.second < b.second.second;
                });

                if (allPostSorted.size() > RATINGS_PAYOUT_MAX) {
                    allPostSorted.resize(RATINGS_PAYOUT_MAX);
                }

                for (auto& it : allPostSorted) {
                    vLotteryPost.push_back(it.first);
                }
            }

            // Users with scores by comment
            if (allCommentSorted.size() > 0) {
                std::sort(allCommentSorted.begin(), allCommentSorted.end(), [](auto & a, auto & b) {
                    return a.second.second < b.second.second;
                });

                if (allCommentSorted.size() > RATINGS_PAYOUT_MAX) {
                    allCommentSorted.resize(RATINGS_PAYOUT_MAX);
                }

                for (auto& it : allCommentSorted) {
                    vLotteryComment.push_back(it.first);
                }
            }
        }
    }
    
	// Create transactions for all winners
    bool ret = false;
    ret = GenerateOuts(nCredit, results, totalAmount, vLotteryPost, OP_WINNER_POST) || ret;
    ret = GenerateOuts(nCredit, results, totalAmount, vLotteryComment, OP_WINNER_COMMENT) || ret;

    return ret;
}

bool GenerateOuts(CAmount nCredit, std::vector<CTxOut>& results, CAmount& totalAmount, std::vector<std::string> winners, opcodetype op_code_type) {
    if (winners.size() > 0 && winners.size() <= 25) {
        CAmount ratingReward = nCredit * 0.5;
        if (op_code_type == OP_WINNER_COMMENT) ratingReward = ratingReward / 10;
        totalAmount += ratingReward;

        int current = 0;
        const int rewardsCount = winners.size();
        CAmount rewardsPool = ratingReward;
        CAmount reward = ratingReward / rewardsCount;

        for (auto addr : boost::adaptors::reverse(winners)) {
            CAmount re;
            if (++current == rewardsCount) {
                re = rewardsPool;
            }
            else {
                rewardsPool = rewardsPool - reward;
                re = reward;
            }

            CTxDestination dest = DecodeDestination(addr);
            CScript scriptPubKey = GetScriptForDestination(dest);
            // TODO (brangr): Include with fork
            // scriptPubKey << op_code_type;
            results.push_back(CTxOut(re, scriptPubKey)); // send to ratings
        }

        return true;
    }

    return false;
}