// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_HTTPRPC_H
#define POCKETCOIN_HTTPRPC_H

#include "rpcapi/rpcapi.h"
#include "util/system.h"
#include "util/ref.h"

namespace util {
class Ref;
} // namespace util

extern std::shared_ptr<CRPCTable> g_privateTable;

class RPC
{
public:
    bool Init(const ArgsManager& args, const util::Ref& context);
    /** Start HTTP RPC subsystem.
    * Precondition; HTTP and RPC has been started.
    */
    bool StartHTTPRPC();
    /** Interrupt HTTP RPC subsystem.
    */
    void InterruptHTTPRPC();
    /** Stop HTTP RPC subsystem.
    * Precondition; HTTP and RPC has been stopped.
    */
    void StopHTTPRPC();

    std::shared_ptr<IRequestProcessor> GetPrivateRequestProcessor() const
    {
        return m_rpcProcessor;
    }
    std::shared_ptr<IRequestProcessor> GetWebRequestProcessor() const
    {
        return m_webRpcProcessor;
    }
private:
    std::shared_ptr<RequestProcessor> m_rpcProcessor;
    std::shared_ptr<RequestProcessor> m_webRpcProcessor;
};


#endif
