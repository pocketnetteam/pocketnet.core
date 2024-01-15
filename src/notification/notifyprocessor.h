#ifndef POCKETCOIN_NOTIFYPROCESSOR_H
#define POCKETCOIN_NOTIFYPROCESSOR_H

#include "eventloop.h"
#include "protectedmap.h"
#include "univalue.h"
#include "websocket/ws.h"
#include "notification/NotificationClient.h"
#include "notification/NotificationTypedefs.h"

#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/repositories/web/NotifierRepository.h"

using namespace PocketDb;

class CBlock;
class CBlockIndex;
class NotifyBlockProcessor;

typedef std::map<std::string, std::string> custom_fields;
namespace notifications {
class NotifyBlockProcessor : public IQueueProcessor<std::pair<CBlock, CBlockIndex*>>
{
public:
    explicit NotifyBlockProcessor(std::shared_ptr<NotifyableStorage> clients);
    ~NotifyBlockProcessor() override;
    void Process(std::pair<CBlock, CBlockIndex*> entry) override;

private:
    void PrepareWSMessage(std::map<std::string, std::vector<UniValue>>& messages, std::string msg_type, std::string addrTo, std::string txid, int64_t txtime, custom_fields cFields);

    std::shared_ptr<NotifyableStorage> m_clients;

    SQLiteDatabaseRef sqliteDbInst;
    NotifierRepositoryRef notifierRepoInst;
};
} // namespace notifications

#endif // POCKETCOIN_NOTIFYPROCESSOR_H
