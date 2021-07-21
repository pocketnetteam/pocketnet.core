// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/CheckpointHelper.h"

using namespace PocketHelpers;

SocialCheckpoints::SocialCheckpoints()
{
    _checkpoints.emplace("d2787346ff8c37e3a3cc428689b757b3550b6ea608b15cddd5554d1e9dac2531", SocialConsensusResult_ChangeInfoLimit);
    _checkpoints.emplace("30064229865e5f18d51725e3c3339facae0bfecd35b0e5a92e9dc1a168748439", SocialConsensusResult_ChangeInfoLimit);
    _checkpoints.emplace("45ad6c18c67717ab02c658e5153cbc97e3e7be9ed964739bb3a6181f89973b52", SocialConsensusResult_ChangeInfoLimit);

    _checkpoints.emplace("8957df7bf252b54a73920fe6a5f78c9abcd8c084d8c3e43b3b0bc0084340802f", SocialConsensusResult_ManyTransactions);
    _checkpoints.emplace("81febe8bf55791717e71ba88f9a80f7559aded019d26b164c7afae0ba6f2fc53", SocialConsensusResult_ManyTransactions);
    _checkpoints.emplace("34d3db603ac0ba65960a3ef2508967ab4a9fc5e55a3c637af7aa50580a0f32dd", SocialConsensusResult_ManyTransactions);
    _checkpoints.emplace("812fca4ea40a58ff99fcd96fefd307f33c4daef4bc13918e7a82a5b92720c2f6", SocialConsensusResult_ManyTransactions);

    _checkpoints.emplace("f7b3a4a6c9c216f72e82cefcc11da9a747cc4980a690efa0d51b6a28d134d46f", SocialConsensusResult_ManyTransactions);
    _checkpoints.emplace("d53997af512427f4cef0a709fde76e28478f225d0219e2c29c31947efbce7f5b", SocialConsensusResult_ManyTransactions);
    _checkpoints.emplace("3efb8d2195dc8e180110cadd8b31025c1246d7b18ac0e78ec035151a363a6e4f", SocialConsensusResult_ManyTransactions);

}