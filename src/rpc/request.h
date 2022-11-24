// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_RPC_REQUEST_H
#define POCKETCOIN_RPC_REQUEST_H

#include "pocketdb/SQLiteConnection.h"
#include "rpc/requestutils.h"
#include <string>

#include <univalue.h>

namespace util {
class Ref;
} // namespace util

class JSONRPCRequest
{
public:
    UniValue id;
    std::string strMethod;
    UniValue params;
    bool fHelp;
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    const util::Ref& context;

    JSONRPCRequest(const util::Ref& context) : id(NullUniValue), params(NullUniValue), fHelp(false), context(context) {}

    //! Initializes request information from another request object and the
    //! given context. The implementation should be updated if any members are
    //! added or removed above.
    JSONRPCRequest(const JSONRPCRequest& other, const util::Ref& context)
        : id(other.id), strMethod(other.strMethod), params(other.params), fHelp(other.fHelp), URI(other.URI),
          authUser(other.authUser), peerAddr(other.peerAddr), context(context)
    {
    }

    void parse(const UniValue& valRequest);

    void SetDbConnection(const DbConnectionRef& _dbConnection);
    const DbConnectionRef& DbConnection() const;

private:
    DbConnectionRef dbConnection;
};

#endif // POCKETCOIN_RPC_REQUEST_H
