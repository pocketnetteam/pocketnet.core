#ifndef SRC_SOCIAL_CONSENSUS_H
#define SRC_SOCIAL_CONSENSUS_H

#include "pocketdb/consensus/social/Blocking.hpp"
#include "pocketdb/consensus/social/BlockingCancel.hpp"
#include "pocketdb/consensus/social/Comment.hpp"
#include "pocketdb/consensus/social/CommentEdit.hpp"
#include "pocketdb/consensus/social/CommentDelete.hpp"
#include "pocketdb/consensus/social/Complain.hpp"
#include "pocketdb/consensus/social/Post.hpp"
#include "pocketdb/consensus/social/ScoreComment.hpp"
#include "pocketdb/consensus/social/ScoreContent.hpp"
#include "pocketdb/consensus/social/Subscribe.hpp"
#include "pocketdb/consensus/social/SubscribeCancel.hpp"
#include "pocketdb/consensus/social/SubscribePrivate.hpp"
#include "pocketdb/consensus/social/User.hpp"
#include "pocketdb/consensus/social/Video.hpp"

namespace PocketConsensus
{
    extern BlockingConsensusFactory BlockingConsensusFactoryInst;
    extern BlockingCancelConsensusFactory BlockingCancelConsensusFactoryInst;
    extern CommentConsensusFactory CommentConsensusFactoryInst;
    extern CommentEditConsensusFactory CommentEditConsensusFactoryInst;
    extern CommentDeleteConsensusFactory CommentDeleteConsensusFactoryInst;
    extern ComplainConsensusFactory ComplainConsensusFactoryInst;
    extern PostConsensusFactory PostConsensusFactoryInst;
    extern ScoreCommentConsensusFactory ScoreCommentConsensusFactoryInst;
    extern ScoreContentConsensusFactory ScoreContentConsensusFactoryInst;
    extern SubscribeConsensusFactory SubscribeConsensusFactoryInst;
    extern SubscribeCancelConsensusFactory SubscribeCancelConsensusFactoryInst;
    extern SubscribePrivateConsensusFactory SubscribePrivateConsensusFactoryInst;
    extern UserConsensusFactory UserConsensusFactoryInst;
    extern VideoConsensusFactory VideoConsensusFactoryInst;

    extern PostConsensusFactoryTest PostConsensusFactoryInstTest;
}; // namespace PocketConsensus


#endif //SRC_SOCIAL_CONSENSUS_H
