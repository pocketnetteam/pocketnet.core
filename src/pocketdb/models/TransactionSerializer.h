#ifndef SRC_TRANSACTIONSERIALIZER_H
#define SRC_TRANSACTIONSERIALIZER_H

#include "Transaction.h"
#include "User.h"
#include "Post.h"

namespace PocketTx
{
    class TransactionSerializer
    {
    public:
        static std::vector<PocketTx::Transaction *> DeserializeBlock(std::string &src);

        static PocketTx::Transaction *DeserializeTransaction(std::string &src);
    };
}


#endif //SRC_TRANSACTIONSERIALIZER_H
