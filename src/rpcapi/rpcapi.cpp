#include "rpcapi.h"

RPCTableFunctionalHandler::RPCTableFunctionalHandler(std::shared_ptr<const CRPCTable> table, const std::function<void(const RequestContext& reqContext, const CRPCTable&, const DbConnectionRef&)>& func)
    : m_table(std::move(table)),
      m_func(func)
{}


void RPCTableFunctionalHandler::Exec(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    m_func(reqContext, *m_table, sqliteConnection);
}


RequestWorkItem::RequestWorkItem(RequestContext reqContext, std::shared_ptr<IRequestHandler> handler)
    : m_reqContext(std::move(reqContext)),
      m_handler(std::move(handler))
{}


void RequestWorkItem::Exec(const DbConnectionRef& sqliteConnection)
{
    m_handler->Exec(m_reqContext, sqliteConnection);
}

RequestHandlerPod::RequestHandlerPod(std::vector<PathRequestHandlerEntry> handlers, int queueLimit, int nThreads)
    : m_handlers(std::move(handlers)),
      m_queue(std::make_shared<QueueLimited<std::unique_ptr<IWorkItem>>>(queueLimit)),
      m_numThreads(nThreads)
{}


PodProcessingResult RequestHandlerPod::Process(const util::Ref& context, const std::string& strURI, const std::string& body, const std::shared_ptr<IReplier>& replier)
{
    for (const auto& pathHandler : m_handlers) {
        bool match = false;
        if (pathHandler.fExactMatch)
            match = (strURI == pathHandler.prefix);
        else
            match = (strURI.substr(0, pathHandler.prefix.size()) == pathHandler.prefix);

        if (match) {
            RequestContext reqContext{context, strURI.substr(pathHandler.prefix.size()), body, replier};
            if (!m_queue->Add(std::make_unique<RequestWorkItem>(std::move(reqContext), pathHandler.requestHandler))) {
                return PodProcessingResult::QueueOverflow;
            }
            return PodProcessingResult::Success;
        }
    }

    return PodProcessingResult::NotFound;
}


bool RequestHandlerPod::Start()
{
    if (!m_workers.empty()) {
        return false;
    }

    for (int i = 0; i < m_numThreads; i++) {
        auto worker = std::make_shared<QueueEventLoopThread<std::unique_ptr<IWorkItem>>>(m_queue, std::make_shared<WorkItemExecutor>());
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
