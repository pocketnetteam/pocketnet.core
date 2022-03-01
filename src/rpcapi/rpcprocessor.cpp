#include "rpcprocessor.h"

void WorkItemExecutor::Process(std::unique_ptr<IWorkItem> entry)
{
    // TODO (losty-nat): segfauilt possibility
    entry->Exec(m_sqliteConnection);
}