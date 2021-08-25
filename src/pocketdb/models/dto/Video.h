#ifndef POCKETTX_VIDEO_H
#define POCKETTX_VIDEO_H

#include "pocketdb/models/dto/Post.h"

namespace PocketTx
{
    using namespace std;

    class Video : public Post
    {
    public:
        Video(const string& hash, int64_t time);
        shared_ptr<UniValue> Serialize() const override;
    };

} // namespace PocketTx

#endif // POCKETTX_VIDEO_H