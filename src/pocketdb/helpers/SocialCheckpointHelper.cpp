// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/CheckpointHelper.h"

using namespace PocketHelpers;

SocialCheckpoints::SocialCheckpoints()
{
    // _checkpoints.emplace("d2787346ff8c37e3a3cc428689b757b3550b6ea608b15cddd5554d1e9dac2531", SocialConsensusResult_ChangeInfoLimit);
    // _checkpoints.emplace("30064229865e5f18d51725e3c3339facae0bfecd35b0e5a92e9dc1a168748439", SocialConsensusResult_ChangeInfoLimit);
    // _checkpoints.emplace("45ad6c18c67717ab02c658e5153cbc97e3e7be9ed964739bb3a6181f89973b52", SocialConsensusResult_ChangeInfoLimit);
    //
    // _checkpoints.emplace("a925f4e6bcfe70199692e581847e7fe75f120b70fffe4818eadf0b931680c128", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("c71c1fa853d7cd21f39e03d4c89a3b54254d9795d3dfa2ff8317706b9e1145f2", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("37e59e5a5f4344d4f86853afae6c54bbccf5ccb3b96d9949f9ea1476aa766684", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("5a4d4aac2235262d2776c4f49d56b2b7291d44b4912a96997c5341daa460613b", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("2a3b57f9e65ba9a6916892bb886ba312600cf78e8286e28f126d5677fa244586", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("d8bb6ca1fd5e9f6738f3678cd781d0b52e74b60acbcf532fb05036f5c9572b64", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("51f1b71ee98c2181d0f5d4d091d930927f9f4fa0fbd7867a1437ad0e59e79e66", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("ab686cb7bc2b5385f35f7c0ab53002eae05d0799e31858fc541be43a6f0c293b", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("31f7dc35ecade4ccb4374fe131f673b5fb8d982fc3a471a0d754b1a54f7f0411", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("32878652bb50ea610e07513d7976d7ad507e5bf153c61cb6d76c7cf8117fa5f0", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("6ece61109e730bed8bd543e557d5c1cb143a2661bf11ada473875efe7d7e3d51", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("bdd4db4904edc131c19348843313f3e9869ba3998b796d03ce620e1cbaa5ecee", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("6778f1b9a502d128139517285dc75a11650b1587e78def91cb552299970dd9cc", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("7e6a136358d87a86f7de17e700ef10a2a35ac7a604fb2b8d2dabbecdb98a2653", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("bcfddbffe205904638bca63c1cac44f54afec8ed3aedba527eafea8049370b0a", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("da9040c6257f4d7021889ddffad3b1513be1c000fd219b833ef13cc93d53899b", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("2b53e7fce1965bd9409de61ff8c47c600d6fee82f5ba5bc9c70ce97d72305a78", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("bfccdd1ff1a948583c3316d19b8a05feb245717d16d9e82f57d8f01ab3c2d2ff", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("7fcc2788bb08fb0de97fe4983a4cf9c856ce68a348d6cdb91459fe7a371f90f8", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("d7c5b2bbf4e0bd39bdc54e773ac0c731f155c957c15790b838372dffb2494bfd", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("0a24721f17f400959eda79d2ca9b6e4edd47ecce9267e908649dd3ceb7b497c5", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("7321445c16fe641c56606d377a29a24d117f2849197bd7bd32054e906401fb8e", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("8957df7bf252b54a73920fe6a5f78c9abcd8c084d8c3e43b3b0bc0084340802f", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("81febe8bf55791717e71ba88f9a80f7559aded019d26b164c7afae0ba6f2fc53", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("34d3db603ac0ba65960a3ef2508967ab4a9fc5e55a3c637af7aa50580a0f32dd", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("812fca4ea40a58ff99fcd96fefd307f33c4daef4bc13918e7a82a5b92720c2f6", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("812fca4ea40a58ff99fcd96fefd307f33c4daef4bc13918e7a82a5b92720c2f6", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("f7b3a4a6c9c216f72e82cefcc11da9a747cc4980a690efa0d51b6a28d134d46f", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("d53997af512427f4cef0a709fde76e28478f225d0219e2c29c31947efbce7f5b", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("3efb8d2195dc8e180110cadd8b31025c1246d7b18ac0e78ec035151a363a6e4f", SocialConsensusResult_DoubleSubscribe);
    //
    // _checkpoints.emplace("aedcd75c2ebae534537f6064ea43d7abb75230eed5db108229465ae8bd268142", SocialConsensusResult_DoubleComplain);
    // _checkpoints.emplace("fc0fa992b8c3c87107da9db7843beaafd255b682cd0466402d37a2a0631fc464", SocialConsensusResult_DoubleComplain);
    // _checkpoints.emplace("74b1c56b84392357f331738d6ab1a63830e3b1bf54c6725851346f1bf92c3dfa", SocialConsensusResult_DoubleComplain);
    // _checkpoints.emplace("c5ba57092a0098fe2c3302ccbb40bc250eec6b8ec12d807b16d66415fe49ffa6", SocialConsensusResult_DoubleComplain);
    // _checkpoints.emplace("f0d173b068d42818c071dfc15530a236f12db54cd89ad57f08dbf46e51a61ac5", SocialConsensusResult_DoubleComplain);
    // _checkpoints.emplace("1cdb6616f2546b10419082962a2c8414d84818618c5b2eeddb53c0b168ca9eab", SocialConsensusResult_DoubleComplain);
    // _checkpoints.emplace("3a15656e5e67fda68a3412f682c5ccfef3510a4007c2d7e69fd7b4c748b2b3ee", SocialConsensusResult_DoubleComplain);
    //
    // _checkpoints.emplace("06ec9b7ee7b89dd71312b7c9c9220730ed025af661c29504620bbd4d4e1fb093", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("08aa6a715e644abfe47f9623bdc4a568b6edb0432e1bf5b59a2cee7565c81226", SocialConsensusResult_DoubleSubscribe);
    // _checkpoints.emplace("87e63741701602706bee96257259054030b8f91c4fcab849aa1aac33cab5d692", SocialConsensusResult_DoubleSubscribe);
    //
    // _checkpoints.emplace("42c9efb79919350c3a19c1988c17fab5d8c911250856c215eeea1ccf004eb012", SocialConsensusResult_InvalideSubscribe);
    //
    // _checkpoints.emplace("5dbb6b1d74de979dad1ae56c8bd368ce0309611bd508cf111119dcbb92582f9d", SocialConsensusResult_NotFound);
    
}