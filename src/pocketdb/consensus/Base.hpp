// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BASE_HPP
#define POCKETCONSENSUS_BASE_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.hpp"
#include "univalue/include/univalue.h"

namespace PocketConsensus
{
    using std::string;
    using std::shared_ptr;
    using std::make_shared;
    using std::map;
    using std::make_tuple;
    using std::tuple;

    using namespace PocketTx;
    using namespace PocketDb;

    enum SocialConsensusResult {
        Success = 0,
        NotRegistered = 1,
        PostLimit = 2,
        ScoreLimit = 3,
        DoubleScore = 4,
        SelfScore = 5,
        ChangeInfoLimit = 6,
        InvalideSubscribe = 7,
        DoubleSubscribe = 8,
        SelfSubscribe = 9,
        Unknown = 10,
        Failed = 11,
        NotFound = 12,
        DoubleComplain = 13,
        SelfComplain = 14,
        ComplainLimit = 15,
        LowReputation = 16,
        ContentSizeLimit = 17,
        NicknameDouble = 18,
        NicknameLong = 19,
        ReferrerSelf = 20,
        FailedOpReturn = 21,
        InvalidBlocking = 22,
        DoubleBlocking = 23,
        SelfBlocking = 24,
        DoublePostEdit = 25,
        PostEditLimit = 26,
        PostEditUnauthorized = 27,
        ManyTransactions = 28,
        CommentLimit = 29,
        CommentEditLimit = 30,
        CommentScoreLimit = 31,
        Blocking = 32,
        Size = 33,
        InvalidParentComment = 34,
        InvalidAnswerComment = 35,
        DoubleCommentEdit = 37,
        SelfCommentScore = 38,
        DoubleCommentDelete = 39,
        DoubleCommentScore = 40,
        OpReturnFailed = 41,
        CommentDeletedEdit = 42,
        ReferrerAfterRegistration = 43,
        NotAllowed = 44
    };

    class BaseConsensus
    {
    public:

        BaseConsensus(int height)
        {
            Height = height;
        }

        virtual ~BaseConsensus()
        {

        }

    protected:
        int Height = 0;
        virtual int CheckpointHeight() { return 0; };
    private:
    };
}

#endif // POCKETCONSENSUS_BASE_HPP
