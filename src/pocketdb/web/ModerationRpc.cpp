// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/ModerationRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetJuryAssigned()
    {
        return RPCHelpMan{"getjuryassigned",
            "\nGet assigned jury.\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Moderator address"},
                {"verdict", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Verdict 1|0 (Default: 0)"},
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
                HelpExampleCli("GetJuryAssigned", "address 2000 10") +
                HelpExampleRpc("GetJuryAssigned", "address 2000 10")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            const string address = request.params[0].get_str();

            bool verdict = false;
            if (request.params[1].isBool())
                verdict = request.params[1].get_bool();

            Pagination pagination{ ChainActiveSafeHeight(), 0, 10, "height", false };
            if (request.params[2].isNum())
                pagination.TopHeight = min(request.params[2].get_int(), pagination.TopHeight);
            if (request.params[3].isNum())
                pagination.PageStart = request.params[3].get_int();
            if (request.params[4].isNum())
                pagination.PageSize = request.params[4].get_int();
            if (request.params[5].isStr())
                pagination.OrderBy = request.params[5].get_str();
            if (request.params[6].isBool())
                pagination.OrderDesc = request.params[6].get_bool();

            auto juryList = request.DbConnection()->ModerationRepoInst->GetJuryAssigned(address, verdict, pagination);

            // Collect content ids
            vector<int64_t> accountIds;
            vector<int64_t> commentIds;
            vector<int64_t> contentIds;
            for (const auto& rcrd : juryList)
            {
                switch (rcrd.ContentType)
                {
                    case TxType::ACCOUNT_USER:
                        accountIds.push_back(rcrd.ContentId);
                        break;
                    case TxType::CONTENT_COMMENT:
                    case TxType::CONTENT_COMMENT_EDIT:
                        commentIds.push_back(rcrd.ContentId);
                        break;
                    default:
                        contentIds.push_back(rcrd.ContentId);
                        break;
                }
            }

            UniValue result(UniValue::VARR);

            // Collect accounts data
            if (!accountIds.empty())
            {
                auto contentMap = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(accountIds);
                for (const auto& rcrd : juryList)
                {
                    auto cnt = contentMap.find(rcrd.ContentId);
                    if (cnt != contentMap.end())
                    {
                        cnt->second.pushKV("jury", rcrd.JuryData);
                        result.push_back(cnt->second);
                    }
                }
            }

            // Collect comments data
            if (!commentIds.empty())
            {
                auto contentMap = request.DbConnection()->WebRpcRepoInst->GetCommentsByIds(commentIds, "");
                for (const auto& rcrd : juryList)
                {
                    auto cnt = contentMap.find(rcrd.ContentId);
                    if (cnt != contentMap.end())
                    {
                        cnt->second.pushKV("jury", rcrd.JuryData);
                        result.push_back(cnt->second);
                    }
                }
            }

            // Collect contents data
            if (!contentIds.empty())
            {
                auto contentMap = request.DbConnection()->WebRpcRepoInst->GetContentsData(contentIds);
                for (const auto& rcrd : juryList)
                {
                    auto cnt = contentMap.find(rcrd.ContentId);
                    if (cnt != contentMap.end())
                    {
                        cnt->second.pushKV("jury", rcrd.JuryData);
                        result.push_back(cnt->second);
                    }
                }
            }

            return result;
        }};
    }

    RPCHelpMan GetJuryModerators()
    {
        return RPCHelpMan{"getjurymoderators",
            "\nGet jury assigned moderators.\n",
            {
                {"jury", RPCArg::Type::STR, RPCArg::Optional::NO, "Jury transaction hash"},
            },
            RPCResult{
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR, "address", "The addresses of jury moderators."},
                }
            },
            RPCExamples{
                HelpExampleCli("getjurymoderators", "juryid") +
                HelpExampleRpc("getjurymoderators", "juryid")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            const string jury = request.params[0].get_str();

            return request.DbConnection()->ModerationRepoInst->GetJuryModerators(jury);
        }};
    }

    RPCHelpMan GetJury()
    {
        return RPCHelpMan{"getjury",
            "\nGet jury information.\n",
            {
                {"jury", RPCArg::Type::STR, RPCArg::Optional::NO, "Jury transaction hash"},
            },
            RPCResult{
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR_HEX, "id", "The jury id hash."},
                    {RPCResult::Type::STR, "address", "The addresses of author of content."},
                    {RPCResult::Type::NUM, "reason", "Reason jury."},
                    {RPCResult::Type::NUM, "verdict", "Verdict jury (0 or 1)."},
                }
            },
            RPCExamples{
                HelpExampleCli("getjury", "juryid") +
                HelpExampleRpc("getjury", "juryid")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            const string jury = request.params[0].get_str();

            return request.DbConnection()->ModerationRepoInst->GetJury(jury);
        }};
    }

    RPCHelpMan GetAllJury()
    {
        return RPCHelpMan{"getalljury",
            "\nGet list of all jury information.\n",
            {
                {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Start height of pagination"},
                {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Start page"},
                {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Size page"},
                {"orderBy", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Order by {Height}"},
                {"desc", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Order desc"},
            },
            RPCResult{
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR_HEX, "id", "The jury id hash."},
                    {RPCResult::Type::STR, "address", "The addresses of author of content."},
                    {RPCResult::Type::NUM, "reason", "Reason jury."},
                    {RPCResult::Type::NUM, "verdict", "Verdict jury (0 or 1)."},
                }
            },
            RPCExamples{
                HelpExampleCli("getjury", "juryid") +
                HelpExampleRpc("getjury", "juryid")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            Pagination pagination{ ChainActiveSafeHeight(), 0, 10, "height", true };

            if (request.params[0].isNum())
                pagination.TopHeight = min(request.params[0].get_int(), pagination.TopHeight);
            if (request.params[1].isNum())
                pagination.PageStart = request.params[1].get_int();
            if (request.params[2].isNum())
                pagination.PageSize = request.params[2].get_int();
            if (request.params[3].isStr())
                pagination.OrderBy = request.params[3].get_str();
            if (request.params[4].isBool())
                pagination.OrderDesc = request.params[4].get_bool();

            return request.DbConnection()->ModerationRepoInst->GetAllJury(pagination);
        }};
    }

    RPCHelpMan GetBans()
    {
        return RPCHelpMan{"getbans",
            "\nGet list bans information.\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address"},
            },
            RPCResult{
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR_HEX, "juryId", "Jury Id."},
                    {RPCResult::Type::NUM, "reason", "Reason jury."},
                    {RPCResult::Type::NUM, "ending", "Height of end ban."},
                }
            },
            RPCExamples{
                HelpExampleCli("getbans", "address") +
                HelpExampleRpc("getbans", "address")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            const string address = request.params[0].get_str();

            return request.DbConnection()->ModerationRepoInst->GetBans(address);
        }};
    }

}