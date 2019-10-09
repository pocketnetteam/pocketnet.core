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

static std::map<Limit, std::map<int, int64_t>> Limits;
void FillLimits(const CChainParams& params) {

    // Forks
    int64_t fork_20190830 = 292800;
    int64_t fork_20190920 = 322700;

    // threshold_reputation
    std::map<int, int64_t> _threshold_reputation;
    _threshold_reputation.insert({ 0, 50 });
    _threshold_reputation.insert({ fork_20190830, 100 });
    Limits.insert(std::make_pair(Limit::threshold_reputation, _threshold_reputation));

    // threshold_reputation_score
    std::map<int, int64_t> _threshold_reputation_score;
    _threshold_reputation_score.insert({ 0, -1000 });
    _threshold_reputation_score.insert({ (int)params.GetConsensus().nHeight_version_1_0_0, 50 });
    _threshold_reputation_score.insert({ fork_20190830, 100 });
    Limits.insert(std::make_pair(Limit::threshold_reputation_score, _threshold_reputation_score));

    // threshold_reputation_complains
    std::map<int, int64_t> _threshold_reputation_complains;
    _threshold_reputation_complains.insert({ 0, 50 });
    _threshold_reputation_complains.insert({ fork_20190830, 100 });
    Limits.insert(std::make_pair(Limit::threshold_reputation_complains, _threshold_reputation_complains));

    // threshold_reputation_blocking
    std::map<int, int64_t> _threshold_reputation_blocking;
    _threshold_reputation_blocking.insert({ 0, 50 });
    _threshold_reputation_blocking.insert({ fork_20190830, 100 });
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
    _bad_reputation.insert({ 0, -50 });
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
