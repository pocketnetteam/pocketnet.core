#ifndef POCKETTX_POST_HPP
#define POCKETTX_POST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Post : public Transaction
    {
    public:

        Post(string hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::CONTENT_POST);
        }

        shared_ptr<UniValue> Serialize() const override
        {
            auto result = Transaction::Serialize();

            result->pushKV("address", *GetAddress());
            result->pushKV("txidRepost", *GetRelayTxHash());
            result->pushKV("txid", *GetRootTxHash()); //TODO (brangr): txidEdit

            return result;
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxHash(val);
            if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok)
            {
                if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                    SetRootTxHash(valTxId);
            }
        }

        shared_ptr<string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetRootTxHash() const { return m_string2; }
        void SetRootTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <string> GetRelayTxHash() const { return m_string3; }
        void SetRelayTxHash(string value) { m_string3 = make_shared<string>(value); }

    protected:

    private:

        void BuildPayload(const UniValue& src) override
        {
            Transaction::BuildPayload(src);

            if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
            else m_payload->SetString1("en");

            if (auto[ok, val] = TryGetStr(src, "caption"); ok) m_payload->SetString2(val);
            if (auto[ok, val] = TryGetStr(src, "message"); ok) m_payload->SetString3(val);
            if (auto[ok, val] = TryGetStr(src, "tags"); ok) m_payload->SetString4(val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) m_payload->SetString7(val);
            if (auto[ok, val] = TryGetStr(src, "images"); ok) m_payload->SetString5(val);
            if (auto[ok, val] = TryGetStr(src, "settings"); ok) m_payload->SetString6(val);
        }

        void BuildHash(const UniValue& src) override
        {
            std::string data;

            data += m_payload->GetString7() ? *m_payload->GetString7() : "";
            data += m_payload->GetString2() ? *m_payload->GetString2() : "";
            data += m_payload->GetString3() ? *m_payload->GetString3() : "";

            if (m_payload->GetString4() && *m_payload->GetString4() != "")
            {
                UniValue tags(UniValue::VARR);
                tags.read(*m_payload->GetString4());
                for (size_t i = 0; i < tags.size(); ++i)
                {
                    data += (i ? "," : "");
                    data += tags[i].get_str();
                }
            }

            if (m_payload->GetString5() && *m_payload->GetString5() != "")
            {
                UniValue images(UniValue::VARR);
                images.read(*m_payload->GetString5());
                for (size_t i = 0; i < images.size(); ++i)
                {
                    data += (i ? "," : "");
                    data += images[i].get_str();
                }
            }

            data += GetRootTxHash() ? *GetRootTxHash() : "";
            data += GetRelayTxHash() ? *GetRelayTxHash() : "";

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_POST_HPP