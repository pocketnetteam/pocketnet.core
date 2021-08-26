// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/CheckpointHelper.h"

using namespace PocketHelpers;
using namespace PocketTx;

SocialCheckpoints::SocialCheckpoints()
{
    _checkpoints.emplace("30064229865e5f18d51725e3c3339facae0bfecd35b0e5a92e9dc1a168748439", tuple(ACCOUNT_USER, SocialConsensusResult_ChangeInfoLimit));
    _checkpoints.emplace("45ad6c18c67717ab02c658e5153cbc97e3e7be9ed964739bb3a6181f89973b52", tuple(ACCOUNT_USER, SocialConsensusResult_ChangeInfoLimit));
    _checkpoints.emplace("d2787346ff8c37e3a3cc428689b757b3550b6ea608b15cddd5554d1e9dac2531", tuple(ACCOUNT_USER, SocialConsensusResult_ChangeInfoLimit));
    _checkpoints.emplace("f7b3a4a6c9c216f72e82cefcc11da9a747cc4980a690efa0d51b6a28d134d46f", tuple(ACCOUNT_USER, SocialConsensusResult_Failed));
    _checkpoints.emplace("d53997af512427f4cef0a709fde76e28478f225d0219e2c29c31947efbce7f5b", tuple(ACCOUNT_USER, SocialConsensusResult_Failed));
    _checkpoints.emplace("3efb8d2195dc8e180110cadd8b31025c1246d7b18ac0e78ec035151a363a6e4f", tuple(ACCOUNT_USER, SocialConsensusResult_Failed));
    _checkpoints.emplace("2da9324dc57c9af92fa4b5ee1f7f0ecace0407e30db65fb13d9faecc43e46938", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("4cf6e53c78f0738a20266562c76b12e12951b87cc0571b280be118843bd30066", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("dedcb086b48c09d4f0083a64859ad1aa2f4d1dc91fcb3d34b5136b50cd09a127", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("d7e8b58a824b6655990a96e16e182e4951635dfe59065d176a55afba42b7e9ed", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("62e32d7edc098b6042377dab0f88d19e837d046bf729cf621c82396ed1b381b3", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("49bcd806b5a19e36f48b8b1d2fd87672f4cf54bca9017682fdcc9b77669bb26d", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("71f06d7118bcaff282b4ff5b2b45570c028595e45bce1e8c5c1d7e96c05531ac", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("e918fea2aa1956127fee9b5d5ddad40eb29d8966583c2e4c834985eed49fd14b", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("16b4413b9e9f7b6751535536d2d4f79320e676c3dce6a51d2a025d5afd914777", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("b7ca6990b7a194b00dbdef427e4e5066493d495a04b08d7215c7535ab70ba5e6", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("9229b3c2d002cbf4bd9fc79037cba61067ab3a2ae1e423bce194fc14738d9a98", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameLong));
    _checkpoints.emplace("45ad6c18c67717ab02c658e5153cbc97e3e7be9ed964739bb3a6181f89973b52", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameDouble));
    _checkpoints.emplace("caba0de8cff0173c3777896a79bfdf4d7227b8dcfb9f10df249601e6c148c436", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameDouble));
    _checkpoints.emplace("aba1c17900f74fb09ce09316ede13927c292172fe6aceabf0ef3638309f715b7", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameDouble));
    _checkpoints.emplace("b11aaacd180160aba063337c6e2b8feed33bcc37cbf9ef9f054f47ac534579a8", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameDouble));
    _checkpoints.emplace("5645011f36792de285fba0b588c0c77c3186bece5b7421749b6efb122c230a8b", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameDouble));
    _checkpoints.emplace("777e9f4c4622865590ec5a092cbef0a1878cd8a102d59579e3c48f2a716fe827", tuple(ACCOUNT_USER, SocialConsensusResult_NicknameDouble));
    
    
    _checkpoints.emplace("34d3db603ac0ba65960a3ef2508967ab4a9fc5e55a3c637af7aa50580a0f32dd", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("812fca4ea40a58ff99fcd96fefd307f33c4daef4bc13918e7a82a5b92720c2f6", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("81febe8bf55791717e71ba88f9a80f7559aded019d26b164c7afae0ba6f2fc53", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("8957df7bf252b54a73920fe6a5f78c9abcd8c084d8c3e43b3b0bc0084340802f", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("a925f4e6bcfe70199692e581847e7fe75f120b70fffe4818eadf0b931680c128", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("c71c1fa853d7cd21f39e03d4c89a3b54254d9795d3dfa2ff8317706b9e1145f2", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("37e59e5a5f4344d4f86853afae6c54bbccf5ccb3b96d9949f9ea1476aa766684", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("5a4d4aac2235262d2776c4f49d56b2b7291d44b4912a96997c5341daa460613b", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("2a3b57f9e65ba9a6916892bb886ba312600cf78e8286e28f126d5677fa244586", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d8bb6ca1fd5e9f6738f3678cd781d0b52e74b60acbcf532fb05036f5c9572b64", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("51f1b71ee98c2181d0f5d4d091d930927f9f4fa0fbd7867a1437ad0e59e79e66", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("ab686cb7bc2b5385f35f7c0ab53002eae05d0799e31858fc541be43a6f0c293b", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("31f7dc35ecade4ccb4374fe131f673b5fb8d982fc3a471a0d754b1a54f7f0411", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("32878652bb50ea610e07513d7976d7ad507e5bf153c61cb6d76c7cf8117fa5f0", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("06ec9b7ee7b89dd71312b7c9c9220730ed025af661c29504620bbd4d4e1fb093", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("6ece61109e730bed8bd543e557d5c1cb143a2661bf11ada473875efe7d7e3d51", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("bdd4db4904edc131c19348843313f3e9869ba3998b796d03ce620e1cbaa5ecee", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("6778f1b9a502d128139517285dc75a11650b1587e78def91cb552299970dd9cc", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("7e6a136358d87a86f7de17e700ef10a2a35ac7a604fb2b8d2dabbecdb98a2653", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("bcfddbffe205904638bca63c1cac44f54afec8ed3aedba527eafea8049370b0a", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("da9040c6257f4d7021889ddffad3b1513be1c000fd219b833ef13cc93d53899b", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("2b53e7fce1965bd9409de61ff8c47c600d6fee82f5ba5bc9c70ce97d72305a78", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("7fcc2788bb08fb0de97fe4983a4cf9c856ce68a348d6cdb91459fe7a371f90f8", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("bfccdd1ff1a948583c3316d19b8a05feb245717d16d9e82f57d8f01ab3c2d2ff", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d7c5b2bbf4e0bd39bdc54e773ac0c731f155c957c15790b838372dffb2494bfd", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("0a24721f17f400959eda79d2ca9b6e4edd47ecce9267e908649dd3ceb7b497c5", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("7321445c16fe641c56606d377a29a24d117f2849197bd7bd32054e906401fb8e", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("77909ad71bf0fd849a8d7a834becca464bca1314d825eb33cf81619d462dd98b", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("dcd07908e6eb4bcd681de36887f7c824a62b6e1ccd69467fa913b8d4742ba62c", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("362aaa7490398bc60f055ec8d9dcff37c44112fa410d299f5b87860e081d1325", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("cbf99c5c5073ccba798222af117984d304b30360d0397d32af45f52c49c5cb8c", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("ebb13581dd746eac25b3a55d1beecafd71896c7cc5535671e737e99b0e9ba274", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d8c44fcb85c9b8ebb476e8751db4e822c3dfed20e31ac45532a78cffd200b27b", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("c4e2e4361c6c90648cb1304d77d5d8ea6f2ad594e7d8b06ce60e64fa0334085e", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("18931dced842bcc993cc1d4ce783c3fb962028b799f434c6e6189f226a40f48b", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("71065aebc67fac14f1fd4532754fd723bd9cd987b86cf2c978c6ad5031624b06", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("86b11f23997a3c63cf4fbdb21552a006d20acc058199f99c29a30393f261bba5", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("fe8ee70e122bf6083f65351b79ab1478ae9861cd5e2e359026723e0922458337", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("122ed2885ef694dc036c4b00a946703d67b439c26755540226460a4e468b0262", tuple(ACTION_SUBSCRIBE, SocialConsensusResult_DoubleSubscribe));

    _checkpoints.emplace("f00f8d35c512ab282130ad8609d4ffa01f3a67011230ecdd903dfe12abde5fcf", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("9ec4404eaf8d3e16e5b02da9e3ec468e3d471cb5b3632c31fcc7388544e6e3cc", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("51effdd1aa5512992b65441161e0c8fded0c24d781480b07bd2300935a89a0c1", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("53ee5afe1edafdeca13f9dadcbcc6fc29b64c365a767691a23b4d914a018a31c", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("a205fa0c8f0b38c78349662c13fdd264f3069c75741551f40d805e0f42e44588", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("749e78a8b5850aa848e957abfdbf100e59a1942629ed49da15d68706c400e847", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("5d0eeb996443c3bfd67cf96c6d0d23912d99c6d25c277d081942625b07a70b6c", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("3393338a971b679df979141f7653aeda7c090df469130d806ff9ea391dda85d8", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("9e890d72a2935e88a8a67f7de0ab7d1cee5603b7cef1550030ac656c4ba87987", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("cfb32596f74c90ed497f729ca8ccd1c5a0373fd792e19227ddbccf5c605b77bd", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("34d87a4e288751fa42416210d884eb96f2391def2a4849e3fc5a874e23d00301", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("f18a28e77a07c342f7988d917b835f0bc6094a5696d8cf24849467c051608f28", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("1d84969a3f9fd948a3bd3cabb5881013155865361221668c367d419004b345bd", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("68e3a4dc6b2d2e27c97983754ce2d8168794c44b3855ede9ef45115574f0e5a3", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("9679ac3dac5d29f5cef3913fb72a2b93dc8d9421118ab8931a7532163b92979d", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d167814ba72ef9045b2d07e90f42902b1ed7009f5f634f8435c8a7895aa4504a", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("2d17c244ad606541326dfcd221e0807079e2d4b57a1943ca215119e108578762", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("c0cf10129f4d3e315345f91579761a6dddfca79640914cf99b084f2679da3092", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("698f709485ba44e537023dbb385b9e6d5efdfd134264428ecf82fbcbcf8fb655", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("2a7d6a597f0f6d1dcd1a6f624348f105e49025ace4c8e7f2e8d80f3a649fd9eb", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("819db1858bcccc019368a8abe722dbcf5fced2241d2395224a29ca078f29ed11", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("1d1f62bce36d43bb3d7054282a0bfdc082a7c4198df16f7c37ccbbf466a4532f", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("94fa10c2de9e9ae862309beb9fc7128fb3f313b19c2ff389eba543e0e5cc7b00", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("b1a635675cf1cec31680aa04ddf90d4246278a3659fb31defcf631f8dcb9fa37", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("a841baa175f2c8d0b152e8e09e43357dc6cacbeaeae8f3a192532832b590fcc0", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("edebb4af73407face7bfbca6d2c67b5b28e7f101f6920c223b7d4f1b68ccc8ca", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("4fd2022604439f6ad0a518de09eae9cbe1e410f4ae20114ea497caa9d28c470c", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d6b21451f035f2dcb435446daaeab472a465ba78965f3b40bf60728a6bbac34e", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("8fa0f2aa3f70b06bf455746f3cbc7e5e2540d9a412237d6fd9a1c215dd2722f3", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("ba667ac80cf623f5603b3b1b238b74938bdcae12ab897fd170e77e28948e6516", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d5fc11cba672e30a500afb4dc40e83b7ed072acd158e289819db6b9db435c383", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("255fbd900102be30b7db16028463707fb35b9e50d8d76052c8f9aa805e8a9e55", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("69c205e48c21a3c70bff62b12c99a51baa69d601d7c7e66daa2941f9b9e99293", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("375c68fbe4d70ff7f779c94ece57790f7d8abeadc532033a568a931d917f5c66", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("b07615f5fd1e20008df1aa2dc11808dba7a3676d3b28346c1fc50c70ad61c31a", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("e7ac37bfc229ce6bcad948a98030a95be6b5929d718d4efcdefbfea428789010", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("ce7da6823ed58784003d4c418dca892d156e8ee5b5f36a76cafdd48cb50861d5", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("e48505877db1563304523d13a5057922a8adc9d0c8aaa4f488b56e18f318545e", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("344fa787784b31b2bec634c42e1d2ec9615e5e2bab5355683bbbba2f744f7168", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("24fb0d85598d46c804c0bbb9092ab46ad0cb4f8a4bbc3819cb88a99c35ba64fa", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("08c412a092d35c17f40b4e675f72255212d8b49e379645ff993d6549df97b505", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("68bc985be29e3e48b3f0fc3d128ef0e7534da63682290ba8b45d34cb020ae6e7", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("2018a6478808b3a6c578b6269c6e1e58673a7a984d14802eb07b5fb01e246ec5", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("545279072e1907993513bc38522de2c69de5a7de68a9c2bd37e12b17de1059a1", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("7820c18503a4a508d676e108266aa18a7cdd9901aded81818b6ea03fb99a4892", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("0c7ba9e40c9ec21ad77ce39d5d86da045d59bcb433d5ab9528b76ceb6035312c", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("cb77b148452771b90f99d7cc55cbad842a93badbbff524a6759cef8d156ea83a", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("d087a49897ef7acaca71eb8806da91cb987b4731ef6b6dee5d517b5ae8f85b9c", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("00df9fa2c2e26477fc5c5c9d06cdbceced7e46b1eecf51e40418ad7fedc45722", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("71f9a64b030a9f87f75e41f7d06e3c673ac5744fd63a456dfc517e38ac7c24f6", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("67222c3bd4168151d1cf7a21af62c4e64662e990ab15476fa66a247e6adec8ae", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("948b6790a7f36c5be1ed230e818be5aa5c2d130605d797d11463074615762912", tuple(ACTION_SUBSCRIBE_PRIVATE, SocialConsensusResult_DoubleSubscribe));

    _checkpoints.emplace("08aa6a715e644abfe47f9623bdc4a568b6edb0432e1bf5b59a2cee7565c81226", tuple(ACTION_SUBSCRIBE_CANCEL, SocialConsensusResult_DoubleSubscribe));
    _checkpoints.emplace("87e63741701602706bee96257259054030b8f91c4fcab849aa1aac33cab5d692", tuple(ACTION_SUBSCRIBE_CANCEL, SocialConsensusResult_DoubleSubscribe));

    _checkpoints.emplace("a9f3b05d3398dd9eef945522e1fdc22cb4638b0fc3c55846bb0ba8a8ecdee21e", tuple(ACTION_SUBSCRIBE_CANCEL, SocialConsensusResult_InvalideSubscribe));
    _checkpoints.emplace("4cd085384b2a752accf8a5cb260e2e6e6d31f76aea449202c4c9e2e1a502a1b1", tuple(ACTION_SUBSCRIBE_CANCEL, SocialConsensusResult_InvalideSubscribe));
    _checkpoints.emplace("42c9efb79919350c3a19c1988c17fab5d8c911250856c215eeea1ccf004eb012", tuple(ACTION_SUBSCRIBE_CANCEL, SocialConsensusResult_InvalideSubscribe));
    _checkpoints.emplace("7f4992ab035b1fdb6d52c70ec2c28788d92a8c3432e76a03474c85b250290848", tuple(ACTION_SUBSCRIBE_CANCEL, SocialConsensusResult_InvalideSubscribe));

    _checkpoints.emplace("7c9f36e79de86e22ad86230596cb32cf3b8be14d7ac8046e7ec56af85fce9734", tuple(ACTION_BLOCKING, SocialConsensusResult_DoubleBlocking));
    _checkpoints.emplace("2e746fd6d1eb858e0dd610f0a3ffe5b2a83f45c6cda26d35805626ca72aa4416", tuple(ACTION_BLOCKING, SocialConsensusResult_DoubleBlocking));
    _checkpoints.emplace("7c9f36e79de86e22ad86230596cb32cf3b8be14d7ac8046e7ec56af85fce9734", tuple(ACTION_BLOCKING, SocialConsensusResult_DoubleBlocking));
    _checkpoints.emplace("2e746fd6d1eb858e0dd610f0a3ffe5b2a83f45c6cda26d35805626ca72aa4416", tuple(ACTION_BLOCKING, SocialConsensusResult_DoubleBlocking));

    _checkpoints.emplace("71569365b642d18a9f771f06dcb7832fbf0b2cd4d07eebcb2d40a7ea4f5c13db", tuple(ACTION_BLOCKING_CANCEL, SocialConsensusResult_InvalidBlocking));
    _checkpoints.emplace("71569365b642d18a9f771f06dcb7832fbf0b2cd4d07eebcb2d40a7ea4f5c13db", tuple(ACTION_BLOCKING_CANCEL, SocialConsensusResult_InvalidBlocking));

    _checkpoints.emplace("aedcd75c2ebae534537f6064ea43d7abb75230eed5db108229465ae8bd268142", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));
    _checkpoints.emplace("fc0fa992b8c3c87107da9db7843beaafd255b682cd0466402d37a2a0631fc464", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));
    _checkpoints.emplace("74b1c56b84392357f331738d6ab1a63830e3b1bf54c6725851346f1bf92c3dfa", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));
    _checkpoints.emplace("c5ba57092a0098fe2c3302ccbb40bc250eec6b8ec12d807b16d66415fe49ffa6", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));
    _checkpoints.emplace("f0d173b068d42818c071dfc15530a236f12db54cd89ad57f08dbf46e51a61ac5", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));
    _checkpoints.emplace("1cdb6616f2546b10419082962a2c8414d84818618c5b2eeddb53c0b168ca9eab", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));
    _checkpoints.emplace("3a15656e5e67fda68a3412f682c5ccfef3510a4007c2d7e69fd7b4c748b2b3ee", tuple(ACTION_COMPLAIN, SocialConsensusResult_DoubleComplain));

    _checkpoints.emplace("5dbb6b1d74de979dad1ae56c8bd368ce0309611bd508cf111119dcbb92582f9d", tuple(ACTION_SCORE_COMMENT, SocialConsensusResult_NotFound));
    
    _checkpoints.emplace("496c190dd819ddf44b13372f6b6ff1453576432e49f94194b1ce0c2417a872c8", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("b764cd5fd330df2fb5300563cdb6066d36f5b254fd109ffc89ddac7d12ed5177", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("fbff57f2d478e438bca26dcafb4b7df59db4cf187f76560b69e652f3ec4015ca", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("9b0d134a145a6bd7afb71d97f59a375e4d3882d31321631414d1feb917b6c2f6", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("3c9158264b260fc809ea8c7e809850ab93e8a0080bbc28fb92d0f21f460b25ef", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("ea8d404aecfad0a6c6e8194b87c4031b2706a6f7dba703f86b3ccaedb7ef57a9", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("ca2f7d7420c6378361de9ea32bdf92c82f6d5d13a4ef8622a8de295b58a7cfe9", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("82af891725c9de26fd2f9f348785b3f563f4a91ae0fc685bf5b03c4a7ea941d3", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("54860e7493eb3f6fd39c36554594c4f54c2a455a2439b4e09f20eec874c8a531", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("64bc84f08cdbb6c6fb70a7411e586d3d97c3552110089d9cd22046958d3973b8", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("01a4bb67489d74125cc9b37c3300498f185351dc666866006674849b4a804888", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("6d468678e9361c3657b2a71e385a24be172e7fcf045b78b30273f9bd9982d77e", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("d67f4a9becadd96c7860a41946727dbd49b6a17c8e341ac27c4e3382a9b5a676", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("b9dde16e8582c08bb5db4e288c000f318f9dcf1736b1de574b1116454ba17655", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("94d2dd14f55eac05be78e0f282d528de99744ed09a173b611075c7f2cf265077", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("3332531136a84d5c85340d95e1959e6ffc84be8a923c5e536f33a2c0ca854493", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("45654958a3fb6c7cdbf3b4b746d58a02f16d4f7ae738cc6355bfbf41a610aa4e", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("b6354e421bd1f12f0b8ef321bea2a549fd53f2185d2d389a36500541a34f2572", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("4313c1b7d07f82883d5d59c406dff0a35b9c8206aaa0641e76d940cdc06e0672", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("34053ebb4bfdbcd72b5c70d5b2b99c02663a039c8bd4ed77889277adcd4405f6", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("6541be6bd2a0211f7233a5b7a354b5a36ac0c0af51da106b77fa917197a45d3d", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("60d09522e200ae48119fa89af38fb1921be7cc5458dd13a7c36dfcb0f9008873", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("eacfe0c37830c0fafe015f923ee1263bac1cc49f055b8b6be116131996b6d771", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("d61470a073ecec3f6c7ddabccd21c629809b28372a6b24f38702e237cd69f221", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("e3a3aedce222d6982ad1562439240828ef2cbad02c8bc72c69d1044e4518e8d2", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("98b1f8aae6800fa7c897ed17278b2b950960af9b72b7aa98fdc335a46f7eda7d", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("9ec07ff6b6ce5f16ed5d0f98d01eef86c5c95e976c3299ea463fbbd5244612e5", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));
    _checkpoints.emplace("aa0d158ef6f646e7c4fe88e23b1d0d25f8d7a5d29f7a102d523e12f4c029c324", tuple(CONTENT_POST, SocialConsensusResult_ContentLimit));

}