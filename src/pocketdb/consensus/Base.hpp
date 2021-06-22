// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BASE_HPP
#define POCKETCONSENSUS_BASE_HPP

#include "univalue/include/univalue.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.hpp"

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
        SocialConsensusResult_Success = 0,
        SocialConsensusResult_NotRegistered = 1,
        SocialConsensusResult_PostLimit = 2,
        SocialConsensusResult_ScoreLimit = 3,
        SocialConsensusResult_DoubleScore = 4,
        SocialConsensusResult_SelfScore = 5,
        SocialConsensusResult_ChangeInfoLimit = 6,
        SocialConsensusResult_InvalideSubscribe = 7,
        SocialConsensusResult_DoubleSubscribe = 8,
        SocialConsensusResult_SelfSubscribe = 9,
        SocialConsensusResult_Unknown = 10,
        SocialConsensusResult_Failed = 11,
        SocialConsensusResult_NotFound = 12,
        SocialConsensusResult_DoubleComplain = 13,
        SocialConsensusResult_SelfComplain = 14,
        SocialConsensusResult_ComplainLimit = 15,
        SocialConsensusResult_LowReputation = 16,
        SocialConsensusResult_ContentSizeLimit = 17,
        SocialConsensusResult_NicknameDouble = 18,
        SocialConsensusResult_NicknameLong = 19,
        SocialConsensusResult_ReferrerSelf = 20,
        SocialConsensusResult_FailedOpReturn = 21,
        SocialConsensusResult_InvalidBlocking = 22,
        SocialConsensusResult_DoubleBlocking = 23,
        SocialConsensusResult_SelfBlocking = 24,
        SocialConsensusResult_DoublePostEdit = 25,
        SocialConsensusResult_PostEditLimit = 26,
        SocialConsensusResult_PostEditUnauthorized = 27,
        SocialConsensusResult_ManyTransactions = 28,
        SocialConsensusResult_CommentLimit = 29,
        SocialConsensusResult_CommentEditLimit = 30,
        SocialConsensusResult_CommentScoreLimit = 31,
        SocialConsensusResult_Blocking = 32,
        SocialConsensusResult_Size = 33,
        SocialConsensusResult_InvalidParentComment = 34,
        SocialConsensusResult_InvalidAnswerComment = 35,
        SocialConsensusResult_DoubleCommentEdit = 37,
        SocialConsensusResult_SelfCommentScore = 38,
        SocialConsensusResult_DoubleCommentDelete = 39,
        SocialConsensusResult_DoubleCommentScore = 40,
        SocialConsensusResult_OpReturnFailed = 41,
        SocialConsensusResult_CommentDeletedEdit = 42,
        SocialConsensusResult_ReferrerAfterRegistration = 43,
        SocialConsensusResult_NotAllowed = 44,
        SocialConsensusResult_AlreadyExists = 45,
    };

    enum AccountMode
    {
        AccountMode_Trial,
        AccountMode_Full
    };

    // --------------------------------------------------------------------------------------

    class BaseConsensus
    {
    public:

        BaseConsensus() { }

        BaseConsensus(int height)
        {
            Height = height;
        }

        virtual ~BaseConsensus() { }

    protected:
        int Height = 0;
        virtual int CheckpointHeight() { return 0; };
    private:
    };
}

#endif // POCKETCONSENSUS_BASE_HPP
