#ifndef POCKETCOIN_RPCAPI_H
#define POCKETCOIN_RPCAPI_H


#include "init.h"
#include "pocketdb/SQLiteConnection.h"
#include "rpc/server.h"
#include "util/ref.h"
#include "eventloop.h"
#include "rpcapi/rpcprocessor.h"
#include "rpc/protocol.h"


#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <map>
#include <vector>



/**
 * Class to perform a per request reply.
 * Should not be handled as a connection for further communication. 
 */
class IReplier
{
public:
    /**
     * @param nStatus - http status code (for all connection types) 
     * @param reply - msg to reply with
     */
    virtual void WriteReply(int nStatus, const std::string& reply = "") = 0;
    virtual void WriteHeader(const std::string& hdr, const std::string& value) = 0; // TODO (losty-rpc): http specific, remove somehow
    virtual bool GetAuthCredentials(std::string& out) = 0; // TODO (losty-rpc): http specific, remove somehow
    virtual const std::string& GetPeerStr() const = 0;
    virtual Statistic::RequestTime GetCreated() const = 0; // TODO (losty-rpc): should not be here
    virtual ~IReplier() = default;
};

struct RequestContext
{
    util::Ref context;
    std::string path;
    std::string body;
    std::shared_ptr<IReplier> replier;
};

/**
 * Class to process requests.
 */
class IRequestHandler
{
public:
    /**
     * @param reqContext - request data
     * @param sqliteConnection - unique sqlite connection provided by worker
     */
    virtual void Exec(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection) = 0;
    virtual ~IRequestHandler() = default;
};

/**
 * Request handler that passes request data, sqlite connection and rpc table to user's defined function.
 */
class RPCTableFunctionalHandler : public IRequestHandler
{
public:
    RPCTableFunctionalHandler(std::shared_ptr<const CRPCTable> table, const std::function<void(const RequestContext& reqContext, const CRPCTable&, const DbConnectionRef&)>& func);
    void Exec(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection) override;

private:
    std::shared_ptr<const CRPCTable> m_table;
    std::function<void(const RequestContext& reqContext, const CRPCTable&, const DbConnectionRef& sqliteConnection)> m_func;
};

class FunctionalHandler : public IRequestHandler
{
public:
    FunctionalHandler(const std::function<void(const RequestContext& reqContext, const DbConnectionRef&)>& func)
        : m_func(func)
    {}
    void Exec(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
    {
        m_func(reqContext, sqliteConnection);
    }
private:
    std::function<void(const RequestContext& reqContext, const DbConnectionRef&)> m_func;
};

/**
 * Executes handler with given request context and sqlite connection
 * provided by worker
 */
class RequestWorkItem : public IWorkItem {
public:
    RequestWorkItem() = default;
    RequestWorkItem(RequestContext reqContext, std::shared_ptr<IRequestHandler> handler);
    void Exec(const DbConnectionRef& sqliteConnection) override;

private:
    RequestContext m_reqContext;
    std::shared_ptr<IRequestHandler> m_handler;
};

/**
 * Struct that represents "path" rules and a request handler that belongs to them.
 * There is only one instance of handler for each request so thread
 * syncronization should be done inside request handler itself.
 */
struct PathRequestHandlerEntry
{
    std::string prefix;
    bool fExactMatch;
    std::shared_ptr<IRequestHandler> requestHandler;
};

/**
 * Available answers for pod processing, so the RequestProcessor could handle different
 * behavior based on this result.
 */
enum class PodProcessingResult
{
    Success,
    NotFound,
    QueueOverflow
};

/**
 * The pod represents a pack of handlers that are attached to only one queue.
 * Pod is also controlling workers for a single queue
 */
class RequestHandlerPod
{
public:
    RequestHandlerPod(std::vector<PathRequestHandlerEntry> handlers, int queueLimit, int nThreads);
    PodProcessingResult Process(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier);

    bool Start();

    void Interrupt();

    // Illegal to call Start() again after this method
    void Stop();

private:
    std::shared_ptr<QueueLimited<std::unique_ptr<IWorkItem>>> m_queue;
    std::vector<PathRequestHandlerEntry> m_handlers;
    std::vector<std::shared_ptr<QueueEventLoopThread<std::unique_ptr<IWorkItem>>>> m_workers;
    int m_numThreads;
};

class IRequestsController {
public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual ~IRequestsController() = default;
};

/**
 * Interface for processing requests
 */
class IRequestProcessor
{
public:
    virtual bool Process(const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier) = 0;
    virtual ~IRequestProcessor() = default;
};

/**
 * Simple agregator to generalize manipulation with pods and provide node context to them.
 */
class RequestProcessor : public IRequestProcessor
{
public:
    RequestProcessor(const util::Ref &context)
        : m_context(context)
    {}
    bool Process(const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier) override
    {
        for (auto& pod : m_pods)
        {
            auto res = pod->Process(m_context, strURI, body, replier);
            if (res == PodProcessingResult::NotFound) {
                // Check all over pods.
                continue;
            } else if (res == PodProcessingResult::Success) {
                // Successfully find request handler. Reply will be sent inside.
                return true;
            } else if (res == PodProcessingResult::QueueOverflow) {
                // Find right handler but its queue is overflowed
                LogPrint(BCLog::RPCERROR, "WARNING: request rejected because http work queue depth exceeded.\n");
                replier->WriteReply(HTTP_INTERNAL_SERVER_ERROR, "Work queue depth exceeded");
                return false;
            }

        }

        // We are here if all pods reported with NotFound
        LogPrint(BCLog::HTTP, "Request from %s not found\n", replier->GetPeerStr());
        replier->WriteReply(HTTP_NOT_FOUND);
        return false;
    }

    void Start()
    {
        for (auto& pod: m_pods) {
            pod->Start();
        }
    }

    void Stop()
    {
        for (auto& pod: m_pods) {
            pod->Stop();
        }
    }

    void RegisterPod(std::shared_ptr<RequestHandlerPod> pod)
    {
        m_pods.emplace_back(std::move(pod));
    }

private:
    std::vector<std::shared_ptr<RequestHandlerPod>> m_pods;
    util::Ref m_context;
};

#endif // POCKETCOIN_RPCAPI_H