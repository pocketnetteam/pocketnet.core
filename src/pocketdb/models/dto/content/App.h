// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_APP_H
#define POCKETTX_APP_H

#include "pocketdb/models/base/SocialEditableTransaction.h"

namespace PocketTx
{
    using namespace std;

    class App : public SocialEditableTransaction
    {
    public:
        App();
        App(const CTransactionRef& tx);

        /*
            Usage:

            s1 - address
            s2 - root tx hash

            p.s1 - { data }
                {
                    "n": "App Name",
                    "d": "App Description",
                    "t": [ "tag1", "tag2", "tag3" ],
                    "u": "app.com",
                    "e": "app@mail.com",
                    "i": "app.com/icon.png",
                    ...
                }
            p.s2 - id

        */

        const optional<string>& GetId() const;
        
    };

} // namespace PocketTx

#endif // POCKETTX_APP_H