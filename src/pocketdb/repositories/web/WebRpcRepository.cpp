// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRpcRepository.h"

#include "pocketdb/helpers/ShortFormHelper.h"

#include <functional>

namespace PocketDb
{
    class NotificationsResult
    {
    public:
        bool HasData(const std::string& hash)
        {
            return m_data.find(hash) != m_data.end();
        }

        void InsertData(const PocketDb::ShortForm& shortForm)
        {
            m_data.insert({shortForm.GetTxData().GetHash(), shortForm});
        }

        void InsertNotifiers(const std::string& hash, std::set<std::string> addresses)
        {
            for (const auto& address: addresses) {
                m_notifiers[address].insert(hash);
            }
        }

        std::set<std::string> GetNotifiersAddresses() const
        {
            std::set<std::string> res;
            for (const auto& notifierEntry: m_notifiers) {
                res.insert(notifierEntry.first);
            }

            return res;
        }

        UniValue Serialize(const std::map<std::string, ShortAccount> accsData) const
        {
            std::map<std::string, std::pair<ShortTxType, int>> hashToIndexMap;
            UniValue data (UniValue::VARR);
            std::vector<UniValue> tmp;
            tmp.reserve(m_data.size());
            for (const auto& shortForm: m_data) {
                hashToIndexMap.insert({shortForm.first, {shortForm.second.GetType(), tmp.size()}});
                tmp.emplace_back(shortForm.second.Serialize());
            }
            data.push_backV(tmp);

            UniValue notifiers (UniValue::VOBJ);
            notifiers.reserveKVSize(m_notifiers.size());
            for (const auto& notifiersEntry: m_notifiers) {
                std::map<ShortTxType, std::vector<UniValue>> txIndicies;
                for (const auto& txHash: notifiersEntry.second) {
                    const auto& txEntry = hashToIndexMap.at(txHash);
                    txIndicies[txEntry.first].emplace_back(txEntry.second);
                }
                UniValue e (UniValue::VOBJ);
                for (const auto& lol: txIndicies) {
                    UniValue indicies (UniValue::VARR);
                    indicies.push_backV(lol.second);
                    e.pushKV(PocketHelpers::ShortTxTypeConvertor::toString(lol.first), indicies, false);
                }
                UniValue notifier (UniValue::VOBJ);
                notifier.pushKV("e", e);
                if (auto acc = accsData.find(notifiersEntry.first); acc != accsData.end()) {
                    notifier.pushKV("i", acc->second.Serialize());
                }
                notifiers.pushKV(notifiersEntry.first, notifier, false);
            }

            UniValue result (UniValue::VOBJ);
            result.pushKV("data", data);
            result.pushKV("notifiers", notifiers);

            return result;
        }

    private:
        std::map<std::string, PocketDb::ShortForm> m_data;
        std::map<std::string, std::set<std::string>> m_notifiers;
    };

    class ShortFormParser : public RowAccessor
    {
    public:
        void Reset(const int& startIndex)
        {
            m_startIndex = startIndex;
        }

        PocketDb::ShortForm ParseFull(sqlite3_stmt* stmt)
        {
            int index = m_startIndex;
            auto [ok, type] = TryGetColumnString(stmt, index++);
            if (!ok) {
                throw std::runtime_error("Missing row type");
            }

            auto txData = ProcessTxData(stmt, index);
            if (!txData) {
                throw std::runtime_error("Missing required fields for tx data in a row");
            }

            auto relatedContent = ProcessTxData(stmt, index);

            return {PocketHelpers::ShortTxTypeConvertor::strToType(type), *txData, relatedContent};
        }

        std::string ParseHash(sqlite3_stmt* stmt)
        {
            auto [ok, hash] = TryGetColumnString(stmt, m_startIndex+1);
            if (!ok) {
                throw std::runtime_error("Failed to extract tx hash from stmt");
            }

            return hash;
        }

        std::optional<std::vector<ShortTxOutput>> ParseOutputs(sqlite3_stmt* stmt)
        {
            auto [ok, str] = TryGetColumnString(stmt, m_startIndex + 9);
            if (ok) {
                return _parseOutputs(str);
            }
            return std::nullopt;
        }

    protected:
        std::optional<ShortTxData> ProcessTxData(sqlite3_stmt* stmt, int& index)
        {
            const auto i = index;

            static const auto stmtOffset = 17;
            index += stmtOffset;

            auto [ok1, hash] = TryGetColumnString(stmt, i);
            auto [ok2, txType] = TryGetColumnInt(stmt, i+1);

            if (ok1 && ok2) {
                ShortTxData txData(hash, (PocketTx::TxType)txType);
                if (auto [ok, val] = TryGetColumnString(stmt, i+2); ok) txData.SetAddress(val);
                if (auto [ok, val] = TryGetColumnInt64(stmt, i+3); ok) txData.SetHeight(val);
                if (auto [ok, val] = TryGetColumnInt64(stmt, i+4); ok) txData.SetBlockNum(val);
                if (auto [ok, val] = TryGetColumnString(stmt, i+5); ok) txData.SetRootTxHash(val);
                if (auto [ok, val] = TryGetColumnInt64(stmt, i+6); ok) txData.SetVal(val);
                if (auto [ok, val] = TryGetColumnString(stmt, i+7); ok) txData.SetInputs(_parseOutputs(val));
                if (auto [ok, val] = TryGetColumnString(stmt, i+8); ok) txData.SetOutputs(_parseOutputs(val));
                if (auto [ok, val] = TryGetColumnString(stmt, i+9); ok) txData.SetDescription(val);
                if (auto [ok, val] = TryGetColumnString(stmt, i+10); ok) txData.SetCommentParentId(val);
                if (auto [ok, val] = TryGetColumnString(stmt, i+11); ok) txData.SetCommentAnswerId(val);
                txData.SetAccount(_processAccount(stmt, i+12));
                if (auto [ok, val] = TryGetColumnString(stmt, i+16); ok) txData.SetMultipleAddresses(_processMultipleAddresses(val));
                return txData;
            }

            return std::nullopt;
        }

        std::optional<ShortAccount> _processAccount(sqlite3_stmt* stmt, const int& index)
        {
            auto [ok1, name] = TryGetColumnString(stmt, index);
            auto [ok2, avatar] = TryGetColumnString(stmt, index+1);
            auto [ok3, badge] = TryGetColumnString(stmt, index+2);
            auto [ok4, reputation] = TryGetColumnInt64(stmt, index+3);
            if (ok1 && ok4) { // TODO (losty): can there be no avatar?
                return ShortAccount(name, avatar, badge, reputation);
            }
            return std::nullopt;
        }

        std::optional<std::vector<ShortTxOutput>> _parseOutputs(const std::string& jsonStr)
        {
            UniValue json (UniValue::VOBJ);
            if (!json.read(jsonStr) || !json.isArray()) return std::nullopt;
            std::vector<ShortTxOutput> res;
            res.reserve(json.size());
            for (int i = 0; i < json.size(); i++) {
                const auto& elem = json[i];
                ShortTxOutput output;

                if (elem.exists("TxHash") && elem["TxHash"].isStr()) output.SetTxHash(elem["TxHash"].get_str());
                if (elem.exists("Value") && elem["Value"].isNum()) output.SetValue(elem["Value"].get_int64());
                if (elem.exists("SpentTxHash") && elem["SpentTxHash"].isStr()) output.SetSpentTxHash(elem["SpentTxHash"].get_str());
                if (elem.exists("AddressHash") && elem["AddressHash"].isStr()) output.SetAddressHash(elem["AddressHash"].get_str());
                if (elem.exists("Number") && elem["Number"].isNum()) output.SetNumber(elem["Number"].get_int());
                if (elem.exists("ScriptPubKey") && elem["ScriptPubKey"].isStr()) output.SetScriptPubKey(elem["ScriptPubKey"].get_str());

                res.emplace_back(std::move(output));
            }

            return res;
        }

        std::optional<std::vector<std::pair<std::string, std::optional<ShortAccount>>>> _processMultipleAddresses(const std::string& jsonStr)
        {
            UniValue json;
            if (!json.read(jsonStr) || !json.isArray() || json.size() <= 0) return std::nullopt;

            std::vector<std::pair<std::string, std::optional<ShortAccount>>> multipleAddresses;
            for (int i = 0; i < json.size(); i++) {
                const auto& entry = json[i];
                if (!entry.isObject() || !entry.exists("address") || !entry["address"].isStr()) {
                    continue;
                }

                auto address = entry["address"].get_str();
                std::optional<ShortAccount> accountData;
                if (entry.exists("account")) {
                    const auto& account = entry["account"];
                    if (account["name"].isStr()) {
                        ShortAccount accData;
                        accData.SetName(account["name"].get_str());
                        if (account["avatar"].isStr()) accData.SetAvatar(account["avatar"].get_str());
                        if (account["badge"].isStr()) accData.SetBadge(account["badge"].get_str());
                        if (account["reputation"].isNull()) accData.SetReputation(account["reputation"].get_int64());
                        accountData = std::move(accData);
                    }
                }

                multipleAddresses.emplace_back(std::make_pair(std::move(address), std::move(accountData)));
            }

            return multipleAddresses;
        }

    private:
        int m_startIndex = 0;
    };

    class EventsReconstructor : public RowAccessor
    {
    public:
        EventsReconstructor()
        {
            m_parser.Reset(0);
        }
        void FeedRow(sqlite3_stmt* stmt)
        {
            m_result.emplace_back(std::move(m_parser.ParseFull(stmt)));
        }

        std::vector<PocketDb::ShortForm> GetResult() const
        {
            return m_result;
        }
    private:
        ShortFormParser m_parser;
        std::vector<PocketDb::ShortForm> m_result;
    };

    class NotificationsReconstructor : public RowAccessor
    {
    public:
        NotificationsReconstructor() {
            m_parser.Reset(1);
        }

        void FeedRow(sqlite3_stmt* stmt)
        {
            std::set<std::string> notifiers;
            auto [ok, addressOne] = TryGetColumnString(stmt, 0);
            if (ok) {
                notifiers.insert(addressOne);
            } else {
                if (auto outputs = m_parser.ParseOutputs(stmt); outputs) {
                    for (const auto& output: *outputs) {
                        if (output.GetAddressHash() && !output.GetAddressHash()->empty()) {
                            notifiers.insert(*output.GetAddressHash());
                        }
                    }
                }
            }
            if (notifiers.empty()) throw std::runtime_error("Missing address of notifier");

            auto txHash = m_parser.ParseHash(stmt);
            if (!m_notifications.HasData(txHash)) {
                m_notifications.InsertData(m_parser.ParseFull(stmt));
            }
            m_notifications.InsertNotifiers(txHash, notifiers);
        }
        NotificationsResult GetResult() const
        {
            return m_notifications;
        }
    private:
        ShortFormParser m_parser;
        NotificationsResult m_notifications;
    };

    class NotificationSummaryReconstructor : public RowAccessor
    {
    public:
        void FeedRow(sqlite3_stmt* stmt)
        {
            auto [ok1, typeStr] = TryGetColumnString(stmt, 0);
            auto [ok2, address] = TryGetColumnString(stmt, 1);
            if (!ok1 || !ok2) return;

            if (auto type = PocketHelpers::ShortTxTypeConvertor::strToType(typeStr); type != ShortTxType::NotSet) {
                m_result[address][type]++;
            }
        }

        auto GetResult() const
        {
            return m_result;
        }
    private:
        std::map<std::string, std::map<ShortTxType, int>> m_result;  
    };


    void WebRpcRepository::Init() {}

    void WebRpcRepository::Destroy() {}

