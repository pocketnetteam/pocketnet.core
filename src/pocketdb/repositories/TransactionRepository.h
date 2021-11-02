// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_H
#define POCKETDB_TRANSACTIONREPOSITORY_H

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/models/base/TransactionOutput.h"
#include "pocketdb/models/base/Rating.h"
#include "pocketdb/models/base/ReturnDtoModels.h"
#include "pocketdb/models/dto/User.h"
#include "pocketdb/models/dto/ScoreContent.h"
#include "pocketdb/models/dto/ScoreComment.h"

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

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
        shared_ptr<PocketBlock> List(const vector<string>& txHashes, bool includePayload = false);
        shared_ptr<Transaction> Get(const string& hash, bool includePayload = false);
        shared_ptr<TransactionOutput> GetTxOutput(const string& txHash, int number);
        bool Exists(const string& hash);
        bool ExistsInChain(const string& hash);

    private:
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

