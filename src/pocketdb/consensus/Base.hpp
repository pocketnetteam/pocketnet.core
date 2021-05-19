// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BASE_HPP
#define POCKETCONSENSUS_BASE_HPP

#include "pocketdb/models/base/Base.hpp"
#include "univalue/include/univalue.h"
#include "pocketdb/pocketnet.h"

namespace PocketConsensus
{
    using std::string;
    using std::shared_ptr;
    using std::make_shared;
    using std::map;
    using std::make_tuple;
    using std::tuple;

    using namespace PocketTx;
    //using namespace PocketDb;

    class BaseConsensus
    {
    public:
        BaseConsensus() = default;
    protected:
        virtual int CheckpointHeight() { return 0; };
    private:
    };

}

#endif // POCKETCONSENSUS_BASE_HPP
