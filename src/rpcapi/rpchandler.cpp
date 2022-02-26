#include "rpchandler.h"

#include "crypto/hmac_sha256.h"
#include "rpc/server.h"
#include "rpcapi/rpcapi.h"
#include "util/system.h"
#include "rpc/register.h"
#include "zmq/zmqrpc.h"
#include "interfaces/chain.h"


static bool InitRPCAuthentication(const ArgsManager& args, std::string& strRPCUserColonPass)
{
    if (args.GetArg("-rpcpassword", "").empty()) {
        LogPrintf("Using random cookie authentication.\n");
        if (!GenerateAuthCookie(&strRPCUserColonPass)) {
            return false;
        }
    } else {
        LogPrintf("Config options rpcuser and rpcpassword will soon be deprecated. Locally-run instances may remove rpcuser to use cookie-based auth, or may be replaced with rpcauth. Please see share/rpcauth for rpcauth auth generation.\n");
        strRPCUserColonPass = args.GetArg("-rpcuser", "") + ":" + args.GetArg("-rpcpassword", "");
    }
    if (!args.GetArg("-rpcauth", "").empty()) {
        LogPrintf("Using rpcauth authentication.\n");
    }
    return true;
}

class Authorizer
{
public:
    explicit Authorizer(const std::string& strRPCUserColonPass)
    {
        m_strRPCUserColonPass = strRPCUserColonPass;
    }
    bool RPCAuthorized(const std::string& strAuth, std::string& strAuthUsernameOut)
    {
        if (m_strRPCUserColonPass.empty()) // Belt-and-suspenders measure if InitRPCAuthentication was not called
            return false;
        if (strAuth.substr(0, 6) != "Basic ")
            return false;
        std::string strUserPass64 = strAuth.substr(6);
        boost::trim(strUserPass64);
        std::string strUserPass = DecodeBase64(strUserPass64);

        if (strUserPass.find(':') != std::string::npos)
            strAuthUsernameOut = strUserPass.substr(0, strUserPass.find(':'));

        //Check if authorized under single-user field
        if (TimingResistantEqual(strUserPass, m_strRPCUserColonPass)) {
            return true;
        }
        return multiUserAuthorized(strUserPass);
    }
protected:
    //This function checks username and password against -rpcauth
    //entries from config file.
    bool multiUserAuthorized(std::string& strUserPass)
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
private:
    std::string m_strRPCUserColonPass;
};


static inline std::string gen_random(const int len) {

    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    srand( (unsigned) time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];


    return tmp_s;

}

static void JSONErrorReply(IReplier* replier, const UniValue& objError, const UniValue& id)
{
    // Send error reply from json-rpc error object
    int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    int code = find_value(objError, "code").get_int();

    if (code == RPC_INVALID_REQUEST)
        nStatus = HTTP_BAD_REQUEST;
    else if (code == RPC_METHOD_NOT_FOUND)
        nStatus = HTTP_NOT_FOUND;

    std::string strReply = JSONRPCReply(NullUniValue, objError, id);

    replier->WriteHeader("Content-Type", "application/json");
    replier->WriteReply(nStatus, strReply);
}

