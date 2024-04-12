
#ifndef POCKETCOIN_RPCPROCESSOR_H
#define POCKETCOIN_RPCPROCESSOR_H


#include "eventloop.h"
#include "pocketdb/SQLiteConnection.h"
#include "util/ref.h"

/**
 * This interface is used to provide worker specific sqlite connection to underlying handler executor
 */
class IWorkItem {
public:
    // TODO (losty-nat): probably there should be more generalization to process requests that do not require sqlite connection.
    //                   It is not currently the case because each request requires it but will be a good improvement.
    virtual void Exec(const DbConnectionRef& sqliteConnection) = 0;
    virtual ~IWorkItem() = default;
};

/**
 * Executes workitem and provides unique sqlite connection to it
 * 
 */
class WorkItemExecutor : public IQueueProcessor<std::unique_ptr<IWorkItem>>
{
public:
    void Process(std::unique_ptr<IWorkItem> entry) override;
private:
    DbConnectionRef m_sqliteConnection = std::make_shared<PocketDb::SQLiteConnection>(false);
};

#endif // POCKETCOIN_RPCPROCESSOR_H