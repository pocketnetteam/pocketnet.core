// brangr
// Collect all transactions pars <sender>-<recipient>
// for antibot module
//-----------------------------------------------------
#include <antibot/antibot.h>
#include <index/addrindex.h>
//-----------------------------------------------------
std::unique_ptr<AntiBot> g_antibot;
//-----------------------------------------------------
AntiBot::AntiBot()
{
}

AntiBot::~AntiBot()
{
}

bool vectorFind(std::vector<std::string>& V, std::string f)
{
    return std::find(V.begin(), V.end(), f) != V.end();
}

void AntiBot::getMode(std::string _address, ABMODE &mode, int &reputation, int64_t &balance, int height) {
    reputation = g_pocketdb->GetUserReputation(_address, height - 1);
    balance = g_pocketdb->GetUserBalance(_address, height);
    if (reputation >= GetActualLimit(Limit::threshold_reputation, height) || balance >= GetActualLimit(Limit::threshold_balance, height))
        mode = Full;
    else
        mode = Trial;
}

void AntiBot::getMode(std::string _address, ABMODE &mode, int height) {
    int reputation = 0;
    int64_t balance = 0;
    getMode(_address, mode, reputation, balance, height);

    if (reputation >= GetActualLimit(Limit::threshold_reputation, height) || balance >= GetActualLimit(Limit::threshold_balance, height))
        mode = Full;
    else
        mode = Trial;
}

int AntiBot::getLimit(CHECKTYPE _type, ABMODE _mode, int height) {
    switch (_type)
    {
    case Post:
        return _mode == Full ? GetActualLimit(Limit::full_post_limit, height) : GetActualLimit(Limit::trial_post_limit, height);
    case PostEdit:
        return _mode == Full ? GetActualLimit(Limit::full_post_edit_limit, height) : GetActualLimit(Limit::trial_post_edit_limit, height);
    case Score:
        return _mode == Full ? GetActualLimit(Limit::full_score_limit, height) : GetActualLimit(Limit::trial_score_limit, height);
    case Complain:
        return _mode == Full ? GetActualLimit(Limit::full_complain_limit, height) : GetActualLimit(Limit::trial_complain_limit, height);
    default:
        return 0;
    }
}
//-----------------------------------------------------

//-----------------------------------------------------
int getLimitsCount(std::string _table, std::string _address, int64_t _time) {
    int count = 0;

    reindexer::QueryResults res;
    if (g_pocketdb->DB()->Select(
        reindexer::Query(_table)
        .Where("address", CondEq, _address)
        .Where("time", CondGe, _time - 86400),
        res
    ).ok()) {
        count = res.Count();
    }

    // Also check mempool
    reindexer::QueryResults mem_res;
    if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, _table), mem_res).ok()) {
        for (auto& m : mem_res) {
            reindexer::Item mItm = m.GetItem();
            std::string table = mItm["table"].As<string>();
            std::string txid_source = mItm["txid_source"].As<string>();

            // Edited posts not count in limits
            if (table == "Posts" && txid_source != "") continue;

            std::string t_src = DecodeBase64(mItm["data"].As<string>());

            reindexer::Item t_itm = g_pocketdb->DB()->NewItem(_table);
            if (t_itm.FromJSON(t_src).ok()) {
                if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address) {
                    count += 1;
                }
            }
        }
    }

    return count;
}
//-----------------------------------------------------

//-----------------------------------------------------
bool AntiBot::CheckRegistration(std::string _address, std::string _txid, int64_t time, bool checkMempool, BlockVTX& blockVtx) {
    if (g_pocketdb->Exists(reindexer::Query("UsersView", 0, 1).Where("address", CondEq, _address))) return true;

    // Or maybe registration in this block?
    if (blockVtx.Exists("Users")) {
        for (auto& mtx : blockVtx.Data["Users"]) {
            if (mtx["txid"].get_str() != _txid && mtx["time"].get_int64() <= time && mtx["address"].get_str() == _address) {
                return true;
            }
        }
    }

    // Or in mempool?
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Users").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Users");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= time && t_itm["address"].As<string>() == _address) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool AntiBot::CheckRegistration(std::string _address) {
    if (g_pocketdb->Exists(reindexer::Query("UsersView", 0, 1).Where("address", CondEq, _address))) return true;
    return false;
}

