// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/AppRepository.h"

namespace PocketDb
{
    vector<string> AppRepository::List(const AppListDto& args)
    {
        vector<string> result;

        string _filters = "";
        
        if (!args.Search.empty())
            _filters += " cross join search on pt.String2 like search.value or pt.String3 like search.value ";

        string _tagsStr = "[]";

        if (!args.Tags.empty()) {
            UniValue _tags(UniValue::VARR);
            for (auto t : args.Tags)
                _tags.push_back(t);

            _tagsStr = _tags.write();
            _filters += " cross join tags on bo.Tag = tags.value ";
        }

        string _orderBy = " ct.Height ";
        if (args.Page.OrderBy == "location")
            _orderBy = " pt.String6 ";
        if (args.Page.OrderBy == "price")
            _orderBy = " pt.Int1 ";
        if (args.Page.OrderDesc)
            _orderBy += " desc ";
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    
                )sql")
                .Bind(
                    args.Language,
                    _tagsStr,
                    _locationStr,
                    args.PriceMax,
                    args.PriceMin,
                    args.Search,
                    args.Page.TopHeight,
                    args.Page.PageSize,
                    args.Page.PageStart * args.Page.PageSize
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

}