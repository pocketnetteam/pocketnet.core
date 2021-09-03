
#ifndef POCKETTX_SCORECOMMENT_H
#define POCKETTX_SCORECOMMENT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class ScoreComment : public Transaction
    {
    public:

        ScoreComment(const string& hash, int64_t time);
        ScoreComment(const std::shared_ptr<const CTransaction>& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        
        void DeserializeRpc(const UniValue& src) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(string value);

        shared_ptr <string> GetCommentTxHash() const;
        void SetCommentTxHash(string value);

        shared_ptr <int64_t> GetValue() const;
        void SetValue(int64_t value);

    protected:
        void DeserializePayload(const UniValue& src) override;

        void BuildHash() override;
    };

} // namespace PocketTx

#endif // POCKETTX_SCORECOMMENT_H
