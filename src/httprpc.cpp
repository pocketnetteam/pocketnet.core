// Copyright (c) 2015-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httprpc.h>

#include <chainparams.h>
#include <crypto/hmac_sha256.h>
#include <httpserver.h>
#include <key_io.h>
#include <random.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <stdio.h>
#include <sync.h>
#include <ui_interface.h>
#include <util.h>
#include <utilstrencodings.h>
#include <walletinitinterface.h>
#include <memory>
#include <numeric>
#include <boost/algorithm/string.hpp> // boost::trim
#include <chrono>
using namespace std::chrono;

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wallet.
 */
class HTTPRPCTimer : public RPCTimerBase
{
public:
    HTTPRPCTimer(struct event_base* eventBase, std::function<void()>& func, int64_t millis) : ev(eventBase, false, func)
    {
        struct timeval tv;
        tv.tv_sec = millis / 1000;
        tv.tv_usec = (millis % 1000) * 1000;
        ev.trigger(&tv);
    }

private:
    HTTPEvent ev;
};

class HTTPRPCTimerInterface : public RPCTimerInterface
{
public:
    explicit HTTPRPCTimerInterface(struct event_base* _base) : base(_base)
    {
    }
    const char* Name() override
    {
        return "HTTP";
    }
    RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) override
    {
        return new HTTPRPCTimer(base, func, millis);
    }

private:
    struct event_base* base;
};


/* Pre-base64-encoded authentication token */
static std::string strRPCUserColonPass;
/* Stored RPC timer interface (for unregistration) */
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;

static void JSONErrorReply(HTTPRequest* req, const UniValue& objError, const UniValue& id)
{
    // Send error reply from json-rpc error object
    int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    int code = find_value(objError, "code").get_int();

    if (code == RPC_INVALID_REQUEST)
        nStatus = HTTP_BAD_REQUEST;
    else if (code == RPC_METHOD_NOT_FOUND)
        nStatus = HTTP_NOT_FOUND;

    std::string strReply = JSONRPCReply(NullUniValue, objError, id);

    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(nStatus, strReply);
}

//This function checks username and password against -rpcauth
//entries from config file.
static bool multiUserAuthorized(std::string strUserPass)
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

static bool HTTPReq(HTTPRequest* req, bool rpcAuthenticate)
{
    // JSONRPC handles only POST
    if (req->GetRequestMethod() != HTTPRequest::POST) {
        LogPrint(BCLog::RPC, "WARNING: Request not POST\n");
        req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
        return false;
    }

    JSONRPCRequest jreq;
    try {
        UniValue valRequest;

        jreq.peerAddr = req->GetPeer().ToString();
        if (rpcAuthenticate) {

            // Check authorization
            std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
            if (!authHeader.first) {
                LogPrint(BCLog::RPC, "WARNING: Request whithour authorization header\n");
                req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
                req->WriteReply(HTTP_UNAUTHORIZED);
                return false;
            }

            if (!RPCAuthorized(authHeader.second, jreq.authUser)) {
                LogPrintf("ThreadRPCServer incorrect password %s attempt from %s\n", authHeader.second, jreq.peerAddr);

                /*  Deter brute-forcing
                    If this results in a DoS the user really
                    shouldn't have their RPC port exposed.
                */
                MilliSleep(250);

                req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
                req->WriteReply(HTTP_UNAUTHORIZED);
                return false;
            }
        }

        if (!valRequest.read(req->ReadBody()))
            throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");
            
        // Set the URI
        jreq.URI = req->GetURI();
        std::string strReply;

        // singleton request
        if (valRequest.isObject()) {

            if (!rpcAuthenticate) {
                UniValue valMethod = find_value(valRequest, "method");
                if (!valMethod.isNull() && valMethod.isStr()) {
                    const std::string& sMethd = valMethod.get_str();
                    const CRPCCommand* pcmd = tableRPC[sMethd];
                    if (!pcmd)
                        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");

                    if (pcmd->pwdRequied) {
                        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
                        req->WriteReply(HTTP_UNAUTHORIZED);
                        return false;
                    }
                } else {
                    throw JSONRPCError(RPC_PARSE_ERROR, "method not found");
                }
            }

            jreq.parse(valRequest);

            auto start = gStatEngineInstance.GetCurrentSystemTime();

            UniValue result = tableRPC.execute(jreq);
            
            auto stop = gStatEngineInstance.GetCurrentSystemTime();

            gStatEngineInstance.AddSample(
                Statistic::RequestSample{
                    jreq.strMethod,
                    start,
                    stop,
                    jreq.peerAddr.substr(0, jreq.peerAddr.find(':')),
                    valRequest.write().size(),
                    result.write().size()
                }
            );

            auto diff = (stop - start);
            LogPrint(BCLog::RPC, "RPC Method time %s (%s) - %ldms\n", jreq.strMethod, jreq.peerAddr.substr(0, jreq.peerAddr.find(':')), diff.count());

            // Send reply
            strReply = JSONRPCReply(result, NullUniValue, jreq.id);

            // array of requests
        } else {
            if (valRequest.isArray()) {
                strReply = JSONRPCExecBatch(jreq, valRequest.get_array());
            } else {
                throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");
            }
        }

        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strReply);
    } catch (const UniValue& objError) {
        LogPrint(BCLog::RPC, "Exception %s\n", objError.write());
        JSONErrorReply(req, objError, jreq.id);
        return false;
    } catch (const std::exception& e) {
        LogPrint(BCLog::RPC, "Exception 2 %s\n", JSONRPCError(RPC_PARSE_ERROR, e.what()).write());
        JSONErrorReply(req, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
        return false;
    }
    return true;
}

static bool HTTPReq_JSONRPC_Anonymous(HTTPRequest* req, const std::string&)
{
    return HTTPReq(req, false);
}

static bool HTTPReq_JSONRPC(HTTPRequest* req, const std::string&)
{
    return HTTPReq(req, true);
}

static bool InitRPCAuthentication()
{
    if (gArgs.GetArg("-rpcpassword", "") == "") {
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
    if (gArgs.GetArg("-rpcauth", "") != "") {
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
    g_pubSocket->RegisterHTTPHandler("/post/", true, HTTPReq_JSONRPC_Anonymous);
    g_pubSocket->RegisterHTTPHandler("/public/", true, HTTPReq_JSONRPC_Anonymous);
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
    g_pubSocket->UnregisterHTTPHandler("/public/", true);
    if (g_wallet_init_interface.HasWalletSupport()) {
        g_socket->UnregisterHTTPHandler("/wallet/", false);
    }
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