bool AntiBot::check_post(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT &result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(_address, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    // Compute count of posts for last 24 hours
    int postsCount = g_pocketdb->SelectCount(Query("Posts").Where("address", CondEq, _address).Where("txidEdit", CondEq, "").Where("time", CondGe, _time - 86400));
    postsCount += g_pocketdb->SelectCount(Query("PostsHistory").Where("address", CondEq, _address).Where("txidEdit", CondEq, "").Where("time", CondGe, _time - 86400));

    // Also check mempool
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Posts").Where("txid_source", CondEq, "").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Posts");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address && t_itm["txidEdit"].As<string>() == "") {
                        postsCount += 1;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Posts")) {
        for (auto& mtx : blockVtx.Data["Posts"]) {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["time"].get_int64() <= _time && mtx["txidEdit"].get_str() == "") {
                postsCount += 1;
            }
        }
    }

    // Check limit
    ABMODE mode;
    getMode(_address, mode, chainActive.Height() + 1);
    int limit = getLimit(Post, mode, chainActive.Height() + 1);
    if (postsCount >= limit) {
        result = ANTIBOTRESULT::PostLimit;
        return false;
    }

    return true;
}

bool AntiBot::check_post_edit(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT &result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();// Original post id
    std::string _txidEdit = oitm["txidEdit"].get_str(); // new transaction txid
    int64_t _time = oitm["time"].get_int64();

    // User registered?
    if (!CheckRegistration(_address, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    // Posts exists?
    reindexer::Item _original_post_itm;
    if (!g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _txid).Where("txidEdit", CondEq, ""), _original_post_itm).ok()) {
        if (!g_pocketdb->SelectOne(Query("PostsHistory").Where("txid", CondEq, _txid).Where("txidEdit", CondEq, ""), _original_post_itm).ok()) {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // You are author? Really?
    if (_original_post_itm["address"].As<string>() != _address) {
        result = ANTIBOTRESULT::PostEditUnauthorized;
        return false;
    }

    // Original post edit only 24 hours
    if (_time - _original_post_itm["time"].As<int64_t>() > GetActualLimit(Limit::edit_post_timeout, chainActive.Height() + 1)) {
        result = ANTIBOTRESULT::PostEditLimit;
        return false;
    }

    // Double edit in block denied
    if (blockVtx.Exists("Posts")) {
        for (auto& mtx : blockVtx.Data["Posts"]) {
            if (mtx["txid"].get_str() == _txid && mtx["txidEdit"].get_str() != _txidEdit) {
                result = ANTIBOTRESULT::DoublePostEdit;
                return false;
            }
        }
    }

    // Double edit in mempool denied
    if (checkMempool) {
        if (g_pocketdb->Exists(reindexer::Query("Mempool").Where("table", CondEq, "Posts").Where("txid_source", CondEq, _txid))) {
            result = ANTIBOTRESULT::DoublePostEdit;
            return false;
        }
    }

    // Check limit
    {
        size_t edit_count = g_pocketdb->SelectCount(Query("Posts").Where("txid", CondEq, _txid).Not().Where("txidEdit", CondEq, ""));
        edit_count += g_pocketdb->SelectCount(Query("PostsHistory").Where("txid", CondEq, _txid).Not().Where("txidEdit", CondEq, ""));
        
        ABMODE mode;
        getMode(_address, mode, chainActive.Height() + 1);
        int limit = getLimit(PostEdit, mode, chainActive.Height() + 1);
        if (edit_count >= limit) {
            result = ANTIBOTRESULT::PostEditLimit;
            return false;
        }
    }

    return true;
}

bool AntiBot::check_score(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT &result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _post = oitm["posttxid"].get_str();
    int _score_value = oitm["value"].get_int();
    int64_t _time = oitm["time"].get_int64();

    if (_score_value < 1 || _score_value > 5) {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    if (!CheckRegistration(_address, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    // Check score to self
    bool not_found = false;
    std::string _post_address;
    reindexer::Item postItm;
    if (g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, _post), postItm).ok()) {
        _post_address = postItm["address"].As<string>();

        // Score to self post
        if (_post_address == _address) {
            result = ANTIBOTRESULT::SelfScore;
            return false;
        }
    }
    else {
        // Post not found
        not_found = true;

        // Maybe in current block?
        if (blockVtx.Exists("Posts")) {
            for (auto& mtx : blockVtx.Data["Posts"]) {
                if (mtx["txid"].get_str() == _post) {
                    _post_address = mtx["address"].get_str();
                    not_found = false;
                    break;
                }
            }
        }

        if (not_found) {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Check double score to post
    reindexer::Item doubleScoreItm;
    if (g_pocketdb->SelectOne(
        reindexer::Query("Scores")
        .Where("address", CondEq, _address)
        .Where("posttxid", CondEq, _post),
        doubleScoreItm
    ).ok()) {
        result = ANTIBOTRESULT::DoubleScore;
        return false;
    }

    // Check limit scores
    reindexer::QueryResults scoresRes;
    if (!g_pocketdb->DB()->Select(
        reindexer::Query("Scores")
        .Where("address", CondEq, _address)
        .Where("time", CondGe, _time - 86400),
        scoresRes
    ).ok()) {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    int scoresCount = scoresRes.Count();

    // Also check mempool
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Scores").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Scores");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address) {
                        scoresCount += 1;
                    }
                    if (t_itm["address"].As<string>() == _address && t_itm["posttxid"].As<string>() == _post) {
                        result = ANTIBOTRESULT::DoubleScore;
                        return false;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Scores")) {
        for (auto& mtx : blockVtx.Data["Scores"]) {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["time"].get_int64() <= _time) {
                scoresCount += 1;
            }

            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["posttxid"].get_str() == _post) {
                result = ANTIBOTRESULT::DoubleScore;
                return false;
            }
        }
    }

    ABMODE mode;
    getMode(_address, mode, chainActive.Height() + 1);
    int limit = getLimit(Score, mode, chainActive.Height() + 1);
    if (scoresCount >= limit) {
        result = ANTIBOTRESULT::ScoreLimit;
        return false;
    }

    // Check OP_RETURN
    std::vector<std::string> vasm;
    boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));

    // Check address and value in asm == reindexer data
    if (vasm.size() >= 4) {
    	std::stringstream _op_return_data;
    	_op_return_data << vasm[3];
    	std::string _op_return_hex = _op_return_data.str();

    	std::string _score_itm_val = _post_address + " " + std::to_string(_score_value);
    	std::string _score_itm_hex = HexStr(_score_itm_val.begin(), _score_itm_val.end());

    	if (_op_return_hex != _score_itm_hex) {
    		result = ANTIBOTRESULT::Failed;
    		return false;
    	}
    }

    return true;
}

