#ifndef POCKETCOIN_NOTIFYPROCESSOR_H
#define POCKETCOIN_NOTIFYPROCESSOR_H

#include "eventloop.h"
#include "protectedmap.h"
#include "univalue.h"
#include "websocket/ws.h"

#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/repositories/web/NotifierRepository.h"

using namespace PocketDb;

class CBlock;
class CBlockIndex;
class WSUser;

typedef std::map<std::string, std::string> custom_fields;

class NotifyBlockProcessor : public IQueueProcessor<std::pair<CBlock, CBlockIndex*>>
{
public:
    explicit NotifyBlockProcessor(std::shared_ptr<ProtectedMap<std::string, WSUser>> WSConnections);
    ~NotifyBlockProcessor() override;
    void Process(std::pair<CBlock, CBlockIndex*> entry) override;

private:
    void PrepareWSMessage(std::map<std::string, std::vector<UniValue>>& messages, std::string msg_type, std::string addrTo, std::string txid, int64_t txtime, custom_fields cFields);
    std::shared_ptr<ProtectedMap<std::string, WSUser>> m_WSConnections;
    
    SQLiteDatabaseRef sqliteDbInst;
    NotifierRepositoryRef notifierRepoInst;
};

#endif // POCKETCOIN_NOTIFYPROCESSOR_H
