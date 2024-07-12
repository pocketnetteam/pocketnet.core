// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_OFFER_H
#define POCKETTX_BARTERON_OFFER_H

#include "pocketdb/models/base/SocialEditableTransaction.h"

namespace PocketTx
{
    using namespace std;

    class BarteronOffer : public SocialEditableTransaction
    {
    public:
        BarteronOffer();
        BarteronOffer(const CTransactionRef& tx);
        
        /*
            Use p.s1 for LANGUAGE
            Use p.s2 for CAPTION
            Use p.s3 for MESSAGE

            Use p.s4 for JSON payload
            {
                ...
                "t": 1 // General offer tag
                "a": [ 1, 2, 3, .. ] // Allowed barter tags
                ...
            }
        */
    };

} // namespace PocketTx

#endif // POCKETTX_BARTERON_OFFER_H