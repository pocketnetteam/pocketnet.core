// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_POCKETTYPES_H
#define POCKETTX_POCKETTYPES_H

#include <string>
#include <vector>

namespace PocketTx
{
    using namespace std;

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

    #define OR_VIDEO "766964656f" // Post for video hosting
    #define OR_ARTICLE "61727469636c65" // Article Post

    #define OR_POLL "706f6c6c"                                // Polling post
    #define OR_POLL_SCORE "706f6c6c53636f7265"                // Score for poll posts
    #define OR_TRANSLATE "7472616e736c617465"                 // Post for translating words
    #define OR_TRANSLATE_SCORE "7472616e736c61746553636f7265" // Score for translate posts

    #define OR_VIDEO_SERVER "766964656f536572766572"       // Video server registration over User (userType = 1)
    #define OR_MESSAGE_SERVER "6d657373616765536572766572" // Messaging server registration over User (userType = 2)
    #define OR_SERVER_PING "73657276657250696e67"          // Server ping over Posts

    #define OR_CONTENT_DELETE "636f6e74656e7444656c657465" // Deleting content
    #define OR_CONTENT_BOOST "636f6e74656e74426f6f7374" // Boost content
    #define OR_ACCOUNT_SETTING "616363536574" // Public account settings (accSet)
    #define OR_ACCOUNT_DELETE "61636344656c" // Public account settings (accSet)

    #define OR_MODERATION_FLAG "6d6f64466c6167" // Flag for moderation
    

    // Int tx type
    enum TxType
    {
        NOT_SUPPORTED = 0,

        TX_DEFAULT = 1,
        TX_COINBASE = 2,
        TX_COINSTAKE = 3,

        ACCOUNT_USER = 100,
        ACCOUNT_VIDEO_SERVER = 101,
        ACCOUNT_MESSAGE_SERVER = 102,
        ACCOUNT_SETTING = 103,
        ACCOUNT_DELETE = 170,

        CONTENT_POST = 200,
        CONTENT_VIDEO = 201,
        CONTENT_ARTICLE = 202,
        // CONTENT_SERVERPING = 203,

        CONTENT_COMMENT = 204,
        CONTENT_COMMENT_EDIT = 205,
        CONTENT_COMMENT_DELETE = 206,

        CONTENT_DELETE = 207,

        BOOST_CONTENT = 208,

        ACTION_SCORE_CONTENT = 300,
        ACTION_SCORE_COMMENT = 301,

        ACTION_SUBSCRIBE = 302,
        ACTION_SUBSCRIBE_PRIVATE = 303,
        ACTION_SUBSCRIBE_CANCEL = 304,

        ACTION_BLOCKING = 305,
        ACTION_BLOCKING_CANCEL = 306,

        ACTION_COMPLAIN = 307,

        MODERATOR_REQUEST = 400, // Some users have the right to choose a moderator
        MODERATOR_REGISTER = 401, // Each moderator must define a list of public key hashes for voting
        MODERATION_FLAG = 410, // Flags are used to mark content that needs moderation
        MODERATION_VOTE = 420, // Votes is used by moderators in the jury process
    };

    // Rating types
    enum RatingType
    {
        RATING_ACCOUNT = 0,
        RATING_CONTENT = 2,
        RATING_COMMENT = 3,
        
        ACCOUNT_LIKERS = 1,
        
        ACCOUNT_LIKERS_POST = 101,
        ACCOUNT_LIKERS_COMMENT_ROOT = 102,
        ACCOUNT_LIKERS_COMMENT_ANSWER = 103,
        ACCOUNT_DISLIKERS_COMMENT_ANSWER = 104,
        
        ACCOUNT_LIKERS_POST_LAST = 111,
        ACCOUNT_LIKERS_COMMENT_ROOT_LAST = 112,
        ACCOUNT_LIKERS_COMMENT_ANSWER_LAST = 113,
        ACCOUNT_DISLIKERS_COMMENT_ANSWER_LAST = 114,
    };

    // Content field types
    enum ContentFieldType
    {
        ContentFieldType_CommentMessage = 0, // Payload.String1
        ContentFieldType_AccountUserName = 1, // Payload.String2
        ContentFieldType_ContentPostCaption = 2, // Payload.String2
        ContentFieldType_ContentVideoCaption = 3, // Payload.String2
        ContentFieldType_ContentPostMessage = 4, // Payload.String3
        ContentFieldType_ContentVideoMessage = 5, // Payload.String3
        ContentFieldType_AccountUserAbout = 6, // Payload.String4
        ContentFieldType_AccountUserUrl = 7, // Payload.String5
        ContentFieldType_ContentPostUrl = 8, // Payload.String7
        ContentFieldType_ContentVideoUrl = 9, // Payload.String7
    };

    // Transaction info for indexing spents and other
    struct TransactionIndexingInfo
    {
        string Hash;
        int BlockNumber;
        int64_t Time;
        TxType Type;
        vector<pair<string, int>> Inputs;

        bool IsAccount() const
        {
            return Type == TxType::ACCOUNT_USER ||
                   Type == TxType::ACCOUNT_DELETE;
        }

        bool IsAccountSetting() const
        {
            return Type == TxType::ACCOUNT_SETTING;
        }

        bool IsContent() const
        {
            return Type == TxType::CONTENT_POST ||
                   Type == TxType::CONTENT_VIDEO ||
                   Type == TxType::CONTENT_ARTICLE ||
                   Type == TxType::CONTENT_DELETE;
        }

        bool IsComment() const
        {
            return Type == TxType::CONTENT_COMMENT ||
                   Type == TxType::CONTENT_COMMENT_EDIT ||
                   Type == TxType::CONTENT_COMMENT_DELETE;
        }

        bool IsBlocking() const
        {
            return Type == TxType::ACTION_BLOCKING ||
                   Type == TxType::ACTION_BLOCKING_CANCEL;
        }

        bool IsSubscribe() const
        {
            return Type == TxType::ACTION_SUBSCRIBE ||
                   Type == TxType::ACTION_SUBSCRIBE_CANCEL ||
                   Type == TxType::ACTION_SUBSCRIBE_PRIVATE;
        }

        bool IsActionScore() const
        {
            return Type == TxType::ACTION_SCORE_COMMENT ||
                   Type == TxType::ACTION_SCORE_CONTENT;
        }

        bool IsBoostContent() const
        {
            return Type == TxType::BOOST_CONTENT;
        }
    };
}

#endif //POCKETTX_POCKETTYPES_H
