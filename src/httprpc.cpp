// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
static std::string strRPCUserColonPass;

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";


//This function checks username and password against -rpcauth
//entries from config file.
static bool multiUserAuthorized(std::string& strUserPass)
{
    if (strUserPass.find(':') == std::string::npos) {
        return false;
    }
    std::string strUser = strUserPass.substr(0, strUserPass.find(':'));
    std::string strPass = strUserPass.substr(strUserPass.find(':') + 1);

    for (const std::string& strRPCAuth : gArgs.GetArgs("-rpcauth")) {
        //Search for multi-user login/pass "rpcauth" from config
        std::vector<std::string> vFields;
        boost::split(vFields, strRPCAuth, boost::is_any_of(":$"));
        if (vFields.size() != 3) {
            //Incorrect formatting in config file
            continue;
        }

        std::string strName = vFields[0];
        if (!TimingResistantEqual(strName, strUser)) {
            continue;
        }

        std::string strSalt = vFields[1];
        std::string strHash = vFields[2];

        static const unsigned int KEY_SIZE = 32;
        unsigned char out[KEY_SIZE];

        CHMAC_SHA256(reinterpret_cast<const unsigned char*>(strSalt.c_str()), strSalt.size()).Write(reinterpret_cast<const unsigned char*>(strPass.c_str()), strPass.size()).Finalize(out);
        std::vector<unsigned char> hexvec(out, out + KEY_SIZE);
        std::string strHashFromPass = HexStr(hexvec);

        if (TimingResistantEqual(strHashFromPass, strHash)) {
            return true;
        }
    }
    return false;
}

static bool RPCAuthorized(const std::string& strAuth, std::string& strAuthUsernameOut)
{
    if (strRPCUserColonPass.empty()) // Belt-and-suspenders measure if InitRPCAuthentication was not called
        return false;
    if (strAuth.substr(0, 6) != "Basic ")
        return false;
    std::string strUserPass64 = strAuth.substr(6);
    boost::trim(strUserPass64);
    std::string strUserPass = DecodeBase64(strUserPass64);

    if (strUserPass.find(':') != std::string::npos)
        strAuthUsernameOut = strUserPass.substr(0, strUserPass.find(':'));

    //Check if authorized under single-user field
    if (TimingResistantEqual(strUserPass, strRPCUserColonPass)) {
        return true;
    }
    return multiUserAuthorized(strUserPass);
}

static bool HTTPReq_JSONRPC_Anonymous(const util::Ref& context, HTTPRequest* req)
{
    return g_webSocket->HTTPReq(req, context, g_webSocket->m_table_rpc);
}

static bool HTTPReq_JSONRPC_Post_Anonymous(const util::Ref& context, HTTPRequest* req)
{
    return g_webSocket->HTTPReq(req, context, g_webSocket->m_table_post_rpc);
}

static bool HTTPReq_JSONRPC(const util::Ref& context, HTTPRequest* req)
{
    auto peerAddr = req->GetPeer().ToString();
    // Check authorization
    std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
    if (!authHeader.first) {
        LogPrint(BCLog::RPC, "WARNING: Request without authorization header\n");
        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    std::string authUser;
    if (!RPCAuthorized(authHeader.second, authUser)) {
        LogPrintf("ThreadRPCServer incorrect password %s attempt from %s\n", authHeader.second, peerAddr);

        /*  Deter brute-forcing
            If this results in a DoS the user really
            shouldn't have their RPC port exposed.
        */
        UninterruptibleSleep(std::chrono::milliseconds{250});

        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    return g_socket->HTTPReq(req, context, g_socket->m_table_rpc);
}

static bool InitRPCAuthentication()
{
    if (gArgs.GetArg("-rpcpassword", "").empty()) {
        LogPrintf("Using random cookie authentication.\n");
        if (!GenerateAuthCookie(&strRPCUserColonPass)) {
            return false;
        }
    } else {
        LogPrintf("Config options rpcuser and rpcpassword will soon be deprecated. Locally-run instances may remove rpcuser to use cookie-based auth, or may be replaced with rpcauth. Please see share/rpcauth for rpcauth auth generation.\n");
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }
    if (!gArgs.GetArg("-rpcauth", "").empty()) {
        LogPrintf("Using rpcauth authentication.\n");
    }
    return true;
}

bool StartHTTPRPC(const util::Ref& context)
{
    LogPrint(BCLog::RPC, "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication())
        return false;

    if (g_socket)
    {
        auto handler = [&context](HTTPRequest* req, const std::string&) { return HTTPReq_JSONRPC(context, req); };
        g_socket->RegisterHTTPHandler("/", true, handler, g_socket->m_workQueue);

        if (g_wallet_init_interface.HasWalletSupport())
            g_socket->RegisterHTTPHandler("/wallet/", false, handler, g_socket->m_workQueue);
    }

    if (g_webSocket)
    {
        auto postAnonymousHandler = [&context](HTTPRequest* req, const std::string&) { return HTTPReq_JSONRPC_Post_Anonymous(context, req); };
        g_webSocket->RegisterHTTPHandler("/post/", false, postAnonymousHandler, g_webSocket->m_workPostQueue);
        auto anonymousHandler = [&context](HTTPRequest* req, const std::string&) { return HTTPReq_JSONRPC_Anonymous(context, req); };
        g_webSocket->RegisterHTTPHandler("/", false, anonymousHandler, g_webSocket->m_workQueue);
    }

    struct event_base* eventBase = EventBase();
    assert(eventBase);
    httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(eventBase);
    RPCSetTimerInterface(httpRPCTimerInterface.get());
    return true;
}

void InterruptHTTPRPC()
{
    LogPrint(BCLog::RPC, "Interrupting HTTP RPC server\n");
}

void StopHTTPRPC()
{
    LogPrint(BCLog::RPC, "Stopping HTTP RPC server\n");
    
    g_socket->UnregisterHTTPHandler("/", true);

    if (g_webSocket)
    {
        g_webSocket->UnregisterHTTPHandler("/post/", false);
        g_webSocket->UnregisterHTTPHandler("/", false);
    }

    if (g_wallet_init_interface.HasWalletSupport()) {
        g_socket->UnregisterHTTPHandler("/wallet/", false);
    }
    
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