bool AntiBot::check_complain(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT &result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _post = oitm["posttxid"].get_str();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(_address, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    // Check score to self
    bool not_found = false;
    reindexer::Item postItm;
    if (g_pocketdb->SelectOne(
        reindexer::Query("Posts")
        .Where("txid", CondEq, _post),
        postItm
    ).ok()) {
        // Score to self post
        if (postItm["address"].As<string>() == _address) {
            result = ANTIBOTRESULT::SelfComplain;
            return false;
        }
    }
    else {
        // Post not found
        not_found = true;

        // Maybe in current block?
        if (blockVtx.Exists("Posts")) {
            for (auto& mtx : blockVtx.Data["Posts"]) {
                if (mtx["txid"].get_str() == _post) {
                    not_found = false;
                    break;
                }
            }
        }

        if (not_found) {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Check double score to post
    reindexer::Item doubleComplainItm;
    if (g_pocketdb->SelectOne(
        reindexer::Query("Complains")
        .Where("address", CondEq, _address)
        .Where("posttxid", CondEq, _post),
        doubleComplainItm
    ).ok()) {
        result = ANTIBOTRESULT::DoubleComplain;
        return false;
    }

    ABMODE mode;
    int reputation = 0;
    int64_t balance = 0;
    getMode(_address, mode, reputation, balance, chainActive.Height() + 1);
    if (reputation < GetActualLimit(Limit::threshold_reputation_complains, chainActive.Height() + 1)) {
        result = ANTIBOTRESULT::LowReputation;
        return false;
    }

    reindexer::QueryResults complainsRes;
    if (!g_pocketdb->DB()->Select(
        reindexer::Query("Complains")
        .Where("address", CondEq, _address)
        .Where("time", CondGe, _time - 86400),
        complainsRes
    ).ok()) {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    int complainCount = complainsRes.Count();

    // Also check mempool
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Complains").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Complains");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address) {
                        complainCount += 1;
                    }
                    if (t_itm["address"].As<string>() == _address && t_itm["posttxid"].As<string>() == _post) {
                        result = ANTIBOTRESULT::DoubleComplain;
                        return false;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Complains")) {
        for (auto& mtx : blockVtx.Data["Complains"]) {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["time"].get_int64() <= _time) {
                complainCount += 1;
            }

            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["posttxid"].get_str() == _post) {
                result = ANTIBOTRESULT::DoubleComplain;
                return false;
            }
        }
    }

    int limit = getLimit(Complain, mode, chainActive.Height() + 1);
    if (complainCount >= limit) {
        result = ANTIBOTRESULT::ComplainLimit;
        return false;
    }

    return true;
}

