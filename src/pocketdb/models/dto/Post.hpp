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

            if (auto[ok, valTxIdEdit] = TryGetStr(src, "txidEdit"); ok) {
                SetHash(valTxIdEdit);

                if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok)
                    SetRootTxHash(valTxId);
            }

        }


        shared_ptr<int64_t> GetRootTxId() const { return m_int1; }
        shared_ptr<string> GetRootTxHash() const { return m_root_tx_hash; }
        void SetRootTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetRootTxHash(string value) { m_root_tx_hash = make_shared<string>(value); }
        
        shared_ptr<int64_t> GetRelayTxId() const { return m_int2; }
        shared_ptr<string> GetRelayTxHash() const { return m_relay_tx_hash; }
        void SetRelayTxId(int64_t value) { m_int2 = make_shared<int64_t>(value); }
        void SetRelayTxHash(string value) { m_relay_tx_hash = make_shared<string>(value); }

    protected:

        shared_ptr<string> m_root_tx_hash = nullptr;
        shared_ptr<string> m_relay_tx_hash = nullptr;

    private:

        void BuildPayload(const UniValue &src) override
        {
            // TODO (brangr): payload as object
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