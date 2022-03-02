#ifndef POCKETCOIN_RPCAPI_H
#define POCKETCOIN_RPCAPI_H


#include "init.h"
#include "pocketdb/SQLiteConnection.h"
#include "rpc/server.h"
#include "util/ref.h"
#include "eventloop.h"
#include "rpcapi/rpcprocessor.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <map>
#include <vector>


class IReplier
{
public:
    virtual void WriteReply(int nStatus, const std::string& reply = "") = 0;
    virtual void WriteHeader(const std::string& hdr, const std::string& value) = 0;
    virtual bool GetAuthCredentials(std::string& out) = 0;
    virtual Statistic::RequestTime GetCreated() const = 0;
};

struct RequestContext
{
    util::Ref context;
    std::string path;
    std::string body;
    std::shared_ptr<IReplier> replier;
};

class IRequestHandler
{
public:
    virtual void Exec(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection) = 0;
};

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


class RequestWorkItem : public IWorkItem {
public:
    RequestWorkItem() = default;
    RequestWorkItem(RequestContext reqContext, std::shared_ptr<IRequestHandler> handler);
    void Exec(const DbConnectionRef& sqliteConnection) override;

private:
    RequestContext m_reqContext;
    std::shared_ptr<IRequestHandler> m_handler;
};

struct PathRequestHandlerEntry
{
    std::string prefix;
    bool fExactMatch;
    std::shared_ptr<IRequestHandler> requestHandler;
};


/**
 * The pod represents a pack of handlers that are attached to only one queue.
 * Pod is also controlling workers for a single queue
 */
class RequestHandlerPod
{
public:
    RequestHandlerPod(std::vector<PathRequestHandlerEntry> handlers, int queueLimit, int nThreads);
    bool Process(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier);

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
};

class IRequestProcessor
{
public:
    virtual bool Process(const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier) = 0;
};

/**
 * Simple agregator to generalize manipulation with pods and provide node context to them.
 * Has 2 interfaces: control and 
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
            // TODO (losty-nat): right handler can be found but request rejected because of queue overflow. Probably handle this inside pod.
            if (pod->Process(m_context, strURI, body, replier)) {
                return true;
            }
        }

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