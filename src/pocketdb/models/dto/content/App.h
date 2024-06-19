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
            s3 - unique app id

            p.s1 - name
            p.s2 - description
            p.s4 - settings

        */

        const optional<string>& GetId() const;
        void SetId(const string& value);

        const optional<string>& GetName() const;
        void SetName(const string& value);

        const optional<string>& GetDescription() const;
        void SetDescription(const string& value);

        const optional<string>& GetSettings() const;
        void SetSettings(const string& value);
    };

} // namespace PocketTx

#endif // POCKETTX_APP_H