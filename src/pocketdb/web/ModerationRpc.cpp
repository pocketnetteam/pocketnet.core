// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/ModerationRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetAssignedJury()
    {
        return RPCHelpMan{"getassignedjury",
                "\nGet assigned jury.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Start height of pagination"},
                    {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Start page"},
                    {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Size page"},
                    {"orderBy", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Order by {Height}"},
                    {"desc", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Order desc"},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "juryId", "The id(s) of jury."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getassignedjury", "address 2000 10") +
                    HelpExampleRpc("getassignedjury", "address 2000 10")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            const string address = request.params[0].get_str();

            Pagination pagination{ ChainActive().Height(), 0, 10, "height", false };

            if (request.params[1].isNum())
                pagination.TopHeight = min(request.params[1].get_int(), pagination.TopHeight);

            if (request.params[2].isNum())
                pagination.PageStart = request.params[2].get_int();

            if (request.params[3].isNum())
                pagination.PageSize = request.params[3].get_int();

            if (request.params[4].isStr())
                pagination.OrderBy = request.params[4].get_str();

            if (request.params[5].isBool())
                pagination.Desc = request.params[5].get_bool();

            return request.DbConnection()->ModerationRepoInst->GetAssignedJury(address, pagination);
        }};
    }

}