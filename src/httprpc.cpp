// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "httprpc.h"

#include "rpcapi/rpcapi.h"
#include "rpcapi/rpchandler.h"


bool RPC::Init(const ArgsManager& args, const util::Ref& context)
{
    RPCFactory factory;
    // TODO (losty-nat): use parameter-provided args instead of gargs
    auto [privateProcessor, webProcessor] = factory.Init(args, context);
    if (!privateProcessor) {
        return false;
    }

    m_rpcProcessor = std::move(privateProcessor);
    m_webRpcProcessor = std::move(webProcessor);

    return true;
}

bool RPC::StartHTTPRPC()
{
    LogPrint(BCLog::RPC, "Starting HTTP RPC server\n");

    m_rpcProcessor->Start();
    if (m_webRpcProcessor) {
        m_webRpcProcessor->Start();
    }

    return true;
}

void RPC::InterruptHTTPRPC()
{
    LogPrint(BCLog::RPC, "Interrupting HTTP RPC server\n");
}

void RPC::StopHTTPRPC()
{
    LogPrint(BCLog::RPC, "Stopping HTTP RPC server\n");

    m_rpcProcessor->Stop();
    m_webRpcProcessor->Stop();
}
