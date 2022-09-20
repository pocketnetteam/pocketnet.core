// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/ShortFormHelper.h"

#include <map>
#include <algorithm>
#include <set>


static const std::map<PocketDb::ShortTxType, std::string>& GetTypesMap() {
    static const std::map<PocketDb::ShortTxType, std::string> typesMap = {
        { PocketDb::ShortTxType::Money, "money" },
        { PocketDb::ShortTxType::Referal, "referal" },
        { PocketDb::ShortTxType::Answer, "answer" },
        { PocketDb::ShortTxType::Comment, "comment" },
        { PocketDb::ShortTxType::Subscriber, "subscriber" },
        { PocketDb::ShortTxType::CommentScore, "commentscore" },
        { PocketDb::ShortTxType::ContentScore, "contentscore" },
        { PocketDb::ShortTxType::PrivateContent, "privatecontent" },
        { PocketDb::ShortTxType::Boost, "boost" },
        { PocketDb::ShortTxType::Repost, "repost" },
        { PocketDb::ShortTxType::Blocking, "blocking" }
    };
    return typesMap;
}

std::string PocketHelpers::ShortTxTypeConvertor::toString(PocketDb::ShortTxType type)
{
    static const auto& typesMap = GetTypesMap();
    auto str = typesMap.find(type);
    if (str != typesMap.end()) {
        return str->second;
    }
    return "";
}

bool PocketHelpers::ShortTxFilterValidator::Notifications::IsFilterAllowed(PocketDb::ShortTxType type)
{
    static const std::set<PocketDb::ShortTxType> allowed = {
        PocketDb::ShortTxType::Money,
        PocketDb::ShortTxType::Answer,
        PocketDb::ShortTxType::PrivateContent,
        PocketDb::ShortTxType::Boost,
        PocketDb::ShortTxType::Referal,
        PocketDb::ShortTxType::Comment,
        PocketDb::ShortTxType::Subscriber,
        PocketDb::ShortTxType::CommentScore,
        PocketDb::ShortTxType::ContentScore,
        PocketDb::ShortTxType::Repost
    };

    return allowed.find(type) != allowed.end();
}

bool PocketHelpers::ShortTxFilterValidator::NotificationsSummary::IsFilterAllowed(PocketDb::ShortTxType type)
{
    static const std::set<PocketDb::ShortTxType> allowed = {
        PocketDb::ShortTxType::Referal,
        PocketDb::ShortTxType::Comment,
        PocketDb::ShortTxType::Subscriber,
        PocketDb::ShortTxType::CommentScore,
        PocketDb::ShortTxType::ContentScore,
        PocketDb::ShortTxType::Repost,
    };

    return allowed.find(type) != allowed.end();
}

bool PocketHelpers::ShortTxFilterValidator::Activities::IsFilterAllowed(PocketDb::ShortTxType type)
{
    static const std::set<PocketDb::ShortTxType> allowed = {
        PocketDb::ShortTxType::Answer,
        PocketDb::ShortTxType::Comment,
        PocketDb::ShortTxType::Subscriber,
        PocketDb::ShortTxType::CommentScore,
        PocketDb::ShortTxType::ContentScore,
        PocketDb::ShortTxType::Boost,
        PocketDb::ShortTxType::Blocking,
    };

    return allowed.find(type) != allowed.end();
}

bool PocketHelpers::ShortTxFilterValidator::Events::IsFilterAllowed(PocketDb::ShortTxType type)
{
    static const std::set<PocketDb::ShortTxType> allowed = {
        PocketDb::ShortTxType::Money,
        PocketDb::ShortTxType::Referal,
        PocketDb::ShortTxType::Answer,
        PocketDb::ShortTxType::Comment,
        PocketDb::ShortTxType::Subscriber,
        PocketDb::ShortTxType::CommentScore,
        PocketDb::ShortTxType::ContentScore,
        PocketDb::ShortTxType::PrivateContent,
        PocketDb::ShortTxType::Boost,
        PocketDb::ShortTxType::Repost,
    };

    return allowed.find(type) != allowed.end();
}

