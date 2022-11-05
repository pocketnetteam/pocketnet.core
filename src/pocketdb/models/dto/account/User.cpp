// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/account/User.h"

namespace PocketTx
{
    User::User() : Transaction()
    {
        SetType(TxType::ACCOUNT_USER);
    }

    User::User(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACCOUNT_USER);
    }

    shared_ptr <UniValue> User::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("referrer", GetReferrerAddress() ? *GetReferrerAddress() : "");
        result->pushKV("regdate", *GetTime());

        result->pushKV("lang", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "en");
        result->pushKV("name", (m_payload && m_payload->GetString2()) ? *m_payload->GetString2() : "");
        result->pushKV("avatar", (m_payload && m_payload->GetString3()) ? *m_payload->GetString3() : "");
        result->pushKV("about", (m_payload && m_payload->GetString4()) ? *m_payload->GetString4() : "");
        result->pushKV("url", (m_payload && m_payload->GetString5()) ? *m_payload->GetString5() : "");
        result->pushKV("pubkey", (m_payload && m_payload->GetString6()) ? *m_payload->GetString6() : "");
        result->pushKV("donations", (m_payload && m_payload->GetString7()) ? *m_payload->GetString7() : "");

        result->pushKV("birthday", 0);
        result->pushKV("gender", 0);
        result->pushKV("id", 0);

        return result;
    }

    void User::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "referrer"); ok) SetReferrerAddress(val);
    }

    void User::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "r"); ok) SetReferrerAddress(val);

        GeneratePayload();

        if (auto[ok, val] = TryGetStr(src, "l"); ok) m_payload->SetString1(val);
        else m_payload->SetString1("en");

        if (auto[ok, val] = TryGetStr(src, "n"); ok) m_payload->SetString2(val);
        else m_payload->SetString2("");

        if (auto[ok, val] = TryGetStr(src, "i"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "a"); ok) m_payload->SetString4(val);
        if (auto[ok, val] = TryGetStr(src, "s"); ok) m_payload->SetString5(val);
        if (auto[ok, val] = TryGetStr(src, "k"); ok) m_payload->SetString6(val);
        if (auto[ok, val] = TryGetStr(src, "b"); ok) m_payload->SetString7(val);
    }

    shared_ptr <string> User::GetAddress() const { return m_string1; }
    void User::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> User::GetReferrerAddress() const { return m_string2; }
    void User::SetReferrerAddress(const string& value) { m_string2 = make_shared<string>(value); }

    // Payload getters
    shared_ptr <string> User::GetPayloadName() const { return GetPayload() ? GetPayload()->GetString2() : nullptr; }
    shared_ptr <string> User::GetPayloadAvatar() const { return GetPayload() ? GetPayload()->GetString3() : nullptr; }
    shared_ptr <string> User::GetPayloadUrl() const { return GetPayload() ? GetPayload()->GetString5() : nullptr; }
    shared_ptr <string> User::GetPayloadLang() const { return GetPayload() ? GetPayload()->GetString1() : nullptr; }
    shared_ptr <string> User::GetPayloadAbout() const { return GetPayload() ? GetPayload()->GetString4() : nullptr; }
    shared_ptr <string> User::GetPayloadDonations() const { return GetPayload() ? GetPayload()->GetString7() : nullptr; }
    shared_ptr <string> User::GetPayloadPubkey() const { return GetPayload() ? GetPayload()->GetString6() : nullptr; }

    void User::DeserializePayload(const UniValue& src)
    {
        Transaction::DeserializePayload(src);

        if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);

        if (auto[ok, val] = TryGetStr(src, "name"); ok) m_payload->SetString2(val);
        else m_payload->SetString2("");

        if (auto[ok, val] = TryGetStr(src, "avatar"); ok) m_payload->SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "about"); ok) m_payload->SetString4(val);
        if (auto[ok, val] = TryGetStr(src, "url"); ok) m_payload->SetString5(val);
        if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) m_payload->SetString6(val);
        if (auto[ok, val] = TryGetStr(src, "donations"); ok) m_payload->SetString7(val);
    }

    string User::BuildHash()
    {
        return BuildHash(true);
    }

    string User::BuildHash(bool includeReferrer)
    {
        std::string data;

        data += m_payload && m_payload->GetString2() ? *m_payload->GetString2() : "";
        data += m_payload && m_payload->GetString5() ? *m_payload->GetString5() : "";
        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        data += m_payload && m_payload->GetString4() ? *m_payload->GetString4() : "";
        data += m_payload && m_payload->GetString3() ? *m_payload->GetString3() : "";
        data += m_payload && m_payload->GetString7() ? *m_payload->GetString7() : "";
        data += includeReferrer && GetReferrerAddress() ? *GetReferrerAddress() : "";
        data += m_payload && m_payload->GetString6() ? *m_payload->GetString6() : "";

        return Transaction::GenerateHash(data);
    }

    string User::PreBuildHash()
    {
        std::string data;

        data += m_payload && m_payload->GetString2() ? *m_payload->GetString2() : "";
        data += m_payload && m_payload->GetString5() ? *m_payload->GetString5() : "";
        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        data += m_payload && m_payload->GetString4() ? *m_payload->GetString4() : "";
        data += m_payload && m_payload->GetString3() ? *m_payload->GetString3() : "";
        data += m_payload && m_payload->GetString7() ? *m_payload->GetString7() : "";
        data += GetReferrerAddress() ? *GetReferrerAddress() : "";
        data += m_payload && m_payload->GetString6() ? *m_payload->GetString6() : "";

        return data;
    }

} // namespace PocketTx

