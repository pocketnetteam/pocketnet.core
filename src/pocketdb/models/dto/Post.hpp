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
            if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) SetRelayTxId(val);

            if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok) {
                SetTxId(valTxIdEdit);

                if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                    SetRootTxId(valTxId);
            }

        }

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
            if (auto[ok, val] = TryGetStr(src, "caption"); ok) payload.pushKV("caption", val);
            if (auto[ok, val] = TryGetStr(src, "message"); ok) payload.pushKV("message", val);
            if (auto[ok, val] = TryGetStr(src, "tags"); ok) payload.pushKV("tags", val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) payload.pushKV("url", val);
            if (auto[ok, val] = TryGetStr(src, "images"); ok) payload.pushKV("images", val);
            if (auto[ok, val] = TryGetStr(src, "settings"); ok) payload.pushKV("settings", val);
            SetPayload(payload);
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;

            if (auto[ok, val] = TryGetStr(src, "url"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "caption"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "message"); ok) data += val;

            if (auto[ok, val] = TryGetStr(src, "tags"); ok) {
                UniValue tags(UniValue::VARR);
                tags.read(val);
                for (size_t i = 0; i < tags.size(); ++i)
                    data += (i ? "," : "") + tags[i].get_str();
            }

            if (auto[ok, val] = TryGetStr(src, "images"); ok) {
                UniValue tags(UniValue::VARR);
                tags.read(val);
                for (size_t i = 0; i < tags.size(); ++i)
                    data += (i ? "," : "") + tags[i].get_str();
            }

            if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok) {
                if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                    data += valTxId;
            }

            if (auto[ok, val] = TryGetStr(src, "txidRepost"); ok) data += val;

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_POST_HPP