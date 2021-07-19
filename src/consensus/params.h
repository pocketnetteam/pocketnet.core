// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_CONSENSUS_PARAMS_H
#define POCKETCOIN_CONSENSUS_PARAMS_H

#include <limits>
#include <map>
#include <string>
#include <uint256.h>

namespace Consensus {

enum DeploymentPos {
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV,    // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
	 * Struct for each individual consensus rule change using BIP9.
	 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
		 *  This is useful for testing, as it means tests don't need to deal with the activation
		 *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
		 *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;
};

/**
	 * Parameters that influence chain consensus.
	 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /* Block hash that is excepted from BIP16 enforcement */
    uint256 BIP16Exception;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /**
		 * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
		 * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
		 * Examples: 1916 for 95%, 1512 for testchains.
		 */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;

    /* Proof of stake parameters */
    int64_t nPosFirstBlock;

    unsigned int nStakeMinAge;
    int64_t nPosTargetSpacing;
    int64_t nPosTargetTimespan;
    int64_t nStakeCombineThreshold;
    int64_t nStakeSplitThreshold;
    int64_t nStakeMinimumThreshold;
    int64_t nStakeMaximumThreshold;

    bool fPosRequiresPeers;
    int nDailyBlockCount;
    unsigned int nModifierInterval;

    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;

    unsigned int nHeight_version_1_0_0_pre;
    std::string sVersion_1_0_0_pre_checkpoint;
    unsigned int nHeight_version_1_0_0;
    unsigned int nHeight_fix_ratings;
    unsigned int nHeight_version_0_18_11;

    unsigned int score_blocking_on;
    unsigned int score_blocking_off;
    unsigned int opreturn_check;
    unsigned int lottery_referral_beg;
    unsigned int lottery_referral_limitation;
    unsigned int checkpoint_0_19_3;
    unsigned int checkpoint_0_19_6;
    //unsigned int checkpoint_non_unique_account_name;
    unsigned int checkpoint_split_content_video;
};
} // namespace Consensus

#endif // POCKETCOIN_CONSENSUS_PARAMS_H
