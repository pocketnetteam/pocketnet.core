#ifndef POCKETCOIN_RPCAPI_H
#define POCKETCOIN_RPCAPI_H


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
    virtual void WriteReply(int nStatus, const std::string& reply) = 0;
    virtual void WriteHeader(const std::string& hdr, const std::string& value) = 0;
    virtual bool GetAuthCredentials(std::string& out) = 0;
};

class IRequestHandler
{
public:
    virtual void Exec(const util::Ref& context, const std::string& strURI, const std::optional<std::string>& body, const std::shared_ptr<IReplier>& replier) = 0;
};

class RPCTableFunctionalHandler : public IRequestHandler
{
public:
    RPCTableFunctionalHandler(std::shared_ptr<const CRPCTable> table, const std::function<void(const util::Ref&, const std::string&, const std::optional<std::string>&, const CRPCTable& table, std::shared_ptr<IReplier>)>& func);
    void Exec(const util::Ref& context, const std::string& strURI, const std::optional<std::string>& body, const std::shared_ptr<IReplier>& replier) override;

private:
    std::shared_ptr<const CRPCTable> m_table;
    std::function<void(const util::Ref&, const std::string&, const std::optional<std::string>&, const CRPCTable&, std::shared_ptr<IReplier>)> m_func;
};

class WorkItem {
public:
    WorkItem(const util::Ref& context, const std::string& strURI, const std::optional<std::string>& body, std::shared_ptr<IReplier> replier, std::shared_ptr<IRequestHandler> handler);
    void Exec();

private:
    util::Ref m_context;
    std::string m_strURI;
    std::optional<std::string> m_body;
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
};

class RequestHandlerPod
{
public:
    RequestHandlerPod(std::vector<PathRequestHandlerEntry> handlers, int queueLimit);
    bool Process(const util::Ref& context, const std::string& strURI, const std::optional<std::string>& body, const std::shared_ptr<IReplier>& replier);

    bool Start(int nThreads);

    void Interrupt();

    // Illegal to call Start() again after this method
    void Stop();

private:
    std::shared_ptr<QueueLimited<WorkItem>> m_queue;
    std::vector<PathRequestHandlerEntry> m_handlers;
    std::vector<QueueEventLoopThread<WorkItem>> m_workers;
};

#endif // POCKETCOIN_RPCAPI_H