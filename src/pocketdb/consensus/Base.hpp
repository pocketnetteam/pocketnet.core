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
        SCR_Success = 0,
        SCR_NotRegistered = 1,
        SCR_PostLimit = 2,
        SCR_ScoreLimit = 3,
        SCR_DoubleScore = 4,
        SCR_SelfScore = 5,
        SCR_ChangeInfoLimit = 6,
        SCR_InvalideSubscribe = 7,
        SCR_DoubleSubscribe = 8,
        SCR_SelfSubscribe = 9,
        SCR_Unknown = 10,
        SCR_Failed = 11,
        SCR_NotFound = 12,
        SCR_DoubleComplain = 13,
        SCR_SelfComplain = 14,
        SCR_ComplainLimit = 15,
        SCR_LowReputation = 16,
        SCR_ContentSizeLimit = 17,
        SCR_NicknameDouble = 18,
        SCR_NicknameLong = 19,
        SCR_ReferrerSelf = 20,
        SCR_FailedOpReturn = 21,
        SCR_InvalidBlocking = 22,
        SCR_DoubleBlocking = 23,
        SCR_SelfBlocking = 24,
        SCR_DoublePostEdit = 25,
        SCR_PostEditLimit = 26,
        SCR_PostEditUnauthorized = 27,
        SCR_ManyTransactions = 28,
        SCR_CommentLimit = 29,
        SCR_CommentEditLimit = 30,
        SCR_CommentScoreLimit = 31,
        SCR_Blocking = 32,
        SCR_Size = 33,
        SCR_InvalidParentComment = 34,
        SCR_InvalidAnswerComment = 35,
        SCR_DoubleCommentEdit = 37,
        SCR_SelfCommentScore = 38,
        SCR_DoubleCommentDelete = 39,
        SCR_DoubleCommentScore = 40,
        SCR_OpReturnFailed = 41,
        SCR_CommentDeletedEdit = 42,
        SCR_ReferrerAfterRegistration = 43,
        SCR_NotAllowed = 44
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
