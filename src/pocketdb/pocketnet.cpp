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
    _scores_one_to_one_depth.insert({ 0, 99999 });
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
};

// Get actual limit for current height
int64_t GetActualLimit(Limit type, int height) {
    return (--Limits[type].upper_bound(height))->second;
}


static std::map<int, std::string> CheckpointsBlocks;
void FillCheckpoints(const CChainParams& params) {
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

    CheckpointsBlocks.emplace(373222, "b8a1527d921a9903fb1020c370595116a9a609dff7d3f5524458e7eef04efde1");
}

bool IsCheckpoint(int height, std::string hash) {
    if (CheckpointsBlocks.find(height) == CheckpointsBlocks.end()) return false;
    return CheckpointsBlocks[height] == hash;
}