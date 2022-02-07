// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/VideoServer.h"

namespace PocketTx
{
    VideoServer::VideoServer() : Transaction()
    {
        SetType(TxType::ACCOUNT_VIDEO_SERVER);
    }

    VideoServer::VideoServer(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACCOUNT_VIDEO_SERVER);
    }

    shared_ptr <UniValue> VideoServer::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("name", (m_payload && m_payload->GetString2()) ? *m_payload->GetString1() : "");
        result->pushKV("ip", (m_payload && m_payload->GetString1()) ? *m_payload->GetString2() : "");
        result->pushKV("domain", (m_payload && m_payload->GetString2()) ? *m_payload->GetString3() : "");
        result->pushKV("about", (m_payload && m_payload->GetString1()) ? *m_payload->GetString4() : "");

        return result;
    }

    void VideoServer::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
    }

    void VideoServer::DeserializeRpc(const UniValue& src)
    {
        // TODO (losty-video): address?
        GeneratePayload();

        if (auto[ok, val] = TryGetStr(src, "n"); ok) m_payload->SetString1(val);
        else m_payload->SetString1("");

        if (auto[ok, val] = TryGetStr(src, "i"); ok) m_payload->SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "d"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "a"); ok) m_payload->SetString4(val);
    }

    shared_ptr <string> VideoServer::GetAddress() const { return m_string1; }
    void VideoServer::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    // Payload getters
    shared_ptr <string> VideoServer::GetPayloadName() const { return GetPayload() ? GetPayload()->GetString1() : nullptr; }
    shared_ptr <string> VideoServer::GetPayloadIP() const { return GetPayload() ? GetPayload()->GetString2() : nullptr; }
    shared_ptr <string> VideoServer::GetPayloadDomain() const { return GetPayload() ? GetPayload()->GetString3() : nullptr; }
    shared_ptr <string> VideoServer::GetPayloadAbout() const { return GetPayload() ? GetPayload()->GetString4() : nullptr; }

    void VideoServer::DeserializePayload(const UniValue& src)
    {
        Transaction::DeserializePayload(src);

        if (auto[ok, val] = TryGetStr(src, "name"); ok) m_payload->SetString1(val);
        else m_payload->SetString2("");

        if (auto[ok, val] = TryGetStr(src, "ip"); ok) m_payload->SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "domain"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "about"); ok) m_payload->SetString4(val);
    }

    string VideoServer::BuildHash()
    {
        return Transaction::GenerateHash(PreBuildHash());
    }

    string VideoServer::PreBuildHash()
    {
        std::string data;

        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        data += m_payload && m_payload->GetString2() ? *m_payload->GetString2() : "";
        data += m_payload && m_payload->GetString3() ? *m_payload->GetString3() : "";
        data += m_payload && m_payload->GetString4() ? *m_payload->GetString4() : "";

        return data;
    }

} // namespace PocketTx

