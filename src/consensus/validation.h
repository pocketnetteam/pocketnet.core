// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_CONSENSUS_VALIDATION_H
#define POCKETCOIN_CONSENSUS_VALIDATION_H

#include <string>
#include <version.h>
#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <primitives/block.h>

/** Index marker for when no witness commitment is present in a coinbase transaction. */
static constexpr int NO_WITNESS_COMMITMENT{-1};

/** Minimum size of a witness commitment structure. Defined in BIP 141. **/
static constexpr size_t MINIMUM_WITNESS_COMMITMENT{38};

/** A "reason" why a transaction was invalid, suitable for determining whether the
  * provider of the transaction should be banned/ignored/disconnected/etc.
  */
enum class TxValidationResult {
    TX_RESULT_UNSET = 0,     //!< initial value. Tx has not yet been rejected
    TX_CONSENSUS,            //!< invalid by consensus rules
    // TODO (losty-fur): probably rename this. 2 below are the same, but first results in node's punishment, second is common situation if current node is much behind others
    TX_SOCIAL_CONSENSUS,     //!< invalid by social (pocketnet) consensus rules that is critical
    TX_SOCIAL_UNWARRANT,     //!< invalid by social (pocketnet) rules that is common if current node is much behind others  
    /**
     * Invalid by a change to consensus rules more recent than SegWit.
     * Currently unused as there are no such consensus rule changes, and any download
     * sources realistically need to support SegWit in order to provide useful data,
     * so differentiating between always-invalid and invalid-by-pre-SegWit-soft-fork
     * is uninteresting.
     */
    TX_RECENT_CONSENSUS_CHANGE,
    TX_INPUTS_NOT_STANDARD,   //!< inputs (covered by txid) failed policy rules
    TX_NOT_STANDARD,          //!< otherwise didn't meet our local policy rules
    TX_MISSING_INPUTS,        //!< transaction was missing some of its inputs
    TX_PREMATURE_SPEND,       //!< transaction spends a coinbase too early, or violates locktime/sequence locks
    TX_POCKET_PREMATURE_SPEND, //!< transaction spends a coinstake too early
    /**
     * Transaction might have a witness prior to SegWit
     * activation, or witness may have been malleated (which includes
     * non-standard witnesses).
     */
    TX_WITNESS_MUTATED,
    /**
     * Transaction is missing a witness.
     */
    TX_WITNESS_STRIPPED,
    /**
     * Tx already in mempool or conflicts with a tx in the chain
     * (if it conflicts with another tx in mempool, we use MEMPOOL_POLICY as it failed to reach the RBF threshold)
     * Currently this is only used if the transaction already exists in the mempool or on chain.
     */
    TX_CONFLICT,
    TX_MEMPOOL_POLICY,        //!< violated mempool's fee/size/descendant/RBF/etc limits
};

/** A "reason" why a block was invalid, suitable for determining whether the
  * provider of the block should be banned/ignored/disconnected/etc.
  * These are much more granular than the rejection codes, which may be more
  * useful for some other use-cases.
  */
enum class BlockValidationResult {
    BLOCK_RESULT_UNSET = 0,  //!< initial value. Block has not yet been rejected
    BLOCK_CONSENSUS,         //!< invalid by consensus rules (excluding any below reasons)
    /**
     * Invalid by a change to consensus rules more recent than SegWit.
     * Currently unused as there are no such consensus rule changes, and any download
     * sources realistically need to support SegWit in order to provide useful data,
     * so differentiating between always-invalid and invalid-by-pre-SegWit-soft-fork
     * is uninteresting.
     */
    BLOCK_RECENT_CONSENSUS_CHANGE,
    BLOCK_CACHED_INVALID,    //!< this block was cached as being invalid and we didn't store the reason why
    BLOCK_INVALID_HEADER,    //!< invalid proof of work or time too old
    BLOCK_MUTATED,           //!< the block's data didn't match the data committed to by the PoW
    BLOCK_MISSING_PREV,      //!< We don't have the previous block the checked one is built on
    BLOCK_INVALID_PREV,      //!< A block this one builds on is invalid
    BLOCK_TIME_FUTURE,       //!< block timestamp was > 2 hours in the future (or our clock is bad)
    BLOCK_CHECKPOINT,        //!< the block failed to meet one of our checkpoints
    // BLOCK_INCOMPLETE         //!< TODO (losty-critical): this is for sure needed somewhere but i forgot where
};



