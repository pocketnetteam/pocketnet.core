// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Post.h"

namespace PocketTx
{
    Post::Post() : Content()
    {
        SetType(TxType::CONTENT_POST);
    }

    Post::Post(const CTransactionRef& tx) : Content(tx)
    {
        SetType(TxType::CONTENT_POST);
    }

    optional<UniValue> Post::Serialize() const
    {
        auto result = Content::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("txidRepost", GetRelayTxHash() ? *GetRelayTxHash() : "");

        // For olf protocol edited content
        // txid     - original content hash
        // txidEdit - actual transaction hash
        result->pushKV("txidEdit", "");
        if (*GetRootTxHash() != *GetHash())
        {
            result->pushKV("txid", *GetRootTxHash());
            result->pushKV("txidEdit", *GetHash());
        }

        result->pushKV("lang", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "en");
        result->pushKV("caption", (m_payload && m_payload->GetString2()) ? *m_payload->GetString2() : "");
        result->pushKV("message", (m_payload && m_payload->GetString3()) ? *m_payload->GetString3() : "");
        result->pushKV("url", (m_payload && m_payload->GetString7()) ? *m_payload->GetString7() : "");
        result->pushKV("settings", (m_payload && m_payload->GetString6()) ? *m_payload->GetString6() : "");

        UniValue vImages(UniValue::VARR);
        if (m_payload && m_payload->GetString5())
            vImages.read(*m_payload->GetString5());
        result->pushKV("images", vImages);

        UniValue vTags(UniValue::VARR);
        if (m_payload && m_payload->GetString4())
            vTags.read(*m_payload->GetString4());
        result->pushKV("tags", vTags);

        return result;
    }

    void Post::Deserialize(const UniValue& src)
    {
        Content::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxHash(val);

        SetRootTxHash(*GetHash());
        if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok)
            if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                SetRootTxHash(valTxId);
    }

    void Post::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxHash(val);

        SetRootTxHash(*GetHash());
        if (auto[ok, val] = TryGetStr(src, "txidEdit"); ok)
            SetRootTxHash(val);

        GeneratePayload();

        if (auto[ok, val] = TryGetStr(src, "l"); ok && val.length() == 2) m_payload->SetString1(val);
        else m_payload->SetString1("en");

        if (auto[ok, val] = TryGetStr(src, "c"); ok) m_payload->SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "m"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "u"); ok) m_payload->SetString7(val);
        if (auto[ok, val] = TryGetStr(src, "s"); ok) m_payload->SetString6(val);
        if (auto[ok, val] = TryGetStr(src, "t"); ok) m_payload->SetString4(val);
        if (auto[ok, val] = TryGetStr(src, "i"); ok) m_payload->SetString5(val);
    }

    void Post::DeserializePayload(const UniValue& src)
    {
        Content::DeserializePayload(src);

        if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
        else m_payload->SetString1("en");

        if (auto[ok, val] = TryGetStr(src, "caption"); ok) m_payload->SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "message"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "tags"); ok) m_payload->SetString4(val);
        if (auto[ok, val] = TryGetStr(src, "url"); ok) m_payload->SetString7(val);
        if (auto[ok, val] = TryGetStr(src, "images"); ok) m_payload->SetString5(val);
        if (auto[ok, val] = TryGetStr(src, "settings"); ok) m_payload->SetString6(val);
    }
    
    const optional<string>& Post::GetRelayTxHash() const { return m_string3; }
    void Post::SetRelayTxHash(const string& value) { m_string3 = value; }

    optional<string> Post::GetPayloadLang() const { return GetPayload() ? GetPayload()->GetString1() : nullopt; }
    optional<string> Post::GetPayloadCaption() const { return GetPayload() ? GetPayload()->GetString2() : nullopt; }
    optional<string> Post::GetPayloadMessage() const { return GetPayload() ? GetPayload()->GetString3() : nullopt; }
    optional<string> Post::GetPayloadTags() const { return GetPayload() ? GetPayload()->GetString4() : nullopt; }
    optional<string> Post::GetPayloadImages() const { return GetPayload() ? GetPayload()->GetString5() : nullopt; }
    optional<string> Post::GetPayloadSettings() const { return GetPayload() ? GetPayload()->GetString6() : nullopt; }
    optional<string> Post::GetPayloadUrl() const { return GetPayload() ? GetPayload()->GetString7() : nullopt; }

    string Post::BuildHash()
    {
        std::string data;

        data += m_payload && m_payload->GetString7() ? *m_payload->GetString7() : "";
        data += m_payload && m_payload->GetString2() ? *m_payload->GetString2() : "";
        data += m_payload && m_payload->GetString3() ? *m_payload->GetString3() : "";

        if (m_payload && m_payload->GetString4() && !(*m_payload->GetString4()).empty())
        {
            UniValue tags(UniValue::VARR);
            tags.read(*m_payload->GetString4());
            for (size_t i = 0; i < tags.size(); ++i)
            {
                data += (i ? "," : "");
                data += tags[i].get_str();
            }
        }

        if (m_payload && m_payload->GetString5() && !(*m_payload->GetString5()).empty())
        {
            UniValue images(UniValue::VARR);
            images.read(*m_payload->GetString5());
            for (size_t i = 0; i < images.size(); ++i)
            {
                data += (i ? "," : "");
                data += images[i].get_str();
            }
        }

        if (GetRootTxHash() && *GetRootTxHash() != *GetHash())
            data += *GetRootTxHash();

        data += GetRelayTxHash() ? *GetRelayTxHash() : "";

        return Content::GenerateHash(data);
    }

    size_t Post::PayloadSize()
    {
        size_t dataSize =
            (GetPayloadUrl() ? GetPayloadUrl()->size() : 0) +
            (GetPayloadCaption() ? GetPayloadCaption()->size() : 0) +
            (GetPayloadMessage() ? GetPayloadMessage()->size() : 0) +
            (GetRelayTxHash() ? GetRelayTxHash()->size() : 0) +
            (GetPayloadSettings() ? GetPayloadSettings()->size() : 0) +
            (GetPayloadLang() ? GetPayloadLang()->size() : 0);

        if (GetRootTxHash() && *GetRootTxHash() != *GetHash())
            dataSize += GetRootTxHash()->size();

        if (GetPayloadTags() && !(*GetPayloadTags()).empty())
        {
            UniValue tags(UniValue::VARR);
            tags.read(*GetPayloadTags());
            for (size_t i = 0; i < tags.size(); ++i)
                dataSize += tags[i].get_str().size();
        }

        if (GetPayloadImages() && !(*GetPayloadImages()).empty())
        {
            UniValue images(UniValue::VARR);
            images.read(*GetPayloadImages());
            for (size_t i = 0; i < images.size(); ++i)
                dataSize += images[i].get_str().size();
        }

        return dataSize;
    }

} // namespace PocketTx