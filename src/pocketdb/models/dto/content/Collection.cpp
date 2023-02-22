// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Collection.h"

namespace PocketTx
{
    Collection::Collection() : Content()
    {
        SetType(TxType::CONTENT_COLLECTION);
    }

    Collection::Collection(const CTransactionRef& tx) : Content(tx)
    {
        SetType(TxType::CONTENT_COLLECTION);
    }

    optional<UniValue> Collection::Serialize() const
    {
        auto result = Content::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");

        // For olf protocol edited content
        // txid     - original content hash
        // txidEdit - actual transaction hash
        result->pushKV("txidEdit", "");
        if (*GetRootTxHash() != *GetHash())
        {
            result->pushKV("txid", *GetRootTxHash());
            result->pushKV("txidEdit", *GetHash());
        }

        result->pushKV("contentTypes", GetContentTypes() ? *GetContentTypes() : 0);

        UniValue vContentIds(UniValue::VARR);
        if (GetContentIds())
            vContentIds.read(*GetContentIds());
        result->pushKV("contentIds", vContentIds);

        result->pushKV("lang", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "en");
        result->pushKV("caption", (m_payload && m_payload->GetString2()) ? *m_payload->GetString2() : "");
        result->pushKV("image", (m_payload && m_payload->GetString3()) ? *m_payload->GetString3() : "");
        result->pushKV("settings", (m_payload && m_payload->GetString4()) ? *m_payload->GetString4() : "");

        return result;
    }

    void Collection::Deserialize(const UniValue& src)
    {
        Content::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);

        SetRootTxHash(*GetHash());
        if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok)
            if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                SetRootTxHash(valTxId);

        if (auto[ok, valIds] = TryGetStr(src, "contentIds"); ok) SetContentIds(valIds);
        if (auto[ok, valTypes] = TryGetInt64(src, "contentTypes"); ok) SetContentTypes(valTypes);
    }

    void Collection::DeserializeRpc(const UniValue& src)
    {
        SetRootTxHash(*GetHash());
        if (auto[ok, val] = TryGetStr(src, "txidEdit"); ok)
            SetRootTxHash(val);

        if (auto[ok, val] = TryGetInt64(src, "contentTypes"); ok) SetContentTypes(val);
        if (auto[ok, val] = TryGetStr(src, "contentIds"); ok) SetContentIds(val);

        GeneratePayload();

        if (auto[ok, val] = TryGetStr(src, "l"); ok && val.length() == 2) m_payload->SetString1(val);
        else m_payload->SetString1("en");

        if (auto[ok, val] = TryGetStr(src, "c"); ok) m_payload->SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "i"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "s"); ok) m_payload->SetString4(val);
    }

    void Collection::DeserializePayload(const UniValue& src)
    {
        Content::DeserializePayload(src);

        if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
        else m_payload->SetString1("en");

        if (auto[ok, val] = TryGetStr(src, "caption"); ok) m_payload->SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "image"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "settings"); ok) m_payload->SetString4(val);
    }

    const optional<int64_t>& Collection::GetContentTypes() const { return m_int1; }
    void Collection::SetContentTypes(const int64_t& value) { m_int1 = value; }

    const optional<string>& Collection::GetContentIds() const { return m_string3; }
    tuple<bool, vector<string>> Collection::GetContentIdsVector()
    {
        vector<string> contentIds;

        UniValue ids(UniValue::VARR);
        ids.read(*GetContentIds());
        for (size_t i = 0; i < ids.size(); ++i)
            contentIds.emplace_back(ids[i].get_str());

        return {contentIds.size() != 0, contentIds};
    }
    void Collection::SetContentIds(const string &value) { m_string3 = value; }

    optional<string> Collection::GetPayloadLang() const { return GetPayload() ? GetPayload()->GetString1() : nullopt; }
    optional<string> Collection::GetPayloadCaption() const { return GetPayload() ? GetPayload()->GetString2() : nullopt; }
    optional<string> Collection::GetPayloadImage() const { return GetPayload() ? GetPayload()->GetString3() : nullopt; }
    optional<string> Collection::GetPayloadSettings() const { return GetPayload() ? GetPayload()->GetString4() : nullopt; }

    string Collection::BuildHash()
    {
        std::string data;

        if (GetRootTxHash() && *GetRootTxHash() != *GetHash())
            data += *GetRootTxHash();

        data += GetContentTypes() ? std::to_string(*GetContentTypes()) : "";

        if (GetContentIds())
        {
            UniValue ids(UniValue::VARR);
            ids.read(*GetContentIds());
            for (size_t i = 0; i < ids.size(); ++i)
            {
                data += (i ? "," : "");
                data += ids[i].get_str();
            }
        }

        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        data += m_payload && m_payload->GetString2() ? *m_payload->GetString2() : "";
        data += m_payload && m_payload->GetString3() ? *m_payload->GetString3() : "";
        data += m_payload && m_payload->GetString4() ? *m_payload->GetString4() : "";

        return Content::GenerateHash(data);
    }

} // namespace PocketTx