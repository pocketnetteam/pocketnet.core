#ifndef POCKETCOIN_RPCHANDLER_H
#define POCKETCOIN_RPCHANDLER_H

#include "rpcapi.h"


class RPCFactory
{
public:
    std::pair<std::shared_ptr<RequestProcessor>, std::shared_ptr<RequestProcessor>> Init(const ArgsManager& args, const util::Ref& context);
};



#endif // POCKETCOIN_RPCHANDLER_H