bool AntiBot::check_changeInfo(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT &result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _address_referrer = oitm["referrer"].get_str();
    std::string _name = oitm["name"].get_str();
    int64_t _time = oitm["time"].get_int64();

    // Get last updated item
    reindexer::Item userItm;
    if (g_pocketdb->SelectOne(
        reindexer::Query("UsersView").Where("address", CondEq, _address),
        userItm
    ).ok()) {
        int64_t userUpdateTime = userItm["time"].As<int64_t>();
        if (_time - userUpdateTime <= GetActualLimit(Limit::change_info_timeout, chainActive.Height() + 1)) {
            result = ANTIBOTRESULT::ChangeInfoLimit;
            return false;
        }
    }

    // Also check mempool
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Users").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Users");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address) {
                        result = ANTIBOTRESULT::ChangeInfoLimit;
                        return false;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Users")) {
        for (auto& mtx : blockVtx.Data["Users"]) {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address) {
                result = ANTIBOTRESULT::ChangeInfoLimit;
                return false;
            }
        }
    }

    // Check long nickname
    if (_name.size() < 1 && _name.size() > 35) {
        result = ANTIBOTRESULT::NicknameLong;
        return false;
    }

    // Check double nickname
    if (g_pocketdb->SelectCount(reindexer::Query("UsersView").Where("name", CondEq, _name).Not().Where("address", CondEq, _address)) > 0) {
        result = ANTIBOTRESULT::NicknameDouble;
        return false;
    }

    // TODO (brangr): block all spaces
    // Check spaces in begin and end
    if (boost::algorithm::ends_with(_name, "%20") || boost::algorithm::starts_with(_name, "%20")) {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    if (_address == _address_referrer) {
        result = ANTIBOTRESULT::ReferrerSelf;
        return false;
    }

    return true;
}

