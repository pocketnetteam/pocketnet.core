#include "rpcapi.h"

RPCTableFunctionalHandler::RPCTableFunctionalHandler(std::shared_ptr<const CRPCTable> table, const std::function<void(const util::Ref&, const std::string&, const std::string&, const CRPCTable& table, std::shared_ptr<IReplier>, const DbConnectionRef&)>& func)
    : m_table(std::move(table)),
      m_func(func)
{}


void RPCTableFunctionalHandler::Exec(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier, const DbConnectionRef& sqliteConnection)
{
    m_func(context, strURI, body, *m_table, replier, sqliteConnection);
}


WorkItem::WorkItem(const util::Ref& context, const std::string& strURI, const std::string& body, std::shared_ptr<IReplier> replier, std::shared_ptr<IRequestHandler> handler)
    : m_context(context),
      m_strURI(strURI),
      m_body(body),
      m_replier(std::move(replier)),
      m_handler(std::move(handler))
{}


void WorkItem::Exec(const DbConnectionRef& sqliteConnection)
{
    m_handler->Exec(m_context, m_strURI, m_body, m_replier, sqliteConnection);
}


void WorkItemExecutor::Process(WorkItem entry)
{
    entry.Exec(m_sqliteConnection);
}


RequestHandlerPod::RequestHandlerPod(std::vector<PathRequestHandlerEntry> handlers, int queueLimit, int nThreads)
    : m_handlers(std::move(handlers)),
      m_queue(std::make_shared<QueueLimited<WorkItem>>(queueLimit)),
      m_numThreads(nThreads)
{}


bool RequestHandlerPod::Process(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier)
{
    for (const auto& pathHandler : m_handlers) {
        bool match = false;
        if (pathHandler.fExactMatch)
            match = (strURI == pathHandler.prefix);
        else
            match = (strURI.substr(0, pathHandler.prefix.size()) == pathHandler.prefix);

        if (match) {
            m_queue->Add(WorkItem(context, strURI.substr(pathHandler.prefix.size()), body, replier, pathHandler.requestHandler));
            return true;
        }
    }

    return false;
}


bool RequestHandlerPod::Start()
{
    if (!m_workers.empty()) {
        return false;
    }

    for (int i = 0; i < m_numThreads; i++) {
        auto worker = std::make_shared<QueueEventLoopThread<WorkItem>>(m_queue, std::make_shared<WorkItemExecutor>());
        worker->Start();
        m_workers.emplace_back(std::move(worker));
    }

    return true;
}


void RequestHandlerPod::Interrupt()
{
    for (auto& worker : m_workers) {
        worker->Stop();
    }
    m_workers.clear();
}


void RequestHandlerPod::Stop()
{
    Interrupt();
    m_queue.reset();
}