/** Template for capturing information about block/transaction validation. This is instantiated
 *  by TxValidationState and BlockValidationState for validation information on transactions
 *  and blocks respectively. */
template <typename Result>
class ValidationState
{
private:
    enum class ModeState {
        M_VALID,   //!< everything ok
        M_INVALID, //!< network rule violation (DoS value may be set)
        M_ERROR,   //!< run-time error
        M_MODE_CONSENSUS, //!< consensus error
    } m_mode{ModeState::M_VALID};
    Result m_result{};
    std::string m_reject_reason;
    std::string m_debug_message;
    bool m_incompleted = false;
    int m_code = 0;
public:
    bool Invalid(Result result,
                 const std::string& reject_reason = "",
                 const std::string& debug_message = "",
                 bool incompleted = false,
                 int code = 0,
                 ModeState modIn = ModeState::M_INVALID)
    {
        m_result = result;
        m_reject_reason = reject_reason;
        m_debug_message = debug_message;
        m_incompleted = incompleted;
        m_code = code;
        if (m_mode != ModeState::M_ERROR) m_mode = modIn;
        return false;
    }
    bool ConsensusFailed(TxValidationResult _txRes,  const std::string &_strRejectReason, unsigned int _chRejectCode = 0) {
        return Invalid(_txRes, _strRejectReason, "", false, _chRejectCode, ModeState::M_MODE_CONSENSUS);
    }
    bool Error(const std::string& reject_reason)
    {
        if (m_mode == ModeState::M_VALID)
            m_reject_reason = reject_reason;
        m_mode = ModeState::M_ERROR;
        return false;
    }
    bool IsValid() const { return m_mode == ModeState::M_VALID; }
    bool IsInvalid() const { return m_mode == ModeState::M_INVALID; }
    bool IsError() const { return m_mode == ModeState::M_ERROR; }
    bool IsConsensusFailed() const { return m_mode == ModeState::M_MODE_CONSENSUS; }
    Result GetResult() const { return m_result; }
    int GetRejectCode()
    {
        return m_code;
    }
    std::string GetRejectReason() const { return m_reject_reason; }
    std::string GetDebugMessage() const { return m_debug_message; }
    std::string ToString() const
    {
        if (IsValid()) {
            return "Valid";
        }

        if (!m_debug_message.empty()) {
            return m_reject_reason + ", " + m_debug_message;
        }

        return m_reject_reason;
    }
    bool Incompleted() const {
        return m_incompleted;
    }
    void SetIncompleted() {
        m_incompleted = true;
    }
};

class TxValidationState : public ValidationState<TxValidationResult> {};
class BlockValidationState : public ValidationState<BlockValidationResult> {};

// These implement the weight = (stripped_size * 4) + witness_size formula,
// using only serialization with and without witness data. As witness_size
// is equal to total_size - stripped_size, this formula is identical to:
// weight = (stripped_size * 3) + total_size.
static inline int64_t GetTransactionWeight(const CTransaction& tx)
{
    return ::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(tx, PROTOCOL_VERSION);
}
static inline int64_t GetBlockWeight(const CBlock& block)
{
    return ::GetSerializeSize(block, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(block, PROTOCOL_VERSION);
}
static inline int64_t GetTransactionInputWeight(const CTxIn& txin)
{
    // scriptWitness size is added here because witnesses and txins are split up in segwit serialization.
    return ::GetSerializeSize(txin, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(txin, PROTOCOL_VERSION) + ::GetSerializeSize(txin.scriptWitness.stack, PROTOCOL_VERSION);
}

/** Compute at which vout of the block's coinbase transaction the witness commitment occurs, or -1 if not found */
inline int GetWitnessCommitmentIndex(const CBlock& block)
{
    int commitpos = NO_WITNESS_COMMITMENT;
    if (!block.vtx.empty()) {
        for (size_t o = 0; o < block.vtx[0]->vout.size(); o++) {
            const CTxOut& vout = block.vtx[0]->vout[o];
            if (vout.scriptPubKey.size() >= MINIMUM_WITNESS_COMMITMENT &&
                vout.scriptPubKey[0] == OP_RETURN &&
                vout.scriptPubKey[1] == 0x24 &&
                vout.scriptPubKey[2] == 0xaa &&
                vout.scriptPubKey[3] == 0x21 &&
                vout.scriptPubKey[4] == 0xa9 &&
                vout.scriptPubKey[5] == 0xed) {
                commitpos = o;
            }
        }
    }
    return commitpos;
}

#endif // POCKETCOIN_CONSENSUS_VALIDATION_H