class RPCTableExecutor
{
public:
    static bool ProcessRPC(const util::Ref& context, const std::string& strURI, const std::string& body, const DbConnectionRef& sqliteConnection, IReplier* req, const CRPCTable& table)
    {
        // JSONRPC handles only POST
        // TODO (losty-nat): handle this
        // if (req->GetRequestMethod() != HTTPRequest::POST) {
        //     LogPrint(BCLog::RPCERROR, "WARNING: Request not POST\n");
        //     req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
        //     return false;
        // }

        string uri;
        string method;
        string peer;
        auto start = gStatEngineInstance.GetCurrentSystemTime();
        bool executeSuccess = true;

        JSONRPCRequest jreq(context);
        try
        {
            UniValue valRequest;

            if (!valRequest.read(body))
                throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");

            // Set the URI
            jreq.URI = strURI;
            std::string strReply;

            // singleton request
            if (valRequest.isObject())
            {
                jreq.parse(valRequest);
                jreq.SetDbConnection(sqliteConnection);

                uri = jreq.URI;
                method = jreq.strMethod;
                peer = jreq.peerAddr.substr(0, jreq.peerAddr.find(':'));
                string prms = jreq.params.write(0, 0);

                auto rpcKey = gen_random(15);
                LogPrint(BCLog::RPC, "RPC started method %s%s (%s) with params: %s\n",
                    uri, method, rpcKey, prms);

                UniValue result = table.execute(jreq);

                auto execute = gStatEngineInstance.GetCurrentSystemTime();

                LogPrint(BCLog::RPC, "RPC executed method %s%s (%s) > %.2fms\n",
                    uri, method, rpcKey, (execute.count() - start.count()));

                // Send reply
                strReply = JSONRPCReply(result, NullUniValue, jreq.id);
            }
            else
            {
                if (valRequest.isArray())
                {
                    strReply = JSONRPCExecBatch(jreq, valRequest.get_array(), table);
                }
                else
                {
                    throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");
                }
            }

            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strReply);
        }
        catch (const UniValue& objError)
        {
            LogPrint(BCLog::RPCERROR, "Exception %s\n", objError.write());
            JSONErrorReply(req, objError, jreq.id);
            executeSuccess = false;
        }
        catch (const std::exception& e)
        {
            LogPrint(BCLog::RPCERROR, "Exception 2 %s\n", JSONRPCError(RPC_PARSE_ERROR, e.what()).write());
            JSONErrorReply(req, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
            executeSuccess = false;
        }

        // Collect statistic data
        if (LogInstance().WillLogCategory(BCLog::STAT))
        {
            auto finish = gStatEngineInstance.GetCurrentSystemTime();

            gStatEngineInstance.AddSample(
                Statistic::RequestSample{
                    uri,
                    req->GetCreated(),
                    start,
                    finish,
                    peer,
                    !executeSuccess,
                    0,
                    0
                }
            );
        }

        return executeSuccess;
    }
};

/** WWW-Authenticate to present with 401 Unauthorized response */
static constexpr auto WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