bool AntiBot::check_subscribe(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT &result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _address_to = oitm["address_to"].get_str();
    bool _private = oitm["private"].get_bool();
    bool _unsubscribe = oitm["unsubscribe"].get_bool();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(_address, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    if (!CheckRegistration(_address_to, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    // Also check mempool
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Subscribes").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Subscribes");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address && t_itm["address_to"].As<string>() == _address_to) {
                        result = ANTIBOTRESULT::ManyTransactions;
                        return false;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Subscribes")) {
        for (auto& mtx : blockVtx.Data["Subscribes"]) {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
                result = ANTIBOTRESULT::ManyTransactions;
                return false;
            }
        }
    }

    if (_address == _address_to) {
        result = ANTIBOTRESULT::SelfSubscribe;
        return false;
    }

    reindexer::Item sItm;
    Error err = g_pocketdb->SelectOne(
        reindexer::Query("SubscribesView")
        .Where("address", CondEq, _address)
        .Where("address_to", CondEq, _address_to),
        sItm
    );

    if (_unsubscribe && !err.ok()) {
        result = ANTIBOTRESULT::InvalideSubscribe;
        return false;
    }

    if (!_unsubscribe && err.ok()) {
        result = ANTIBOTRESULT::DoubleSubscribe;
        return false;
    }

    return true;
}

