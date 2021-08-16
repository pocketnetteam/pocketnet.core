// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/CheckpointHelper.h"

using namespace PocketHelpers;

LotteryCheckpoints::LotteryCheckpoints()
{
    _checkpoints.emplace(355728, "ff718a5f0d8fb33aa5454a77cb34d0d009a421a86ff67e81275c68a9220f0ecd");
    _checkpoints.emplace(355729, "e8dd221233ef8955049179d38edd945f202c7672c08919a4dcca4b91b59d1359");
    _checkpoints.emplace(355730, "9b9aa4c3931a91f024a866a69f15979813b3d068a1545f3a96e9ef9e2641d193");
    _checkpoints.emplace(355735, "5483eb752c49c50fc5701efcb3efd638acede37cc7d78ee90cc8e41b53791240");
    _checkpoints.emplace(355736, "0024dd9843b582faa3a89ab750f52cb75f39f58b425e59761dec74aef4fce209");
    _checkpoints.emplace(355737, "237b66bbf8768277e36308e035e3911a58f9fbac1a183ba3c6bbd154b3cb51f6");
    _checkpoints.emplace(357329, "e2b1cf6b6769f9191dc331cb5161ebadd495983d49e7823ab5e2112631245120");
    _checkpoints.emplace(357459, "dbc4632a7dc3a05513c232f8d8fbd332ae16ea9eefd2842d8d965bb1001f15ea");
    _checkpoints.emplace(357507, "e0b4739f2ccbf485b11e14288d9ed000d4de2c9a3c2fe472c7bbff925244475f");
    _checkpoints.emplace(357549, "0bf85d1a73b6f69af4f8e9c2553231988c643339795eacd47f7a6a63d616c73a");
    _checkpoints.emplace(357555, "ff4af52df4d96762a36943f976b94ba20f5e1d3636b95b81dca39b11fdb89452");
    _checkpoints.emplace(357636, "00e2f7a68a1719271aa01450dcc98811a88a6e72576aa96f4aad9863b8df0ba2");
    _checkpoints.emplace(357835, "3a4f7a7df6f25a27f6c556810366f78da8aa16011a66959ab7c13f1372457103");
    _checkpoints.emplace(360029, "77326a586bfde7750e5fc942d6a2932acfdf9c9250381cc22026ed72ddce7f75");
    _checkpoints.emplace(360175, "44f7d16d8b76ac547faf1a63a99ac3086898bae9c031950987192c46ea27d735");
    _checkpoints.emplace(361934, "b72d9ec3712be053b07ae8e7ce55df739f543d16eac11f19de4f11ccb1e3540c");
    _checkpoints.emplace(361945, "17d68d65f8d2b9c5f012ca50aa22d292e2c6d2390a1acde7a491605aece66205");
    _checkpoints.emplace(361947, "ae0bec9a7200811f0761242e71b33add2a2a181fc00e903dfa06950bdb0c221a");
    _checkpoints.emplace(361955, "092fe8d9d260dc9e301210dc155ca7594d6284fee1d339c8d5ec3b85f1c72c0f");
    _checkpoints.emplace(365473, "66fbf8c4ee51058b6cbc75ed02c215db7dd9651aed305e16beffd044de7f1712");
    _checkpoints.emplace(365474, "32176e745a8013b260afbe4168007a25d8e6465749573a2f794df1edd82a1056");
    _checkpoints.emplace(366215, "c4e12f070c48c88f0738dbbdf918d4705f2b2b7827cccde019f03abf7bb85eb0");
    _checkpoints.emplace(366426, "228d7b995324642b31841428c27033cb3748fe17851ac8432a14f466b21ac15d");
    _checkpoints.emplace(366455, "d8594e1e14b591ab1fff611cd2d402d44d23711a3739e65809e3786e759aef93");
    _checkpoints.emplace(369165, "f4022299ec6890b71a16fa41a7ce4d9411ae88135eb9700d4257f5754b4a08e6");
    _checkpoints.emplace(369609, "e96c7ab86d96f3f157b08cccfa2641b17af1fe13226826b31981d184af0f2d6a");
    _checkpoints.emplace(369682, "d39b34fe1db359c2c2f47900b42a52327ae636b44f17436d7edf59419f240756");
    _checkpoints.emplace(370664, "97fe021d7c99ac0029adf25650fe74dd1f45c26a8be3185f7084c36a7505a320");
    _checkpoints.emplace(370692, "743b543e168739670b01446ea3ecf23cf59ff340fe6611fe1c9cab12923c3de2");
    _checkpoints.emplace(372179, "6ff637108b23435971b7aea4fe9e0a7c564581433d03c7d889ef6c9b6b034500");
    _checkpoints.emplace(372180, "e6cb167746d786dd2c84ef08e7fc9e61dae6157445fdbaad128b402984393bc7");
    _checkpoints.emplace(372426, "1030210c393e2f8e0f841c6a9547e623d78b35b11b7fc19c9c77aeefb6a42e4d");
    _checkpoints.emplace(373166, "9bce5b36493eeeeb40a64c3702f7d341eaeea3e70e3e3432646770d448f484e6");
    _checkpoints.emplace(373168, "6be5138294b706b06d86b90ac19458443e2244b0d60e59a700eff4a5076cea52");
    _checkpoints.emplace(373259, "a3f0a10b0c0733a35f2bb00da68562598b45745844227ded618c9eb723e524e6");
    _checkpoints.emplace(373605, "5a7d146d7535d17b0067123ccc644cf4dc617fc2ec99b47634d9989a473796f5");
    _checkpoints.emplace(373639, "d4dbd76656936416ab1cc6584c9517a9f54aa77897792996f1f1352a8dce63f8");
    _checkpoints.emplace(374456, "7e93f9259e9a1ce463539446d61940582329789fa804337398a202cd1a0a6f57");
    _checkpoints.emplace(374458, "01d10e315928bb5619341984e2c91e304433b0c2facc1f965004a36abf545a21");
    _checkpoints.emplace(374726, "52f89b6208c77f41078f3ec184cc0a4b670d5e4466da347610fb76db70f7515e");
    _checkpoints.emplace(374730, "5e909ecf11c8e0e13d4e62ace01e37905851ea5388fb21041481608e111ce7fe");
    _checkpoints.emplace(374820, "b1166a1673140ce85ad6934b1e170f7596f8cb97033d434b61004d165fdade29");
    _checkpoints.emplace(374861, "cfab349c68a98b8f6cf8dd1cb83e09bf140e72cf3ab48559df75c0f0ae5a5d85");
    _checkpoints.emplace(374944, "58260bfe434629407acd72f2dee8c48984dddcf137eaf8933290c5ed26529052");
    _checkpoints.emplace(374965, "a4f01af1686bb8502704a2f199ad23da1b4b54f190b81a99fec2a7773df2abaa");
    _checkpoints.emplace(376642, "d41502757d49a0a67cb76218de5de21c76250fa3464533799e80469b014b664e");
    _checkpoints.emplace(376900, "0cb8967e40fb6549b505abc03092b0d77bfa4b67b11a191e5e78b3462aab2d15");
    _checkpoints.emplace(376901, "78ea617182fb5f5a80ccdb0b44907430726b30ba48a17d11279558645213c26b");
    _checkpoints.emplace(377678, "97a23982034342bb797323a82ef0f2ec45317a95772267fc4d410d1e1671c147");
    _checkpoints.emplace(377729, "c945f861dcd7091de0e2bd4e0e487cded91f9eaf5565002ce9a4341cb9bc9d09");
    _checkpoints.emplace(378278, "08b8617d9b14e629f6faab155974aa4c7900e816a2a81bdce7e223660f93f798");
    _checkpoints.emplace(378308, "2e805cd488c4c3d708eac063ec3027c549e3cbdc1939c1b52f50f9ce75ef3ca0");
    _checkpoints.emplace(378966, "3b3f38f5527adf9f94e1bb3600aec082691eea6198eead71316ddd6a00f29118");
    _checkpoints.emplace(382536, "46d5831c2f7e3ac1e719db7909e77d3f5aba1415931b851ad50768e97c54391a");
    _checkpoints.emplace(382538, "fdb0b23b12cd567dfa539061b5cc8d93dfad8db2d6a04daff64d0368bd13c0cc");
    _checkpoints.emplace(382539, "fad01113fa7674b8a696efe7b2484508148ebce4fc711b0c015afd7381bc1565");
    _checkpoints.emplace(382544, "17c3f0d199d61e0175880cf6412c48d320dad6bdf991019f5c6ec9d37f167c1a");
    _checkpoints.emplace(382554, "6a89c2d8eba914938a593f8612f52565aa4d603fc191ca31e25a0f2a9ebe9666");
    _checkpoints.emplace(382625, "0220bea6bce2e012cf12e6c53d37f9ea0eafb22b35691dd785366997dc2ea5b8");
    _checkpoints.emplace(388191, "69591ff21896135332a9f4b84fcbfe5da1bfc209bda10aeaec08c59a7ad06ef6");
    _checkpoints.emplace(388192, "b24b0cf25679b0d43cb4014ed874fa07e73c597c332b328d6bd2314b81757ec5");
    _checkpoints.emplace(388371, "3b790f075fe0a7670e7907b1b04169de7691e22be678bdff2204ff4375c34cea");
    _checkpoints.emplace(413599, "2383eff8a73d146ca7fca1472b352adf7fc642d40a0600dc99fe273ab7e0c242");
    _checkpoints.emplace(413613, "5d906b2dc460c935b18b4ef8fc248df576d7a794eed1c49092f90d8a8ec0866a");
    _checkpoints.emplace(413635, "723a3eb018307912220582bf1271d459c7d0401aec0a7836a4b8858620ce022d");
    _checkpoints.emplace(413662, "1b4fa351d7e89b659daf9b9fd8f4e0c9cdfaf7b9b393b61e32fb4cee55b980cc");
};