PocketDb::ShortTxType PocketHelpers::ShortTxTypeConvertor::strToType(const std::string& typeStr)
{
    static const auto& typesMap = GetTypesMap();
    auto type = std::find_if(typesMap.begin(), typesMap.end(), [&](const auto& elem) { return elem.second == typeStr; });
    if (type != typesMap.end()) {
        return type->first;
    }
    return PocketDb::ShortTxType::NotSet;
}

std::optional<std::vector<PocketDb::ShortTxOutput>> _parseOutputs(const std::string& jsonStr)
{
    UniValue json (UniValue::VOBJ);
    if (!json.read(jsonStr) || !json.isArray()) return std::nullopt;
    std::vector<PocketDb::ShortTxOutput> res;
    res.reserve(json.size());
    for (int i = 0; i < json.size(); i++) {
        const auto& elem = json[i];
        PocketDb::ShortTxOutput output;

        if (elem.exists("TxHash") && elem["TxHash"].isStr()) output.SetTxHash(elem["TxHash"].get_str());
        if (elem.exists("Value") && elem["Value"].isNum()) output.SetValue(elem["Value"].get_int64());
        if (elem.exists("SpentTxHash") && elem["SpentTxHash"].isStr()) output.SetSpentTxHash(elem["SpentTxHash"].get_str());
        if (elem.exists("AddressHash") && elem["AddressHash"].isStr()) output.SetAddressHash(elem["AddressHash"].get_str());
        if (elem.exists("Number") && elem["Number"].isNum()) output.SetNumber(elem["Number"].get_int());
        if (elem.exists("ScriptPubKey") && elem["ScriptPubKey"].isStr()) output.SetScriptPubKey(elem["ScriptPubKey"].get_str());
        if (elem.exists("Account") && elem["Account"].isStr()) {
            const auto& accJson = elem["Account"];
            PocketDb::ShortAccount accData;
            if (accJson.exists("Lang") && accJson["Lang"].isStr()) accData.SetLang(accJson["Lang"].get_str());
            if (accJson.exists("Name") && accJson["Name"].isStr()) accData.SetName(accJson["Name"].get_str());
            if (accJson.exists("Avatar") && accJson["Avatar"].isStr()) accData.SetAvatar(accJson["Avatar"].get_str());
            if (accJson.exists("Rep") && accJson["Rep"].isNum()) accData.SetReputation(accJson["Rep"].get_int64());
            output.SetAccount(accData);
        }

        res.emplace_back(std::move(output));
    }

    return res;
}

