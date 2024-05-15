// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_H
#define POCKETDB_TRANSACTIONREPOSITORY_H

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/models/base/TransactionOutput.h"
#include "pocketdb/models/base/Rating.h"
#include "pocketdb/models/base/DtoModels.h"
#include "pocketdb/helpers/DbViewHelper.h"

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    struct CollectData
    {
        explicit CollectData(std::string _txHash)
            : txHash(std::move(_txHash))
        {}
        
        const std::string txHash;
        PTransactionRef ptx;
        std::vector<TransactionOutput> outputs;
        std::vector<TransactionInput> inputs;
        TxContextualData txContextData;
        std::optional<Payload> payload;
    };

    struct StakeKernelHashTx
    {
        string BlockHash;
        int64_t TxTime;
        int64_t OutValue;
    };

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        //  Base transaction operations
        void InsertTransactions(PocketBlock& pocketBlock);
        PocketBlockRef List(const vector<string>& txHashes, bool includePayload = false, bool includeInputs = false, bool includeOutputs = false);
        PTransactionRef Get(const string& hash, bool includePayload = false, bool includeInputs = false, bool includeOutputs = false);
        PTransactionOutputRef GetTxOutput(const string& txHash, int number);
        shared_ptr<StakeKernelHashTx> GetStakeKernelHashTx(const string& txHash, int number);

        bool Exists(const string& hash);
        bool Exists(vector<string>& txHashes);
        bool ExistsLast(const string& hash);
        int MempoolCount();

        void CleanTransaction(const string& hash);
        vector<string> GetMempoolTxHashes();

        // TODO (lostystyg): need?
        // optional<string> TxIdToHash(const int64_t& id);
        // optional<int64_t> TxHashToId(const string& hash);
        // optional<string> AddressIdToHash(const int64_t& id);
        // optional<int64_t> AddressHashToId(const string& hash);

    private:
        void InsertRegistry(const set<string>& strings);
        void InsertRegistryLists(const set<string>& lists);
        void InsertList(const std::string& list, const std::string& txHash);
        void InsertTransactionInputs(const vector<TransactionInput>& intputs, const string& txHash);
        void InsertTransactionOutputs(const vector<TransactionOutput>& outputs, const string& txHash);
        void InsertTransactionPayload(const Payload& payload);
        void InsertTransactionModel(const CollectData& ptx);

        map<string,int64_t> GetTxIds(const vector<string>& txHashes);

    protected:
        tuple<bool, PTransactionRef> CreateTransactionFromListRow(
            Cursor& cursor, bool includedPayload);

    };

    typedef std::shared_ptr<TransactionRepository> TransactionRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_H

