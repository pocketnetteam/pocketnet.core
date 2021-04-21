#include "Transaction.h"

using namespace PocketTx;

void Transaction::Deserialize(const UniValue &src)
{
    assert(src.exists("txid"));
    SetTxId(src["txid"].get_str());

    assert(src.exists("time"));
    SetTxTime(src["time"].get_int64());

    assert(src.exists("address"));
    SetAddress(src["address"].get_str());
}

std::string Transaction::Serialize(const PocketTxType &txType)
{
    std::string data;
    // TODO (brangr): implement

    return data;
}