std::optional<std::vector<std::pair<std::string, std::optional<PocketDb::ShortAccount>>>> _processMultipleAddresses(const std::string& jsonStr)
{
    UniValue json;
    if (!json.read(jsonStr) || !json.isArray() || json.size() <= 0) return std::nullopt;

    std::vector<std::pair<std::string, std::optional<PocketDb::ShortAccount>>> multipleAddresses;
    for (int i = 0; i < json.size(); i++) {
        const auto& entry = json[i];
        if (!entry.isObject() || !entry.exists("address") || !entry["address"].isStr()) {
            continue;
        }

        auto address = entry["address"].get_str();
        std::optional<PocketDb::ShortAccount> accountData;
        if (entry.exists("account")) {
            const auto& account = entry["account"];
            if (account["name"].isStr()) {
                PocketDb::ShortAccount accData;
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

bool PocketHelpers::NotificationsResult::HasData(const int64_t& blocknum)
{
    return m_txArrIndicies.find(blocknum) != m_txArrIndicies.end();
}

void PocketHelpers::NotificationsResult::InsertData(const PocketDb::ShortForm& shortForm)
{
    if (HasData(*shortForm.GetTxData().GetBlockNum())) return;
    m_txArrIndicies.insert({*shortForm.GetTxData().GetBlockNum(), m_data.size()});
    m_data.emplace_back(shortForm.Serialize(false));
}

void PocketHelpers::NotificationsResult::InsertNotifiers(const int64_t& blocknum, PocketDb::ShortTxType contextType, std::map<std::string, std::optional<PocketDb::ShortAccount>> addresses)
{
    for (const auto& address: addresses) {
        auto& notifierEntry = m_notifiers[address.first];
        notifierEntry.notifications[contextType].emplace_back(m_txArrIndicies.at(blocknum));
        if (!notifierEntry.account)
            notifierEntry.account = address.second;
    }
}

UniValue PocketHelpers::NotificationsResult::Serialize() const
{
    UniValue notifiersUni (UniValue::VOBJ);
    notifiersUni.reserveKVSize(m_notifiers.size());

    for (const auto& notifier: m_notifiers) {
        const auto& address = notifier.first;
        const auto& notifierEntry = notifier.second;

        UniValue notifierData (UniValue::VOBJ);
        notifierData.reserveKVSize(notifierEntry.notifications.size());
        for (const auto& contextTypeIndicies: notifierEntry.notifications) {
            UniValue indicies (UniValue::VARR);
            indicies.push_backV(contextTypeIndicies.second);
            notifierData.pushKV(PocketHelpers::ShortTxTypeConvertor::toString(contextTypeIndicies.first), indicies, false);
        }

        UniValue notifierUniObj (UniValue::VOBJ);
        if (const auto& accData = notifierEntry.account; accData.has_value()) {
            notifierUniObj.pushKV("i", accData->Serialize(), false);
        }
        notifierUniObj.pushKV("e", std::move(notifierData), false);

        notifiersUni.pushKV(address, notifierUniObj, false);
    }

    UniValue data (UniValue::VARR);
    data.push_backV(m_data);

    UniValue res (UniValue::VOBJ);
    res.pushKV("data", data, false);
    res.pushKV("notifiers", notifiersUni,false);
    return res;
}

void PocketHelpers::ShortFormParser::Reset(const int& startIndex)
{
    m_startIndex = startIndex;
}

PocketDb::ShortForm PocketHelpers::ShortFormParser::ParseFull(sqlite3_stmt* stmt)
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

int64_t PocketHelpers::ShortFormParser::ParseBlockNum(sqlite3_stmt* stmt)
{
    auto [ok, val] = TryGetColumnInt64(stmt, m_startIndex+5);
    if (!ok) {
        throw std::runtime_error("Failed to extract blocknum from stmt");
    }

    return val;
}

PocketDb::ShortTxType PocketHelpers::ShortFormParser::ParseType(sqlite3_stmt* stmt)
{
    auto [ok, val] = TryGetColumnString(stmt, m_startIndex);
    if (!ok) {
        throw std::runtime_error("Failed to extract short tx type");
    }

    auto type = PocketHelpers::ShortTxTypeConvertor::strToType(val);
    if (type == PocketDb::ShortTxType::NotSet) {
        throw std::runtime_error("Failed to parse extracted short tx type");
    }

    return type;
}

std::string PocketHelpers::ShortFormParser::ParseHash(sqlite3_stmt* stmt)
{
    auto [ok, hash] = TryGetColumnString(stmt, m_startIndex+1);
    if (!ok) {
        throw std::runtime_error("Failed to extract tx hash from stmt");
    }

    return hash;
}

std::optional<std::vector<PocketDb::ShortTxOutput>> PocketHelpers::ShortFormParser::ParseOutputs(sqlite3_stmt* stmt)
{
    auto [ok, str] = TryGetColumnString(stmt, m_startIndex + 9);
    if (ok) {
        return _parseOutputs(str);
    }
    return std::nullopt;
}

std::optional<PocketDb::ShortAccount> PocketHelpers::ShortFormParser::ParseAccount(sqlite3_stmt* stmt, const int& index)
{
    auto [ok1, lang] = TryGetColumnString(stmt, index);
    auto [ok2, name] = TryGetColumnString(stmt, index+1);
    auto [ok3, avatar] = TryGetColumnString(stmt, index+2);
    auto [ok4, reputation] = TryGetColumnInt64(stmt, index+3);
    if (ok2 && ok4) { // TODO (losty): can there be no avatar?
        auto acc = PocketDb::ShortAccount(name, avatar, reputation);
        if (ok1) acc.SetLang(lang);
        return acc;
    }
    return std::nullopt;
}

std::optional<PocketDb::ShortTxData> PocketHelpers::ShortFormParser::ProcessTxData(sqlite3_stmt* stmt, int& index)
{
    const auto i = index;

    static const auto stmtOffset = 17;
    index += stmtOffset;

    auto [ok1, hash] = TryGetColumnString(stmt, i);
    auto [ok2, txType] = TryGetColumnInt(stmt, i+1);

    if (ok1 && ok2) {
        PocketDb::ShortTxData txData(hash, (PocketTx::TxType)txType);
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
        txData.SetAccount(ParseAccount(stmt, i+12));
        if (auto [ok, val] = TryGetColumnString(stmt, i+16); ok) txData.SetMultipleAddresses(_processMultipleAddresses(val));
        return txData;
    }

    return std::nullopt;
}

PocketHelpers::EventsReconstructor::EventsReconstructor()
{
    m_parser.Reset(0);
}

void PocketHelpers::EventsReconstructor::FeedRow(sqlite3_stmt* stmt)
{
    m_result.emplace_back(std::move(m_parser.ParseFull(stmt)));
}

std::vector<PocketDb::ShortForm> PocketHelpers::EventsReconstructor::GetResult() const
{
    return m_result;
}

PocketHelpers::NotificationsReconstructor::NotificationsReconstructor()
{
    m_parser.Reset(5);
}

void PocketHelpers::NotificationsReconstructor::FeedRow(sqlite3_stmt* stmt)
{
    // Notifiers data for current row context. Possible more than one notifier for the same context
    std::map<std::string, std::optional<PocketDb::ShortAccount>> notifiers;
    // Collecting addresses and accounts for notifiers. Required data can be in 2 places:
    //  - First column in query
    //  - Outputs (e.x. for money)
    //  TODO (losty): generalize collecting account data because there could be more variants in the future
    auto [ok, addressOne] = TryGetColumnString(stmt, 0);
    if (ok) {
        auto pulp = m_parser.ParseAccount(stmt, 1);
        notifiers.insert(std::move(std::make_pair(std::move(addressOne), std::move(pulp))));
    } else {
        if (auto outputs = m_parser.ParseOutputs(stmt); outputs) {
            for (const auto& output: *outputs) {
                if (output.GetAddressHash() && !output.GetAddressHash()->empty()) {
                    notifiers.insert({*output.GetAddressHash(), output.GetAccount()});
                }
            }
        }
    }
    if (notifiers.empty()) throw std::runtime_error("Missing address of notifier");

    auto blockNum = m_parser.ParseBlockNum(stmt); // blocknum is a unique key of tx because we are looking for txs in a single block
    // Do not perform parsing sql if we already has this tx
    if (!m_notifications.HasData(blockNum)) {
        m_notifications.InsertData(m_parser.ParseFull(stmt));
    }
    m_notifications.InsertNotifiers(blockNum, m_parser.ParseType(stmt), std::move(notifiers));
}

PocketHelpers::NotificationsResult PocketHelpers::NotificationsReconstructor::GetResult() const
{
    return m_notifications;
}

void PocketHelpers::NotificationSummaryReconstructor::FeedRow(sqlite3_stmt* stmt)
{
    auto [ok1, typeStr] = TryGetColumnString(stmt, 0);
    auto [ok2, address] = TryGetColumnString(stmt, 1);
    if (!ok1 || !ok2) return;

    if (auto type = PocketHelpers::ShortTxTypeConvertor::strToType(typeStr); type != PocketDb::ShortTxType::NotSet) {
        m_result[address][type]++;
    }
}