bool AntiBot::check_blocking(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _address_to = oitm["address_to"].get_str();
    bool _unblocking = oitm["unblocking"].get_bool();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(_address, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    if (!CheckRegistration(_address_to, _txid, _time, checkMempool, blockVtx)) {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    if (_address == _address_to) {
        result = ANTIBOTRESULT::SelfBlocking;
        return false;
    }

    //-----------------------
    // Also check mempool
    if (checkMempool) {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Blocking").Not().Where("txid", CondEq, _txid), res).ok()) {
            for (auto& m : res) {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Blocking");
                if (t_itm.FromJSON(t_src).ok()) {
                    if (t_itm["time"].As<int64_t>() <= _time && t_itm["address"].As<string>() == _address && t_itm["address_to"].As<string>() == _address_to) {
                        result = ANTIBOTRESULT::ManyTransactions;
                        return false;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Blocking")) {
        for (auto& mtx : blockVtx.Data["Blocking"]) {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
                result = ANTIBOTRESULT::ManyTransactions;
                return false;
            }
        }
    }

    reindexer::Item sItm;
    Error err = g_pocketdb->SelectOne(
        reindexer::Query("BlockingView")
            .Where("address", CondEq, _address)
            .Where("address_to", CondEq, _address_to),
        sItm);

    if (_unblocking && !err.ok()) {
        result = ANTIBOTRESULT::InvalideBlocking;
        return false;
    }

    if (!_unblocking && err.ok()) {
        result = ANTIBOTRESULT::DoubleBlocking;
        return false;
    }

    return true;
}

bool AntiBot::check_item_size(UniValue oitm, CHECKTYPE _type, ANTIBOTRESULT &result, int height) {
    int _limit = oitm["size"].get_int();

    if (_type == CHECKTYPE::Post) _limit = GetActualLimit(Limit::max_post_size, height);
    if (_type == CHECKTYPE::User) _limit = GetActualLimit(Limit::max_user_size, height);

    if (oitm["size"].get_int() > _limit) {
        result = ANTIBOTRESULT::ContentSizeLimit;
        return false;
    }

    return true;
}
//-----------------------------------------------------

//-----------------------------------------------------
void AntiBot::CheckTransactionRIItem(UniValue oitm, ANTIBOTRESULT& resultCode) {
    BlockVTX blockVtx;
    CheckTransactionRIItem(oitm, blockVtx, true, resultCode);
}

void AntiBot::CheckTransactionRIItem(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, ANTIBOTRESULT& resultCode) {
    resultCode = ANTIBOTRESULT::Success;
    std::string table = oitm["table"].get_str();
    
    // If `item` with `txid` already in reindexer db - skip checks
    std::string _txid_check_exists = oitm["txid"].get_str();
    if (table == "Posts" && oitm["txidEdit"].get_str() != "") _txid_check_exists = oitm["txidEdit"].get_str();
    if (g_addrindex->CheckRItemExists(table, _txid_check_exists)) return;

    // Check consistent transaction and reindexer::Item
    {
        // Sorry, this life can be a lot of things.:)
        std::map<std::string, std::string> op_return_checkpoints;
        op_return_checkpoints.insert_or_assign("5741a02961547b401f9f9be17bd2c220bc6a98b4ff4d7909543e44adf3cb57e9", "603d2953b635a5963ad26da7f4d945e58ad511707c983cf11f96eadaa8511fa6");

        std::vector<std::string> vasm;
        boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));
        if (vasm.size() < 3) {
            resultCode = ANTIBOTRESULT::FailedOpReturn;
            return;
        }

        if ( ( vasm[2] != oitm["data_hash"].get_str() && vasm[2] != op_return_checkpoints[oitm["txid"].get_str()] ) ) {
            if (table == "Users" && vasm[2] != oitm["data_hash_without_ref"].get_str()) {
                resultCode = ANTIBOTRESULT::FailedOpReturn;
                return;
            }
        }
    }

    // Hard fork for old inconcistents antibot rules
    if (chainActive.Height() <= Params().GetConsensus().nHeight_version_1_0_0_pre) return;

    if (table == "Posts") {
        if (!check_item_size(oitm, Post, resultCode, chainActive.Height() + 1)) return;
        if (oitm["txidEdit"].get_str() != "") {
            check_post_edit(oitm, blockVtx, checkMempool, resultCode);
        } else {
            check_post(oitm, blockVtx, checkMempool, resultCode);
        }
    }
    else if (table == "Scores") {
        check_score(oitm, blockVtx, checkMempool, resultCode);
    }
    else if (table == "Complains") {
        check_complain(oitm, blockVtx, checkMempool, resultCode);
    }
    else if (table == "Subscribes") {
        check_subscribe(oitm, blockVtx, checkMempool, resultCode);
    }
	else if (table == "Blocking") {
        check_blocking(oitm, blockVtx, checkMempool, resultCode);
    }
    else if (table == "Users") {
        if (!check_item_size(oitm, User, resultCode, chainActive.Height() + 1)) return;
        check_changeInfo(oitm, blockVtx, checkMempool, resultCode);
    }
    else {
        resultCode = ANTIBOTRESULT::Unknown;
    }
}

bool AntiBot::CheckInputs(CTransactionRef& tx) {
    for (auto& in : tx->vin) {
        if (!g_pocketdb->Exists(
                reindexer::Query("UTXO")
                .Where("txid", CondEq, in.prevout.hash.GetHex())
                .Where("txout", CondEq, (int)in.prevout.n)
                .Where("spent_block", CondEq, 0))
        ) {
            return false;
        }
    }

    return true;
}

bool AntiBot::CheckBlock(BlockVTX& blockVtx) {
    for (auto& t : blockVtx.Data) {
        for (auto& mtx : t.second) {
            ANTIBOTRESULT resultCode = ANTIBOTRESULT::Success;
            CheckTransactionRIItem(mtx, blockVtx, false, resultCode);
            if (resultCode != ANTIBOTRESULT::Success) {
                LogPrintf("Transaction check with the AntiBot failed (%s) %s %s\n", mtx["txid"].get_str(), resultCode, t.first);

                // Skip next transactions - already error
                return false;
            }
        }
    }

    return true;
}

// TODO (brangr): replace time argument to block height
bool AntiBot::GetUserState(std::string _address, int64_t _time, UserStateItem& _state)
{
    _state = UserStateItem(_address);

    _state.user_registration_date = g_addrindex->GetUserRegistrationDate(_address);
    _state.address_registration_date = g_addrindex->GetAddressRegistrationDate(_address);

    ABMODE mode;
    int reputation = 0;
    int64_t balance = 0;
    getMode(_address, mode, reputation, balance, chainActive.Height() + 1);

    _state.trial = (mode == Trial);
    _state.reputation = reputation;
    _state.balance = balance;

    int postsCount = getLimitsCount("Posts", _address, chainActive.Tip()->nTime);
    int postsLimit = getLimit(Post, mode, chainActive.Height() + 1);
    _state.post_spent = postsCount;
    _state.post_unspent = postsLimit - postsCount;

    int scoresCount = getLimitsCount("Scores", _address, chainActive.Tip()->nTime);
    int scoresLimit = getLimit(Score, mode, chainActive.Height() + 1);
    _state.score_spent = scoresCount;
    _state.score_unspent = scoresLimit - scoresCount;

    int complainsCount = getLimitsCount("Complains", _address, chainActive.Tip()->nTime);
    int complainsLimit = getLimit(Complain, mode, chainActive.Height() + 1);
    _state.complain_spent = complainsCount;
    _state.complain_unspent = complainsLimit - complainsCount;

    int allow_reputation_blocking = GetActualLimit(Limit::threshold_reputation_blocking, chainActive.Height() + 1);
    _state.number_of_blocking = g_pocketdb->SelectCount(reindexer::Query("BlockingView").Where("address_to", CondEq, _address).Where("address_reputation", CondGe, allow_reputation_blocking));

    return true;
}
//-----------------------------------------------------

//-----------------------------------------------------
bool AntiBot::AllowModifyReputation(std::string _score_address, int height) {
    // Ignore scores from users with rating < Antibot::Limit::threshold_reputation_score
    int64_t _min_user_reputation = GetActualLimit(Limit::threshold_reputation_score, height);
    int _user_reputation = g_pocketdb->GetUserReputation(_score_address, height);
    if (_user_reputation < _min_user_reputation) return false;
    
    // All is OK
    return true;
}

bool AntiBot::AllowModifyReputation(std::string _score_address, std::string _post_address, int height, std::string _txid, int64_t _tx_time) {
    // Disable reputation increment if from one address to one address > 2 scores over day
    int64_t _max_scores_one_to_one = GetActualLimit(Limit::scores_one_to_one, height);
    size_t scores_one_to_one_count = g_pocketdb->SelectCount(
        reindexer::Query("Scores")
            .Where("address", CondEq, _score_address)
            .Where("time", CondGe, _tx_time - 86400)
            .Where("time", CondLt, _tx_time)
            .Not().Where("txid", CondEq, _txid)
        .InnerJoin("posttxid", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, _post_address))
    );
    if (scores_one_to_one_count >= _max_scores_one_to_one) return false;
    
    // All is OK
    return true;
}

bool AntiBot::AllowLottery(std::string _score_address, std::string _post_address, int height, std::string _txid, int64_t _tx_time) {
    if (!AllowModifyReputation(_score_address, height)) return false;

    // Disable reputation increment if from one address to one address > 2 scores over day
    int64_t _max_scores_one_to_one = GetActualLimit(Limit::scores_one_to_one, height);
    size_t scores_one_to_one_count = g_pocketdb->SelectCount(
        reindexer::Query("Scores")
            .Where("address", CondEq, _score_address)
            .Where("time", CondGe, _tx_time - 86400)
            .Where("time", CondLt, _tx_time)
            .Where("value", CondSet, {4, 5})
            .Not().Where("txid", CondEq, _txid)
        .InnerJoin("posttxid", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, _post_address))
    );
    if (scores_one_to_one_count >= _max_scores_one_to_one) return false;
    
    // All is OK
    return true;
}