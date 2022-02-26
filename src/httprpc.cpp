// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcapi/rpcapi.h"
#include "rpcapi/rpchandler.h"
#include "util/time.h"
#include <chrono>
#include <httprpc.h>

#include <httpserver.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <node/ui_interface.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/ref.h>
#include <walletinitinterface.h>
#include <memory>
#include <boost/algorithm/string.hpp> // boost::trim
#include <crypto/hmac_sha256.h>

/* Stored RPC timer interface (for unregistration) */
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;

/* Pre-base64-encoded authentication token */

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

    struct event_base* eventBase = EventBase();
    assert(eventBase);
    httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(eventBase);
    RPCSetTimerInterface(httpRPCTimerInterface.get());
    return true;
}

void RPC::InterruptHTTPRPC()
{
    LogPrint(BCLog::RPC, "Interrupting HTTP RPC server\n");
}

void RPC::StopHTTPRPC()
{
    LogPrint(BCLog::RPC, "Stopping HTTP RPC server\n");
    
    // TODO (losty-nat): unregistering handlers!
    // if (g_socket)
    // {
    //     g_socket->UnregisterHTTPHandler("/", true);
    //     if (g_wallet_init_interface.HasWalletSupport()) {
    //         g_socket->UnregisterHTTPHandler("/wallet/", false);
    //     }
    // }

    // if (g_webSocket)
    // {
    //     g_webSocket->UnregisterHTTPHandler("/post/", false);
    //     g_webSocket->UnregisterHTTPHandler("/", false);
    // }

    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
