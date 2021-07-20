#ifndef POCKETTX_VIDEO_HPP
#define POCKETTX_VIDEO_HPP

#include "pocketdb/models/dto/Post.hpp"

namespace PocketTx
{

    class Video : public Post
    {
    public:

        Video(string& hash, int64_t time, string& opReturn) : Post(hash, time, opReturn)
        {
            SetType(PocketTxType::CONTENT_VIDEO);
        }

        shared_ptr <UniValue> Serialize() const override
        {
            auto result = Post::Serialize();

            result->pushKV("type", 1);

            return result;
        }

    protected:

    };

} // namespace PocketTx

#endif // POCKETTX_VIDEO_HPP