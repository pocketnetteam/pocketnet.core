#include "pocketdb/models/dto/Video.h"

namespace PocketTx
{
    Video::Video(const string& hash, int64_t time) : Post(hash, time)
    {
        SetType(PocketTxType::CONTENT_VIDEO);
    }

    shared_ptr <UniValue> Video::Serialize() const
    {
        auto result = Post::Serialize();

        result->pushKV("type", 1);

        return result;
    }

} // namespace PocketTx