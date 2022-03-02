// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCOIN_REST_H
#define POCKETCOIN_REST_H

#include "rpcapi/rpcapi.h"
#include <util/ref.h>
#include "util/system.h"


class Rest
{
public:
    bool Init(const ArgsManager& args, const util::Ref& context);
    /** Start HTTP REST subsystem.
    * Precondition; HTTP and RPC has been started.
    */
    bool StartREST();
    /** Interrupt RPC REST subsystem.
    */
    void InterruptREST();
    /** Stop HTTP REST subsystem.
    * Precondition; HTTP and RPC has been stopped.
    */
    void StopREST();

    std::shared_ptr<IRequestProcessor> GetRestRequestProcessor() const
    {
        return m_restRequestProcessor;
    }

    std::shared_ptr<IRequestProcessor> GetStaticRequestProcessor() const
    {
        return m_staticRequestProcessor;
    }

private:
    std::shared_ptr<RequestProcessor> m_restRequestProcessor;
    std::shared_ptr<RequestProcessor> m_staticRequestProcessor;
};

#endif // POCKETCOIN_REST_H