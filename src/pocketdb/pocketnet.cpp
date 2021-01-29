#include "pocketdb/pocketnet.h"
#include "logging.h"


bool IsPocketTX(const CTxOut& out) {
    if (out.scriptPubKey.size() > 0 && out.scriptPubKey[0] == OP_RETURN) {
        std::string asmStr = ScriptToAsmStr(out.scriptPubKey);
        std::istringstream iss(asmStr);
        std::vector<std::string> vasm(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

        if (vasm.size() > 1) {
            std::string rTable;
            return ConvertOPToTableName(vasm[1], rTable);
        }
    }

    return false;
}

bool IsPocketTX(const CTransaction& tx)
{
    if (tx.vout.size() > 0) {
        const CTxOut out = tx.vout[0];
        return IsPocketTX(out);
    }

    return false;
}

bool IsPocketTX(const CTransactionRef& tx)
{
    if (tx->vout.size() > 0) {
        const CTxOut out = tx->vout[0];
        return IsPocketTX(out);
    }

    return false;
}

std::string PocketTXType(const CTransactionRef& tx)
{
    if (tx->vout.size() > 0) {
        const CTxOut out = tx->vout[0];
        if (out.scriptPubKey.size() > 0 && out.scriptPubKey[0] == OP_RETURN) {
            std::string asmStr = ScriptToAsmStr(out.scriptPubKey);
            std::istringstream iss(asmStr);
            std::vector<std::string> vasm(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

            if (vasm.size() > 1) {
                return vasm[1];
            }
        }
    }
    
    return "";
}

// Transaction type convert to reindexer table name
bool ConvertOPToTableName(std::string op, std::string& ri_table)
{
    bool ret = true;

    if (op == OR_POST)
        ri_table = "Posts";
    else if (op == OR_POSTEDIT)
        ri_table = "Posts";
    else if (op == OR_SCORE)
        ri_table = "Scores";
    else if (op == OR_COMPLAIN)
        ri_table = "Complains";
    else if (op == OR_SUBSCRIBE)
        ri_table = "Subscribes";
    else if (op == OR_SUBSCRIBEPRIVATE)
        ri_table = "Subscribes";
    else if (op == OR_UNSUBSCRIBE)
        ri_table = "Subscribes";
    else if (op == OR_USERINFO)
        ri_table = "Users";
    else if (op == OR_BLOCKING)
        ri_table = "Blocking";
    else if (op == OR_UNBLOCKING)
        ri_table = "Blocking";
    else if (op == OR_COMMENT)
        ri_table = "Comment";
    else if (op == OR_COMMENT_EDIT)
        ri_table = "Comment";
    else if (op == OR_COMMENT_DELETE)
        ri_table = "Comment";
    else if (op == OR_COMMENT_SCORE)
        ri_table = "CommentScores";
    else
        ret = false;

    return ret;
}

// User reputation - double value in integer
// i.e. 213 = 21.3
// i.e. 45  = 4.5
static std::map<Limit, std::map<int, int64_t>> Limits;
void FillLimits(const CChainParams& params) {

    // Forks
    int64_t fork_20190830 = 292800;
    int64_t fork_20190920 = 322700;

    // threshold_reputation
    std::map<int, int64_t> _threshold_reputation;
    _threshold_reputation.insert({ 0, 500 });
    _threshold_reputation.insert({ fork_20190830, 1000 });
    Limits.insert(std::make_pair(Limit::threshold_reputation, _threshold_reputation));

    // threshold_reputation_score
    std::map<int, int64_t> _threshold_reputation_score;
    _threshold_reputation_score.insert({ 0, -10000 });
    _threshold_reputation_score.insert({ (int)params.GetConsensus().nHeight_version_1_0_0, 500 });
    _threshold_reputation_score.insert({ fork_20190830, 1000 });
    Limits.insert(std::make_pair(Limit::threshold_reputation_score, _threshold_reputation_score));

    // threshold_reputation_complains
    std::map<int, int64_t> _threshold_reputation_complains;
    _threshold_reputation_complains.insert({ 0, 500 });
    _threshold_reputation_complains.insert({ fork_20190830, 1000 });
    Limits.insert(std::make_pair(Limit::threshold_reputation_complains, _threshold_reputation_complains));

    // threshold_reputation_blocking
    std::map<int, int64_t> _threshold_reputation_blocking;
    _threshold_reputation_blocking.insert({ 0, 500 });
    _threshold_reputation_blocking.insert({ fork_20190830, 1000 });
    Limits.insert(std::make_pair(Limit::threshold_reputation_blocking, _threshold_reputation_blocking));

    // threshold_balance
    std::map<int, int64_t> _threshold_balance;
    _threshold_balance.insert({ 0, 50 * COIN });
    Limits.insert(std::make_pair(Limit::threshold_balance, _threshold_balance));

    // trial_post_limit
    std::map<int, int64_t> _trial_post_limit;
    _trial_post_limit.insert({ 0, 15 });
    Limits.insert(std::make_pair(Limit::trial_post_limit, _trial_post_limit));

    // trial_post_edit_limit
    std::map<int, int64_t> _trial_post_edit_limit;
    _trial_post_edit_limit.insert({ 0, 5 });
    Limits.insert(std::make_pair(Limit::trial_post_edit_limit, _trial_post_edit_limit));

    // trial_score_limit
    std::map<int, int64_t> _trial_score_limit;
    _trial_score_limit.insert({ 0, 45 });
    _trial_score_limit.insert({ 175600, 100 });
    Limits.insert(std::make_pair(Limit::trial_score_limit, _trial_score_limit));

    // trial_complain_limit
    std::map<int, int64_t> _trial_complain_limit;
    _trial_complain_limit.insert({ 0, 6 });
    Limits.insert(std::make_pair(Limit::trial_complain_limit, _trial_complain_limit));

    // full_post_limit
    std::map<int, int64_t> _full_post_limit;
    _full_post_limit.insert({ 0, 30 });
    Limits.insert(std::make_pair(Limit::full_post_limit, _full_post_limit));

    // full_post_edit_limit
    std::map<int, int64_t> _full_post_edit_limit;
    _full_post_edit_limit.insert({ 0, 5 });
    Limits.insert(std::make_pair(Limit::full_post_edit_limit, _full_post_edit_limit));

    // full_score_limit
    std::map<int, int64_t> _full_score_limit;
    _full_score_limit.insert({ 0, 90 });
    _full_score_limit.insert({ 175600, 200 });
    Limits.insert(std::make_pair(Limit::full_score_limit, _full_score_limit));

    // full_complain_limit
    std::map<int, int64_t> _full_complain_limit;
    _full_complain_limit.insert({ 0, 12 });
    Limits.insert(std::make_pair(Limit::full_complain_limit, _full_complain_limit));

    // change_info_timeout
    std::map<int, int64_t> _change_info_timeout;
    _change_info_timeout.insert({ 0, 3600 });
    Limits.insert(std::make_pair(Limit::change_info_timeout, _change_info_timeout));

    // edit_post_timeout
    std::map<int, int64_t> _edit_post_timeout;
    _edit_post_timeout.insert({ 0, 86400 });
    Limits.insert(std::make_pair(Limit::edit_post_timeout, _edit_post_timeout));

    // max_user_size
    std::map<int, int64_t> _max_user_size;
    _max_user_size.insert({ 0, 2000 });
    Limits.insert(std::make_pair(Limit::max_user_size, _max_user_size)); // 2Kb

    // max_post_size
    std::map<int, int64_t> _max_post_size;
    _max_post_size.insert({ 0, 60000 });
    Limits.insert(std::make_pair(Limit::max_post_size, _max_post_size)); // 60Kb

    // bad_reputation
    std::map<int, int64_t> _bad_reputation;
    _bad_reputation.insert({ 0, -500 });
    Limits.insert(std::make_pair(Limit::bad_reputation, _bad_reputation));
    
    // scores_one_to_one
    std::map<int, int64_t> _scores_one_to_one;
    _scores_one_to_one.insert({ 0, 99999 });
    _scores_one_to_one.insert({ 225000, 2 });
    Limits.insert(std::make_pair(Limit::scores_one_to_one, _scores_one_to_one));

    // scores_one_to_one_over_comment
    std::map<int, int64_t> _scores_one_to_one_over_comment;
    _scores_one_to_one_over_comment.insert({ 0, 20 });
    Limits.insert(std::make_pair(Limit::scores_one_to_one_over_comment, _scores_one_to_one_over_comment));

    // scores_one_to_one time
    std::map<int, int64_t> _scores_one_to_one_depth;
    _scores_one_to_one_depth.insert({ 0, 336*24*3600 });
    _scores_one_to_one_depth.insert({ 225000, 1*24*3600 });
    _scores_one_to_one_depth.insert({ fork_20190830, 7*24*3600 });
    _scores_one_to_one_depth.insert({ fork_20190920, 2*24*3600 });
    Limits.insert(std::make_pair(Limit::scores_one_to_one_depth, _scores_one_to_one_depth));

    // trial_comment_limit
    std::map<int, int64_t> _trial_comment_limit;
    _trial_comment_limit.insert({ 0, 150 });
    Limits.insert(std::make_pair(Limit::trial_comment_limit, _trial_comment_limit));

    // trial_comment_edit_limit
    std::map<int, int64_t> _trial_comment_edit_limit;
    _trial_comment_edit_limit.insert({ 0, 5 });
    Limits.insert(std::make_pair(Limit::trial_comment_edit_limit, _trial_comment_edit_limit));

    // trial_comment_score_limit
    std::map<int, int64_t> _trial_comment_score_limit;
    _trial_comment_score_limit.insert({ 0, 300 });
    Limits.insert(std::make_pair(Limit::trial_comment_score_limit, _trial_comment_score_limit));

    // full_comment_limit
    std::map<int, int64_t> _full_comment_limit;
    _full_comment_limit.insert({ 0, 300 });
    Limits.insert(std::make_pair(Limit::full_comment_limit, _full_comment_limit));

    // full_comment_edit_limit
    std::map<int, int64_t> _full_comment_edit_limit;
    _full_comment_edit_limit.insert({ 0, 5 });
    Limits.insert(std::make_pair(Limit::full_comment_edit_limit, _full_comment_edit_limit));

    // full_comment_score_limit
    std::map<int, int64_t> _full_comment_score_limit;
    _full_comment_score_limit.insert({ 0, 600 });
    Limits.insert(std::make_pair(Limit::full_comment_score_limit, _full_comment_score_limit));

    // comment_size_limit
    std::map<int, int64_t> _comment_size_limit;
    _comment_size_limit.insert({ 0, 2000 });
    Limits.insert(std::make_pair(Limit::comment_size_limit, _comment_size_limit));

    // edit_comment_timeout
    std::map<int, int64_t> _edit_comment_timeout;
    _edit_comment_timeout.insert({ 0, 86400 });
    Limits.insert(std::make_pair(Limit::edit_comment_timeout, _edit_comment_timeout));

    // scores_depth_modify_reputation
    std::map<int, int64_t> _scores_depth_modify_reputation;
    _scores_depth_modify_reputation.insert({ 0, 336*24*3600 });
    _scores_depth_modify_reputation.insert({ fork_20190920, 30*24*3600 });
    Limits.insert(std::make_pair(Limit::scores_depth_modify_reputation, _scores_depth_modify_reputation));

    // lottery_referral_depth
    std::map<int, int64_t> _lottery_referral_depth;
    _lottery_referral_depth.insert({ 0, 30*24*3600 });
    Limits.insert(std::make_pair(Limit::lottery_referral_depth, _lottery_referral_depth));
    
};

// Get actual limit for current height
int64_t GetActualLimit(Limit type, int height) {
    return (--Limits[type].upper_bound(height))->second;
}


static std::map<int, std::string> CheckpointsBlocks;
void FillCheckpointsBlocks(const CChainParams& params)
{
    // PoS
    CheckpointsBlocks.emplace(355728, "ff718a5f0d8fb33aa5454a77cb34d0d009a421a86ff67e81275c68a9220f0ecd");
    CheckpointsBlocks.emplace(355729, "e8dd221233ef8955049179d38edd945f202c7672c08919a4dcca4b91b59d1359");
    CheckpointsBlocks.emplace(355730, "9b9aa4c3931a91f024a866a69f15979813b3d068a1545f3a96e9ef9e2641d193");
    CheckpointsBlocks.emplace(355735, "5483eb752c49c50fc5701efcb3efd638acede37cc7d78ee90cc8e41b53791240");
    CheckpointsBlocks.emplace(355736, "0024dd9843b582faa3a89ab750f52cb75f39f58b425e59761dec74aef4fce209");
    CheckpointsBlocks.emplace(355737, "237b66bbf8768277e36308e035e3911a58f9fbac1a183ba3c6bbd154b3cb51f6");
    CheckpointsBlocks.emplace(357329, "e2b1cf6b6769f9191dc331cb5161ebadd495983d49e7823ab5e2112631245120");
    CheckpointsBlocks.emplace(357459, "dbc4632a7dc3a05513c232f8d8fbd332ae16ea9eefd2842d8d965bb1001f15ea");
    CheckpointsBlocks.emplace(357507, "e0b4739f2ccbf485b11e14288d9ed000d4de2c9a3c2fe472c7bbff925244475f");
    CheckpointsBlocks.emplace(357549, "0bf85d1a73b6f69af4f8e9c2553231988c643339795eacd47f7a6a63d616c73a");
    CheckpointsBlocks.emplace(357555, "ff4af52df4d96762a36943f976b94ba20f5e1d3636b95b81dca39b11fdb89452");
    CheckpointsBlocks.emplace(357636, "00e2f7a68a1719271aa01450dcc98811a88a6e72576aa96f4aad9863b8df0ba2");
    CheckpointsBlocks.emplace(357835, "3a4f7a7df6f25a27f6c556810366f78da8aa16011a66959ab7c13f1372457103");
    CheckpointsBlocks.emplace(360029, "77326a586bfde7750e5fc942d6a2932acfdf9c9250381cc22026ed72ddce7f75");
    CheckpointsBlocks.emplace(360175, "44f7d16d8b76ac547faf1a63a99ac3086898bae9c031950987192c46ea27d735");
    CheckpointsBlocks.emplace(361934, "b72d9ec3712be053b07ae8e7ce55df739f543d16eac11f19de4f11ccb1e3540c");
    CheckpointsBlocks.emplace(361945, "17d68d65f8d2b9c5f012ca50aa22d292e2c6d2390a1acde7a491605aece66205");
    CheckpointsBlocks.emplace(361947, "ae0bec9a7200811f0761242e71b33add2a2a181fc00e903dfa06950bdb0c221a");
    CheckpointsBlocks.emplace(361955, "092fe8d9d260dc9e301210dc155ca7594d6284fee1d339c8d5ec3b85f1c72c0f");
    CheckpointsBlocks.emplace(365473, "66fbf8c4ee51058b6cbc75ed02c215db7dd9651aed305e16beffd044de7f1712");
    CheckpointsBlocks.emplace(365474, "32176e745a8013b260afbe4168007a25d8e6465749573a2f794df1edd82a1056");
    CheckpointsBlocks.emplace(366215, "c4e12f070c48c88f0738dbbdf918d4705f2b2b7827cccde019f03abf7bb85eb0");
    CheckpointsBlocks.emplace(366426, "228d7b995324642b31841428c27033cb3748fe17851ac8432a14f466b21ac15d");
    CheckpointsBlocks.emplace(366455, "d8594e1e14b591ab1fff611cd2d402d44d23711a3739e65809e3786e759aef93");
    CheckpointsBlocks.emplace(369165, "f4022299ec6890b71a16fa41a7ce4d9411ae88135eb9700d4257f5754b4a08e6");
    CheckpointsBlocks.emplace(369609, "e96c7ab86d96f3f157b08cccfa2641b17af1fe13226826b31981d184af0f2d6a");
    CheckpointsBlocks.emplace(369682, "d39b34fe1db359c2c2f47900b42a52327ae636b44f17436d7edf59419f240756");
    CheckpointsBlocks.emplace(370664, "97fe021d7c99ac0029adf25650fe74dd1f45c26a8be3185f7084c36a7505a320");
    CheckpointsBlocks.emplace(370692, "743b543e168739670b01446ea3ecf23cf59ff340fe6611fe1c9cab12923c3de2");
    CheckpointsBlocks.emplace(372179, "6ff637108b23435971b7aea4fe9e0a7c564581433d03c7d889ef6c9b6b034500");
    CheckpointsBlocks.emplace(372180, "e6cb167746d786dd2c84ef08e7fc9e61dae6157445fdbaad128b402984393bc7");
    CheckpointsBlocks.emplace(372426, "1030210c393e2f8e0f841c6a9547e623d78b35b11b7fc19c9c77aeefb6a42e4d");
    CheckpointsBlocks.emplace(373166, "9bce5b36493eeeeb40a64c3702f7d341eaeea3e70e3e3432646770d448f484e6");
    CheckpointsBlocks.emplace(373168, "6be5138294b706b06d86b90ac19458443e2244b0d60e59a700eff4a5076cea52");
    CheckpointsBlocks.emplace(373259, "a3f0a10b0c0733a35f2bb00da68562598b45745844227ded618c9eb723e524e6");
    CheckpointsBlocks.emplace(373605, "5a7d146d7535d17b0067123ccc644cf4dc617fc2ec99b47634d9989a473796f5");
    CheckpointsBlocks.emplace(373639, "d4dbd76656936416ab1cc6584c9517a9f54aa77897792996f1f1352a8dce63f8");
    CheckpointsBlocks.emplace(374456, "7e93f9259e9a1ce463539446d61940582329789fa804337398a202cd1a0a6f57");
    CheckpointsBlocks.emplace(374458, "01d10e315928bb5619341984e2c91e304433b0c2facc1f965004a36abf545a21");
    CheckpointsBlocks.emplace(374726, "52f89b6208c77f41078f3ec184cc0a4b670d5e4466da347610fb76db70f7515e");
    CheckpointsBlocks.emplace(374730, "5e909ecf11c8e0e13d4e62ace01e37905851ea5388fb21041481608e111ce7fe");
    CheckpointsBlocks.emplace(374820, "b1166a1673140ce85ad6934b1e170f7596f8cb97033d434b61004d165fdade29");
    CheckpointsBlocks.emplace(374861, "cfab349c68a98b8f6cf8dd1cb83e09bf140e72cf3ab48559df75c0f0ae5a5d85");
    CheckpointsBlocks.emplace(374944, "58260bfe434629407acd72f2dee8c48984dddcf137eaf8933290c5ed26529052");
    CheckpointsBlocks.emplace(374965, "a4f01af1686bb8502704a2f199ad23da1b4b54f190b81a99fec2a7773df2abaa");
    CheckpointsBlocks.emplace(376642, "d41502757d49a0a67cb76218de5de21c76250fa3464533799e80469b014b664e");
    CheckpointsBlocks.emplace(376900, "0cb8967e40fb6549b505abc03092b0d77bfa4b67b11a191e5e78b3462aab2d15");
    CheckpointsBlocks.emplace(376901, "78ea617182fb5f5a80ccdb0b44907430726b30ba48a17d11279558645213c26b");
    CheckpointsBlocks.emplace(377678, "97a23982034342bb797323a82ef0f2ec45317a95772267fc4d410d1e1671c147");
    CheckpointsBlocks.emplace(377729, "c945f861dcd7091de0e2bd4e0e487cded91f9eaf5565002ce9a4341cb9bc9d09");
    CheckpointsBlocks.emplace(378278, "08b8617d9b14e629f6faab155974aa4c7900e816a2a81bdce7e223660f93f798");
    CheckpointsBlocks.emplace(378308, "2e805cd488c4c3d708eac063ec3027c549e3cbdc1939c1b52f50f9ce75ef3ca0");
    CheckpointsBlocks.emplace(378966, "3b3f38f5527adf9f94e1bb3600aec082691eea6198eead71316ddd6a00f29118");
    CheckpointsBlocks.emplace(382538, "fdb0b23b12cd567dfa539061b5cc8d93dfad8db2d6a04daff64d0368bd13c0cc");
    CheckpointsBlocks.emplace(382539, "fad01113fa7674b8a696efe7b2484508148ebce4fc711b0c015afd7381bc1565");
    CheckpointsBlocks.emplace(382544, "17c3f0d199d61e0175880cf6412c48d320dad6bdf991019f5c6ec9d37f167c1a");
    CheckpointsBlocks.emplace(382554, "6a89c2d8eba914938a593f8612f52565aa4d603fc191ca31e25a0f2a9ebe9666");
    CheckpointsBlocks.emplace(382625, "0220bea6bce2e012cf12e6c53d37f9ea0eafb22b35691dd785366997dc2ea5b8");
    CheckpointsBlocks.emplace(388191, "69591ff21896135332a9f4b84fcbfe5da1bfc209bda10aeaec08c59a7ad06ef6");
    CheckpointsBlocks.emplace(388192, "b24b0cf25679b0d43cb4014ed874fa07e73c597c332b328d6bd2314b81757ec5");
    CheckpointsBlocks.emplace(388371, "3b790f075fe0a7670e7907b1b04169de7691e22be678bdff2204ff4375c34cea");
    CheckpointsBlocks.emplace(413599, "2383eff8a73d146ca7fca1472b352adf7fc642d40a0600dc99fe273ab7e0c242");
    CheckpointsBlocks.emplace(413613, "5d906b2dc460c935b18b4ef8fc248df576d7a794eed1c49092f90d8a8ec0866a");
    CheckpointsBlocks.emplace(413635, "723a3eb018307912220582bf1271d459c7d0401aec0a7836a4b8858620ce022d");
    CheckpointsBlocks.emplace(413662, "1b4fa351d7e89b659daf9b9fd8f4e0c9cdfaf7b9b393b61e32fb4cee55b980cc");    

	// Antibot
    CheckpointsBlocks.emplace(373222, "b8a1527d921a9903fb1020c370595116a9a609dff7d3f5524458e7eef04efde1");
    CheckpointsBlocks.emplace(382536, "46d5831c2f7e3ac1e719db7909e77d3f5aba1415931b851ad50768e97c54391a");
    CheckpointsBlocks.emplace(1019525, "e3950d1fce3abe7688d5709bfa14cd9850d5a4d0d32c4ab3c4eaa0d3319d8eec");
}

bool IsCheckpointBlock(int height, std::string hash) {
    if (CheckpointsBlocks.find(height) == CheckpointsBlocks.end()) return false;
    return CheckpointsBlocks[height] == hash;
}

static std::vector<std::string> CheckpointsTransactions;
void FillCheckpointsTransactions(const CChainParams& params)
{
    CheckpointsTransactions.push_back("f00f8d35c512ab282130ad8609d4ffa01f3a67011230ecdd903dfe12abde5fcf");
    CheckpointsTransactions.push_back("9ec4404eaf8d3e16e5b02da9e3ec468e3d471cb5b3632c31fcc7388544e6e3cc");
    CheckpointsTransactions.push_back("51effdd1aa5512992b65441161e0c8fded0c24d781480b07bd2300935a89a0c1");
    CheckpointsTransactions.push_back("7e530bd7e1b6ecebf514756129a2e43bf03958edefbcfff9360e28c51ffb20e0");
    CheckpointsTransactions.push_back("9c86226e0ca07c2bd021d8754572d5a227560c4858d133262ed834971100cdf5");
    CheckpointsTransactions.push_back("ebbd469d8567b7644564d298e6300c31fa6bb592bdd32c9fc1de07bebce9ee62");
    CheckpointsTransactions.push_back("3847a247d60865c3ae499e3de63ed8063862ff7652046d00a3bc724f2a71cb82");
    CheckpointsTransactions.push_back("53ee5afe1edafdeca13f9dadcbcc6fc29b64c365a767691a23b4d914a018a31c");
    CheckpointsTransactions.push_back("2087ec178c2cb2fe8840b1f3dd2f8dd8300bcb2a17cfde00acf0cb488004a546");
    CheckpointsTransactions.push_back("c74641d493d7d102748d2ed54de855eb399ecbb70b1c7e5854d4194009da3c5e");
    CheckpointsTransactions.push_back("a98aa7df032c16c57efe153b8d916de6b0882e192bd6d25ab09ac982e3079aba");
    CheckpointsTransactions.push_back("ac7ddefa0efb21606014f0ffe5bdaa890da9dd59c59922c51d0eb72582ecb0c9");
    CheckpointsTransactions.push_back("7fdcd94e189ff9829ccf8dca264f7b5577c2b7d52faa283ecea3917ec112e0ca");
    CheckpointsTransactions.push_back("6e4fb0248d5a2de72e83ef7c4f5df6a42bc5443fb2fb3683c7be3b4b689103eb");
    CheckpointsTransactions.push_back("d7ab57b7dbcc5383b38ab6dd5e1f7e7daa622e715f85d1648638168b6d7f8703");
    CheckpointsTransactions.push_back("bb2676fb51c5e4a9004197245beb104d84ab205696292e47ad69d90f5d90a303");
    CheckpointsTransactions.push_back("7bcace22ba0b3db12c70f16a9802fab0122a2b2505f69f9f6128a50a1ffb650e");
    CheckpointsTransactions.push_back("87e7606e8d4a15d3dd2e014f9b738d9477bda02b57fd2ba8ac887fcad272951b");
    CheckpointsTransactions.push_back("15be0addbfaf35e05970023b54e69f0758cd31b432cb6335599804c19e5dab22");
    CheckpointsTransactions.push_back("8d502590ac91a3abb454cabeb5f8c3bbde2533d3c84278115d4daba00b392f26");
    CheckpointsTransactions.push_back("0c8d0202e0650cbd5eb4a226a98664b587aa7c0b56cf074bf5444a618e51d555");
    CheckpointsTransactions.push_back("c54642c4a955070b93faf39c3de0fef9da69875694872a6106c41a45f52e7657");
    CheckpointsTransactions.push_back("888e18c0f95e9fe1d1ed808c4bfe2bddaef31e7d7614c8223e548ff691790b87");
    CheckpointsTransactions.push_back("efee870fcb4042ed963fd337e3d7fe353d5dfa5e66dc7bbdcff7e3df8a4de7ca");
    CheckpointsTransactions.push_back("045908a0a86cf525943eec3641bf4f604ac25eef6e618f92a841ac5efbce78dd");
    CheckpointsTransactions.push_back("638455551564d3ec6f350d0afb9842a49bf87cb4ad36da463c55a0ee1ed332ec");
    CheckpointsTransactions.push_back("4e2b5e40f28f1133562c5ab07ec47146ce74f55df71351527bc62a5e179d530f");
    CheckpointsTransactions.push_back("c3c8d1a7dc7039c8bf2a3388de3766b601b42a55270d60e179788a4bf9f3a3b2");
    CheckpointsTransactions.push_back("a205fa0c8f0b38c78349662c13fdd264f3069c75741551f40d805e0f42e44588");
    CheckpointsTransactions.push_back("3f599f11eeed6fd69c45fcc8f73668ecbc575df2b3eebfe567b81b8dd3a367d5");
    CheckpointsTransactions.push_back("0f186b661bb3666e5bbd1dc61b7d50223f9125256a327e00aba25e62b538a1dc");
    CheckpointsTransactions.push_back("7a9c4929e1e8a6e9d5d827ff0ba89a80ab3100080126fdca0ff6b93d3429874c");
    CheckpointsTransactions.push_back("f35e6e2ba1ee6d32c9c9fd4476d340a5644bd032c410e2fc257c5255c8608fc3");
    CheckpointsTransactions.push_back("749e78a8b5850aa848e957abfdbf100e59a1942629ed49da15d68706c400e847");
    CheckpointsTransactions.push_back("164e6db187d935b1bd3a4cbd2af541be6848979a4783e114ff755c87124f1551");
    CheckpointsTransactions.push_back("0435ba4832791332b14f11ae810296aaef98dd560a73eab6d2d01329dade46bf");
    CheckpointsTransactions.push_back("4271ba4040dd044160eba4f6fef57f4d5bd0f9614752f3640f3f6d78625b9410");
    CheckpointsTransactions.push_back("a8611e3813c7ee567f44cbe53ce25d19d76ae5920b939e948c19c7c969475d4a");
    CheckpointsTransactions.push_back("5a8e5fd6506f5cc8f8b9ed4c7ac57bed675fb47411018387b03a17ed541f1c71");
    CheckpointsTransactions.push_back("bad8541da87f08d59d6241cbc538a902a27f8baa5b0d1095b36d3d37d987daf6");
    CheckpointsTransactions.push_back("a798427f74abdc0a839fab679db597bb84919746004a82932c0c049620294ef9");
    CheckpointsTransactions.push_back("5d0eeb996443c3bfd67cf96c6d0d23912d99c6d25c277d081942625b07a70b6c");
    CheckpointsTransactions.push_back("519a095a90cae3b62a5725af2e871acd59a1a0385200440b167279ebe493ec9b");
    CheckpointsTransactions.push_back("f78453ffb67d19bd75f03d880bc3e0812fdddd22f1d7534e5b0d44e5cd954570");
    CheckpointsTransactions.push_back("d000bf49402e180104303eb13087af5baaf4c6113f31f0bf06188578d8c069cc");
    CheckpointsTransactions.push_back("596d561f08907ad3b5ba9eeb5e3c6afcd201e9754531b1e23c48fa59f66bc420");
    CheckpointsTransactions.push_back("8b07a75dea5c2b318246298b189a419519d9e9006c27e2a4256127c91f8672bc");
    CheckpointsTransactions.push_back("3393338a971b679df979141f7653aeda7c090df469130d806ff9ea391dda85d8");
    CheckpointsTransactions.push_back("9e890d72a2935e88a8a67f7de0ab7d1cee5603b7cef1550030ac656c4ba87987");
    CheckpointsTransactions.push_back("cfb32596f74c90ed497f729ca8ccd1c5a0373fd792e19227ddbccf5c605b77bd");
    CheckpointsTransactions.push_back("34d87a4e288751fa42416210d884eb96f2391def2a4849e3fc5a874e23d00301");
    CheckpointsTransactions.push_back("f18a28e77a07c342f7988d917b835f0bc6094a5696d8cf24849467c051608f28");
    CheckpointsTransactions.push_back("a210373d9f547b6c067ccbeb4ff7d066aec6d963ba65bc4becdde9679777559d");
    CheckpointsTransactions.push_back("56da1f5959f73fdd38c5329e026fd21a3657ffe75b71343c8d32e687185c85b3");
    CheckpointsTransactions.push_back("1d84969a3f9fd948a3bd3cabb5881013155865361221668c367d419004b345bd");
    CheckpointsTransactions.push_back("68e3a4dc6b2d2e27c97983754ce2d8168794c44b3855ede9ef45115574f0e5a3");
    CheckpointsTransactions.push_back("6194419c04736781100ba432412a1f2e63eb05c17025db2a71340b4888048bdf");
    CheckpointsTransactions.push_back("e8c667e29456e5b501a5e68b455468b4dc0e1bd77561fed3fbfe02f62028ff99");
    CheckpointsTransactions.push_back("9679ac3dac5d29f5cef3913fb72a2b93dc8d9421118ab8931a7532163b92979d");
    CheckpointsTransactions.push_back("77909ad71bf0fd849a8d7a834becca464bca1314d825eb33cf81619d462dd98b");
    CheckpointsTransactions.push_back("d167814ba72ef9045b2d07e90f42902b1ed7009f5f634f8435c8a7895aa4504a");
    CheckpointsTransactions.push_back("17c232c198b7dc381245648088b070d53e572f31a60ce4695ce02a29220e2b2f");
    CheckpointsTransactions.push_back("2d17c244ad606541326dfcd221e0807079e2d4b57a1943ca215119e108578762");
    CheckpointsTransactions.push_back("c0cf10129f4d3e315345f91579761a6dddfca79640914cf99b084f2679da3092");
    CheckpointsTransactions.push_back("ff7b33404ce8a716eb3056bc29be92a95ba8bbaddf4fc32f6fa633f8b703c7b0");
    CheckpointsTransactions.push_back("698f709485ba44e537023dbb385b9e6d5efdfd134264428ecf82fbcbcf8fb655");
    CheckpointsTransactions.push_back("ed3c7f996c55b528aeed78c3696d3413fc1c28f4e16efb7bf59ee5ff26db6bce");
    CheckpointsTransactions.push_back("2a7d6a597f0f6d1dcd1a6f624348f105e49025ace4c8e7f2e8d80f3a649fd9eb");
    CheckpointsTransactions.push_back("a46dfe70ff5a1090b7a2543c31c660b5de523341fc0cba336e89f44e86eb9fe6");
    CheckpointsTransactions.push_back("819db1858bcccc019368a8abe722dbcf5fced2241d2395224a29ca078f29ed11");
    CheckpointsTransactions.push_back("01490e5b7e4d5ffd21c67aefdb59ae7e51edb8f095d9a8e02cb440db37898e43");
    CheckpointsTransactions.push_back("cb6a33aee2f933e3198ff9edf8c9a4a958f3a0953c6bebfb2b0a42aa5c25e6ec");
    CheckpointsTransactions.push_back("5d2f8afebb34cb7ae18ed36060f802c5ec99ec7ee7e294e16903856b97393865");
    CheckpointsTransactions.push_back("1d1f62bce36d43bb3d7054282a0bfdc082a7c4198df16f7c37ccbbf466a4532f");
    CheckpointsTransactions.push_back("94fa10c2de9e9ae862309beb9fc7128fb3f313b19c2ff389eba543e0e5cc7b00");
    CheckpointsTransactions.push_back("a33e591dc0b8918a9781f95bbe42719da91f9bebb82b716f7ffb4082507e901d");
    CheckpointsTransactions.push_back("19a08b78e935b47ab91003c325d810fa5ef110ef591f4d83992e3e515dfffda9");
    CheckpointsTransactions.push_back("9536d7bd5132b3994ef47a70371674517c6f5c1e4c4000890c90412ac4ec10e4");
    CheckpointsTransactions.push_back("73a20c40d0b318725d88ebc1438e2082db800be34d404a26994c6b54a5714a37");
    CheckpointsTransactions.push_back("1f093757e7d0783833333f28fdb19493d87548061a0a81ba34f638e0331ec476");
    CheckpointsTransactions.push_back("7ecf297230ecbc55b014ce302cde241d9362c50f2aadbc3f476dd9ae8a6aaa9f");
    CheckpointsTransactions.push_back("9b7b1049b6de44416b879691bd0dbb5bb4c6352cde2747d625f82e664c08f1ba");
    CheckpointsTransactions.push_back("7f0ab92c0b92bcb4e7dc9c12050a2419b9aefa1fbb260bb65468c5c5c4457ac3");
    CheckpointsTransactions.push_back("b1a635675cf1cec31680aa04ddf90d4246278a3659fb31defcf631f8dcb9fa37");
    CheckpointsTransactions.push_back("709715f1420660c7f2c1ac5ccaf9a12712b66aece43b25829fa754e3b204bf57");
    CheckpointsTransactions.push_back("897eca2d187863ef126815201b13bd748318126115ed87806995e810252ac59e");
    CheckpointsTransactions.push_back("fa3dd182ad1dc287b14a9fbceb8c33ccd8b894e8f2187d8eeef0ed5efb77bec9");
    CheckpointsTransactions.push_back("c57c47c49a775443a3191abe469294e8dd6549524ef07368872063c15a98e324");
    CheckpointsTransactions.push_back("17d25ae7b6e06bf1ddcbb43d10cfdd4ad933d86c4ec3609132eb1b5d8139634a");
    CheckpointsTransactions.push_back("986dac41d232dd3443b12a4b8153176f9dfa86997e8793e57c2c0488690456ed");
    CheckpointsTransactions.push_back("a841baa175f2c8d0b152e8e09e43357dc6cacbeaeae8f3a192532832b590fcc0");
    CheckpointsTransactions.push_back("edebb4af73407face7bfbca6d2c67b5b28e7f101f6920c223b7d4f1b68ccc8ca");
    CheckpointsTransactions.push_back("5e1db09656b76c0ca3c8e15a3dd9775bb6d3e77d8c9abffa8d9e989337d854ea");
    CheckpointsTransactions.push_back("4fd2022604439f6ad0a518de09eae9cbe1e410f4ae20114ea497caa9d28c470c");
    CheckpointsTransactions.push_back("d6b21451f035f2dcb435446daaeab472a465ba78965f3b40bf60728a6bbac34e");
    CheckpointsTransactions.push_back("dcd07908e6eb4bcd681de36887f7c824a62b6e1ccd69467fa913b8d4742ba62c");
    CheckpointsTransactions.push_back("8fa0f2aa3f70b06bf455746f3cbc7e5e2540d9a412237d6fd9a1c215dd2722f3");
    CheckpointsTransactions.push_back("7b52be957c18d20fd1eff2b4ae67215d67f0805dafe40ac929d8b21c044e2f0a");
    CheckpointsTransactions.push_back("ba667ac80cf623f5603b3b1b238b74938bdcae12ab897fd170e77e28948e6516");
    CheckpointsTransactions.push_back("d5fc11cba672e30a500afb4dc40e83b7ed072acd158e289819db6b9db435c383");
    CheckpointsTransactions.push_back("255fbd900102be30b7db16028463707fb35b9e50d8d76052c8f9aa805e8a9e55");
    CheckpointsTransactions.push_back("362aaa7490398bc60f055ec8d9dcff37c44112fa410d299f5b87860e081d1325");
    CheckpointsTransactions.push_back("69c205e48c21a3c70bff62b12c99a51baa69d601d7c7e66daa2941f9b9e99293");
    CheckpointsTransactions.push_back("375c68fbe4d70ff7f779c94ece57790f7d8abeadc532033a568a931d917f5c66");
    CheckpointsTransactions.push_back("b07615f5fd1e20008df1aa2dc11808dba7a3676d3b28346c1fc50c70ad61c31a");
    CheckpointsTransactions.push_back("3bdb1046ed07357cf828ce66fd22f12d9e8b3c3b0a02d174f4bd03681cadab42");
    CheckpointsTransactions.push_back("8ab207c295bc13da47da5947b5d93fdfd5a78136841ee44659489af7537c4dd0");
    CheckpointsTransactions.push_back("e7ac37bfc229ce6bcad948a98030a95be6b5929d718d4efcdefbfea428789010");
    CheckpointsTransactions.push_back("ce7da6823ed58784003d4c418dca892d156e8ee5b5f36a76cafdd48cb50861d5");
    CheckpointsTransactions.push_back("cbf99c5c5073ccba798222af117984d304b30360d0397d32af45f52c49c5cb8c");
    CheckpointsTransactions.push_back("e48505877db1563304523d13a5057922a8adc9d0c8aaa4f488b56e18f318545e");
}

bool IsCheckpointTransaction(std::string hash) {
    return (std::find(CheckpointsTransactions.begin(), CheckpointsTransactions.end(), hash) != CheckpointsTransactions.end());
}


bool GetInputAddress(uint256 txhash, int n, std::string& address)
{
    uint256 hash_block;
    CTransactionRef tx;
    //-------------------------
    if (!GetTransaction(txhash, tx, Params().GetConsensus(), hash_block)) return false;
    const CTxOut& txout = tx->vout[n];
    CTxDestination destAddress;
    const CScript& scriptPubKey = txout.scriptPubKey;
    bool fValidAddress = ExtractDestination(scriptPubKey, destAddress);
    if (!fValidAddress) return false;
    address = EncodeDestination(destAddress);
    //-------------------------
    return true;
}
bool GetTransactionData(std::string txid, std::string& address)
{
    CTransactionRef tx;
    return GetTransactionData(txid, address, tx);
}
bool GetTransactionData(std::string txid, std::string& address, CTransactionRef& tx)
{
    uint256 hash_block;
    uint256 hash_tx;
    hash_tx.SetHex(txid);
    if (!GetTransaction(hash_tx, tx, Params().GetConsensus(), hash_block, true, nullptr)) return false;
    return GetInputAddress(tx->vin[0].prevout.hash, tx->vin[0].prevout.n, address);
}


bool FindPocketNetAsmString(const CTransactionRef& tx, std::vector<std::string>& vasm)
{
    std::string asmStr;
    if (!FindPocketNetAsmString(tx, asmStr)) return false;
    boost::split(vasm, asmStr, boost::is_any_of("\t "));
    //-------------------------
    return true;
}
bool FindPocketNetAsmString(const CTransactionRef& tx, std::string& asmStr)
{
    const CTxOut& txout = tx->vout[0];
    if (txout.scriptPubKey[0] == OP_RETURN) {
        asmStr = ScriptToAsmStr(txout.scriptPubKey);
        return true;
    }

    return false;
}

bool GetPocketnetTXType(const CTransactionRef& tx, std::string& ri_table)
{
    std::vector<std::string> vasm;
    if (!FindPocketNetAsmString(tx, vasm)) return false;
    return ConvertOPToTableName(vasm[1], ri_table);
}
bool IsPocketnetTransaction(const CTransactionRef& tx)
{
    std::string _ri_table = "";
    return GetPocketnetTXType(tx, _ri_table);
}
bool IsPocketnetTransaction(const CTransaction& tx)
{
    return IsPocketnetTransaction(MakeTransactionRef(tx));
}