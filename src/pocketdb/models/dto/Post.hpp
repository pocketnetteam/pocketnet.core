#ifndef POCKETTX_POST_HPP
#define POCKETTX_POST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{
    using namespace std;

    class Post : public Transaction
    {
    public:

        Post(string& hash, int64_t time, string& opReturn) : Transaction(hash, time, opReturn)
        {
            SetType(PocketTxType::CONTENT_POST);
        }

        shared_ptr<UniValue> Serialize() const override
        {
            auto result = Transaction::Serialize();

            result->pushKV("address", GetAddress() ? *GetAddress() : "");
            result->pushKV("txidRepost", GetRelayTxHash() ? *GetRelayTxHash() : "");

            // For olf protocol edited content
            // txid     - original content hash
            // txidEdit - actual transaction hash
            if (*GetRootTxHash() == *GetHash())
            {
                result->pushKV("txid", *GetHash());
                result->pushKV("txidEdit", "");
            }
            else
            {
                result->pushKV("txid", *GetRootTxHash());
                result->pushKV("txidEdit", *GetHash());
            }

            result->pushKV("lang", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "en");
            result->pushKV("caption", (m_payload && m_payload->GetString2()) ? *m_payload->GetString2() : "");
            result->pushKV("message", (m_payload && m_payload->GetString3()) ? *m_payload->GetString3() : "");
            result->pushKV("tags", (m_payload && m_payload->GetString4()) ? *m_payload->GetString4() : "");
            result->pushKV("url", (m_payload && m_payload->GetString7()) ? *m_payload->GetString7() : "");
            result->pushKV("images", (m_payload && m_payload->GetString5()) ? *m_payload->GetString5() : "");
            result->pushKV("settings", (m_payload && m_payload->GetString6()) ? *m_payload->GetString6() : "");

            result->pushKV("type", 0);
            result->pushKV("caption_", "");
            result->pushKV("message_", "");
            result->pushKV("scoreSum", 0);
            result->pushKV("scoreCnt", 0);
            result->pushKV("reputation", 0);

            return result;
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxHash(val);

            SetRootTxHash(*GetHash());
            if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok)
                if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                    SetRootTxHash(valTxId);
        }

        shared_ptr<string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetRootTxHash() const { return m_string2; }
        void SetRootTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr<string> GetRelayTxHash() const { return m_string3; }
        void SetRelayTxHash(string value) { m_string3 = make_shared<string>(value); }

        bool IsEdit() const { return *m_string2 != *m_hash; }

    protected:

        void DeserializePayload(const UniValue& src) override
        {
            Transaction::DeserializePayload(src);

            if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
            else m_payload->SetString1("en");

            if (auto[ok, val] = TryGetStr(src, "caption"); ok) m_payload->SetString2(val);
            if (auto[ok, val] = TryGetStr(src, "message"); ok) m_payload->SetString3(val);
            if (auto[ok, val] = TryGetStr(src, "tags"); ok) m_payload->SetString4(val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) m_payload->SetString7(val);
            if (auto[ok, val] = TryGetStr(src, "images"); ok) m_payload->SetString5(val);
            if (auto[ok, val] = TryGetStr(src, "settings"); ok) m_payload->SetString6(val);
        }

        void BuildHash() override
        {
            std::string data;

            data += m_payload->GetString7() ? *m_payload->GetString7() : "";
            data += m_payload->GetString2() ? *m_payload->GetString2() : "";
            data += m_payload->GetString3() ? *m_payload->GetString3() : "";

            if (m_payload->GetString4() && !(*m_payload->GetString4()).empty())
            {
                UniValue tags(UniValue::VARR);
                tags.read(*m_payload->GetString4());
                for (size_t i = 0; i < tags.size(); ++i)
                {
                    data += (i ? "," : "");
                    data += tags[i].get_str();
                }
            }

            if (m_payload->GetString5() && !(*m_payload->GetString5()).empty())
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

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_POST_HPP