//
// Created by joknek on 29.06.2021.
//

#ifndef SRC_POCKETSCORESRPC_H
#define SRC_POCKETSCORESRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"


namespace PocketWeb
{
    using std::string;
    using std::vector;

    class PocketScoresRpc
    {
    public:
        UniValue GetAddressScores(const JSONRPCRequest& request);
        UniValue GetPostScores(const JSONRPCRequest& request);
        UniValue GetPageScores(const JSONRPCRequest& request);
    };
}


#endif //SRC_POCKETSCORESRPC_H
