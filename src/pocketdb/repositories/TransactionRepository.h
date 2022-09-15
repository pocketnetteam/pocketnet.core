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

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase& db) : BaseRepository(db) {}
        void Init() override {}
        void Destroy() override {}

        //  Base transaction operations
        void InsertTransactions(PocketBlock& pocketBlock);
        PocketBlockRef List(const vector<string>& txHashes, bool includePayload = false, bool includeInputs = false, bool includeOutputs = false);
        PTransactionRef Get(const string& hash, bool includePayload = false, bool includeInputs = false, bool includeOutputs = false);
        PTransactionOutputRef GetTxOutput(const string& txHash, int number);

        bool Exists(const string& hash);
        bool ExistsInChain(const string& hash);
        int MempoolCount();

        void CleanTransaction(const string& hash);
        void CleanMempool();
        void Clean();

    private:
        void InsertTransactionInputs(const PTransactionRef& ptx);
        void InsertTransactionOutputs(const PTransactionRef& ptx);
        void InsertTransactionPayload(const PTransactionRef& ptx);
        void InsertTransactionModel(const PTransactionRef& ptx);

    protected:
        tuple<bool, PTransactionRef> CreateTransactionFromListRow(
            const shared_ptr<sqlite3_stmt*>& stmt, bool includedPayload);

    };

    typedef std::shared_ptr<TransactionRepository> TransactionRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_H

