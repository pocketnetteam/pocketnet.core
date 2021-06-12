// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_TRANSACTION_HPP
#define POCKETCONSENSUS_TRANSACTION_HPP

#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/pocketnet.h"

namespace PocketConsensus
{
    class TransactionConsensus
    {
    public:
        static bool Validate(PocketBlock& pBlock)
        {
            // TODO (brangr): implement
            // std::vector<std::string> vasm;
            // boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));
            // if (vasm.size() < 3) {
            //     resultCode = ANTIBOTRESULT::FailedOpReturn;
            //     return;
            // }

            // if (vasm[2] != oitm["data_hash"].get_str()) {
            //     if (table != "Users" || (table == "Users" && vasm[2] != oitm["data_hash_without_ref"].get_str())) {
            //         LogPrintf("FailedOpReturn vasm: %s - oitm: %s\n", vasm[2], oitm.write());
            //         resultCode = ANTIBOTRESULT::FailedOpReturn;
            //         return;
            //     }
            // }

            

            return true;
        }
    };

}

#endif // POCKETCONSENSUS_TRANSACTION_HPP
