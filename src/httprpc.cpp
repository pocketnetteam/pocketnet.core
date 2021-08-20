// Copyright (c) 2015-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httprpc.h>

#include <httpserver.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <ui_interface.h>
#include <util.h>
#include <utilstrencodings.h>
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

static bool HTTPReq_JSONRPC_Anonymous(HTTPRequest* req, const std::string&)
{
    return g_pubSocket->HTTPReq(req);
}

static bool HTTPReq_JSONRPC(HTTPRequest* req, const std::string&)
{
    auto peerAddr = req->GetPeer().ToString();
    // Check authorization
    std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
    if (!authHeader.first) {
        LogPrint(BCLog::RPC, "WARNING: Request whithour authorization header\n");
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
        MilliSleep(250);

        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    return g_socket->HTTPReq(req);
}

static bool InitRPCAuthentication()
{
    if (gArgs.GetArg("-rpcpassword", "").empty()) {
        LogPrintf("No rpcpassword set - using random cookie authentication.\n");
        if (!GenerateAuthCookie(&strRPCUserColonPass)) {
            uiInterface.ThreadSafeMessageBox(
                _("Error: A fatal internal error occurred, see debug.log for details"), // Same message as AbortNode
                "", CClientUIInterface::MSG_ERROR);
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

bool StartHTTPRPC()
{
    LogPrint(BCLog::RPC, "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication())
        return false;

    g_socket->RegisterHTTPHandler("/", true, HTTPReq_JSONRPC);
    g_pubSocket->RegisterHTTPHandler("/", false, HTTPReq_JSONRPC_Anonymous);
    g_pubSocket->RegisterHTTPHandler("/post/", false, HTTPReq_JSONRPC_Anonymous);
    g_pubSocket->RegisterHTTPHandler("/public/", false, HTTPReq_JSONRPC_Anonymous);
    if (g_wallet_init_interface.HasWalletSupport()) {
        g_socket->RegisterHTTPHandler("/wallet/", false, HTTPReq_JSONRPC);
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
    g_pubSocket->UnregisterHTTPHandler("/post/", true);
    g_pubSocket->UnregisterHTTPHandler("/public/", false);
    if (g_wallet_init_interface.HasWalletSupport()) {
        g_socket->UnregisterHTTPHandler("/wallet/", false);
    }
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
