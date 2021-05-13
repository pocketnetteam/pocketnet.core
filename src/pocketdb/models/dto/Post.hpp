#ifndef POCKETTX_POST_HPP
#define POCKETTX_POST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Post : public Transaction
    {
    public:

        Post() : Transaction()
        {
            SetType(PocketTxType::CONTENT_POST);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);

            if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxHash(val);

            if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok)
            {
                SetHash(valTxIdEdit);

                if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                    SetRootTxHash(valTxId);
            }
        }


        shared_ptr <int64_t> GetRootTxId() const { return m_int1; }
        shared_ptr <string> GetRootTxHash() const { return m_root_tx_hash; }
        void SetRootTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetRootTxHash(string value) { m_root_tx_hash = make_shared<string>(value); }

        shared_ptr <int64_t> GetRelayTxId() const { return m_int2; }
        shared_ptr <string> GetRelayTxHash() const { return m_relay_tx_hash; }
        void SetRelayTxId(int64_t value) { m_int2 = make_shared<int64_t>(value); }
        void SetRelayTxHash(string value) { m_relay_tx_hash = make_shared<string>(value); }

    protected:

        shared_ptr <string> m_root_tx_hash = nullptr;
        shared_ptr <string> m_relay_tx_hash = nullptr;

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

            data += m_payload->GetString7Str();
            data += m_payload->GetString2Str();
            data += m_payload->GetString3Str();

            if (m_payload->GetString4() != nullptr) {
                UniValue tags(UniValue::VARR);
                tags.read(m_payload->GetString4Str());
                for (size_t i = 0; i < tags.size(); ++i)
                {
                    data += (i ? "," : "");
                    data += tags[i].get_str();
                }
            }

            if (m_payload->GetString5() != nullptr) {
                UniValue images(UniValue::VARR);
                images.read(m_payload->GetString5Str());
                for (size_t i = 0; i < images.size(); ++i)
                {
                    data += (i ? "," : "");
                    data += images[i].get_str();
                }
            }

            data += GetRootTxHash() == nullptr ? "" : *GetRootTxHash();
            data += GetRelayTxHash() == nullptr ? "" : *GetRelayTxHash();

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_POST_HPP