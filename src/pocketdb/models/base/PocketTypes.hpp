// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_POCKETTYPES_HPP
#define POCKETTX_POCKETTYPES_HPP

#include <univalue.h>

namespace PocketTx
{
    // OpReturn hex codes
#define OR_SCORE "7570766f74655368617265"
#define OR_COMPLAIN "636f6d706c61696e5368617265"
#define OR_POST "7368617265"
#define OR_POSTEDIT "736861726565646974"
#define OR_SUBSCRIBE "737562736372696265"
#define OR_SUBSCRIBEPRIVATE "73756273637269626550726976617465"
#define OR_UNSUBSCRIBE "756e737562736372696265"
#define OR_USERINFO "75736572496e666f" // (userType = 0 ala gender)
#define OR_BLOCKING "626c6f636b696e67"
#define OR_UNBLOCKING "756e626c6f636b696e67"

#define OR_COMMENT "636f6d6d656e74"
#define OR_COMMENT_EDIT "636f6d6d656e7445646974"
#define OR_COMMENT_DELETE "636f6d6d656e7444656c657465"
#define OR_COMMENT_SCORE "6353636f7265"

#define OR_VIDEO "766964656f"                      // Post for video hosting
#define OR_VERIFICATION "766572696669636174696f6e" // User verification post

#define OR_POLL "706f6c6c"                                // Polling post
#define OR_POLL_SCORE "706f6c6c53636f7265"                // Score for poll posts
#define OR_TRANSLATE "7472616e736c617465"                 // Post for translating words
#define OR_TRANSLATE_SCORE "7472616e736c61746553636f7265" // Score for translate posts

#define OR_VIDEO_SERVER "766964656f536572766572"       // Video server registration over User (userType = 1)
#define OR_MESSAGE_SERVER "6d657373616765536572766572" // Messaging server registration over User (userType = 2)
#define OR_SERVER_PING "73657276657250696e67"          // Server ping over Posts

    // Int tx type
    enum PocketTxType
    {
        NOT_SUPPORTED = 0,

        TX_DEFAULT = 1,
        TX_COINBASE = 2,
        TX_COINSTAKE = 3,

        ACCOUNT_USER = 100,
        ACCOUNT_VIDEO_SERVER = 101,
        ACCOUNT_MESSAGE_SERVER = 102,

        CONTENT_POST = 200,
        CONTENT_VIDEO = 201,
        CONTENT_TRANSLATE = 202,
        CONTENT_SERVERPING = 203,
        CONTENT_COMMENT = 204,
        CONTENT_COMMENT_DELETE = 205,

        ACTION_SCORE_POST = 300,
        ACTION_SCORE_COMMENT = 301,

        ACTION_SUBSCRIBE = 302,
        ACTION_SUBSCRIBE_PRIVATE = 303,
        ACTION_SUBSCRIBE_CANCEL = 304,

        ACTION_BLOCKING = 305,
        ACTION_BLOCKING_CANCEL = 306,

        ACTION_COMPLAIN = 307,
    };

    // For transaction anonymous fields Int1, Int2, ...
    enum RatingType
    {
        RATING_ACCOUNT,
        RATING_ACCOUNT_LIKERS,
        RATING_POST,
        RATING_COMMENT,
    };
}

#endif //POCKETTX_POCKETTYPES_HPP