// {privateProcessor, webProcessor}
std::pair<std::shared_ptr<RequestProcessor>, std::shared_ptr<RequestProcessor>> RPCFactory::Init(const ArgsManager& args, const util::Ref& context)
{
    int workQueueMainDepth = std::max((long) args.GetArg("-rpcworkqueue", DEFAULT_HTTP_WORKQUEUE), 1L);
    int workQueuePostDepth = std::max((long) args.GetArg("-rpcpostworkqueue", DEFAULT_HTTP_POST_WORKQUEUE), 1L);
    int workQueuePublicDepth = std::max((long) args.GetArg("-rpcpublicworkqueue", DEFAULT_HTTP_PUBLIC_WORKQUEUE), 1L);
    int workQueueStaticDepth = std::max((long) args.GetArg("-rpcstaticworkqueue", DEFAULT_HTTP_STATIC_WORKQUEUE), 1L);
    int workQueueRestDepth = std::max((long) args.GetArg("-rpcrestworkqueue", DEFAULT_HTTP_REST_WORKQUEUE), 1L);

    int rpcMainThreads = std::max((long) args.GetArg("-rpcthreads", DEFAULT_HTTP_THREADS), 1L);
    int rpcPostThreads = std::max((long) args.GetArg("-rpcpostthreads", DEFAULT_HTTP_POST_THREADS), 1L);
    int rpcPublicThreads = std::max((long) args.GetArg("-rpcpublicthreads", DEFAULT_HTTP_PUBLIC_THREADS), 1L);
    int rpcStaticThreads = std::max((long) args.GetArg("-rpcstaticthreads", DEFAULT_HTTP_STATIC_THREADS), 1L);
    int rpcRestThreads = std::max((long) args.GetArg("-rpcrestthreads", DEFAULT_HTTP_REST_THREADS), 1L);

    const auto& node = EnsureNodeContext(context);
    // General private socket
    auto privateTable = std::make_shared<CRPCTable>();
    RegisterBlockchainRPCCommands(*privateTable);
    RegisterNetRPCCommands(*privateTable);
    RegisterMiscRPCCommands(*privateTable);
    RegisterMiningRPCCommands(*privateTable);
    RegisterRawTransactionRPCCommands(*privateTable);

    for (const auto& client : node.chain_clients) {
        // TODO (losty-nat): pass table inside
        client->registerRpcs();
    }
#if ENABLE_ZMQ
    RegisterZMQRPCCommands(*privateTable);
#endif

    std::string strRPCUserColonPass;
    if (!InitRPCAuthentication(args, strRPCUserColonPass))
    {
        // TODO (losty-nat): log error
        return {nullptr, nullptr};
    }
    std::function<void(const util::Ref& context, const std::string& strURI, const std::string& body, const CRPCTable& table, std::shared_ptr<IReplier> replier, const DbConnectionRef& sqliteConnection)> authFunc = [authorizer = std::make_shared<Authorizer>(strRPCUserColonPass)](const util::Ref& context, const std::string& strURI, const std::string& body, const CRPCTable& table, std::shared_ptr<IReplier> replier, const DbConnectionRef& sqliteConnection) mutable {
        // TODO (losty-nat): peer from replier
        // auto peerAddr = req->GetPeer().ToString();
        // Check authorization
        std::string authStr;

        if (!replier->GetAuthCredentials(authStr)) {
            LogPrint(BCLog::RPC, "WARNING: Request without authorization header\n");
            replier->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
            replier->WriteReply(HTTP_UNAUTHORIZED);
            return false;
        }

        std::string authUser;
        if (!authorizer->RPCAuthorized(authStr, authUser)) {
            LogPrintf("ThreadRPCServer incorrect password %s attempt from\n", authStr /*TODO (losty-nat):, peerAddr*/);

            /*  Deter brute-forcing
                If this results in a DoS the user really
                shouldn't have their RPC port exposed.
            */
            UninterruptibleSleep(std::chrono::milliseconds{250});

            replier->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
            replier->WriteReply(HTTP_UNAUTHORIZED);
            return false;
        }

        return RPCTableExecutor::ProcessRPC(context, strURI, body, sqliteConnection, replier.get(), table);
    };
    auto handlerPrivate = std::make_shared<RPCTableFunctionalHandler>(privateTable, authFunc);
    std::function<void(const util::Ref& context, const std::string& strURI, const std::string& body, const CRPCTable& table, std::shared_ptr<IReplier> replier, const DbConnectionRef& sqliteConnection)> commonFunc = [](const util::Ref& context, const std::string& strURI, const std::string& body, const CRPCTable& table, std::shared_ptr<IReplier> replier, const DbConnectionRef& sqliteConnection) {
        return RPCTableExecutor::ProcessRPC(context, strURI, body, sqliteConnection, replier.get(), table);
    };

    auto privateRequestProcessor = std::make_shared<RequestProcessor>(context);
    std::shared_ptr<RequestProcessor> webRequestProcessor;
    // Additional pocketnet seocket
    if (args.GetBoolArg("-api", true))
    {
        webRequestProcessor = std::make_shared<RequestProcessor>(context);
        auto webTable = std::make_shared<CRPCTable>();
        auto webPostTable = std::make_shared<CRPCTable>();
        RegisterPocketnetWebRPCCommands(*webTable, *webPostTable);
        auto web = std::make_shared<RPCTableFunctionalHandler>(webTable, commonFunc);
        auto webPost = std::make_shared<RPCTableFunctionalHandler>(webPostTable, commonFunc);
        auto podWeb = std::make_shared<RequestHandlerPod>(std::vector<PathRequestHandlerEntry>{{"/", false, web}}, workQueuePublicDepth, rpcPublicThreads);
        auto podWebPost = std::make_shared<RequestHandlerPod>(std::vector<PathRequestHandlerEntry>{{"/post/", false, web}}, workQueuePostDepth, rpcPostThreads);

        webRequestProcessor->RegisterPod(std::move(podWeb));
        webRequestProcessor->RegisterPod(std::move(podWebPost));
    }

    auto podPrivate = std::make_shared<RequestHandlerPod>(std::vector<PathRequestHandlerEntry>{{"/", true, handlerPrivate}}, workQueueMainDepth, rpcMainThreads);
    privateRequestProcessor->RegisterPod(std::move(podPrivate));
    
    return {privateRequestProcessor, webRequestProcessor};
}
