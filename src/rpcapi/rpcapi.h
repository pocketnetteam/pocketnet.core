#ifndef POCKETCOIN_RPCAPI_H
#define POCKETCOIN_RPCAPI_H


#include "init.h"
#include "pocketdb/SQLiteConnection.h"
#include "rpc/server.h"
#include "util/ref.h"
#include "eventloop.h"

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

class IRequestHandler
{
public:
    virtual void Exec(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier, const DbConnectionRef& sqliteConnection) = 0;
};

class RPCTableFunctionalHandler : public IRequestHandler
{
public:
    RPCTableFunctionalHandler(std::shared_ptr<const CRPCTable> table, const std::function<void(const util::Ref&, const std::string&, const std::string&, const CRPCTable& table, std::shared_ptr<IReplier>, const DbConnectionRef&)>& func);
    void Exec(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier, const DbConnectionRef& sqliteConnection) override;

private:
    std::shared_ptr<const CRPCTable> m_table;
    std::function<void(const util::Ref&, const std::string&, const std::string&, const CRPCTable&, std::shared_ptr<IReplier>, const DbConnectionRef& sqliteConnection)> m_func;
};

class WorkItem {
public:
    WorkItem() = default;
    WorkItem(const util::Ref& context, const std::string& strURI, const std::string& body, std::shared_ptr<IReplier> replier, std::shared_ptr<IRequestHandler> handler);
    void Exec(const DbConnectionRef& sqliteConnection);

private:
    util::Ref m_context;
    std::string m_strURI;
    std::string m_body;
    std::shared_ptr<IReplier> m_replier;
    std::shared_ptr<IRequestHandler> m_handler;
};

struct PathRequestHandlerEntry
{
    std::string prefix;
    bool fExactMatch;
    std::shared_ptr<IRequestHandler> requestHandler;
};

class WorkItemExecutor : public IQueueProcessor<WorkItem>
{
public:
    void Process(WorkItem entry) override;
private:
    DbConnectionRef m_sqliteConnection = std::make_shared<PocketDb::SQLiteConnection>();
};

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
    std::shared_ptr<QueueLimited<WorkItem>> m_queue;
    std::vector<PathRequestHandlerEntry> m_handlers;
    std::vector<std::shared_ptr<QueueEventLoopThread<WorkItem>>> m_workers;
    int m_numThreads;
};

class IRequestProcessor
{
public:
    virtual bool Process(const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier) = 0;
};

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

    void RegisterPod(std::shared_ptr<RequestHandlerPod> pod)
    {
        m_pods.emplace_back(std::move(pod));
    }

private:
    std::vector<std::shared_ptr<RequestHandlerPod>> m_pods;
    util::Ref m_context;
};

#endif // POCKETCOIN_RPCAPI_H