    UniValue WebRpcRepository::GetAddressId(const string& address)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT String1, Id
            FROM Transactions
            WHERE Type in (100,170)
              and Height is not null
              and Last = 1
              and String1 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) result.pushKV("id", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressId(int64_t id)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT String1, Id
            FROM Transactions
            WHERE Type in (100,170)
              and Height is not null
              and Last = 1
              and Id = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt64(stmt, 1, id);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) result.pushKV("id", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetUserAddress(const string& name)
    {
        UniValue result(UniValue::VARR);
        
        if (name.empty()) return result;

        auto _name = EscapeValue(name);

        string sql = R"sql(
            select p.String2, u.String1
            from Payload p indexed by Payload_String2_nocase_TxHash
            cross join Transactions u indexed by Transactions_Hash_Height
                on u.Type in (100) and u.Height > 0 and u.Hash = p.TxHash and u.Last = 1
            where p.String2 like ? escape '\'
            limit 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, _name);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("name", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressesRegistrationDates(const vector<string>& addresses)
    {
        auto result = UniValue(UniValue::VARR);

        if (addresses.empty())
            return result;

        string sql = R"sql(
            select u.String1, u.Time, u.Hash
            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            where u.Type in (100)
            and u.Last in (0,1)
            and u.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
            and u.Height = (
                select min(uf.Height)
                from Transactions uf indexed by Transactions_Id
                where uf.Id = u.Id
            )
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            for (size_t i = 0; i < addresses.size(); i++)
                TryBindStatementText(stmt, (int) i + 1, addresses[i]);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("time", valueStr);
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("txid", valueStr);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetTopAddresses(int count)
    {
        UniValue result(UniValue::VARR);

        auto sql = R"sql(
            select AddressHash, Value
            from Balances indexed by Balances_Last_Value
            where Last = 1
            order by Value desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue addr(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) addr.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) addr.pushKV("balance", value);
                result.push_back(addr);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountState(const string& address, int heightWindow)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select
                u.Id as AddressId,
                u.String1 as Address,
                u.Type,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (200) and p.Hash=p.String2 and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as PostSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (201) and p.Hash=p.String2 and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as VideoSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (202) and p.Hash=p.String2 and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as ArticleSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (204) and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as CommentSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (300) and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as ScoreSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (301) and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as ScoreCommentSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (307) and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as ComplainSpent,

                (select count() from Transactions p indexed by Transactions_Type_String1_Height_Time_Int1
                    where p.Type in (410) and p.String1=u.String1 and (p.Height>=? or p.Height isnull)) as FlagsSpent

            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id

            where u.Type in (100, 170)
            and u.Height is not null
            and u.String1 = ?
            and u.Last = 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, heightWindow);
            TryBindStatementInt(stmt, 2, heightWindow);
            TryBindStatementInt(stmt, 3, heightWindow);
            TryBindStatementInt(stmt, 4, heightWindow);
            TryBindStatementInt(stmt, 5, heightWindow);
            TryBindStatementInt(stmt, 6, heightWindow);
            TryBindStatementInt(stmt, 7, heightWindow);
            TryBindStatementInt(stmt, 8, heightWindow);
            TryBindStatementText(stmt, 9, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                int i = 0;
                if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("address_id", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) result.pushKV("address", value);

                bool isDeleted = false;
                if (auto[ok, Type] = TryGetColumnInt(*stmt, i++); ok)
                {
                    isDeleted = (Type==TxType::ACCOUNT_DELETE);
                    if (isDeleted) result.pushKV("deleted", true);
                }

                if (!isDeleted) {
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("post_spent", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("video_spent", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("article_spent", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("comment_spent", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("score_spent", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("comment_score_spent", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("complain_spent", value);
                
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) result.pushKV("mod_flag_spent", value);
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountSetting(const string& address)
    {
        string result;

        string sql = R"sql(
            select p.String1
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            join Payload p on p.TxHash = t.Hash
            where t.Type in (103)
              and t.Last = 1
              and t.Height is not null
              and t.String1 = ?
            limit 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result = value;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetUserStatistic(const vector<string>& addresses, const int nHeight, const int depthR, const int depthC, const int cntC)
    {
        UniValue result(UniValue::VARR);

        if (addresses.empty())
            return  result;

        string addressesWhere = join(vector<string>(addresses.size(), "?"), ",");

        string sql = R"sql(
            select
                u.String1 as Address,

                ifnull((
                  select count(1)
                  from Transactions ru indexed by Transactions_Type_Last_String2_Height
                  where ru.Type in (100)
                    and ru.Last in (0,1)
                    and ru.Height <= ?
                    and ru.Height > ?
                    and ru.String2 = u.String1
                    and ru.ROWID = (
                      select min(ru1.ROWID)
                      from Transactions ru1 indexed by Transactions_Id
                      where ru1.Id = ru.Id
                      limit 1
                    )
                  ),0) as ReferralsCountHist,

                (
                  select count(1)
                  from (
                         select c.String1
                         from Transactions p indexed by Transactions_String1_Last_Height
                         join Transactions c indexed by Transactions_Type_Last_String3_Height
                            on c.Type in (204)
                            and c.Last in (0,1)
                            and c.String3 = p.String2
                            and c.Height <= ?
                            and c.Height > ?
                            and c.Hash = c.String2
                            and c.String1 != u.String1
                         where p.Type in (200,201,202)
                            and p.Last = 1
                            and p.String1 = u.String1
                            and p.Height > 0
                         group by c.String1
                         having count(*) > ?)
                ) as CommentatorsCountHist

            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            where u.Type in (100)
              and u.Last = 1
              and u.String1 in ( )sql" + addressesWhere + R"sql( )
              and u.Height is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            // Referrals
            TryBindStatementInt(stmt, i++, nHeight);
            TryBindStatementInt(stmt, i++, nHeight - depthR);
            // Commentators
            TryBindStatementInt(stmt, i++, nHeight);
            TryBindStatementInt(stmt, i++, nHeight - depthC);
            TryBindStatementInt(stmt, i++, cntC);
            // Addresses
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                auto[ok0, address] = TryGetColumnString(*stmt, 0);
                auto[ok1, ReferralsCountHist] = TryGetColumnInt(*stmt, 1);
                auto[ok2, CommentatorsCountHist] = TryGetColumnInt(*stmt, 2);

                record.pushKV("address", address);
                record.pushKV("histreferals", ReferralsCountHist);
                record.pushKV("commentators", CommentatorsCountHist);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<tuple<string, int64_t, UniValue>> WebRpcRepository::GetAccountProfiles(
        const vector<string>& addresses,
        const vector<int64_t>& ids,
        bool shortForm,
        int firstFlagsDepth)
    {
        vector<tuple<string, int64_t, UniValue>> result{};

        if (addresses.empty() && ids.empty())
            return result;
        
        string where;
        if (!addresses.empty())
            where += " and u.String1 in (" + join(vector<string>(addresses.size(), "?"), ",") + ") ";
        if (!ids.empty())
            where += " and u.Id in (" + join(vector<string>(ids.size(), "?"), ",") + ") ";

        string fullProfileSql = "";
        if (!shortForm)
        {
            fullProfileSql = R"sql(

                , (
                    select json_group_array(json_object('adddress', subs.String2, 'private', case when subs.Type == 303 then 'true' else 'false' end))
                    from Transactions subs indexed by Transactions_Type_Last_String1_Height_Id
                    cross join Transactions uas indexed by Transactions_Type_Last_String1_Height_Id
                      on uas.String1 = subs.String2 and uas.Type = 100 and uas.Last = 1 and uas.Height is not null
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String1 = u.String1
                ) as Subscribes
                
                , (
                    select json_group_array(subs.String1)
                    from Transactions subs indexed by Transactions_Type_Last_String2_Height
                    cross join Transactions uas indexed by Transactions_Type_Last_String1_Height_Id
                      on uas.String1 = subs.String1 and uas.Type = 100 and uas.Last = 1 and uas.Height is not null
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String2 = u.String1
                ) as Subscribers

                , (
                    select json_group_array(ub.String1)
                    from BlockingLists bl
                    join Transactions ub indexed by Transactions_Type_Last_Height_Id
                    on ub.Id = bl.IdTarget and ub.Type = 100 and ub.Last = 1 and ub.Height is not null
                    where bl.IdSource = u.id
                ) as Blockings

            )sql";
        }

        string sql = R"sql(
            select
                  u.Hash as AccountHash
                , u.String1 as Address
                , u.Id
                , u.Type
                , ifnull(p.String2,'') as Name
                , ifnull(p.String3,'') as Avatar
                , ifnull(p.String7,'') as Donations
                , ifnull(u.String2,'') as Referrer

                , ifnull((
                    select count()
                    from Transactions po indexed by Transactions_Type_Last_String1_Height_Id
                    where po.Type in (200,201,202) and po.Last = 1 and po.Height > 0 and po.String1 = u.String1)
                ,0) as PostsCount

                , ifnull((
                    select count()
                    from Transactions po indexed by Transactions_Type_Last_String1_Height_Id
                    where po.Type in (207) and po.Last = 1 and po.Height > 0 and po.String1 = u.String1)
                ,0) as DelCount

                , ifnull((
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=0 and r.Id=u.Id and r.Last=1)
                ,0) as Reputation

                , (
                    select count()
                    from Transactions subs indexed by Transactions_Type_Last_String1_String2_Height
                    cross join Transactions uas indexed by Transactions_Type_Last_String1_Height_Id
                      on uas.String1 = subs.String2 and uas.Type = 100 and uas.Last = 1 and uas.Height is not null
                    where subs.Type in (302,303) and subs.Height > 0 and subs.Last = 1 and subs.String1 = u.String1
                ) as SubscribesCount

                , (
                    select count()
                    from Transactions subs indexed by Transactions_Type_Last_String2_String1_Height
                    cross join Transactions uas indexed by Transactions_Type_Last_String1_Height_Id
                      on uas.String1 = subs.String1 and uas.Type = 100 and uas.Last = 1 and uas.Height is not null
                    where subs.Type in (302,303) and subs.Height > 0 and subs.Last = 1 and subs.String2 = u.String1
                ) as SubscribersCount

                , (
                    select count()
                    from BlockingLists bl
                    where bl.IdSource = u.id
                ) as BlockingsCount

                , ifnull((
                    select sum(lkr.Value)
                    from Ratings lkr indexed by Ratings_Type_Id_Last_Height
                    where lkr.Type in (111,112,113) and lkr.Id = u.Id and lkr.Last = 1
                ),0) as Likers

                , ifnull(p.String6,'') as Pubkey
                , ifnull(p.String4,'') as About
                , ifnull(p.String1,'') as Lang
                , ifnull(p.String5,'') as Url
                , u.Time

                , (
                    select reg.Time
                    from Transactions reg indexed by Transactions_Id
                    where reg.Id=u.Id and reg.Height is not null order by reg.Height asc limit 1
                ) as RegistrationDate

                , (
                    select json_group_object(gr.Type, gr.Cnt)
                    from (
                      select (f.Int1)Type, (count())Cnt
                      from Transactions f indexed by Transactions_Type_Last_String3_Height
                      where f.Type in ( 410 )
                        and f.Last = 0
                        and f.String3 = u.String1
                        and f.Height > 0
                      group by f.Int1
                    )gr
                ) as FlagsJson

                , (
                    select json_group_object(gr.Type, gr.Cnt)
                    from (
                      select (f.Int1)Type, (count())Cnt
                      from Transactions f indexed by Transactions_Type_Last_String3_Height
                      cross join (
                        select min(fp.Height)minHeight
                        from Transactions fp
                        where fp.Type in (200,201,202)
                          and fp.String1 = u.String1
                          and fp.Hash = fp.String2
                          and fp.Last in (0, 1)
                          and fp.Height > 0
                      )fp
                      where f.Type in (410)
                        and f.Last = 0
                        and f.String3 = u.String1
                        and f.Height >= fp.minHeight
                        and f.Height <= (fp.minHeight + ?)
                        group by f.Int1
                    )gr
                ) as FirstFlagsCount

                )sql" + fullProfileSql + R"sql(

            from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
            left join Payload p on p.TxHash=u.Hash

            where u.Type in (100, 170)
              and u.Last = 1
              and u.Height is not null
              )sql" + where + R"sql(
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            // Bind parameters
            int i = 1;
            TryBindStatementInt(stmt, i++, firstFlagsDepth * 1440);
            for (const string& address : addresses)
                TryBindStatementText(stmt, i++, address);
            for (int64_t id : ids)
                TryBindStatementInt64(stmt, i++, id);

            // Fetch data
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                int i = 0;
                auto[ok0, hash] = TryGetColumnString(*stmt, i++);
                auto[ok1, address] = TryGetColumnString(*stmt, i++);
                auto[ok2, id] = TryGetColumnInt64(*stmt, i++);
                auto[ok3, Type] = TryGetColumnInt(*stmt, i++);
                bool isDeleted = (Type==TxType::ACCOUNT_DELETE);

                record.pushKV("hash", hash);
                record.pushKV("address", address);
                record.pushKV("id", id);
                if (IsDeveloper(address)) record.pushKV("dev", true);
                if (isDeleted) record.pushKV("deleted", true);

                if(!isDeleted) {
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("name", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("i", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("b", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("r", value);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("postcnt", value);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("dltdcnt", value);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("reputation", value / 10.0);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("subscribes_count", value);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("subscribers_count", value);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("blockings_count", value);
                    if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("likers_count", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("k", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("a", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("l", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("s", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) record.pushKV("update", value);
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, i++); ok) record.pushKV("regdate", value);

                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) {
                        UniValue flags(UniValue::VOBJ);
                        flags.read(value);
                        record.pushKV("flags", flags);
                    }

                    if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) {
                        UniValue flags(UniValue::VOBJ);
                        flags.read(value);
                        record.pushKV("firstFlags", flags);
                    }

                    if (!shortForm) {

                        if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) {
                            UniValue subscribes(UniValue::VARR);
                            subscribes.read(value);
                            record.pushKV("subscribes", subscribes);
                        }

                        if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) {
                            UniValue subscribes(UniValue::VARR);
                            subscribes.read(value);
                            record.pushKV("subscribers", subscribes);
                        }

                        if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) {
                            UniValue subscribes(UniValue::VARR);
                            subscribes.read(value);
                            record.pushKV("blocking", subscribes);
                        }
                    }
                }

                result.emplace_back(address, id, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetAccountProfiles(const vector<string>& addresses, bool shortForm, int firstFlagsDepth)
    {
        map<string, UniValue> result{};

        auto _result = GetAccountProfiles(addresses, {}, shortForm, firstFlagsDepth);
        for (const auto[address, id, record] : _result)
            result.insert_or_assign(address, record);

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetAccountProfiles(const vector<int64_t>& ids, bool shortForm, int firstFlagsDepth)
    {
        map<int64_t, UniValue> result{};

        auto _result = GetAccountProfiles({}, ids, shortForm, firstFlagsDepth);
        for (const auto[address, id, record] : _result)
            result.insert_or_assign(id, record);

        return result;
    }

    UniValue WebRpcRepository::GetLastComments(int count, int height, const string& lang)
    {
        auto func = __func__;
        auto result = UniValue(UniValue::VARR);

        auto sql = R"sql(
            select
              c.String2   as CommentTxHash,
              p.String2   as ContentTxHash,
              c.String1   as CommentAddress,
              c.Time      as CommentTime,
              c.Height    as CommentHeight,
              pc.String1  as CommentMessage,
              c.String4   as CommentParent,
              c.String5   as CommentAnswer,

              p.String1   as AddressContent,
              ifnull(
                (select String1 from Transactions where Hash = c.String4),
              '')   as AddressCommentParent,
              ifnull(
                (select String1 from Transactions where Hash = c.String5),
              '')   as AddressCommentAnswer,

              (
                select count(1)
                from Transactions sc indexed by Transactions_Type_Last_String2_Height
                where sc.Type = 301 and sc.Last in (0,1) and sc.Height > 0 and sc.String2 = c.String2 and sc.Int1 = 1
              ) as ScoreUp,

              (
                select count(1)
                from Transactions sc indexed by Transactions_Type_Last_String2_Height
                where sc.Type = 301 and sc.Last in (0,1) and sc.Height > 0 and sc.String2 = c.String2 and sc.Int1 = -1
              ) as ScoreDown,

              rc.Value    as CommentRating,

              (case when c.Hash != c.String2 then 1 else 0 end) as CommentEdit,

              (
                  select o.Value
                  from TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                  where o.TxHash = c.Hash and o.AddressHash = p.String1 and o.AddressHash != c.String1
              ) as Donate

            from Transactions c indexed by Transactions_Height_Id

            cross join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id
              on ua.String1 = c.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null

            cross join Transactions p indexed by Transactions_Type_Last_String2_Height
              on p.Type in (200,201,202) and p.Last = 1 and p.Height > 0 and p.String2 = c.String3

            cross join Payload pp indexed by Payload_String1_TxHash
              on pp.TxHash = p.Hash and pp.String1 = ?

            cross join Payload pc
              on pc.TxHash = c.Hash

            cross join Ratings rc indexed by Ratings_Type_Id_Last_Value
              on rc.Type = 3 and rc.Last = 1 and rc.Id = c.Id and rc.Value >= 0

            where c.Type in (204,205)
              and c.Last = 1
              and c.Height > (? - 600)

              and not exists (
                select 1
                from BlockingLists bl
                join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                where us.String1 = p.String1 and ut.String1 = c.String1
              )

            order by c.Height desc
            limit ?
        )sql";

        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, i++, lang);
            TryBindStatementInt(stmt, i++, height);
            TryBindStatementInt(stmt, i++, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                int i = 0;
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("id", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("postid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok)
                {
                    record.pushKV("time", value);
                    record.pushKV("timeUpd", value);
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("block", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("msg", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("addressContent", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("addressCommentParent", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("addressCommentAnswer", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("scoreUp", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("scoreDown", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, i++); ok) record.pushKV("edit", value == 1);
                if (auto[ok, value] = TryGetColumnString(*stmt, i++); ok)
                {
                    record.pushKV("donation", "true");
                    record.pushKV("amount", value);
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetLastComments(const vector<int64_t>& ids, const string& address)
    {
        auto func = __func__;
        map<int64_t, UniValue> result;

        string sql = R"sql(
            select
                cmnt.contentId,
                cmnt.commentId,
                c.Type,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                (select corig.Time from Transactions corig where corig.Hash = c.String2)Time,
                c.Time as TimeUpdate,
                c.Height,
                (select p.String1 from Payload p where p.TxHash = c.Hash)Message,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = 1) as ScoreUp,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1) as ScoreDown,

                (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Id = c.Id AND r.Type=3 and r.Last=1) as Reputation,

                (select count(*) from Transactions ch indexed by Transactions_Type_Last_String4_Height
                    where ch.Type in (204,205,206) and ch.Last = 1 and ch.Height is not null and ch.String4 = c.String2) as ChildrensCount,

                ifnull((select scr.Int1 from Transactions scr indexed by Transactions_Type_Last_String1_String2_Height
                    where scr.Type = 301 and scr.Last in (0,1) and scr.Height is not null and scr.String1 = ? and scr.String2 = c.String2),0) as MyScore,

                (
                    select o.Value
                    from TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                    where o.TxHash = c.Hash and o.AddressHash = cmnt.ContentAddressHash and o.AddressHash != c.String1
                ) as Donate

            from (
                select

                    (t.Id)contentId,
                    (t.String1)ContentAddressHash,

                    -- Last comment for content record
                    (
                        select c1.Id
                        
                        from Transactions c1 indexed by Transactions_Type_Last_String3_Height

                        cross join Transactions uac indexed by Transactions_Type_Last_String1_Height_Id
                          on uac.String1 = c1.String1 and uac.Type = 100 and uac.Last = 1 and uac.Height is not null

                        left join TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                            on o.TxHash = c1.Hash and o.AddressHash = t.String1 and o.AddressHash != c1.String1 and o.Value > ?

                        where c1.Type in (204, 205)
                          and c1.Last = 1
                          and c1.Height is not null
                          and c1.String3 = t.String2
                          and c1.String4 is null

                          -- exclude commenters blocked by the author of the post 
                          and not exists (
                            select 1
                            from BlockingLists bl
                            join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                            join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                            where us.String1 = t.String1 and ut.String1 = c1.String1
                          )

                        order by o.Value desc, c1.Id desc
                        limit 1
                    )commentId

                from Transactions t indexed by Transactions_Last_Id_Height

                cross join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id
                  on ua.String1 = t.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null

                where t.Type in (200,201,202,207)
                    and t.Last = 1
                    and t.Height is not null
                    and t.Id in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )

            ) cmnt

            join Transactions c indexed by Transactions_Last_Id_Height
                on c.Type in (204,205) and c.Last = 1 and c.Height is not null and c.Id = cmnt.commentId
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt64(stmt, i++, (int64_t)(0.5 * COIN));
            for (int64_t id : ids)
                TryBindStatementInt64(stmt, i++, id);

            // ---------------------------
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[okContentId, contentId] = TryGetColumnInt(*stmt, 0);
                auto[okCommentId, commentId] = TryGetColumnInt(*stmt, 1);
                auto[okType, txType] = TryGetColumnInt(*stmt, 2);
                auto[okRoot, rootTxHash] = TryGetColumnString(*stmt, 3);

                record.pushKV("id", rootTxHash);
                record.pushKV("cid", commentId);
                record.pushKV("edit", (TxType)txType == CONTENT_COMMENT_EDIT);
                record.pushKV("deleted", (TxType)txType == CONTENT_COMMENT_DELETE);

                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("postid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("timeUpd", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("block", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("msg", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok) record.pushKV("scoreUp", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) record.pushKV("scoreDown", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("children", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 16); ok) record.pushKV("myScore", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 17); ok)
                {
                    record.pushKV("donation", "true");
                    record.pushKV("amount", value);
                }
                                
                result.emplace(contentId, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash)
    {
        auto func = __func__;
        auto result = UniValue(UniValue::VARR);

        string parentWhere = " and c.String4 is null ";
        if (!parentHash.empty())
            parentWhere = " and c.String4 = ? ";

        auto sql = R"sql(
            select

                c.Type,
                c.Hash,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                r.Time AS RootTime,
                c.Time,
                c.Height,
                pl.String1 AS Msg,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Height is not null and sc.String2 = c.String2 and sc.Int1 = 1 and sc.Last in (0,1)) as ScoreUp,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Height is not null and sc.String2 = c.String2 and sc.Int1 = -1 and sc.Last in (0,1)) as ScoreDown,

                (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Id=c.Id and r.Type=3 and r.Last=1) as Reputation,

                sc.Int1 as MyScore,

                (
                    select count(1)
                    from Transactions s indexed by Transactions_Type_Last_String4_Height
                    -- exclude deleted accounts
                    cross join Transactions uac indexed by Transactions_Type_Last_String1_Height_Id
                    on uac.String1 = s.String1 and uac.Type = 100 and uac.Last = 1 and uac.Height is not null
                    where s.Type in (204, 205)
                      and s.Height is not null
                      and s.String4 = c.String2
                      and s.Last = 1
                      -- exclude commenters blocked by the author of the post
                      and not exists (
                        select 1
                        from BlockingLists bl
                        join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                        join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                        where us.String1 = t.String1 and ut.String1 = s.String1
                      )
                ) AS ChildrenCount,

                o.Value as Donate

            from Transactions c indexed by Transactions_Type_Last_String3_Height

            -- exclude deleted accounts
            cross join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id
                on ua.String1 = c.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null

            join Transactions r ON c.String2 = r.Hash

            join Payload pl ON pl.TxHash = c.Hash

            join Transactions t indexed by Transactions_Type_Last_String2_Height
                on t.Type in (200,201,202) and t.Last = 1 and t.Height is not null and t.String2 = c.String3

            left join Transactions sc indexed by Transactions_Type_String1_String2_Height
                on sc.Type in (301) and sc.Height is not null and sc.String2 = c.String2 and sc.String1 = ?

            left join TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                on o.TxHash = c.Hash and o.AddressHash = t.String1 and o.AddressHash != c.String1

            where c.Type in (204, 205, 206)
                and c.Height is not null
                and c.Last = 1
                and c.String3 = ?
                -- exclude commenters blocked by the author of the post
                and not exists (
                    select 1
                    from BlockingLists bl
                    join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                    join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                    where us.String1 = t.String1 and ut.String1 = c.String1
                )
                )sql" + parentWhere + R"sql(
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;
            TryBindStatementText(stmt, i++, addressHash);
            TryBindStatementText(stmt, i++, postHash);
            if (!parentHash.empty())
                TryBindStatementText(stmt, i++, parentHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                //auto[ok0, txHash] = TryGetColumnString(stmt, 1);
                auto[ok1, rootTxHash] = TryGetColumnString(*stmt, 2);
                record.pushKV("id", rootTxHash);

                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok)
                    record.pushKV("postid", value);

                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("timeUpd", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("block", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("msg", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) record.pushKV("scoreUp", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok) record.pushKV("scoreDown", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("myScore", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("children", value);

                if (auto[ok, value] = TryGetColumnString(*stmt, 16); ok)
                {
                    record.pushKV("amount", value);
                    record.pushKV("donation", "true");
                }

                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    switch (static_cast<TxType>(value))
                    {
                        case PocketTx::CONTENT_COMMENT:
                            record.pushKV("deleted", false);
                            record.pushKV("edit", false);
                            break;
                        case PocketTx::CONTENT_COMMENT_EDIT:
                            record.pushKV("deleted", false);
                            record.pushKV("edit", true);
                            break;
                        case PocketTx::CONTENT_COMMENT_DELETE:
                            record.pushKV("deleted", true);
                            record.pushKV("edit", true);
                            break;
                        default:
                            break;
                    }
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetCommentsByHashes(const vector<string>& cmntHashes, const string& addressHash)
    {
        auto result = UniValue(UniValue::VARR);

        if (cmntHashes.empty())
            return result;

        auto sql = R"sql(
            select

                c.Type,
                c.Hash,
                c.String2 as RootTxHash,
                c.String3 as PostTxHash,
                c.String1 as AddressHash,
                r.Time AS RootTime,
                c.Time,
                c.Height,
                pl.String1 AS Msg,
                c.String4 as ParentTxHash,
                c.String5 as AnswerTxHash,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Height is not null and sc.String2 = c.String2 and sc.Int1 = 1 and sc.Last in (0,1)) as ScoreUp,

                (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                    where sc.Type=301 and sc.Height is not null and sc.String2 = c.String2 and sc.Int1 = -1 and sc.Last in (0,1)) as ScoreDown,

                (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Id=c.Id and r.Type=3 and r.Last=1) as Reputation,

                sc.Int1 as MyScore,

                (
                    select count(1)
                    from Transactions s indexed by Transactions_Type_Last_String4_Height
                    where s.Type in (204, 205)
                      and s.Height is not null
                      and s.String4 = c.String2
                      and s.Last = 1
                      -- exclude commenters blocked by the author of the post
                      and not exists (
                        select 1
                        from BlockingLists bl
                        join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                        join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                        where us.String1 = t.String1 and ut.String1 = s.String1
                      )
                ) AS ChildrenCount,

                o.Value as Donate,

                (
                    select 1
                    from BlockingLists bl
                    join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                    join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                    where us.String1 = t.String1 and ut.String1 = c.String1
                    limit 1
                )Blocked

            from Transactions c indexed by Transactions_Type_Last_String2_Height

            join Transactions r ON c.String2 = r.Hash

            join Payload pl ON pl.TxHash = c.Hash

            join Transactions t indexed by Transactions_Type_Last_String2_Height
                on t.Type in (200,201,202) and t.Last = 1 and t.Height is not null and t.String2 = c.String3

            left join Transactions sc indexed by Transactions_Type_String1_String2_Height
                on sc.Type in (301) and sc.Height is not null and sc.String2 = c.String2 and sc.String1 = ?

            left join TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                on o.TxHash = c.Hash and o.AddressHash = t.String1 and o.AddressHash != c.String1

            where c.Type in (204, 205, 206)
                and c.Height is not null
                and c.Last = 1
                and c.String2 in ( )sql" + join(vector<string>(cmntHashes.size(), "?"), ",") + R"sql( )

        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;
            TryBindStatementText(stmt, i++, addressHash);
            for (const string& cmntHash : cmntHashes)
                TryBindStatementText(stmt, i++, cmntHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                //auto[ok0, txHash] = TryGetColumnString(stmt, 1);
                auto[ok1, rootTxHash] = TryGetColumnString(*stmt, 2);
                record.pushKV("id", rootTxHash);

                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok)
                    record.pushKV("postid", value);

                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("timeUpd", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("block", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("msg", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) record.pushKV("scoreUp", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok) record.pushKV("scoreDown", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("myScore", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("children", value);

                if (auto[ok, value] = TryGetColumnString(*stmt, 16); ok)
                {
                    record.pushKV("amount", value);
                    record.pushKV("donation", "true");
                }

                if (auto[ok, value] = TryGetColumnInt(*stmt, 17); ok && value > 0) record.pushKV("blck", 1);

                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    switch (static_cast<TxType>(value))
                    {
                        case PocketTx::CONTENT_COMMENT:
                            record.pushKV("deleted", false);
                            record.pushKV("edit", false);
                            break;
                        case PocketTx::CONTENT_COMMENT_EDIT:
                            record.pushKV("deleted", false);
                            record.pushKV("edit", true);
                            break;
                        case PocketTx::CONTENT_COMMENT_DELETE:
                            record.pushKV("deleted", true);
                            record.pushKV("edit", true);
                            break;
                        default:
                            break;
                    }
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetPagesScores(const vector<string>& postHashes, const vector<string>& commentHashes, const string& addressHash)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (!postHashes.empty())
        {
            string sql = R"sql(
                select
                    sc.String2 as ContentTxHash,
                    sc.Int1 as MyScoreValue

                from Transactions sc indexed by Transactions_Type_Last_String1_String2_Height

                where sc.Type in (300)
                  and sc.Last in (0,1)
                  and sc.Height is not null
                  and sc.String1 = ?
                  and sc.String2 in ( )sql" + join(vector<string>(postHashes.size(), "?"), ",") + R"sql( )
            )sql";

            TryTransactionStep(func, [&]()
            {
                auto stmt = SetupSqlStatement(sql);

                int i = 1;
                TryBindStatementText(stmt, i++, addressHash);
                for (const auto& postHash: postHashes)
                    TryBindStatementText(stmt, i++, postHash);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("value", value);

                    result.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        if (!commentHashes.empty())
        {
            string sql = R"sql(
                select
                    c.String2 as RootTxHash,

                    (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                        where sc.Type in (301) and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = 1) as ScoreUp,

                    (select count(1) from Transactions sc indexed by Transactions_Type_Last_String2_Height
                        where sc.Type in (301) and sc.Last in (0,1) and sc.Height is not null and sc.String2 = c.Hash and sc.Int1 = -1) as ScoreDown,

                    (select r.Value from Ratings r indexed by Ratings_Type_Id_Last_Value where r.Id=c.Id and r.Type=3 and r.Last=1) as Reputation,

                    msc.Int1 AS MyScore

                from Transactions c indexed by Transactions_Type_Last_String2_Height

                left join Transactions msc indexed by Transactions_Type_String1_String2_Height
                    on msc.Type in (301) and msc.Height is not null and msc.String2 = c.String2 and msc.String1 = ?

                where c.Type in (204, 205)
                    and c.Last = 1
                    and c.Height is not null
                    and c.String2 in ( )sql" + join(vector<string>(commentHashes.size(), "?"), ",") + R"sql( )
            )sql";

            TryTransactionStep(func, [&]()
            {
                auto stmt = SetupSqlStatement(sql);

                int i = 1;
                TryBindStatementText(stmt, i++, addressHash);
                for (const auto& commentHash: commentHashes)
                    TryBindStatementText(stmt, i++, commentHash);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("cmntid", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("scoreUp", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("scoreDown", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("myScore", value);

                    result.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        return result;
    }

    UniValue WebRpcRepository::GetPostScores(const string& postTxHash)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select
                s.String2 as ContentTxHash,
                s.String1 as ScoreAddressHash,
                p.String2 as AccountName,
                p.String3 as AccountAvatar,
                r.Value as AccountReputation,
                s.Int1 as ScoreValue

            from Transactions s indexed by Transactions_Type_Last_String2_Height

            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = s.String1
            
            left join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Type = 0 and r.Last = 1 and r.Id = u.Id

            cross join Payload p on p.TxHash = u.Hash

            where s.Type in (300)
              and s.Last in (0,1)
              and s.Height > 0
              and s.String2 = ?
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, postTxHash);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("value", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressScores(const vector<string>& postHashes, const string& address)
    {
        UniValue result(UniValue::VARR);

        string postWhere;
        if (!postHashes.empty())
        {
            postWhere += " and s.String2 in ( ";
            postWhere += join(vector<string>(postHashes.size(), "?"), ",");
            postWhere += " ) ";
        }

        string sql = R"sql(
            select
                s.String2 as posttxid,
                s.String1 as address,
                up.String2 as name,
                up.String3 as avatar,
                ur.Value as reputation,
                s.Int1 as value
            from Transactions s
            join Transactions u on u.Type in (100) and u.Height is not null and u.String1 = s.String1 and u.Last = 1
            join Payload up on up.TxHash = u.Hash
            left join (select ur.* from Ratings ur where ur.Type=0 and ur.Last=1) ur on ur.Id = u.Id
            where s.Type in (300)
                and s.String1 = ?
                and s.Height is not null
                )sql" + postWhere + R"sql()
            order by s.Time desc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            int i = 2;
            for (const auto& postHash: postHashes)
                TryBindStatementText(stmt, i++, postHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 4); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 5); ok) record.pushKV("value", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountRaters(const string& address)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select u.id,
                   u.String1           address,
                   up.String2          name,
                   up.String3          avatar,
                   up.String4          about,
                   (
                       select reg.Time
                       from Transactions reg indexed by Transactions_Id
                       where reg.Id = u.Id
                         and reg.Height is not null
                       order by reg.Height asc
                       limit 1
                   )                   registrationDate,
                   ifnull(ur.Value, 0) reputation,
                   raters.ratingsCount
            from (
                     select address, sum(ratingsCount) ratingsCount
                     from (
                              select rating.String1 address, count(1) ratingsCount
                              from Transactions content indexed by Transactions_Type_Last_String1_String2_Height
                              join Transactions rating indexed by Transactions_Type_Last_String2_Height
                                on rating.String2 = content.String2
                                    and rating.Type = 300
                                    and rating.Last in (0, 1)
                                    and rating.Int1 = 5
                                    and rating.Height is not null
                              where content.Type in (200, 201, 202)
                                and content.Last in (0, 1)
                                and content.Hash = content.String2
                                and content.String1 = ?
                                and content.Height is not null
                              group by rating.String1

                              union

                              select rating.String1 address, count(1) ratingsCount
                              from Transactions content indexed by Transactions_Type_Last_String1_String2_Height
                              join Transactions rating indexed by Transactions_Type_Last_String2_Height
                                on rating.String2 = content.String2
                                    and rating.Type = 301
                                    and rating.Last in (0, 1)
                                    and rating.Int1 = 1
                                    and rating.Height is not null
                              where content.Type in (204)
                                and content.Last in (0, 1)
                                and content.Hash = content.String2
                                and content.String1 = ?
                                and content.Height is not null
                              group by rating.String1
                         )
                     group by address
                ) raters
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
              on u.Type = 100 and u.Last = 1 and u.String1 = raters.address and u.Height is not null
            cross join Payload up on up.TxHash = u.Hash
            left join Ratings ur indexed by Ratings_Type_Id_Last_Value on ur.Type = 0 and ur.Id = u.Id and ur.Last = 1
            order by raters.ratingsCount desc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("id", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("about", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 5); ok) record.pushKV("regdate", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 6); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 7); ok) record.pushKV("ratingscnt", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetSubscribesAddresses(const string& address, const vector<TxType>& types)
    {
        UniValue result(UniValue::VARR);

        // TODO (brangr) (v0.21): implement
        // Should return pagination list of account profiles

        // string sql = R"sql(
        //     select
        //         String1 as AddressHash,
        //         String2 as AddressToHash,
        //         case
        //             when Type = 303 then 1
        //             else 0
        //         end as Private
        //     from Transactions indexed by Transactions_Type_Last_String1_String2_Height
        //     where Type in ( )sql" + join(types | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( )
        //       and Last = 1
        //       and Height > 0
        //       and String1 = ?
        // )sql";
        //
        // TryTransactionStep(__func__, [&]()
        // {
        //     auto stmt = SetupSqlStatement(sql);
        //
        //     int i = 1;
        //     TryBindStatementText(stmt, i++, address);
        //
        //     while (sqlite3_step(*stmt) == SQLITE_ROW)
        //     {
        //         UniValue record(UniValue::VOBJ);
        //         auto[ok, address] = TryGetColumnString(*stmt, 0);
        //
        //         if (auto[ok1, value] = TryGetColumnString(*stmt, 1); ok1) record.pushKV("adddress", value);
        //         if (auto[ok2, value] = TryGetColumnString(*stmt, 2); ok2) record.pushKV("private", value);
        //
        //         result[address].push_back(record);
        //     }
        //
        //     FinalizeSqlStatement(*stmt);
        // });

        return result;
    }

    UniValue WebRpcRepository::GetSubscribersAddresses(const string& address, const vector<TxType>& types)
    {
        // TODO (brangr) (v0.21): implement
        // Should return pagination list of account profiles
        return UniValue(UniValue::VARR);
    }

    UniValue WebRpcRepository::GetBlockings(const string& address)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select
                  bl.IdTarget
                from BlockingLists bl
                cross join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                cross join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                where us.String1 = ?
            )sql");
            TryBindStatementText(stmt, 1, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result.push_back(value);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    
    UniValue WebRpcRepository::GetBlockers(const string& address)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select
                  bl.IdSource
                from BlockingLists bl
                cross join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1 and ut.Height is not null
                cross join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1 and us.Height is not null
                where ut.String1 = ?
            )sql");
            TryBindStatementText(stmt, 1, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result.push_back(value);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<string> WebRpcRepository::GetTopAccounts(int topHeight, int countOut, const string& lang,
        const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& adrsExcluded, const vector<string>& tagsExcluded, int depth,
        int badReputationLimit)
    {
        auto func = __func__;
        vector<string> result;

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string langFilter;
        if (!lang.empty())
            langFilter += " cross join Payload p indexed by Payload_String1_TxHash on p.TxHash = u.Hash and p.String1 = ? ";

        string sql = R"sql(
            select t.String1

            from Transactions t indexed by Transactions_Last_Id_Height

            cross join Ratings cr indexed by Ratings_Type_Id_Last_Value
                on cr.Type = 2 and cr.Last = 1 and cr.Id = t.Id and cr.Value > 0

            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            )sql" + langFilter + R"sql(

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Last = 1
                and t.String3 is null
                and t.Height > ?
                and t.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " group by t.String1 ";
        sql += " order by count(*) desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight - depth);
            TryBindStatementInt(stmt, i++, topHeight);

            TryBindStatementInt(stmt, i++, badReputationLimit);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);

            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            TryBindStatementInt(stmt, i++, countOut);

            // ---------------------------------------------
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetTags(const string& lang, int pageSize, int pageStart)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select q.Lang, q.Value, q.cnt
            from (
                select t.Lang, t.Value, (select count(1) from web.TagsMap tm where tm.TagId = t.Id)cnt
                from web.Tags t indexed by Tags_Lang_Id
                where t.Lang = ?
            )q
            order by cnt desc
            limit ?
            offset ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, lang);
            TryBindStatementInt(stmt, i++, pageSize);
            TryBindStatementInt(stmt, i++, pageStart);
                

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, vLang] = TryGetColumnString(*stmt, 0);
                auto[ok1, vValue] = TryGetColumnString(*stmt, 1);
                auto[ok2, vCount] = TryGetColumnInt(*stmt, 2);

                UniValue record(UniValue::VOBJ);
                record.pushKV("tag", vValue);
                record.pushKV("count", vCount);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<int64_t> WebRpcRepository::GetContentIds(const vector<string>& txHashes)
    {
        vector<int64_t> result;

        if (txHashes.empty())
            return result;

        string sql = R"sql(
            select Id
            from Transactions indexed by Transactions_Hash_Height
            where Hash in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
              and Height is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            
            int i = 1;
            for (const string& txHash : txHashes)
                TryBindStatementText(stmt, i++, txHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string,string> WebRpcRepository::GetContentsAddresses(const vector<string>& txHashes)
    {
        map<string, string> result;

        if (txHashes.empty())
            return result;

        string sql = R"sql(
            select Hash, String1
            from Transactions
            where Hash in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
              and Height is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const string& txHash : txHashes)
                TryBindStatementText(stmt, i++, txHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, contenthash] = TryGetColumnString(*stmt, 0);
                auto[ok1, contentaddress] = TryGetColumnString(*stmt, 1);
                if(ok0 && ok1)
                    result.emplace(contenthash,contentaddress);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetUnspents(const vector<string>& addresses, int height,
        vector<pair<string, uint32_t>>& mempoolInputs)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select
                o.TxHash,
                o.Number,
                o.AddressHash,
                o.Value,
                o.ScriptPubKey,
                t.Type,
                o.TxHeight
            from TxOutputs o indexed by TxOutputs_SpentHeight_AddressHash
            join Transactions t on t.Hash=o.TxHash
            where o.AddressHash in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
              and o.TxHeight is not null
              and o.SpentHeight is null
            order by o.TxHeight asc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& address: addresses)
                TryBindStatementText(stmt, i++, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[ok0, txHash] = TryGetColumnString(*stmt, 0);
                auto[ok1, txOut] = TryGetColumnInt(*stmt, 1);

                string _txHash = txHash;
                int _txOut = txOut;
                // Exclude outputs already used as inputs in mempool
                if (!ok0 || !ok1 || find_if(
                    mempoolInputs.begin(),
                    mempoolInputs.end(),
                    [&](const pair<string, uint32_t>& itm)
                    {
                        return itm.first == _txHash && itm.second == _txOut;
                    })  != mempoolInputs.end())
                    continue;

                record.pushKV("txid", txHash);
                record.pushKV("vout", txOut);

                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 3); ok)
                {
                    record.pushKV("amount", ValueFromAmount(value));
                    record.pushKV("amountSat", value);
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("scriptPubKey", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok)
                {
                    record.pushKV("coinbase", value == 2 || value == 3);
                    record.pushKV("pockettx", value > 3);
                }
                if (auto[ok, value] = TryGetColumnInt(*stmt, 6); ok)
                {
                    record.pushKV("confirmations", height - value);
                    record.pushKV("height", value);
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<int, UniValue> WebRpcRepository::GetContentLanguages(int height)
    {
        int resultCount = 0;
        UniValue resultData(UniValue::VOBJ);

        string sql = R"sql(
            select c.Type,
                   p.String1 as lang,
                   count(*) as cnt
            from Transactions c
            join Payload p on p.TxHash = c.Hash
            where c.Type in (200, 201, 202)
              and c.Last = 1
              and c.Height is not null
              and c.Height > ?
            group by c.Type, p.String1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, height);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[okType, typeInt] = TryGetColumnInt(*stmt, 0);
                auto[okLang, lang] = TryGetColumnString(*stmt, 1);
                auto[okCount, count] = TryGetColumnInt(*stmt, 2);
                if (!okType || !okLang || !okCount)
                    continue;

                auto type = TransactionHelper::TxStringType((TxType) typeInt);

                if (resultData.At(type).isNull())
                    resultData.pushKV(type, UniValue(UniValue::VOBJ));

                resultData.At(type).pushKV(lang, count);
                resultCount += count;
            }

            FinalizeSqlStatement(*stmt);
        });

        return {resultCount, resultData};
    }

    tuple<int, UniValue> WebRpcRepository::GetLastAddressContent(const string& address, int height, int count)
    {
        int resultCount = 0;
        UniValue resultData(UniValue::VARR);

        // Get count
        string sqlCount = R"sql(
            select count(*)
            from Transactions
            where Type in (200, 201, 202)
              and Last = 1
              and Height is not null
              and Height > ?
              and String1 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sqlCount);
            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    resultCount = value;

            FinalizeSqlStatement(*stmt);
        });

        // Try get last N records
        if (resultCount > 0)
        {
            string sql = R"sql(
                select t.String2 as txHash,
                    t.Time,
                    t.Height,
                    t.String1 as addrFrom,
                    p.String2 as nameFrom,
                    p.String3 as avatarFrom
                from Transactions t
                cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                    on u.String1 = t.String1 and u.Type in (100) and u.Last = 1 and u.Height > 0
                cross join Payload p on p.TxHash = u.Hash
                where t.Type in (200, 201, 202)
                    and t.Last = 1
                    and t.Height is not null
                    and t.Height > ?
                    and t.String1 = ?
                order by t.Height desc
                limit ?
            )sql";

            TryTransactionStep(__func__, [&]()
            {
                auto stmt = SetupSqlStatement(sql);
                TryBindStatementInt(stmt, 1, height);
                TryBindStatementText(stmt, 2, address);
                TryBindStatementInt(stmt, 3, count);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    auto[okHash, hash] = TryGetColumnString(*stmt, 0);
                    auto[okTime, time] = TryGetColumnInt64(*stmt, 1);
                    auto[okHeight, block] = TryGetColumnInt(*stmt, 2);
                    if (!okHash || !okTime || !okHeight)
                        continue;

                    UniValue record(UniValue::VOBJ);
                    record.pushKV("txid", hash);
                    record.pushKV("time", time);
                    record.pushKV("nblock", block);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("addrFrom", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("nameFrom", value);
                    if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("avatarFrom", value);
                    resultData.push_back(record);
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        return {resultCount, resultData};
    }
    
    UniValue WebRpcRepository::GetContentsForAddress(const string& address)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string sql = R"sql(
            select

                t.Id,
                t.String2 as RootTxHash,
                t.Time,
                p.String2 as Caption,
                p.String3 as Message,
                p.String6 as Settings,
                ifnull(r.Value,0) as Reputation,

                (select count(*) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2) as ScoresCount,

                ifnull((select sum(scr.Int1) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2),0) as ScoresSum

            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            left join Payload p on t.Hash = p.TxHash
            left join Ratings r indexed by Ratings_Type_Id_Last_Height
                on r.Type = 2 and r.Last = 1 and r.Id = t.Id

            where t.Type in (200, 201, 202)
                and t.Last = 1
                and t.String1 = ?
            order by t.Height desc
            limit 50
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[ok0, id] = TryGetColumnInt64(*stmt, 0);
                auto[ok1, hash] = TryGetColumnString(*stmt, 1);
                auto[ok2, time] = TryGetColumnString(*stmt, 2);
                auto[ok3, caption] = TryGetColumnString(*stmt, 3);
                auto[ok4, message] = TryGetColumnString(*stmt, 4);
                auto[ok5, settings] = TryGetColumnString(*stmt, 5);
                auto[ok6, reputation] = TryGetColumnString(*stmt, 6);
                auto[ok7, scoreCnt] = TryGetColumnString(*stmt, 7);
                auto[ok8, scoreSum] = TryGetColumnString(*stmt, 8);
                
                if (ok3) record.pushKV("content", HtmlUtils::UrlDecode(caption));
                else record.pushKV("content", HtmlUtils::UrlDecode(message).substr(0, 100));

                record.pushKV("txid", hash);
                record.pushKV("time", time);
                record.pushKV("reputation", reputation);
                record.pushKV("settings", settings);
                record.pushKV("scoreSum", scoreSum);
                record.pushKV("scoreCnt", scoreCnt);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedRelayedContent(const string& address, int height)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                r.String2 as RootTxHash,
                r.String3 as RelayTxHash,
                r.String1 as AddressHash,
                r.Time,
                r.Height
            from Transactions r
            join Transactions p on p.Hash = r.String3 and p.String1 = ?
            where r.Type in (200, 201, 202)
              and r.Last = 1
              and r.Height is not null
              and r.Height > ?
              and r.String3 is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("msg", "reshare");
                if (auto[ok, val] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", val);
                if (auto[ok, val] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txidRepost", val);
                if (auto[ok, val] = TryGetColumnString(*stmt, 2); ok) record.pushKV("addrFrom", val);
                if (auto[ok, val] = TryGetColumnInt64(*stmt, 3); ok) record.pushKV("time", val);
                if (auto[ok, val] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("nblock", val);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedContentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                s.String1 as address,
                s.Hash,
                s.Time,
                s.String2 as posttxid,
                s.Int1 as value,
                s.Height
            from Transactions c indexed by Transactions_Type_Last_String1_Height_Id
            join Transactions s indexed by Transactions_Type_Last_String2_Height
                on s.Type in (300) and s.Last in (0,1) and s.String2 = c.String2 and s.Height is not null and s.Height > ?
            where c.Type in (200, 201, 202)
              and c.Last = 1
              and c.Height is not null
              and c.String1 = ?
            order by s.Time desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);
            TryBindStatementInt(stmt, 3, limit);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "event");
                record.pushKV("mesType", "upvoteShare");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("upvoteVal", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok) record.pushKV("nblock", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                s.String1 as address,
                s.Hash,
                s.Time,
                s.String2 as commenttxid,
                s.Int1 as value,
                s.Height
            from Transactions c indexed by Transactions_Type_Last_String1_Height_Id
            join Transactions s indexed by Transactions_Type_Last_String2_Height
                on s.Type in (301) and s.String2 = c.String2 and s.Height is not null and s.Height > ?
            where c.Type in (204, 205)
              and c.Last = 1
              and c.Height is not null
              and c.String1 = ?
            order by s.Time desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementInt(stmt, 3, limit);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "event");
                record.pushKV("mesType", "cScore");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("commentid", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("upvoteVal", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok) record.pushKV("nblock", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetMissedTransactions(const string& address, int height, int count)
    {
        map<string, UniValue> result;

        string sql = R"sql(
            select
                o.TxHash,
                t.Time,
                o.Value,
                o.TxHeight,
                t.Type
            from TxOutputs o indexed by TxOutputs_TxHeight_AddressHash
            join Transactions t on t.Hash = o.TxHash
            where o.AddressHash = ?
              and o.TxHeight > ?
            order by o.TxHeight desc
            limit ?
         )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementInt(stmt, 3, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[okTxHash, txHash] = TryGetColumnString(*stmt, 0);
                if (!okTxHash) continue;

                UniValue record(UniValue::VOBJ);
                record.pushKV("txid", txHash);
                record.pushKV("addr", address);
                record.pushKV("msg", "transaction");
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("amount", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("nblock", value);

                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok)
                {
                    auto stringType = TransactionHelper::TxStringType((TxType) value);
                    if (!stringType.empty())
                        record.pushKV("type", stringType);
                }

                result.emplace(txHash, record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentAnswers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                a.String2 as RootTxHash,
                a.Time,
                a.Height,
                a.String1 as addrFrom,
                a.String3 as posttxid,
                a.String4 as parentid,
                a.String5 as answerid
            from Transactions c indexed by Transactions_Type_Last_String1_String2_Height
            join Transactions a indexed by Transactions_Type_Last_Height_String5_String1
                on a.Type in (204, 205) and a.Height > ? and a.Last = 1 and a.String5 = c.String2 and a.String1 != c.String1
            where c.Type in (204, 205)
              and c.Last = 1
              and c.Height is not null
              and c.String1 = ?
            order by a.Height desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);
            TryBindStatementInt(stmt, 3, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "comment");
                record.pushKV("mesType", "answer");
                record.pushKV("reason", "answer");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok) record.pushKV("nblock", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("answerid", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedPostComments(const string& address, const vector<string>& excludePosts,
        int height, int count)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                c.String2 as RootTxHash,
                c.Time,
                c.Height,
                c.String1 as addrFrom,
                c.String3 as posttxid,
                c.String4 as  parentid,
                c.String5 as  answerid,
                (
                    select o.Value
                    from TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                    where o.TxHash = c.Hash and o.AddressHash = p.String1 and o.AddressHash != c.String1
                ) as Donate
            from Transactions p indexed by Transactions_Type_Last_String1_String2_Height
            join Transactions c indexed by Transactions_Type_Last_String3_Height
                on c.Type in (204, 205) and c.Height > ? and c.Last = 1 and c.String3 = p.String2 and c.String1 != p.String1
            where p.Type in (200, 201, 202)
              and p.Last = 1
              and p.Height is not null
              and p.String1 = ?
              and p.String2 not in ( )sql" + join(vector<string>(excludePosts.size(), "?"), ",") + R"sql( )
            order by c.Height desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementInt(stmt, i++, height);
            TryBindStatementText(stmt, i++, address);
            for (const auto& excludePost: excludePosts)
                TryBindStatementText(stmt, i++, excludePost);
            TryBindStatementInt(stmt, i, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "comment");
                record.pushKV("mesType", "post");
                record.pushKV("reason", "post");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok) record.pushKV("nblock", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("parentid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("answerid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok)
                {
                    record.pushKV("donation", "true");
                    record.pushKV("amount", value);
                }

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedSubscribers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                subs.Type,
                subs.Hash,
                subs.Time,
                subs.Height,
                subs.String1 as addrFrom,
                p.String2 as nameFrom,
                p.String3 as avatarFrom
            from Transactions subs indexed by Transactions_Type_Last_String2_Height
             cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on subs.String1 = u.String1 and u.Type in (100)
                    and u.Height is not null
                    and u.Last = 1
             cross join Payload p on p.TxHash = u.Hash
            where subs.Type in (302, 303, 304)
                and subs.Height > ?
                and subs.Last = 1
                and subs.String2 = ?
            order by subs.Height desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);
            TryBindStatementInt(stmt, 3, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "event");
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    switch (value)
                    {
                        case 302: record.pushKV("mesType", "subscribe"); break;
                        case 303: record.pushKV("mesType", "subscribePrivate"); break;
                        case 304: record.pushKV("mesType", "unsubscribe"); break;
                        default: record.pushKV("mesType", "unknown"); break;
                    }
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("nblock", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("nameFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("avatarFrom", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedBoosts(const string& address, int height, int count)
    {
        vector<UniValue> result;

        string sql = R"sql(
            select
                tBoost.String1 as boostAddress,
                tBoost.Hash,
                tBoost.Time,
                tBoost.Height,
                tBoost.String2 as contenttxid,
                tBoost.Int1 as boostAmount,
                p.String2 as boostName,
                p.String3 as boostAvatar
            from Transactions tBoost indexed by Transactions_Type_Last_Height_Id
            join Transactions tContent indexed by Transactions_Type_Last_String1_String2_Height
                on tContent.String2=tBoost.String2 and tContent.Last = 1 and tContent.Height > 0 and tContent.Type in (200, 201, 202)
            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.String1 = tBoost.String1 and u.Type in (100) and u.Last = 1 and u.Height > 0
            join Payload p
                on p.TxHash = u.Hash
            where tBoost.Type in (208)
              and tBoost.Last in (0, 1)
              and tBoost.Height > ?
              and tContent.String1 = ?
            order by tBoost.Time desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);
            TryBindStatementInt(stmt, 3, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("addr", address);
                record.pushKV("msg", "event");
                record.pushKV("mesType", "contentBoost");
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("addrFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("txid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("nblock", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 5); ok) record.pushKV("boostAmount", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("nameFrom", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) record.pushKV("avatarFrom", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::SearchLinks(const vector<string>& links, const vector<int>& contentTypes,
        const int nHeight, const int countOut)
    {
        UniValue result(UniValue::VARR);

        if (links.empty())
            return result;

        string contentTypesWhere = join(vector<string>(contentTypes.size(), "?"), ",");
        string  linksWhere = join(vector<string>(links.size(), "?"), ",");

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Hash_Height
            join Payload p indexed by Payload_String7 on p.TxHash = t.Hash
            where t.Type in ( )sql" + contentTypesWhere + R"sql( )
                and t.Height <= ?
                and t.Last = 1
                and p.String7 in ( )sql" + linksWhere + R"sql( )
            limit ?
        )sql";

        vector<int64_t> ids;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, nHeight);

            for (const auto& link: links)
                TryBindStatementText(stmt, i++, link);

            TryBindStatementInt(stmt, i++, countOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, "");
        result.push_backV(contents);

        return result;
    }

    UniValue WebRpcRepository::GetContentsStatistic(const vector<string>& addresses, const vector<int>& contentTypes)
    {
        UniValue result(UniValue::VARR);

        if (addresses.empty())
            return result;

        string sql = R"sql(
            select

              sum(s.Int1) as scrSum,
              count(1) as scrCnt,
              count(distinct s.String1) as countLikers

            from Transactions v indexed by Transactions_Type_Last_String1_Height_Id

            join Transactions s indexed by Transactions_Type_Last_String2_Height
              on s.Type in ( 300 ) and s.Last in (0,1) and s.Height > 0 and s.String2 = v.String2

            where v.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
              and v.Last = 1
              and v.Height > 0
              and v.String1 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementText(stmt, i++, addresses[0]);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                record.pushKV("address", addresses[0]);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("scoreSum", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok) record.pushKV("scoreCnt", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok) record.pushKV("countLikers", value);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // ------------------------------------------------------
    // Feeds

    UniValue WebRpcRepository::GetHotPosts(int countOut, const int depth, const int nHeight, const string& lang,
        const vector<int>& contentTypes, const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select t.Id

            from Transactions t indexed by Transactions_Type_Last_String3_Height

            join Payload p indexed by Payload_String1_TxHash
                on p.String1 = ? and t.Hash = p.TxHash

            join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Type = 2 and r.Last = 1 and r.Id = t.Id and r.Value > 0

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                and t.Last = 1
                and t.Height is not null
                and t.Height <= ?
                and t.Height > ?
                and t.String3 is null

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

            order by r.Value desc
            limit ?
        )sql";

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, lang);
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementInt(stmt, i++, nHeight);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, badReputationLimit);
            TryBindStatementInt(stmt, i++, countOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, address);
        result.push_backV(contents);

        return result;
    }

    vector<UniValue> WebRpcRepository::GetContentsData(const vector<int64_t>& ids, const string& address)
    {
        auto func = __func__;
        vector<UniValue> result{};

        if (ids.empty())
            return result;

        string sql = R"sql(
            select
                t.String2 as RootTxHash,
                t.Id,
                case when t.Hash != t.String2 then 'true' else null end edit,
                t.String3 as RelayTxHash,
                t.String1 as AddressHash,
                t.Time,
                p.String1 as Lang,
                t.Type,
                p.String2 as Caption,
                p.String3 as Message,
                p.String7 as Url,
                p.String4 as Tags,
                p.String5 as Images,
                p.String6 as Settings,

                (select count() from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2) as ScoresCount,

                ifnull((select sum(scr.Int1) from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2),0) as ScoresSum,

                (select count() from Transactions rep indexed by Transactions_Type_Last_String3_Height
                    where rep.Type in (200,201,202) and rep.Last = 1 and rep.Height is not null and rep.String3 = t.String2) as Reposted,

                (
                    select count()
                    from Transactions s indexed by Transactions_Type_Last_String3_Height
                    where s.Type in (204, 205)
                      and s.Height is not null
                      and s.String3 = t.String2
                      and s.Last = 1
                      -- exclude commenters blocked by the author of the post
                      and not exists (
                        select 1
                        from BlockingLists bl
                        join Transactions us on us.Id = bl.IdSource and us.Type = 100 and us.Last = 1
                        join Transactions ut on ut.Id = bl.IdTarget and ut.Type = 100 and ut.Last = 1
                        where us.String1 = t.String1 and ut.String1 = s.String1
                      )
                ) AS CommentsCount,
                
                ifnull((select scr.Int1 from Transactions scr indexed by Transactions_Type_Last_String1_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String1 = ? and scr.String2 = t.String2),0) as MyScore

            from Transactions t indexed by Transactions_Last_Id_Height
            cross join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id
                on ua.String1 = t.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null
            left join Payload p on t.Hash = p.TxHash
            where t.Height is not null
              and t.Last = 1
              and t.Id in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
        )sql";

        // Get posts
        unordered_map<int64_t, UniValue> tmpResult{};
        vector<string> authors;
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            TryBindStatementText(stmt, i++, address);

            for (int64_t id : ids)
                TryBindStatementInt64(stmt, i++, id);

            // ---------------------------
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto[okHash, txHash] = TryGetColumnString(*stmt, 0);
                auto[okId, txId] = TryGetColumnInt64(*stmt, 1);
                record.pushKV("txid", txHash);
                record.pushKV("id", txId);

                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("edit", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("repost", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok)
                {
                    authors.emplace_back(value);
                    record.pushKV("address", value);
                }
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) record.pushKV("time", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) record.pushKV("l", value); // lang
                if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) record.pushKV("c", value); // caption
                if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) record.pushKV("m", value); // message
                if (auto[ok, value] = TryGetColumnString(*stmt, 10); ok) record.pushKV("u", value); // url
                
                if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok)
                {
                    record.pushKV("type", TransactionHelper::TxStringType((TxType) value));
                    if ((TxType)value == CONTENT_DELETE)
                        record.pushKV("deleted", "true");
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok)
                {
                    UniValue t(UniValue::VARR);
                    t.read(value);
                    record.pushKV("t", t);
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok)
                {
                    UniValue ii(UniValue::VARR);
                    ii.read(value);
                    record.pushKV("i", ii);
                }

                if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok)
                {
                    UniValue s(UniValue::VOBJ);
                    s.read(value);
                    record.pushKV("s", s);
                }

                if (auto [ok, value] = TryGetColumnString(*stmt, 14); ok) record.pushKV("scoreCnt", value);
                if (auto [ok, value] = TryGetColumnString(*stmt, 15); ok) record.pushKV("scoreSum", value);
                if (auto [ok, value] = TryGetColumnInt(*stmt, 16); ok && value > 0) record.pushKV("reposted", value);
                if (auto [ok, value] = TryGetColumnInt(*stmt, 17); ok) record.pushKV("comments", value);

                if (!address.empty())
                {
                    if (auto [ok, value] = TryGetColumnString(*stmt, 18); ok)
                        record.pushKV("myVal", value);
                }

                tmpResult[txId] = record;                
            }

            FinalizeSqlStatement(*stmt);
        });

        // ---------------------------------------------
        // Get last comments for all posts
        auto lastComments = GetLastComments(ids, address);
        for (auto& record : tmpResult)
            record.second.pushKV("lastComment", lastComments[record.first]);

        // ---------------------------------------------
        // Get profiles for posts
        auto profiles = GetAccountProfiles(authors);
        for (auto& record : tmpResult)
            record.second.pushKV("userprofile", profiles[record.second["address"].get_str()]);

        // ---------------------------------------------
        // Place in result data with source sorting
        for (auto& id : ids)
            result.push_back(tmpResult[id]);

        return result;
    }

    UniValue WebRpcRepository::GetTopFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int depth, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and t.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " cross join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sql = R"sql(
            select t.Id

            from Transactions t indexed by Transactions_Type_Last_Height_Id

            cross join Ratings cr indexed by Ratings_Type_Id_Last_Value
                on cr.Type = 2 and cr.Last = 1 and cr.Id = t.Id and cr.Value > 0

            )sql" + langFilter + R"sql(

            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Last = 1
                --and t.String3 is null
                and t.Height > ?
                and t.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " order by cr.Value desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;

        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight - depth);
            TryBindStatementInt(stmt, i++, topHeight);

            TryBindStatementInt(stmt, i++, badReputationLimit);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);

            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);

            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            TryBindStatementInt(stmt, i++, countOut);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                ids.push_back(contentId);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData(ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetMostCommentedFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int depth, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        // if (topContentId > 0)
        //     contentIdWhere = " and t.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " cross join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sql = R"sql(
            select t.Id,

            (
            select count()
            from Transactions s --indexed by Transactions_Type_Last_String3_Height
            where s.Type in (204, 205) and s.Last = 1 and s.String3 = t.String2 and s.Height is not null
                -- exclude commenters blocked by the author of the post
                and not exists (
                                select 1
                                from Transactions b --indexed by Transactions_Type_Last_String1_String2_Height
                                where b.Type in (305) and b.Last = 1 and b.String1 = t.String1 and b.String2 = s.String1 and b.Height > 0
                                )
            ) commentsCount

            from Transactions t indexed by Transactions_Type_Last_Height_Id

            --cross join Ratings cr indexed by Ratings_Type_Id_Last_Value
            --    on cr.Type = 2 and cr.Last = 1 and cr.Id = t.Id and cr.Value > 0

            )sql" + langFilter + R"sql(

            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Last = 1
                --and t.String3 is null
                and t.Height > ?
                and t.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        // ---------------------------------------------

        vector<pair<int64_t, int>> idswithcomments;
        vector<int64_t> ids;

        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight - depth);
            TryBindStatementInt(stmt, i++, topHeight);

            TryBindStatementInt(stmt, i++, badReputationLimit);

            // if (topContentId > 0)
            //     TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);

            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);

            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                auto[ok1, commentsCount] = TryGetColumnInt(*stmt, 1);

                idswithcomments.emplace_back(contentId, commentsCount);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (!idswithcomments.empty())
        {
            std::sort(idswithcomments.begin(), idswithcomments.end(),
                [] (const auto &x, const auto &y) {return x.second > y.second;});

            std::transform(idswithcomments.begin(), idswithcomments.end(), std::back_inserter(ids),
                [](decltype(idswithcomments)::value_type const &pair) {
                    return pair.first;
                });

            if (ids.size() > countOut)
                ids = {ids.begin(), ids.begin() + countOut};

            // Get content data
            if (!ids.empty())
            {
                auto contents = GetContentsData(ids, address);
                result.push_backV(contents);
            }
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetProfileFeed(const string& addressFeed, int countOut, int pageNumber, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, const string& keyword, const string& orderby, const string& ascdesc)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressFeed.empty())
            return result;

        // ---------------------------------------------
        string _keyword;
        if(!keyword.empty())
        {
            _keyword = "\"" + keyword + "\"" + " OR " + keyword + "*";
        }

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and t.Id < ? ";

        string accountExistence = " join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id "
                                  " on ua.String1 = t.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sorting = "t.Id ";
        if (orderby == "comment")
        {
            sorting = R"sql(
                (
                    select count()
                    from Transactions s indexed by Transactions_Type_Last_String3_Height
                    where s.Type in (204, 205)
                      and s.Height is not null
                      and s.String3 = t.String2
                      and s.Last = 1
                      -- exclude commenters blocked by the author of the post
                      and not exists (
                        select 1
                        from Transactions b indexed by Transactions_Type_Last_String1_Height_Id
                        where b.Type in (305)
                            and b.Last = 1
                            and b.Height > 0
                            and b.String1 = t.String1
                            and b.String2 = s.String1
                      )
                )
            )sql";
        }
        if (orderby == "score")
        {
            sorting = R"sql(
                (
                    select count() from Transactions scr indexed by Transactions_Type_Last_String2_Height
                    where scr.Type = 300 and scr.Last in (0,1) and scr.Height is not null and scr.String2 = t.String2
                )
            )sql";
        }
        sorting += " " + ascdesc;

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            )sql" + accountExistence + R"sql(
            )sql" + langFilter + R"sql(
            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Height > 0
                and t.Height <= ?
                and t.Last = 1
                and t.String1 = ?
                )sql" + contentIdWhere   + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        if(!_keyword.empty())
        {
            sql += R"sql(
                and t.id in (
                    select cm.ContentId
                    from web.Content c
                    join web.ContentMap cm on c.ROWID = cm.ROWID
                    where cm.FieldType in (2, 3, 4, 5)
                        and c.Value match ?
                )
            )sql";
        }

        sql += R"sql( order by
        )sql" + sorting   + R"sql(
         limit ?
         offset ?
        )sql";

        // ---------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight);
            
            TryBindStatementText(stmt, i++, addressFeed);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);

            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);

            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!_keyword.empty())
                TryBindStatementText(stmt, i++, _keyword);

            TryBindStatementInt(stmt, i++, countOut);

            TryBindStatementInt(stmt, i++, pageNumber * countOut);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData(ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetSubscribesFeed(const string& addressFeed, int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, const vector<string>& addresses_extended)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressFeed.empty())
            return result;

        // ---------------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and cnt.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = cnt.Hash and p.String1 = ? ";

        string sql = R"sql(
            select cnt.Id

            from Transactions cnt indexed by Transactions_Type_Last_String1_Height_Id

            cross join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id
                on ua.String1 = cnt.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null

            )sql" + langFilter + R"sql(

            where cnt.Type in )sql" + contentTypesWhere + R"sql(
              and cnt.Last = 1
              and cnt.String1 in (
                  select subs.String2
                  from Transactions subs indexed by Transactions_Type_Last_String1_Height_Id
                  where subs.Type in (302,303)
                    and subs.Last = 1
                    and subs.String1 = ?
                    and subs.Height > 0
                    )sql" + (!addresses_extended.empty() ? (" union select value from json_each(json_array(" + join(vector<string>(addresses_extended.size(), "?"), ",") + "))") : "") + R"sql(
              )
              and cnt.Height > 0
              and cnt.Height <= ?
            
            )sql" + contentIdWhere + R"sql(

        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and cnt.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and cnt.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and cnt.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and cnt.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " order by cnt.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementText(stmt, i++, addressFeed);
            if (!addresses_extended.empty())
                for (const auto& adr_ex: addresses_extended)
                    TryBindStatementText(stmt, i++, adr_ex);

            TryBindStatementInt(stmt, i++, topHeight);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);

            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);

            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            TryBindStatementInt(stmt, i++, countOut);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData(ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetHistoricalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and t.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sql = R"sql(
            select t.Id

            from Transactions t indexed by Transactions_Last_Id_Height

            )sql" + langFilter + R"sql(

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Last = 1
                and t.String3 is null
                and t.Height > 0
                and t.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " order by t.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;

        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight);

            TryBindStatementInt(stmt, i++, badReputationLimit);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);
            
            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);
            
            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }
                    
            TryBindStatementInt(stmt, i++, countOut);

            // ---------------------------------------------
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                ids.push_back(contentId);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData(ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetHierarchicalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        // ---------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sql = R"sql(
            select
                (t.Id)ContentId,
                ifnull(pr.Value,0)ContentRating,
                ifnull(ur.Value,0)AccountRating,
                torig.Height,
                
                ifnull((
                    select sum(ifnull(pr.Value,0))
                    from (
                        select p.Id
                        from Transactions p indexed by Transactions_Type_Last_String1_Height_Id
                        where p.Type in ( )sql" + contentTypesFilter + R"sql( )
                            and p.Last = 1
                            and p.String1 = t.String1
                            and p.Height < torig.Height
                            and p.Height > (torig.Height - ?)
                        order by p.Height desc
                        limit ?
                    )q
                    left join Ratings pr indexed by Ratings_Type_Id_Last_Height
                        on pr.Type = 2 and pr.Id = q.Id and pr.Last = 1
                ), 0)SumRating

            from Transactions t indexed by Transactions_Type_Last_String3_Height

            )sql" + langFilter + R"sql(

            join Transactions torig indexed by Transactions_Id on torig.Height > 0 and torig.Id = t.Id and torig.Hash = torig.String2

            left join Ratings pr indexed by Ratings_Type_Id_Last_Height
                on pr.Type = 2 and pr.Last = 1 and pr.Id = t.Id

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in ( )sql" + contentTypesFilter + R"sql( )
                and t.Last = 1
                and t.String3 is null
                and t.Height <= ?
                and t.Height > ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?
        )sql";

        if (!tags.empty())
        {
            sql += R"sql( and t.id in (
                select tm.ContentId
                from web.Tags tag indexed by Tags_Lang_Value_Id
                join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                    on tag.Id = tm.TagId
                where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
            ) )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        // ---------------------------------------------
        vector<HierarchicalRecord> postsRanks;
        double dekay = (contentTypes.size() == 1 && contentTypes[0] == CONTENT_VIDEO) ? dekayVideo : dekayContent;

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, durationBlocksForPrevPosts);

            TryBindStatementInt(stmt, i++, cntPrevPosts);
            
            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementInt(stmt, i++, topHeight);
            TryBindStatementInt(stmt, i++, topHeight - cntBlocksForResult);

            TryBindStatementInt(stmt, i++, badReputationLimit);
            
            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);
            
            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);
            
            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            // ---------------------------------------------
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                HierarchicalRecord record{};

                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                auto[ok1, contentRating] = TryGetColumnInt(*stmt, 1);
                auto[ok2, accountRating] = TryGetColumnInt(*stmt, 2);
                auto[ok3, contentOrigHeight] = TryGetColumnInt(*stmt, 3);
                auto[ok4, contentScores] = TryGetColumnInt(*stmt, 4);

                record.Id = contentId;
                record.LAST5 = 1.0 * contentScores;
                record.UREP = accountRating;
                record.PREP = contentRating;
                record.DREP = pow(dekayRep, (topHeight - contentOrigHeight));
                record.DPOST = pow(dekay, (topHeight - contentOrigHeight));

                postsRanks.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        // ---------------------------------------------
        // Calculate content ratings
        int nElements = postsRanks.size();
        for (auto& iPostRank : postsRanks)
        {
            double _LAST5R = 0;
            double _UREPR = 0;
            double _PREPR = 0;

            double boost = 0;
            if (nElements > 1)
            {
                for (auto jPostRank : postsRanks)
                {
                    if (iPostRank.LAST5 > jPostRank.LAST5)
                        _LAST5R += 1;
                    if (iPostRank.UREP > jPostRank.UREP)
                        _UREPR += 1;
                    if (iPostRank.PREP > jPostRank.PREP)
                        _PREPR += 1;
                }

                iPostRank.LAST5R = 1.0 * (_LAST5R * 100) / (nElements - 1);
                iPostRank.UREPR = min(iPostRank.UREP, 1.0 * (_UREPR * 100) / (nElements - 1)) * (iPostRank.UREP < 0 ? 2.0 : 1.0);
                iPostRank.PREPR = min(iPostRank.PREP, 1.0 * (_PREPR * 100) / (nElements - 1)) * (iPostRank.PREP < 0 ? 2.0 : 1.0);
            }
            else
            {
                iPostRank.LAST5R = 100;
                iPostRank.UREPR = 100;
                iPostRank.PREPR = 100;
            }

            iPostRank.POSTRF = 0.4 * (0.75 * (iPostRank.LAST5R + boost) + 0.25 * iPostRank.UREPR) * iPostRank.DREP + 0.6 * iPostRank.PREPR * iPostRank.DPOST;
        }

        // Sort results
        sort(postsRanks.begin(), postsRanks.end(), greater<HierarchicalRecord>());

        // Build result list
        bool found = false;
        int64_t minPostRank = topContentId;
        vector<int64_t> resultIds;
        for(auto iter = postsRanks.begin(); iter < postsRanks.end() && (int)resultIds.size() < countOut; iter++)
        {
            if (iter->Id < minPostRank || minPostRank == 0)
                minPostRank = iter->Id;

            // Find start position
            if (!found && topContentId > 0)
            {
                if (iter->Id == topContentId)
                    found = true;

                continue;
            }

            // Save id for get data
            resultIds.push_back(iter->Id);
        }

        // Get content data
        if (!resultIds.empty())
        {
            auto contents = GetContentsData(resultIds, address);
            result.push_backV(contents);
        }

        // ---------------------------------------------
        // If not completed - request historical data
        int lack = countOut - (int)resultIds.size();
        if (lack > 0)
        {
            UniValue histContents = GetHistoricalFeed(lack, minPostRank, topHeight, lang, tags, contentTypes,
                txidsExcluded, adrsExcluded, tagsExcluded, address, badReputationLimit);

            result.push_backV(histContents.getValues());
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetBoostFeed(int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& adrsExcluded, const vector<string>& tagsExcluded,
        int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = tc.Hash and p.String1 = ? ";

        string sql = R"sql(
            select
                tc.Id contentId,
                tb.String2 contentHash,
                sum(tb.Int1) as sumBoost

            from Transactions tb indexed by Transactions_Type_Last_String2_Height
            join Transactions tc indexed by Transactions_Type_Last_String2_Height
                on tc.String2 = tb.String2 and tc.Last = 1 and tc.Type in )sql" + contentTypesWhere + R"sql( and tc.Height > 0
                --and tc.String3 is null--?????

            )sql" + langFilter + R"sql(

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = tc.String1

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where tb.Type = 208
                and tb.Last in (0, 1)
                and tb.Height <= ?
                and tb.Height > ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and tc.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and tc.String2 not in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ";
        if (!adrsExcluded.empty()) sql += " and tc.String1 not in ( " + join(vector<string>(adrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and tc.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx indexed by Tags_Lang_Value_Id
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " group by tc.Id, tb.String2";
        sql += " order by sum(tb.Int1) desc";

        // ---------------------------------------------

        vector<int64_t> ids;

        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            if (!lang.empty()) TryBindStatementText(stmt, i++, lang);

            TryBindStatementInt(stmt, i++, topHeight);
            TryBindStatementInt(stmt, i++, topHeight - cntBlocksForResult);

            TryBindStatementInt(stmt, i++, badReputationLimit);

            if (!tags.empty())
            {
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            if (!txidsExcluded.empty())
                for (const auto& extxid: txidsExcluded)
                    TryBindStatementText(stmt, i++, extxid);

            if (!adrsExcluded.empty())
                for (const auto& exadr: adrsExcluded)
                    TryBindStatementText(stmt, i++, exadr);

            if (!tagsExcluded.empty())
            {
                for (const auto& extag: tagsExcluded)
                    TryBindStatementText(stmt, i++, extag);

                if (!lang.empty())
                    TryBindStatementText(stmt, i++, lang);
            }

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, contentId] = TryGetColumnInt64(*stmt, 0);
                auto[ok1, contentHash] = TryGetColumnString(*stmt, 1);
                auto[ok2, sumBoost] = TryGetColumnInt64(*stmt, 2);
                UniValue boost(UniValue::VOBJ);
                boost.pushKV("id", contentId);
                boost.pushKV("txid", contentHash);
                boost.pushKV("boost", sumBoost);
                result.push_back(boost);
            }

            FinalizeSqlStatement(*stmt);
        });

        // Complete!
        return result;
    }

    // TODO (o1q): Remove this method when the client gui switches to new methods
    UniValue WebRpcRepository::GetProfileFeedOld(const string& addressFrom, const string& addressTo, int64_t topContentId,
        int count, const string& lang, const vector<string>& tags, const vector<int>& contentTypes)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressTo.empty())
            return result;

        // ---------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string topContentIdFilter;
        if (topContentId > 0)
            topContentIdFilter = " and t.Id < ? ";

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            where t.Type in ( )sql" + contentTypesFilter + R"sql( )
                and t.Height > 0
                and t.Last = 1
                and t.String1 = ?
                )sql" + topContentIdFilter + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

        sql += " order by t.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementText(stmt, i++, addressTo);

            if (topContentId > 0)
                TryBindStatementInt64(stmt, i++, topContentId);

            if (!tags.empty())
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

            TryBindStatementInt(stmt, i++, count);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, addressFrom);
        result.push_backV(contents);

        return result;
    }

    // TODO (o1q): Remove this method when the client gui switches to new methods
    UniValue WebRpcRepository::GetSubscribesFeedOld(const string& addressFrom, int64_t topContentId, int count,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressFrom.empty())
            return result;

        // ---------------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string topContentIdFilter;
        if (topContentId > 0)
            topContentIdFilter = " and cnt.Id < ? ";

        string sql = R"sql(
            select cnt.Id

            from Transactions cnt indexed by Transactions_Type_Last_String1_Height_Id

            join Transactions subs indexed by Transactions_Type_Last_String1_String2_Height
                on subs.Type in (302,303)
               and subs.Last = 1
               and subs.Height > 0
               and subs.String1 = ?
               and subs.String2 = cnt.String1

            where cnt.Type in ( )sql" + contentTypesFilter + R"sql( )
              and cnt.Last = 1
              and cnt.Height > 0

            )sql" + topContentIdFilter + R"sql(

        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and cnt.id in (
                    select tm.ContentId
                    from web.Tags tag indexed by Tags_Lang_Value_Id
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

        sql += " order by cnt.Id desc ";
        sql += " limit ? ";

        // ---------------------------------------------------

        vector<int64_t> ids;
        TryTransactionStep(func, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, i++, addressFrom);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            if (topContentId > 0)
                TryBindStatementInt(stmt, i++, topContentId);

            if (!tags.empty())
                for (const auto& tag: tags)
                    TryBindStatementText(stmt, i++, tag);

            TryBindStatementInt(stmt, i++, count);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        if (ids.empty())
            return result;

        auto contents = GetContentsData(ids, addressFrom);
        result.push_backV(contents);

        return result;
    }

    // ------------------------------------------------------

    vector<int64_t> WebRpcRepository::GetRandomContentIds(const string& lang, int count, int height)
    {
        vector<int64_t> result;

        auto sql = R"sql(
            select t.Id, t.String1, r.Value, p.String1

            from Transactions t indexed by Transactions_Last_Id_Height

            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type = 100 and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            cross join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Type = 0 and r.Last = 1 and r.Id = u.Id and r.Value > 0

            cross join Payload p indexed by Payload_String1_TxHash
                on p.TxHash = t.Hash and p.String1 = ?

            where t.Last = 1
              and t.Height > 0
              and t.Id in (
                select tr.Id
                from Transactions tr indexed by Transactions_Type_Last_Height_Id
                where tr.Type in (200,201,202)
                  and tr.Last = 1
                  and tr.Height > ?
                order by random()
                limit ?
              )

            order by random()
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, i++, lang);
            TryBindStatementInt(stmt, i++, height);
            TryBindStatementInt(stmt, i++, count * 100);
            TryBindStatementInt(stmt, i++, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok) result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRpcRepository::GetContentActions(const string& postTxHash)
    {
        auto func = __func__;
        UniValue resultScores(UniValue::VARR);
        UniValue resultBoosts(UniValue::VARR);
        UniValue resultDonations(UniValue::VARR);
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            --scores
            select
                s.String2 as ContentTxHash,
                s.String1 as AddressHash,
                p.String2 as AccountName,
                p.String3 as AccountAvatar,
                r.Value as AccountReputation,
                s.Int1 as ScoreValue,
                0 as sumBoost,
                0 as sumDonation
            from Transactions s indexed by Transactions_Type_Last_String2_Height
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on  u.String1 = s.String1 and u.Type in (100) and u.Last = 1 and u.Height > 0
            left join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Id = u.Id and r.Type = 0 and r.Last = 1
            cross join Payload p on p.TxHash = u.Hash
            where s.Type in (300)
              and s.Last in (0,1)
              and s.Height > 0
              and s.String2 = ?

            --boosts
            union

            select
                tb.String2 as ContentTxHash,
                tb.String1 as AddressHash,
                p.String2 as AccountName,
                p.String3 as AccountAvatar,
                r.Value as AccountReputation,
                0 as ScoreValue,
                tb.sumBoost as sumBoost,
                0 as sumDonation
            from
            (
            select
                b.String1,
                b.String2,
                sum(b.Int1) as sumBoost
            from Transactions b indexed by Transactions_Type_Last_String2_Height
            where b.Type in (208)
              and b.Last in (0,1)
              and b.Height > 0
              and b.String2 = ?
            group by
                b.String1,
                b.String2
            )tb
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.String1 = tb.String1 and u.Type in (100) and u.Last = 1 and u.Height > 0
            left join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Id = u.Id and r.Type = 0 and r.Last = 1
            cross join Payload p on p.TxHash = u.Hash

            --donations
            union

            select
                td.String3 as ContentTxHash,
                td.String1 as AddressHash,
                p.String2 as AccountName,
                p.String3 as AccountAvatar,
                r.Value as AccountReputation,
                0 as ScoreValue,
                0 as sumBoost,
                td.sumDonation as sumDonation
            from
            (
            select
                comment.String1,
                comment.String3,
                sum(o.Value) as sumDonation
            from Transactions comment indexed by Transactions_Type_Last_String3_Height
            join Transactions content indexed by Transactions_Type_Last_String2_Height
                on content.String2 = comment.String3
                    and content.Type in (200,201,202)
                    and content.Last = 1
                    and content.Height is not null
            join TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                on o.TxHash = comment.Hash
                    and o.AddressHash = content.String1
                    and o.AddressHash != comment.String1
                    and o.Value > 0
            where comment.Type in (204, 205, 206)
                and comment.Height is not null
                and comment.Last = 1
                and comment.String3 = ?
            group by
                comment.String1,
                comment.String3
            )td
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.String1 = td.String1 and u.Type in (100) and u.Last = 1 and u.Height > 0
            left join Ratings r indexed by Ratings_Type_Id_Last_Value
                on r.Id = u.Id and r.Type = 0 and r.Last = 1
            cross join Payload p on p.TxHash = u.Hash
        )sql";

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, postTxHash);
            TryBindStatementText(stmt, i++, postTxHash);
            TryBindStatementText(stmt, i++, postTxHash);

            // ---------------------------------------------

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) record.pushKV("reputation", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 5); ok && value > 0) {
                    record.pushKV("value", value);
                    resultScores.push_back(record);
                }
                if (auto[ok, value] = TryGetColumnInt(*stmt, 6); ok && value > 0) {
                    record.pushKV("value", value);
                    resultBoosts.push_back(record);
                }
                if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok && value > 0) {
                    record.pushKV("value", value);
                    resultDonations.push_back(record);
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        result.pushKV("scores",resultScores);
        result.pushKV("boosts",resultBoosts);
        result.pushKV("donations",resultDonations);

        return result;
    };

    // Choosing predicate for function above based on filters.
    std::function<bool(const ShortTxType&)> _choosePredicate(const std::set<ShortTxType>& filters) {
        if (filters.empty()) {
            // No filters mean that we should perform all selects
            return [&filters](...) { return true; };
        } else {
            // Perform only selects that are specified in filters.
            return [&filters](const ShortTxType& select) { return filters.find(select) != filters.end(); };
        }
    };

    // Method used to construct sql query and required bindings from provided selects based on filters
    template <class QueryParams>
    static inline auto _constructSelectsBasedOnFilters(
                const std::set<ShortTxType>& filters,
                const std::map<ShortTxType, ShortFormSqlEntry<std::shared_ptr<sqlite3_stmt*>&, QueryParams>>& selects,
                const std::string& footer, const std::string& separator = "union")
    {
        auto predicate = _choosePredicate(filters);

        // Binds that should be performed to constructed query
        std::vector<std::function<void(std::shared_ptr<sqlite3_stmt*>&, int&, QueryParams const&)>> binds;
        // Query elemets that will be used to construct full query
        std::vector<std::string> queryElems;
        for (const auto& select: selects) {
            if (predicate(select.first)) {
                queryElems.emplace_back(select.second.query);
                queryElems.emplace_back(separator);
                binds.emplace_back(select.second.binding);
            }
        }

        if (queryElems.empty()) {
            throw std::runtime_error("Failed to construct query for requested filters");
        }
        queryElems.pop_back(); // Dropping last "union"

        std::stringstream ss;
        for (const auto& elem: queryElems) {
            ss << elem;
        }
        ss << footer;

        // Full query and required binds in correct order
        return std::pair { ss.str(), binds };
    }

    std::vector<ShortForm> WebRpcRepository::GetActivities(const std::string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const std::set<ShortTxType>& filters)
    {
        // This is required because we want static bind functors for optimization so parameters can't be captured there
        struct QueryParams {
            // Handling all by reference
            const std::string& address;
            const int64_t& heightMax;
            const int64_t& heightMin;
            const int64_t& blockNumMax;
        } queryParams{address, heightMax, heightMin, blockNumMax};

        static const std::map<ShortTxType, ShortFormSqlEntry<std::shared_ptr<sqlite3_stmt*>&, QueryParams>> selects = {
        {
            ShortTxType::Answer, { R"sql(
            -- My answers to other's comments
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Answer) + R"sql(')TP,
                a.Hash,
                a.Type,
                null,
                a.Height as Height,
                a.BlockNum as BlockNum,
                a.String2,
                null,
                null,
                null,
                pa.String1,
                a.String4,
                a.String5,
                null,
                null,
                null,
                null,
                null,
                c.Hash,
                c.Type,
                c.String1,
                c.Height,
                c.BlockNum,
                c.String2,
                null,
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'Number', Number,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs i
                    where i.SpentTxHash = c.Hash
                ),
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs o
                    where o.TxHash = c.Hash
                        and o.TxHeight = c.Height
                    order by o.Number
                ),
                pc.String1,
                c.String4,
                c.String5,
                pca.String2,
                pca.String3,
                null, -- Badge
                ifnull(rca.Value,0),
                null

            from Transactions c indexed by Transactions_Type_Last_String2_Height -- My comments

            left join Payload pc
                on pc.TxHash = c.Hash

            join Transactions a indexed by Transactions_Type_String1_Height_Time_Int1 -- Other answers
                on a.Type in (204, 205, 206)
                and a.Height > ?
                and (a.Height < ? or (a.Height = ? and a.BlockNum < ?))
                and a.String5 = c.String2
                and a.String1 != c.String1
                and a.String1 = ?

            left join Payload pa
                on pa.TxHash = a.Hash

            left join Transactions ca indexed by Transactions_Type_Last_String1_Height_Id
                on ca.Type = 100
                and ca.Last = 1
                and ca.String1 = c.String1
                and ca.Height > 0

            left join Payload pca
                on pca.TxHash = ca.Hash

            left join Ratings rca indexed by Ratings_Type_Id_Last_Height
                on rca.Type = 0
                and rca.Id = ca.Id
                and rca.Last = 1

            where c.Type in (204, 205)
              and c.Last = 1
              and c.Height > 0

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams) {
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::Comment, { R"sql(
            -- Comments for my content
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                c.Hash,
                c.Type,
                null,
                c.Height as Height,
                c.BlockNum as BlockNum,
                c.String2,
                null,
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'Number', Number,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs i
                    where i.SpentTxHash = c.Hash
                ),
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs o
                    where o.TxHash = c.Hash
                        and o.TxHeight = c.Height
                    order by o.Number
                ),
                pc.String1,
                c.String4,
                c.String5,
                null,
                null,
                null,
                null,
                null,
                p.Hash,
                p.Type,
                p.String1,
                p.Height,
                p.BlockNum,
                p.String2,
                null,
                null,
                null,
                pp.String2,
                null,
                null,
                pap.String2,
                pap.String3,
                null,
                ifnull(rap.Value, 0),
                null

            from Transactions p indexed by Transactions_Type_Last_String2_Height

            join Transactions c indexed by Transactions_Type_String1_Height_Time_Int1
                on c.Type in (204, 205, 206)
                and c.String3 = p.String2
                and c.String1 != p.String1
                and c.Height > ?
                and (c.Height < ? or (c.Height = ? and c.BlockNum < ?))
                and c.String1 = ?

            left join TxOutputs oc indexed by TxOutputs_TxHash_AddressHash_Value
                on oc.TxHash = c.Hash and oc.AddressHash = p.String1 and oc.AddressHash != c.String1 -- TODO: c.Hash or c.String2 or clast.Hash???

            left join Payload pc
                on pc.TxHash = c.Hash

            left join Payload pp
                on pp.TxHash = p.Hash

            left join Transactions ap indexed by Transactions_Type_Last_String1_Height_Id -- accounts of commentators
                on ap.String1 = p.String1
                and ap.Last = 1
                and ap.Type = 100
                and ap.Height > 0

            left join Payload pap
                on pap.TxHash = ap.Hash

            left join Ratings rap indexed by Ratings_Type_Id_Last_Height
                on rap.Type = 0
                and rap.Id = ap.Id
                and rap.Last = 1

            where p.Type in (200,201,202)
                and p.Last = 1
                and p.Height > 0

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::Subscriber, { R"sql(
            -- Subscribers
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                subs.Hash,
                subs.Type,
                null,
                subs.Height as Height,
                subs.BlockNum as BlockNum,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                u.Hash,
                u.Type,
                u.String1,
                u.Height,
                u.BlockNum,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                pu.String2,
                pu.String3,
                null,
                ifnull(ru.Value,0),
                null

            from Transactions subs indexed by Transactions_Type_String1_Height_Time_Int1

            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100)
                and u.Last = 1
                and u.String1 = subs.String2
                and u.Height > 0

            left join Payload pu
                on pu.TxHash = u.Hash

            left join Ratings ru indexed by Ratings_Type_Id_Last_Height
                on ru.Type = 0
                and ru.Id = u.Id
                and ru.Last = 1

            where subs.Type in (302, 303, 304)
                and subs.String1 = ?
                and subs.Height > ?
                and (subs.Height < ? or (subs.Height = ? and subs.BlockNum < ?))

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
            }
        }},

        {
            ShortTxType::CommentScore, { R"sql(
            -- Comment scores
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                s.Hash,
                s.Type,
                null,
                s.Height as Height,
                s.BlockNum as BlockNum,
                null,
                s.Int1,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                c.Hash,
                c.Type,
                c.String1,
                c.Height,
                c.BlockNum,
                c.String2,
                null,
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'Number', Number,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs i
                    where i.SpentTxHash = c.Hash
                ),
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs o
                    where o.TxHash = c.Hash
                        and o.TxHeight = c.Height
                    order by o.Number
                ),
                pc.String1,
                c.String4,
                c.String5,
                pac.String2,
                pac.String3,
                null,
                ifnull(rac.Value,0),
                null

            from Transactions c indexed by Transactions_Type_Last_String2_Height

            left join Payload pc
                on pc.TxHash = c.Hash

            join Transactions s indexed by Transactions_Type_Last_String1_Height_Id
                on s.Type = 301
                and s.Last = 0
                and s.String2 = c.String2
                and s.Height > ?
                and (s.Height < ? or (s.Height = ? and s.BlockNum < ?))
                and s.String1 = ?

            left join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                on ac.Type = 100
                and ac.Last = 1
                and ac.String1 = c.String1
                and ac.Height > 0

            left join Payload pac
                on pac.TxHash = ac.Hash

            left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                on rac.Type = 0
                and rac.Id = ac.Id
                and rac.Last = 1

            where c.Type in (204,205)
                and c.Last = 1
                and c.Height > 0

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::ContentScore, { R"sql(
            -- Content scores
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql(')TP,
                s.Hash,
                s.Type,
                null,
                s.Height as Height,
                s.BlockNum as BlockNum,
                null,
                s.Int1,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                c.Hash,
                c.Type,
                c.String1,
                c.Height,
                c.BlockNum,
                c.String2,
                null,
                null,
                null,
                pc.String2,
                null,
                null,
                pac.String2,
                pac.String3,
                null,
                ifnull(rac.Value,0),
                null

            from Transactions c indexed by Transactions_Type_Last_String2_Height

            left join Payload pc
                on pc.TxHash = c.Hash

            join Transactions s indexed by Transactions_Type_Last_String1_Height_Id
                on s.Type = 300
                and s.Last = 0
                and s.String2 = c.String2
                and s.Height > ?
                and (s.Height < ? or (s.Height = ? and s.BlockNum < ?))
                and s.String1 = ?

            left join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                on ac.Type = 100
                and ac.Last = 1
                and ac.String1 = c.String1
                and ac.Height > 0

            left join Payload pac
                on pac.TxHash = ac.Hash

            left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                on rac.Type = 0
                and rac.Id = ac.Id
                and rac.Last = 1

            where c.Type in (200, 201, 202)
                and c.Last = 1
                and c.Height > 0

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::Boost, { R"sql(
            -- Boosts for my content
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Boost) + R"sql(')TP,
                tBoost.Hash,
                tboost.Type,
                null,
                tBoost.Height as Height,
                tBoost.BlockNum as BlockNum,
                null,
                null,
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'Number', Number,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs i
                    where i.SpentTxHash = tBoost.Hash
                ),
                (
                    select json_group_array(json_object(
                            'Value', Value,
                            'AddressHash', AddressHash,
                            'ScriptPubKey', ScriptPubKey
                            ))
                    from TxOutputs o
                    where o.TxHash = tBoost.Hash
                        and o.TxHeight = tBoost.Height
                    order by o.Number
                ),
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                tContent.Hash,
                tContent.Type,
                tContent.String1,
                tContent.Height,
                tContent.BlockNum,
                tContent.String2,
                null,
                null,
                null,
                pContent.String2,
                null,
                null,
                pac.String2,
                pac.String3,
                null,
                ifnull(rac.Value,0),
                null

            from Transactions tBoost indexed by Transactions_Type_Last_String1_Height_Id

            join Transactions tContent indexed by Transactions_Type_Last_String2_Height
                on tContent.Type in (200,201,202)
                and tContent.Last in (0,1)
                and tContent.Height > 0
                and tContent.String2 = tBoost.String2

            left join Payload pContent
                on pContent.TxHash = tContent.Hash
            
            left join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                on ac.String1 = tContent.String1
                and ac.Type = 100
                and ac.Last = 1
                and ac.Height > 0

            left join Payload pac
                on pac.TxHash = ac.Hash

            left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                on rac.Type = 0
                and rac.Id = ac.Id
                and rac.Last = 1

            where tBoost.Type in (208)
                and tBoost.Last in (0,1)
                and tBoost.String1 = ?
                and tBoost.Height > ?
                and (tBoost.Height < ? or (tBoost.Height = ? and tBoost.BlockNum < ?))

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
            }
        }},

        {
            ShortTxType::Blocking, { R"sql(
            -- My blockings and unblockings
            select
                ('blocking')TP,
                b.Hash,
                b.Type,
                ac.String1,
                b.Height as Height,
                b.BlockNum as BlockNum,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                pac.String2,
                pac.String3,
                null,
                ifnull(rac.Value,0),
                (
                    select json_group_array(
                        json_object(
                            'address', mac.String1,
                            'account', json_object(
                                'name', pmac.String2,
                                'avatar', pmac.String3,
                                'reputation', ifnull(rmac.Value,0)
                            )
                        )
                    )
                    from Transactions mac indexed by Transactions_Type_Last_String1_Height_Id

                    left join Payload pmac
                        on pmac.TxHash = mac.Hash

                    left join Ratings rmac indexed by Ratings_Type_Id_Last_Height
                        on rmac.Type = 0
                        and rmac.Id = mac.Id
                        and rmac.Last = 1

                    where mac.String1 in (select value from json_each(b.String3))
                        and mac.Type = 100
                        and mac.Last = 1
                        and mac.Height > 0
                ),
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null

            from Transactions b indexed by Transactions_Type_String1_Height_Time_Int1

            left join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                on ac.String1 = b.String2
                and ac.Type = 100
                and ac.Last = 1
                and ac.Height > 0

            left join Payload pac
                on pac.TxHash = ac.Hash

            left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                on rac.Type = 0
                and rac.Id = ac.Id
                and rac.Last = 1

            where b.Type in (305,306)
                and b.String1 = ?
                and b.Height > ?
                and (b.Height < ? or (b.Height = ? and b.BlockNum < ?))

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
            }
        }},
        };

        static const auto footer = R"sql(

            -- Global order and limit for pagination
            order by Height desc, BlockNum desc
            limit 10

        )sql";

        auto [elem1, elem2] = _constructSelectsBasedOnFilters(filters, selects, footer);
        auto& sql = elem1;
        auto& binds = elem2;

        EventsReconstructor reconstructor;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            for (const auto& bind: binds) {
                bind(stmt, i, queryParams);
            }

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                reconstructor.FeedRow(*stmt);
            }

            FinalizeSqlStatement(*stmt);
        });

        return reconstructor.GetResult();
    }

    std::map<std::string, ShortAccount> WebRpcRepository::GetShortAccountsForAddresses(const std::set<std::string>& addresses)
    {
        auto sql = R"sql(
            select
                ac.String1,
                p.String1,
                p.String2,
                p.String3,
                ifnull(r.Value,0)

            from Transactions ac

            left join Payload p
                on p.TxHash = ac.Hash

            left join Ratings r indexed by Ratings_Type_Id_Last_Height
                on r.Type = 0
                and r.Id = ac.Id
                and r.Last = 1

            where ac.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                and ac.Type = 100
                and ac.Last = 1
        )sql";

        std::map<std::string, ShortAccount> res;        
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            for (const auto& address: addresses) {
                TryBindStatementText(stmt, i, address);
                i++;
            }

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                std::string address;
                if (auto [ok, val] = TryGetColumnString(*stmt, 0); ok)
                    address = val;
                else 
                    continue; // TODO (losty): error

                ShortAccount acc;
                if (auto [ok, val] = TryGetColumnString(*stmt, 1); ok) acc.SetLang(val);
                if (auto [ok, val] = TryGetColumnString(*stmt, 2); ok) acc.SetName(val);
                if (auto [ok, val] = TryGetColumnString(*stmt, 3); ok) acc.SetAvatar(val);
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 4); ok) acc.SetReputation(val);
                res.insert({address, acc});
            }

            FinalizeSqlStatement(*stmt);
        });

        return res;
    }

    UniValue WebRpcRepository::GetNotifications(int64_t height, const std::set<ShortTxType>& filters)
    {
        struct QueryParams {
            // Handling all by reference
            const int64_t& height;
        } queryParams {height};

        // Static because it will not be changed for entire node run

        static const auto heightBinder =
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.height);
            };

        static const std::map<ShortTxType, ShortFormSqlEntry<std::shared_ptr<sqlite3_stmt*>&, QueryParams>> selects = {
        {
            ShortTxType::Money, { R"sql(
                -- Incoming money
                select
                    null, -- Will be filled 
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Money) + R"sql(')TP,
                    t.Hash,
                    t.Type,
                    null,
                    t.Height as Height,
                    t.BlockNum as BlockNum,
                    null,
                    null,
                    (
                        select json_group_array(json_object(
                                'Value', Value,
                                'Number', Number,
                                'AddressHash', AddressHash,
                                'ScriptPubKey', ScriptPubKey
                                ))
                        from TxOutputs i
                        where i.SpentTxHash = t.Hash
                    ),
                    (
                        select json_group_array(json_object(
                                'Value', Value,
                                'AddressHash', AddressHash,
                                'ScriptPubKey', ScriptPubKey
                                ))
                        from TxOutputs o
                        where o.TxHash = t.Hash
                            and o.TxHeight = t.Height
                        order by o.Number
                    )

                from Transactions t indexed by Transactions_Height_Type

                where t.Type in (1,2,3) -- 1 default money transfer, 2 coinbase, 3 coinstake
                    and t.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::Referal, { R"sql(
                -- referals
                select
                    t.String2,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Referal) + R"sql(')TP,
                    t.Hash,
                    t.Type,
                    t.String1,
                    t.Height as Height,
                    t.BlockNum as BlockNum,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    p.String2,
                    p.String3,
                    null,
                    ifnull(r.Value,0) -- TODO (losty): do we need rating if referal is always a new user?

                from Transactions t indexed by Transactions_Height_Type

                left join Payload p
                    on p.TxHash = t.Hash

                left join Ratings r indexed by Ratings_Type_Id_Last_Height
                    on r.Type = 0
                    and r.Id = t.Id
                    and r.Last = 1

                where t.Type = 100
                    and t.String2 is not null
                    and t.Height = ?
                    and (select count(*) from Transactions tt indexed by Transactions_Id_Last where tt.Id = t.Id) = 1 -- Only original
        )sql",
            heightBinder
        }},

        {
            ShortTxType::Answer, { R"sql(
                -- Comment answers
                select
                    c.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Answer) + R"sql(')TP,
                    a.Hash,
                    a.Type,
                    a.String1,
                    a.Height as Height,
                    a.BlockNum as BlockNum,
                    a.String2,
                    null,
                    null,
                    null,
                    pa.String1,
                    a.String4,
                    a.String5,
                    paa.String2,
                    paa.String3,
                    null,
                    ifnull(ra.Value,0),
                    null,
                    post.Hash,
                    post.Type,
                    post.String1,
                    post.Height,
                    post.BlockNum,
                    post.String2,
                    null,
                    null,
                    null,
                    null,
                    null,
                    ppost.String2,
                    papost.String2,
                    papost.String3,
                    null,
                    ifnull(rapost.Value,0)

                from Transactions a indexed by Transactions_Height_Type -- Other answers

                join Transactions c indexed by Transactions_Type_Last_String2_Height -- My comments
                    on c.Type in (204, 205)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String2 = a.String5
                    and c.String1 != a.String1
                    
                left join Transactions post indexed by Transactions_Type_Last_String2_Height
                    on post.Type in (200, 201, 202)
                    and post.Last = 1
                    and post.String2 = a.String3

                left join Payload ppost
                    on ppost.TxHash = post.Hash

                left join Transactions apost indexed by Transactions_Type_Last_String1_String2_Height
                    on apost.Type = 100
                    and apost.Last = 1
                    and apost.String1 = post.String1
                
                left join Payload papost
                    on papost.TxHash = apost.Hash
                
                left join Ratings rapost indexed by Ratings_Type_Id_Last_Height
                    on rapost.Type = 0
                    and rapost.Id = apost.Id
                    and rapost.Last = 1

                left join Payload pa
                    on pa.TxHash = a.Hash

                left join Transactions aa indexed by Transactions_Type_Last_String1_Height_Id
                    on aa.Type = 100
                    and aa.Last = 1
                    and aa.String1 = a.String1
                    and aa.Height > 0

                left join Payload paa
                    on paa.TxHash = aa.Hash

                left join Ratings ra indexed by Ratings_Type_Id_Last_Height
                    on ra.Type = 0
                    and ra.Id = aa.Id
                    and ra.Last = 1

                where a.Type = 204 -- only orig
                    and a.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::Comment, { R"sql(
                -- Comments for my content
                select
                    p.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                    c.Hash,
                    c.Type,
                    c.String1,
                    c.Height as Height,
                    c.BlockNum as BlockNum,
                    c.String2,
                    null,
                    (
                        select json_group_array(json_object(
                                'Value', Value,
                                'Number', Number,
                                'AddressHash', AddressHash,
                                'ScriptPubKey', ScriptPubKey
                                ))
                        from TxOutputs i
                        where i.SpentTxHash = c.Hash
                    ),
                    (
                        select json_group_array(json_object(
                                'Value', Value,
                                'AddressHash', AddressHash,
                                'ScriptPubKey', ScriptPubKey
                                ))
                        from TxOutputs o
                        where o.TxHash = c.Hash
                            and o.TxHeight = c.Height
                        order by o.Number
                    ),
                    pc.String1,
                    null,
                    null,
                    pac.String2,
                    pac.String3,
                    null,
                    ifnull(rac.Value,0),
                    null,
                    p.Hash,
                    p.Type,
                    null,
                    p.Height,
                    p.BlockNum,
                    p.String2,
                    null,
                    null,
                    null,
                    pp.String2

                from Transactions c indexed by Transactions_Height_Type

                join Transactions p indexed by Transactions_Type_Last_String2_Height
                    on p.Type in (200,201,202)
                    and p.Last = 1
                    and p.Height > 0
                    and p.String2 = c.String3
                    and p.String1 != c.String1

                left join Payload pc
                    on pC.TxHash = c.Hash

                left join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                    on ac.String1 = c.String1
                    and ac.Last = 1
                    and ac.Type = 100
                    and ac.Height > 0

                left join Payload pac
                    on pac.TxHash = ac.Hash

                left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                    on rac.Type = 0
                    and rac.Id = ac.Id
                    and rac.Last = 1
                
                left join Payload pp
                    on pp.TxHash = p.Hash

                where c.Type = 204 -- only orig
                    and c.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::Subscriber, { R"sql(
                -- Subscribers
                select
                    subs.String2,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                    subs.Hash,
                    subs.Type,
                    subs.String1,
                    subs.Height as Height,
                    subs.BlockNum as BlockNum,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    pu.String2,
                    pu.String3,
                    null,
                    ifnull(ru.Value,0)

                from Transactions subs indexed by Transactions_Height_Type

                left join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                    on u.Type in (100)
                    and u.Last = 1
                    and u.String1 = subs.String1
                    and u.Height > 0

                left join Payload pu
                    on pu.TxHash = u.Hash

                left join Ratings ru indexed by Ratings_Type_Id_Last_Height
                    on ru.Type = 0
                    and ru.Id = u.Id
                    and ru.Last = 1

                where subs.Type in (302, 303) -- Ignoring unsubscribers?
                    and subs.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::CommentScore, { R"sql(
                -- Comment scores
                select
                    c.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                    s.Hash,
                    s.Type,
                    s.String1,
                    s.Height as Height,
                    s.BlockNum as BlockNum,
                    null,
                    s.Int1,
                    null,
                    null,
                    null,
                    null,
                    null,
                    pacs.String2,
                    pacs.String3,
                    null,
                    ifnull(racs.Value,0),
                    null,
                    c.Hash,
                    c.Type,
                    null,
                    c.Height, -- TODO (losty): original?
                    c.BlockNum,
                    c.String2,
                    null,
                    null,
                    null,
                    ps.String1,
                    c.String4,
                    c.String5

                from Transactions s indexed by Transactions_Height_Type

                join Transactions c indexed by Transactions_Type_Last_String2_Height
                    on c.Type in (204,205)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String2 = s.String2

                left join Payload ps
                    on ps.TxHash = c.Hash

                join Transactions acs indexed by Transactions_Type_Last_String1_Height_Id
                    on acs.Type = 100
                    and acs.Last = 1
                    and acs.String1 = s.String1
                    and acs.Height > 0

                left join Payload pacs
                    on pacs.TxHash = acs.Hash

                left join Ratings racs indexed by Ratings_Type_Id_Last_Height
                    on racs.Type = 0
                    and racs.Id = acs.Id
                    and racs.Last = 1

                where s.Type = 301
                    and s.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::ContentScore, { R"sql(
                -- Content scores
                select
                    c.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql(')TP,
                    s.Hash,
                    s.Type,
                    s.String1,
                    s.Height as Height,
                    s.BlockNum as BlockNum,
                    null,
                    s.Int1,
                    null,
                    null,
                    null,
                    null,
                    null,
                    pacs.String2,
                    pacs.String3,
                    null,
                    ifnull(racs.Value,0),
                    null,
                    c.Hash,
                    c.Type,
                    null,
                    c.Height, -- TODO (losty): original?
                    c.BlockNum,
                    c.String2,
                    null,
                    null,
                    null,
                    ps.String2

                from Transactions s indexed by Transactions_Height_Type

                join Transactions c indexed by Transactions_Type_Last_String2_Height
                    on c.Type in (200, 201, 202)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String2 = s.String2

                left join Payload ps
                    on ps.TxHash = c.Hash

                join Transactions acs indexed by Transactions_Type_Last_String1_Height_Id
                    on acs.Type = 100
                    and acs.Last = 1
                    and acs.String1 = s.String1
                    and acs.Height > 0

                left join Payload pacs
                    on pacs.TxHash = acs.Hash

                left join Ratings racs indexed by Ratings_Type_Id_Last_Height
                    on racs.Type = 0
                    and racs.Id = acs.Id
                    and racs.Last = 1

                where s.Type = 300
                    and s.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::PrivateContent, { R"sql(
                -- Content from private subscribers
                select
                    subs.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::PrivateContent) + R"sql(')TP,
                    c.Hash,
                    c.Type,
                    c.String1,
                    c.Height as Height,
                    c.BlockNum as BlockNum,
                    c.String2,
                    null,
                    null,
                    null,
                    p.String2,
                    null,
                    null,
                    pac.String2,
                    pac.String3,
                    null,
                    ifnull(rac.Value,0),
                    null,
                    r.Hash,
                    r.Type,
                    r.String1,
                    r.Height,
                    r.BlockNum,
                    r.String2,
                    null,
                    null,
                    null,
                    pr.String2

                from Transactions c indexed by Transactions_Height_Type -- content for private subscribers

                join Transactions subs indexed by Transactions_Type_Last_String2_Height -- Subscribers private
                    on subs.Type = 303
                    and subs.Last = 1
                    and subs.String2 = c.String1
                    and subs.Height > 0

                left join Transactions r -- related content - possible reposts
                    on r.String2 = c.String3
                    and r.Type in (200,201,202)
                    and r.Last = 1

                left join Payload pr
                    on pr.TxHash = r.Hash

                cross join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                    on ac.Type = 100
                    and ac.Last = 1
                    and ac.String1 = c.String1
                    and ac.Height > 0

                cross join Payload p
                    on p.TxHash = c.Hash

                cross join Payload pac
                    on pac.TxHash = ac.Hash

                left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                    on rac.Type = 0
                    and rac.Id = ac.Id
                    and rac.Last = 1

                where c.Type in (200,201,202)
                    and c.Hash = c.String2 -- only orig
                    and c.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::Boost, { R"sql(
                -- Boosts for my content
                select
                    tContent.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Boost) + R"sql(')TP,
                    tBoost.Hash,
                    tboost.Type,
                    tBoost.String1,
                    tBoost.Height as Height,
                    tBoost.BlockNum as BlockNum,
                    tBoost.String2,
                    null,
                    (
                        select json_group_array(json_object(
                                'Value', Value,
                                'Number', Number,
                                'AddressHash', AddressHash,
                                'ScriptPubKey', ScriptPubKey
                                ))
                        from TxOutputs i
                        where i.SpentTxHash = tBoost.Hash
                    ),
                    (
                        select json_group_array(json_object(
                                'Value', Value,
                                'AddressHash', AddressHash,
                                'ScriptPubKey', ScriptPubKey
                                ))
                        from TxOutputs o
                        where o.TxHash = tBoost.Hash
                            and o.TxHeight = tBoost.Height
                        order by o.Number
                    ),
                    null,
                    null,
                    null,
                    pac.String2,
                    pac.String3,
                    null,
                    ifnull(rac.Value,0),
                    null,
                    tContent.Hash,
                    tContent.Type,
                    null,
                    tContent.Height,
                    tContent.BlockNum,
                    tContent.String2,
                    null,
                    null,
                    null,
                    pContent.String2

                from Transactions tBoost indexed by Transactions_Type_Last_Height_Id

                left join Transactions tContent indexed by Transactions_Type_Last_String2_Height
                    on tContent.Type in (200,201,202)
                    and tContent.Last in (0,1)
                    and tContent.Height > 0
                    and tContent.String2 = tBoost.String2

                left join Payload pContent
                    on pContent.TxHash = tContent.Hash

                left join Transactions ac indexed by Transactions_Type_Last_String1_Height_Id
                    on ac.String1 = tBoost.String1
                    and ac.Type = 100
                    and ac.Last = 1
                    and ac.Height > 0

                left join Payload pac
                    on pac.TxHash = ac.Hash

                left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                    on rac.Type = 0
                    and rac.Id = ac.Id
                    and rac.Last = 1

                where tBoost.Type in (208)
                    and tBoost.Last in (0,1)
                    and tBoost.Height = ?
        )sql",
            heightBinder
        }},

        {
            ShortTxType::Repost, { R"sql(
                -- Reposts
                select
                    p.String1,
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Repost) + R"sql(')TP,
                    r.Hash,
                    r.Type,
                    r.String1,
                    r.Height as Height,
                    r.BlockNum as BlockNum,
                    r.String2,
                    null,
                    null,
                    null,
                    pr.String2,
                    null,
                    null,
                    par.String2,
                    par.String3,
                    null,
                    ifnull(rar.Value,0),
                    null,
                    p.Hash,
                    p.Type,
                    null,
                    p.Height,
                    p.BlockNum,
                    p.String2,
                    null,
                    null,
                    null,
                    pp.String2

                from Transactions r

                join Transactions p indexed by Transactions_Type_Last_String2_Height
                    on p.Type in (200,201,202)
                    and p.Last = 1
                    and p.String2 = r.String3
                    and p.Height > 0

                left join Payload pp
                    on pp.TxHash = p.Hash

                left join Payload pr
                    on pr.TxHash = r.Hash

                join Transactions ar indexed by Transactions_Type_Last_String1_Height_Id
                    on ar.Type = 100
                    and ar.Last = 1
                    and ar.String1 = r.String1
                    and ar.Height > 0

                left join Payload par
                    on par.TxHash = ar.Hash

                left join Ratings rar indexed by Ratings_Type_Id_Last_Height
                    on rar.Type = 0
                    and rar.Id = ar.Id
                    and rar.Last = 1

                where r.Type in (200,201,202)
                    and r.Hash = r.String2 -- Only orig
                    and r.Height = ?
                    and r.String3 is not null
            )sql",
            heightBinder
        }}
        };

        auto predicate = _choosePredicate(filters);
        
        NotificationsReconstructor reconstructor;
        for(const auto& select: selects) {
            if (predicate(select.first)) {
                const auto& selectData = select.second;
                TryTransactionStep(__func__, [&]()
                {
                    auto stmt = SetupSqlStatement(selectData.query);

                    int i = 1;
                    selectData.binding(stmt, i, queryParams);

                    while (sqlite3_step(*stmt) == SQLITE_ROW)
                        reconstructor.FeedRow(*stmt);

                    FinalizeSqlStatement(*stmt);
                });
            }
        }

        auto notificationResult = reconstructor.GetResult();
        auto accsData = GetShortAccountsForAddresses(notificationResult.GetNotifiersAddresses());
        return notificationResult.Serialize(accsData);
    }

    std::vector<ShortForm> WebRpcRepository::GetEventsForAddresses(const std::string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const std::set<ShortTxType>& filters)
    {
        // This is required because we want static bind functors for optimization so parameters can't be captured there 
        struct QueryParams {
            // Handling all by reference
            const std::string& address;
            const int64_t& heightMax;
            const int64_t& heightMin;
            const int64_t& blockNumMax;
        } queryParams {address, heightMax, heightMin, blockNumMax};

        static const std::map<ShortTxType, ShortFormSqlEntry<std::shared_ptr<sqlite3_stmt*>&, QueryParams>> selects = {
        {
            ShortTxType::Money, { R"sql(
                -- Incoming money
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Money) + R"sql(')TP,
                    t.Hash,
                    t.Type,
                    i.AddressHash,
                    t.Height as Height,
                    t.BlockNum as BlockNum,
                    o.Value,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null

                from TxOutputs o indexed by TxOutputs_AddressHash_TxHeight_TxHash

                join Transactions t indexed by Transactions_Hash_Type_Height
                    on t.Hash = o.TxHash
                    and t.Type in (1,2,3) -- 1 default money transfer, 2 coinbase, 3 coinstake
                    and t.Height > ?
                    and (t.Height < ? or (t.Height = ? and t.BlockNum < ?))

                join TxOutputs i indexed by TxOutputs_SpentTxHash
                    on i.SpentTxHash = o.TxHash
                    and i.Number = (select min(ii.Number) from TxOutputs ii where ii.SpentTxHash = o.TxHash)
                    and i.AddressHash != o.AddressHash  -- TODO (brangr, lostystyg): exclude coinstake first transaction

                where o.AddressHash = ?
                    and o.TxHeight > ?
                    and o.TxHeight < ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::Referal, { R"sql(
                -- referals
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Referal) + R"sql(')TP,
                    t.Hash,
                    t.Type,
                    t.String1,
                    t.Height as Height,
                    t.BlockNum as BlockNum,
                    null,
                    null,
                    p.String2,
                    p.String3,
                    p.String4,
                    ifnull(r.Value,0),
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null

                from Transactions t indexed by Transactions_Type_Last_String2_Height

                left join Payload p
                    on p.TxHash = t.Hash

                left join Ratings r indexed by Ratings_Type_Id_Last_Height
                    on r.Type = 0
                    and r.Id = t.Id
                    and r.Last = 1

                where t.Type = 100
                    and t.Last in (0,1)
                    and t.String2 = ?
                    and t.Height > ?
                    and (t.Height < ? or (t.Height = ? and t.BlockNum < ?))
                    and t.ROWID = (select min(tt.ROWID) from Transactions tt where tt.Id = t.Id)
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
            }
        }},

        {
            ShortTxType::Answer, { R"sql(
                -- Comment answers
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Answer) + R"sql(')TP,
                    a.Hash,
                    a.Type,
                    a.String1,
                    orig.Height as Height,
                    a.BlockNum as BlockNum,
                    null,
                    pa.String1,
                    paa.String2,
                    paa.String3,
                    paa.String4,
                    ifnull(ra.Value,0),
                    c.Hash,
                    c.Type,
                    null,
                    c.Height,
                    c.BlockNum,
                    null,
                    pc.String1,
                    null,
                    null,
                    null,
                    null

                from Transactions c indexed by Transactions_Type_Last_String1_String2_Height -- My comments

                left join Payload pc
                    on pc.TxHash = c.Hash

                join Transactions a indexed by Transactions_Type_Last_String5_Height -- Other answers
                    on a.Type in (204, 205) and a.Last = 1
                    and a.Height > ?
                    and (a.Height < ? or (a.Height = ? and a.BlockNum < ?))
                    and a.String5 = c.String2
                    and a.String1 != c.String1

                join Transactions orig indexed by Transactions_Hash_Height
                    on orig.Hash = a.String2

                left join Payload pa
                    on pa.TxHash = a.Hash

                left join Transactions aa
                    on aa.Type = 100
                    and aa.Last = 1
                    and aa.String1 = a.String1
                    and aa.Height > 0

                left join Payload paa
                    on paa.TxHash = aa.Hash

                left join Ratings ra indexed by Ratings_Type_Id_Last_Height
                    on ra.Type = 0
                    and ra.Id = aa.Id
                    and ra.Last = 1

                where c.Type in (204, 205)
                and c.Last = 1
                and c.String1 = ?
                and c.Height > 0
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::Comment, { R"sql(
                -- Comments for my content
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                    c.Hash,
                    c.Type,
                    c.String1,
                    c.Height as Height,
                    c.BlockNum as BlockNum,
                    oc.Value,
                    pc.String1,
                    pac.String2,
                    pac.String3,
                    pac.String4,
                    ifnull(rac.Value,0),
                    p.Hash,
                    p.Type,
                    null,
                    p.Height,
                    p.BlockNum,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null

                from Transactions p indexed by Transactions_String1_Last_Height

                join Transactions c indexed by Transactions_Type_Last_String3_Height
                    on c.Type in (204,205)
                    and c.Last = 1
                    and c.String3 = p.String2
                    and c.String1 != p.String1
                    and c.Hash = c.String2
                    and c.Height > ?
                    and (c.Height < ? or (c.Height = ? and c.BlockNum < ?))

                left join TxOutputs oc indexed by TxOutputs_TxHash_AddressHash_Value
                    on oc.TxHash = c.Hash and oc.AddressHash = p.String1 and oc.AddressHash != c.String1 

                left join Payload pc
                    on pC.TxHash = c.Hash

                join Transactions ac -- accounts of commentators
                    on ac.String1 = c.String1
                    and ac.Last = 1
                    and ac.Type = 100
                    and ac.Height > 0

                left join Payload pac
                    on pac.TxHash = ac.Hash

                left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                    on rac.Type = 0
                    and rac.Id = ac.Id
                    and rac.Last = 1

                where p.Type in (200,201,202)
                    and p.Last = 1
                    and p.Height > 0
                    and p.String1 = ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::Subscriber, { R"sql(
                -- Subscribers
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                    subs.Hash,
                    subs.Type,
                    subs.String1,
                    subs.Height as Height,
                    subs.BlockNum as BlockNum,
                    null,
                    null,
                    pu.String2,
                    pu.String3,
                    pu.String4,
                    ifnull(ru.Value,0),
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null

                from Transactions subs --indexed by Transactions_Type_Last_String2_Height

                join Transactions u --indexed by Transactions_Type_Last_String1_Height_Id
                    on u.Type in (100)
                    and u.Last = 1
                    and u.String1 = subs.String1
                    and u.Height > 0

                left join Payload pu
                    on pu.TxHash = u.Hash

                left join Ratings ru indexed by Ratings_Type_Id_Last_Height
                    on ru.Type = 0
                    and ru.Id = u.Id
                    and ru.Last = 1

                where subs.Type in (302, 303) -- Ignoring unsubscribers?
                    and subs.Last = 1
                    and subs.String2 = ?
                    and subs.Height > ?
                    and (subs.Height < ? or (subs.Height = ? and subs.BlockNum < ?))
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
            }
        }},

        {
            ShortTxType::CommentScore, { R"sql(
                -- Comment scores
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                    s.Hash,
                    s.Type,
                    s.String1,
                    s.Height as Height,
                    s.BlockNum as BlockNum,
                    s.Int1,
                    null,
                    pacs.String2,
                    pacs.String3,
                    pacs.String4,
                    ifnull(racs.Value,0),
                    c.Hash,
                    c.Type,
                    null,
                    c.Height, -- TODO (losty): original?
                    c.BlockNum,
                    null,
                    ps.String1,
                    null,
                    null,
                    null,
                    null

                from Transactions c indexed by Transactions_Type_Last_String1_Height_Id

                join Transactions s indexed by Transactions_Type_Last_String2_Height
                    on s.Type = 301
                    and s.Last = 0
                    and s.String2 = c.String2
                    and s.Height > ?
                    and (s.Height < ? or (s.Height = ? and s.BlockNum < ?))

                left join Payload ps
                    on ps.TxHash = c.Hash

                join Transactions acs
                    on acs.Type = 100
                    and acs.Last = 1
                    and acs.String1 = s.String1
                    and acs.Height > 0

                left join Payload pacs
                    on pacs.TxHash = acs.Hash

                left join Ratings racs indexed by Ratings_Type_Id_Last_Height
                    on racs.Type = 0
                    and racs.Id = acs.Id
                    and racs.Last = 1

                where c.Type in (204,205)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String1 = ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::ContentScore, { R"sql(
                -- Content scores
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql(')TP,
                    s.Hash,
                    s.Type,
                    s.String1,
                    s.Height as Height,
                    s.BlockNum as BlockNum,
                    s.Int1,
                    null,
                    pacs.String2,
                    pacs.String3,
                    pacs.String4,
                    ifnull(racs.Value,0),
                    c.Hash,
                    c.Type,
                    null,
                    c.Height, -- TODO (losty): original?
                    c.BlockNum,
                    null,
                    pc.String2,
                    null,
                    null,
                    null,
                    null

                from Transactions c indexed by Transactions_Type_Last_String1_Height_Id

                join Transactions s indexed by Transactions_Type_Last_String2_Height
                    on s.Type = 300
                    and s.Last = 0
                    and s.String2 = c.String2
                    and s.Height > ?
                    and (s.Height < ? or (s.Height = ? and s.BlockNum < ?))

                left join Payload pc
                    on pc.TxHash = c.Hash

                join Transactions acs
                    on acs.Type = 100
                    and acs.Last = 1
                    and acs.String1 = s.String1
                    and acs.Height > 0

                left join Payload pacs
                    on pacs.TxHash = acs.Hash

                left join Ratings racs indexed by Ratings_Type_Id_Last_Height
                    on racs.Type = 0
                    and racs.Id = acs.Id
                    and racs.Last = 1

                where c.Type in (200, 201, 202)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String1 = ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::PrivateContent, { R"sql(
                -- Content from private subscribers
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::PrivateContent) + R"sql(')TP,
                    c.Hash,
                    c.Type,
                    c.String1,
                    c.Height as Height,
                    c.BlockNum as BlockNum,
                    null,
                    p.String2,
                    pac.String2,
                    pac.String3,
                    pac.String4,
                    ifnull(rac.Value,0),
                    null, -- TODO (losty): probably reposts here?
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null,
                    null

                from Transactions subs indexed by Transactions_Type_Last_String1_Height_Id -- Subscribers private

                join Transactions c indexed by Transactions_Type_Last_String1_Height_Id -- content for private subscribers
                    on c.Type in (200,201,202)
                    and c.Last = 1 -- TODO (losty): last = 1 and c.Hash = c.String2 ?????
                    and c.String1 = subs.String2
                    -- and c.Hash = c.String2 --  TODO (losty): Only first content record
                    and c.Height > ?
                    and (c.Height < ? or (c.Height = ? and c.BlockNum < ?))

                left join Payload p
                    on p.TxHash = c.Hash
                
                join Transactions ac
                    on ac.Type = 100
                    and ac.Last = 1
                    and ac.String1 = c.String1
                    and ac.Height > 0

                left join Payload pac
                    on pac.TxHash = ac.Hash

                left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                    on rac.Type = 0
                    and rac.Id = ac.Id
                    and rac.Last = 1
                    
                where subs.Type = 303
                    and subs.Last = 1
                    and subs.Height > 0
                    and subs.String1 = ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }},

        {
            ShortTxType::Boost, { R"sql(
                -- Boosts for my content
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Boost) + R"sql(')TP,
                    tBoost.Hash,
                    tboost.Type,
                    tBoost.String1,
                    tBoost.Height as Height,
                    tBoost.BlockNum as BlockNum,
                    tBoost.Int1,
                    null,
                    pac.String2,
                    pac.String3,
                    pac.String4,
                    ifnull(rac.Value,0),
                    tContent.Hash,
                    tContent.Type,
                    null,
                    tContent.Height,
                    tContent.BlockNum,
                    null,
                    pContent.String2,
                    null,
                    null,
                    null,
                    null

                from Transactions tBoost indexed by Transactions_Type_Last_Height_Id

                join Transactions tContent indexed by Transactions_Type_Last_String1_String2_Height
                    on tContent.Type in (200,201,202)
                    and tContent.Last in (0,1)
                    and tContent.Height > 0
                    and tContent.String1 = ?
                    and tContent.String2 = tBoost.String2
                left join Payload pContent
                    on pContent.TxHash = tContent.Hash
                
                join Transactions ac
                    on ac.String1 = tBoost.String1
                    and ac.Type = 100
                    and ac.Last = 1
                    and ac.Height > 0

                left join Payload pac
                    on pac.TxHash = ac.Hash

                left join Ratings rac indexed by Ratings_Type_Id_Last_Height
                    on rac.Type = 0
                    and rac.Id = ac.Id
                    and rac.Last = 1

                where tBoost.Type in (208)
                    and tBoost.Last in (0,1)
                    and tBoost.Height > ?
                    and (tBoost.Height < ? or (tBoost.Height = ? and tBoost.BlockNum < ?))
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementText(stmt, i++, queryParams.address);
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
            }
        }},

        {
            ShortTxType::Repost, { R"sql(
                -- Reposts
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Repost) + R"sql(')TP,
                    r.Hash,
                    r.Type,
                    r.String1,
                    r.Height as Height, -- TODO (losty): orig height maybe???
                    r.BlockNum as BlockNum,
                    null,
                    pr.String2,
                    par.String2,
                    par.String3,
                    par.String4,
                    ifnull(rar.Value,0),
                    p.Hash,
                    p.Type,
                    null,
                    p.Height,
                    p.BlockNum,
                    null,
                    pp.String2,
                    null,
                    null,
                    null,
                    null

                from Transactions p indexed by Transactions_Type_Last_String1_Height_Id

                join Payload pp
                    on pp.TxHash = p.Hash 

                join Transactions r indexed by Transactions_Type_Last_String3_Height
                    on r.Type in (200,201,202)
                    and r.Last = 1
                    and r.String3 = p.String2
                    and r.Height > ?
                    and (r.Height < ? or (r.Height = ? and r.BlockNum < ?))

                left join Payload pr
                    on pr.TxHash = r.Hash

                join Transactions ar
                    on ar.Type = 100
                    and ar.Last = 1
                    and ar.String1 = r.String1
                    and ar.Height > 0

                left join Payload par
                    on par.TxHash = ar.Hash

                left join Ratings rar indexed by Ratings_Type_Id_Last_Height
                    on rar.Type = 0
                    and rar.Id = ar.Id
                    and rar.Last = 1

                where p.Type in (200,201,202)
                    and p.Last = 1
                    and p.Height > 0
                    and p.String1 = ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                TryBindStatementInt64(stmt, i++, queryParams.blockNumMax);
                TryBindStatementText(stmt, i++, queryParams.address);
            }
        }}};

        static const auto footer = R"sql(

            -- Global order and limit for pagination
            order by Height desc, BlockNum desc
            limit 10

        )sql";
        
        auto [elem1, elem2] = _constructSelectsBasedOnFilters(filters, selects, footer);
        // A bit dirty hack because structure bindings can't be captured by lambda function.
        auto& sql = elem1;
        auto& binds = elem2;

        EventsReconstructor reconstructor;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            for (const auto& bind: binds) {
                bind(stmt, i, queryParams);
            }

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                reconstructor.FeedRow(*stmt);
            }

            FinalizeSqlStatement(*stmt);
        });
        return reconstructor.GetResult();
    }


    std::map<std::string, std::map<ShortTxType, int>> WebRpcRepository::GetNotificationsSummary(int64_t heightMax, int64_t heightMin, const std::set<std::string>& addresses, const std::set<ShortTxType>& filters)
    {
        struct QueryParams {
            // Handling all by reference
            const int64_t& heightMax;
            const int64_t& heightMin;
            const std::set<std::string>& addresses;
        } queryParams {heightMax, heightMin, addresses};

        const std::map<ShortTxType, ShortFormSqlEntry<std::shared_ptr<sqlite3_stmt*>&, QueryParams>> selects = {
        {
            ShortTxType::Referal, { R"sql(
                -- referals
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Referal) + R"sql(')TP,
                    t.String2

                from Transactions t indexed by Transactions_Type_Last_String2_Height

                where t.Type = 100
                    and t.Last in (0,1)
                    and t.Height between ? and ?
                    and t.String2 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                    and t.ROWID = (select min(tt.ROWID) from Transactions tt indexed by Transactions_Id where tt.Id = t.Id)
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                for (const auto& address: queryParams.addresses) {
                    TryBindStatementText(stmt, i++, address);
                }
            }
        }},

        {
            ShortTxType::Comment, { R"sql(
                -- Comments for my content
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                    p.String1

                from Transactions c indexed by Transactions_Type_Last_String3_Height

                join Transactions p indexed by Transactions_String1_Last_Height
                    on p.Type in (200,201,202)
                    and p.Last = 1
                    and p.Height > 0
                    and p.String2 = c.String3
                    and p.String1 != c.String1
                    and p.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )

                where c.Type = 204 -- only orig
                    and c.Last in (0,1)
                    and c.Height between ? and ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                for (const auto& address: queryParams.addresses) {
                    TryBindStatementText(stmt, i++, address);
                }
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::Subscriber, { R"sql(
                -- Subscribers
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                    subs.String2

                from Transactions subs indexed by Transactions_Type_Last_String2_Height

                where subs.Type in (302, 303) -- Ignoring unsubscribers?
                    and subs.Last in (0,1)
                    and subs.Height between ? and ?
                    and subs.String2 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
                for (const auto& address: queryParams.addresses) {
                    TryBindStatementText(stmt, i++, address);
                }
            }
        }},

        {
            ShortTxType::CommentScore, { R"sql(
                -- Comment scores
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                    c.String1

                from Transactions s indexed by Transactions_Type_Last_String2_Height

                join Transactions c indexed by Transactions_Type_Last_String1_String2_Height
                    on c.Type in (204,205)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String2 = s.String2
                    and c.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )

                where s.Type = 301
                    and s.Last = 0
                    and s.Height between ? and ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                for (const auto& address: queryParams.addresses) {
                    TryBindStatementText(stmt, i++, address);
                }
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::ContentScore, { R"sql(
                -- Content scores
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql(')TP,
                    c.String1

                from Transactions s indexed by Transactions_Type_Last_String2_Height

                join Transactions c indexed by Transactions_Type_Last_String1_String2_Height
                    on c.Type in (200, 201, 202)
                    and c.Last = 1
                    and c.Height > 0
                    and c.String2 = s.String2
                    and c.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )

                where s.Type = 300
                    and s.Last = 0
                    and s.Height between ? and ?
        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                for (const auto& address: queryParams.addresses) {
                    TryBindStatementText(stmt, i++, address);
                }
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
            }
        }},


        {
            ShortTxType::Repost, { R"sql(
                -- Reposts
                select
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Repost) + R"sql(')TP,
                    p.String1

                from Transactions r indexed by Transactions_Type_Last_String3_Height

                join Transactions p indexed by Transactions_Type_Last_String1_String2_Height
                    on p.String2 = r.String3
                    and p.Last = 1
                    and p.Type in (200,201,202)
                    and p.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )

                where r.Type in (200,201,202)
                    and r.Last in (0,1)
                    and r.Hash = r.String2 -- Only orig
                    and r.Height between ? and ?
                    and r.String3 is not null

        )sql",
            [this](std::shared_ptr<sqlite3_stmt*>& stmt, int& i, QueryParams const& queryParams){
                for (const auto& address: queryParams.addresses) {
                    TryBindStatementText(stmt, i++, address);
                }
                TryBindStatementInt64(stmt, i++, queryParams.heightMin);
                TryBindStatementInt64(stmt, i++, queryParams.heightMax);
            }
        }}
        };
        
        auto [elem1, elem2] = _constructSelectsBasedOnFilters(filters, selects, "", "union all");
        auto& sql = elem1;
        auto& binds = elem2;

        NotificationSummaryReconstructor reconstructor;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;

            for (const auto& bind: binds) {
                bind(stmt, i, queryParams);
            }

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                reconstructor.FeedRow(*stmt);
            }

            FinalizeSqlStatement(*stmt);
        });
        return reconstructor.GetResult();
    }
}
