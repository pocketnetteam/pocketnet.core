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
            SetTxType(PocketTxType::POST_CONTENT);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "lang"); ok) SetLang(val);
            if (auto[ok, val] = TryGetStr(src, "txidEdit"); ok) SetRootTxId(val);
            if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxId(val);
        }

        shared_ptr<int64_t> GetId() const { return m_int1; }
        void SetId(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr<string> GetLang() const { return m_string1; }
        void SetLang(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetRootTxId() const { return m_string2; }
        void SetRootTxId(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr<string> GetRelayTxId() const { return m_string3; }
        void SetRelayTxId(string value) { m_string3 = make_shared<string>(value); }

    private:

        void BuildPayload(const UniValue &src) override
        {
            UniValue payload(UniValue::VOBJ);
            if (auto[ok, val] = TryGetInt64(src, "birthday"); ok) payload.pushKV("birthday", val);
            if (auto[ok, val] = TryGetInt64(src, "gender"); ok) payload.pushKV("gender", val);
            if (auto[ok, val] = TryGetStr(src, "avatar"); ok) payload.pushKV("avatar", val);
            if (auto[ok, val] = TryGetStr(src, "about"); ok) payload.pushKV("about", val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) payload.pushKV("url", val);
            if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) payload.pushKV("pubkey", val);
            if (auto[ok, val] = TryGetStr(src, "donations"); ok) payload.pushKV("donations", val);
            SetPayload(payload.write());
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            if (auto[ok, val] = TryGetInt64(src, "name"); ok) data += std::to_string(val);
            if (auto[ok, val] = TryGetInt64(src, "url"); ok) data += std::to_string(val);
            if (auto[ok, val] = TryGetStr(src, "lang"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "about"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "avatar"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "donations"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "referrer"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) data += val;
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_POST_HPP