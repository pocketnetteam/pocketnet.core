// brangr
// Collect all transactions pars <sender>-<recipient>
// for antibot module
//-----------------------------------------------------
#include <antibot/antibot.h>
#include <index/addrindex.h>

//-----------------------------------------------------
std::unique_ptr <AntiBot> g_antibot;
//-----------------------------------------------------
AntiBot::AntiBot()
{
}

AntiBot::~AntiBot()
{
}

bool vectorFind(std::vector <std::string>& V, std::string f)
{
    return std::find(V.begin(), V.end(), f) != V.end();
}

void AntiBot::getMode(std::string _address, ABMODE& mode, int& reputation, int64_t& balance, int height)
{
    reputation = g_pocketdb->GetUserReputation(_address, height - 1);
    balance = g_pocketdb->GetUserBalance(_address, height);
    if (reputation >= GetActualLimit(Limit::threshold_reputation, height) ||
        balance >= GetActualLimit(Limit::threshold_balance, height))
        mode = Full;
    else
        mode = Trial;
}

void AntiBot::getMode(std::string _address, ABMODE& mode, int height)
{
    int reputation = 0;
    int64_t balance = 0;
    getMode(_address, mode, reputation, balance, height);

    if (reputation >= GetActualLimit(Limit::threshold_reputation, height) ||
        balance >= GetActualLimit(Limit::threshold_balance, height))
        mode = Full;
    else
        mode = Trial;
}

int AntiBot::getLimit(CHECKTYPE _type, ABMODE _mode, int height)
{
    switch (_type)
    {
        case Post:
            return _mode == Full ? GetActualLimit(Limit::full_post_limit, height) : GetActualLimit(
                Limit::trial_post_limit, height);
        case PostEdit:
            return _mode == Full ? GetActualLimit(Limit::full_post_edit_limit, height) : GetActualLimit(
                Limit::trial_post_edit_limit, height);
        case CheckType_ContentVideo:
            return _mode == Full ? GetActualLimit(Limit::full_video_limit, height) : GetActualLimit(
                Limit::trial_video_limit, height);
        case CheckType_ContentVideoEdit:
            return _mode == Full ? GetActualLimit(Limit::full_video_edit_limit, height) : GetActualLimit(
                Limit::trial_video_edit_limit, height);
        case Score:
            return _mode == Full ? GetActualLimit(Limit::full_score_limit, height) : GetActualLimit(
                Limit::trial_score_limit, height);
        case Complain:
            return _mode == Full ? GetActualLimit(Limit::full_complain_limit, height) : GetActualLimit(
                Limit::trial_complain_limit, height);
        case Comment:
            return _mode == Full ? GetActualLimit(Limit::full_comment_limit, height) : GetActualLimit(
                Limit::trial_comment_limit, height);
        case CommentEdit:
            return _mode == Full ? GetActualLimit(Limit::full_comment_edit_limit, height) : GetActualLimit(
                Limit::trial_comment_edit_limit, height);
        case CommentScore:
            return _mode == Full ? GetActualLimit(Limit::full_comment_score_limit, height) : GetActualLimit(
                Limit::trial_comment_score_limit, height);
        default:
            return 0;
    }
}
//-----------------------------------------------------

//-----------------------------------------------------
int getLimitsCount(std::string _table, std::string _address)
{
    int count = 0;

    auto query = reindexer::Query(_table).Where("address", CondEq, _address).Where("block", CondGe,
        chainActive.Height() - 1440);
    if (_table == "Comment") query = query.Where("last", CondEq, true);
    count += g_pocketdb->SelectCount(query);

    // Also check mempool
    reindexer::QueryResults mem_res;
    if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, _table), mem_res).ok())
    {
        for (auto& m : mem_res)
        {
            reindexer::Item mItm = m.GetItem();
            std::string table = mItm["table"].As<string>();
            std::string txid_source = mItm["txid_source"].As<string>();

            // Edited posts not count in limits
            if (table == "Posts" && txid_source != "") continue;

            std::string t_src = DecodeBase64(mItm["data"].As<string>());

            reindexer::Item t_itm = g_pocketdb->DB()->NewItem(_table);
            if (t_itm.FromJSON(t_src).ok() && t_itm["address"].As<string>() == _address)
            {
                count += 1;
            }
        }
    }

    return count;
}
//-----------------------------------------------------

//-----------------------------------------------------
bool AntiBot::CheckRegistration(UniValue oitm, std::string address, bool checkMempool, bool checkTime_19_3, int height,
    BlockVTX& blockVtx, ANTIBOTRESULT& result)
{
    std::string txType = oitm["type"].get_str();
    std::string txId = oitm["txid"].get_str();
    int64_t time = oitm["time"].get_int64();

    int userType = -1;

    // First restore user from DB
    reindexer::Item userItm;
    if (userType < 0 && g_pocketdb->SelectOne(
        reindexer::Query("UsersView", 0, 1).Where("address", CondEq, address).Where("block", CondLt, height),
        userItm).ok())
    {
        userType = userItm["gender"].As<int>();
    }

    // Or maybe registration in this block?
    if (userType < 0 && blockVtx.Exists("Users"))
    {
        for (auto& mtx : blockVtx.Data["Users"])
        {
            if (mtx["txid"].get_str() != txId && mtx["address"].get_str() == address)
            {
                if (!checkTime_19_3 || mtx["time"].get_int64() <= time)
                    userType = mtx["userType"].get_int();
            }
        }
    }

    // Or in mempool?
    if (userType < 0 && checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Users").Not().Where("txid", CondEq, txId), res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Users");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == address)
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= time)
                            userType = t_itm["gender"].As<int>();
                    }
                }
            }
        }
    }

    // Not found :(
    if (userType < 0)
    {
        result = ANTIBOTRESULT::NotRegistered;
        return false;
    }

    return true;
}

bool AntiBot::check_item_size(UniValue oitm, CHECKTYPE _type, int height, ANTIBOTRESULT& result)
{
    int _limit = oitm["size"].get_int();
    std::string table = oitm["table"].get_str();

    if (_type == CHECKTYPE::Post) _limit = GetActualLimit(Limit::max_post_size, height);
    if (_type == CHECKTYPE::User) _limit = GetActualLimit(Limit::max_user_size, height);

    if (oitm["size"].get_int() > _limit)
    {
        result = ANTIBOTRESULT::ContentSizeLimit;
        return false;
    }

    return true;
}

//-----------------------------------------------------

bool AntiBot::check_post(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, bool splitContent, int height, ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();
    std::string _txidRepost = oitm["txidRepost"].get_str();
    int _tx_content_type = oitm["contentType"].get_int();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    if (_txidRepost != "")
    {
        reindexer::Item _repost_post_itm;
        if (!g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _txidRepost).Where("block", CondLt, height),
            _repost_post_itm).ok())
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }

        if (_tx_content_type != ContentType::ContentPost)
        {
            result = ANTIBOTRESULT::NotAllowed;
            return false;
        }
    }

    // Compute count of posts for last 24 hours
    int postsCount = 0;

    // ------------------------------------------

    auto query = Query("Posts")
        .Where("address", CondEq, _address)
        .Where("txidEdit", CondEq, "")
        .Where("block", CondLt, height);

    if (checkTime_19_6) query = query.Where("time", CondGe, _time - 86400);
    else query = query.Where("block", CondGe, height - 1440);

    if (splitContent) query = query.Where("type", CondEq, (int)ContentType::ContentPost);

    postsCount += g_pocketdb->SelectCount(query);

    // ------------------------------------------

    // Also get posts from history
    auto queryHist = Query("PostsHistory")
        .Where("address", CondEq, _address)
        .Where("txidEdit", CondEq, "")
        .Where("block", CondLt, height);

    if (checkTime_19_6) queryHist = queryHist.Where("time", CondGe, _time - 86400);
    else queryHist = queryHist.Where("block", CondGe, height - 1440);

    if (splitContent) queryHist = queryHist.Where("type", CondEq, (int)ContentType::ContentPost);

    postsCount += g_pocketdb->SelectCount(queryHist);

    // ------------------------------------------

    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
                reindexer::Query("Mempool")
                    .Where("table", CondEq, "Posts")
                    .Where("txid_source", CondEq, "")
                    .Not()
                    .Where("txid", CondEq, _txid),
                res)
            .ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Posts");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == _address && t_itm["txidEdit"].As<string>().empty())
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                            if (!splitContent || t_itm["type"].As<int>() == (int)ContentType::ContentPost)
                                postsCount += 1;
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Posts"))
    {
        for (auto& mtx : blockVtx.Data["Posts"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address &&
                mtx["txidEdit"].get_str().empty())
            {
                if (!checkTime_19_3 || mtx["time"].get_int64() <= _time)
                    if (!splitContent || mtx["type"].get_int() == (int)ContentType::ContentPost)
                        postsCount += 1;
            }
        }
    }

    // Check limit
    ABMODE mode;
    getMode(_address, mode, height);
    int limit = getLimit(Post, mode, height);
    if (postsCount >= limit)
    {
        result = ANTIBOTRESULT::PostLimit;
        return false;
    }

    return true;
}

bool AntiBot::check_post_edit(const UniValue& oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, bool splitContent, int height, ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();         // Original post id
    std::string _txidEdit = oitm["txidEdit"].get_str(); // new transaction txid
    std::string _txidRepost = oitm["txidRepost"].get_str();
    int _tx_content_type = oitm["contentType"].get_int();
    int64_t _time = oitm["time"].get_int64();

    // User registered?
    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Posts exists?
    reindexer::Item _original_post_itm;
    if (!g_pocketdb->SelectOne(Query("Posts")
        .Where("txid", CondEq, _txid)
        .Where("txidEdit", CondEq, "")
        .Where("block", CondLt, height), _original_post_itm).ok())
    {
        if (!g_pocketdb->SelectOne(Query("PostsHistory")
            .Where("txid", CondEq, _txid)
            .Where("txidEdit", CondEq, "")
            .Where("block", CondLt, height), _original_post_itm).ok())
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Disable change type
    if (_tx_content_type != _original_post_itm["type"].As<int>())
    {
        result = ANTIBOTRESULT::ChangeTxType;
        return false;
    }

    // You are author? Really?
    if (_original_post_itm["address"].As<string>() != _address)
    {
        result = ANTIBOTRESULT::PostEditUnauthorized;
        return false;
    }

    // Original post edit only 24 hours
    auto depth = checkTime_19_6
        ? (_time - _original_post_itm["time"].As<int64_t>())
        : (height - _original_post_itm["block"].As<int>());

    if (depth > GetActualLimit(Limit::edit_post_timeout, height))
    {
        result = ANTIBOTRESULT::PostEditLimit;
        return false;
    }

    // Check repost
    if (_txidRepost != "")
    {
        reindexer::Item _repost_post_itm;
        if (!g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _txidRepost).Where("block", CondLt, height),
            _repost_post_itm).ok())
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }

        if (_tx_content_type != ContentType::ContentPost)
        {
            result = ANTIBOTRESULT::NotAllowed;
            return false;
        }
    }

    // Double edit in block denied
    if (blockVtx.Exists("Posts"))
    {
        for (auto& mtx : blockVtx.Data["Posts"])
        {
            if (mtx["txid"].get_str() == _txid && mtx["txidEdit"].get_str() != _txidEdit)
            {
                result = ANTIBOTRESULT::DoublePostEdit;
                return false;
            }
        }
    }

    // Double edit in mempool denied
    if (checkMempool)
    {
        if (g_pocketdb->Exists(
            reindexer::Query("Mempool").Where("table", CondEq, "Posts").Where("txid_source", CondEq, _txid)))
        {
            result = ANTIBOTRESULT::DoublePostEdit;
            return false;
        }
    }

    // Check limit
    {
        size_t edit_count = g_pocketdb->SelectCount(
            Query("Posts")
                .Where("txid", CondEq, _txid)
                .Not().Where("txidEdit", CondEq, "")
                .Where("block", CondLt, height));

        edit_count += g_pocketdb->SelectCount(
            Query("PostsHistory")
                .Where("txid", CondEq, _txid)
                .Not().Where("txidEdit", CondEq, "")
                .Where("block", CondLt, height));

        ABMODE mode;
        getMode(_address, mode, height);
        size_t limit = getLimit(PostEdit, mode, height);
        if (edit_count >= limit)
        {
            result = ANTIBOTRESULT::PostEditLimit;
            return false;
        }
    }

    return true;
}


bool AntiBot::check_video(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, int height, ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();
    std::string _txidRepost = oitm["txidRepost"].get_str();
    int _tx_content_type = oitm["contentType"].get_int();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(oitm, _address, checkMempool, false, height, blockVtx, result))
        return false;

    if (_txidRepost != "")
    {
        result = ANTIBOTRESULT::NotAllowed;
        return false;
    }

    // Compute count of posts for last 24 hours
    int postsCount = 0;

    auto query = Query("Posts")
        .Where("address", CondEq, _address)
        .Where("txidEdit", CondEq, "")
        .Where("block", CondLt, height)
        .Where("block", CondGe, height - 1440)
        .Where("type", CondEq, (int) ContentType::ContentVideo);

    postsCount += g_pocketdb->SelectCount(query);

    // Also get posts from history
    auto queryHist = Query("PostsHistory")
        .Where("address", CondEq, _address)
        .Where("txidEdit", CondEq, "")
        .Where("block", CondLt, height)
        .Where("block", CondGe, height - 1440)
        .Where("type", CondEq, (int) ContentType::ContentVideo);

    postsCount += g_pocketdb->SelectCount(queryHist);

    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
                reindexer::Query("Mempool")
                    .Where("table", CondEq, "Posts")
                    .Where("txid_source", CondEq, "")
                    .Not()
                    .Where("txid", CondEq, _txid),
                res)
            .ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Posts");
                if (t_itm.FromJSON(t_src).ok())
                    if (t_itm["type"].As<int>() == (int) ContentType::ContentVideo)
                        if (t_itm["address"].As<string>() == _address && t_itm["txidEdit"].As<string>().empty())
                            postsCount += 1;
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Posts"))
    {
        for (auto& mtx : blockVtx.Data["Posts"])
        {
            if (mtx["txid"].get_str() != _txid &&
                mtx["address"].get_str() == _address &&
                mtx["txidEdit"].get_str().empty() &&
                mtx["type"].get_int() == (int) ContentType::ContentVideo)
            {
                postsCount += 1;
            }
        }
    }

    // Check limit
    ABMODE mode;
    getMode(_address, mode, height);
    int limit = getLimit(CheckType_ContentVideo, mode, height);
    if (postsCount >= limit)
    {
        result = ANTIBOTRESULT::PostLimit;
        return false;
    }

    return true;
}

bool AntiBot::check_video_edit(const UniValue& oitm, BlockVTX& blockVtx, bool checkMempool, int height,
    ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();         // Original post id
    std::string _txidEdit = oitm["txidEdit"].get_str(); // new transaction txid
    std::string _txidRepost = oitm["txidRepost"].get_str();
    int _tx_content_type = oitm["contentType"].get_int();
    int64_t _time = oitm["time"].get_int64();

    // User registered?
    if (!CheckRegistration(oitm, _address, checkMempool, false, height, blockVtx, result))
        return false;

    // Posts exists?
    reindexer::Item _original_post_itm;
    if (!g_pocketdb->SelectOne(Query("Posts")
        .Where("txid", CondEq, _txid)
        .Where("txidEdit", CondEq, "")
        .Where("block", CondLt, height), _original_post_itm).ok())
    {
        if (!g_pocketdb->SelectOne(Query("PostsHistory")
            .Where("txid", CondEq, _txid)
            .Where("txidEdit", CondEq, "")
            .Where("block", CondLt, height)
            , _original_post_itm).ok())
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Disable change type
    if (_tx_content_type != _original_post_itm["type"].As<int>())
    {
        result = ANTIBOTRESULT::ChangeTxType;
        return false;
    }

    // You are author? Really?
    if (_original_post_itm["address"].As<string>() != _address)
    {
        result = ANTIBOTRESULT::PostEditUnauthorized;
        return false;
    }

    // Original post edit only 24 hours
    auto depth = (height - _original_post_itm["block"].As<int>());
    if (depth > GetActualLimit(Limit::edit_video_timeout, height))
    {
        result = ANTIBOTRESULT::PostEditLimit;
        return false;
    }

    // Check repost
    if (_txidRepost != "")
    {
        result = ANTIBOTRESULT::NotAllowed;
        return false;
    }

    // Double edit in block denied
    if (blockVtx.Exists("Posts"))
    {
        for (auto& mtx : blockVtx.Data["Posts"])
        {
            if (mtx["txid"].get_str() == _txid && mtx["txidEdit"].get_str() != _txidEdit)
            {
                result = ANTIBOTRESULT::DoublePostEdit;
                return false;
            }
        }
    }

    // Double edit in mempool denied
    if (checkMempool)
    {
        if (g_pocketdb->Exists(
            reindexer::Query("Mempool").Where("table", CondEq, "Posts").Where("txid_source", CondEq, _txid)))
        {
            result = ANTIBOTRESULT::DoublePostEdit;
            return false;
        }
    }

    // Check limit
    {
        size_t edit_count = g_pocketdb->SelectCount(
            Query("Posts")
                .Where("txid", CondEq, _txid)
                .Not().Where("txidEdit", CondEq, "")
                .Where("block", CondLt, height)
        );

        edit_count += g_pocketdb->SelectCount(
            Query("PostsHistory")
                .Where("txid", CondEq, _txid)
                .Not().Where("txidEdit", CondEq, "")
                .Where("block", CondLt, height)
        );

        ABMODE mode;
        getMode(_address, mode, height);
        size_t limit = getLimit(CheckType_ContentVideoEdit, mode, height);
        if (edit_count >= limit)
        {
            result = ANTIBOTRESULT::PostEditLimit;
            return false;
        }
    }

    return true;
}


bool AntiBot::check_score(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _post = oitm["posttxid"].get_str();
    int _score_value = oitm["value"].get_int();
    int64_t _time = oitm["time"].get_int64();

    if (_score_value < 1 || _score_value > 5)
    {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Check score to self
    bool not_found = false;
    std::string _post_address;
    reindexer::Item postItm;
    if (g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, _post).Where("block", CondLt, height),
        postItm).ok())
    {
        _post_address = postItm["address"].As<string>();

        // Score to self post
        if (_post_address == _address)
        {
            result = ANTIBOTRESULT::SelfScore;
            return false;
        }
    }
    else
    {
        // Post not found
        not_found = true;

        // Maybe in current block?
        if (blockVtx.Exists("Posts"))
        {
            for (auto& mtx : blockVtx.Data["Posts"])
            {
                if (mtx["txid"].get_str() == _post)
                {
                    _post_address = mtx["address"].get_str();
                    not_found = false;
                    break;
                }
            }
        }

        if (not_found)
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Blocking
    if (height >= Params().GetConsensus().score_blocking_on && height < Params().GetConsensus().score_blocking_off &&
        g_pocketdb->Exists(
            Query("BlockingView").Where("address", CondEq, _post_address).Where("address_to", CondEq, _address).Where(
                "block", CondLt, height)))
    {
        result = ANTIBOTRESULT::Blocking;
        return false;
    }

    // Check double score to post
    reindexer::Item doubleScoreItm;
    if (g_pocketdb->SelectOne(
            reindexer::Query("Scores")
                .Where("address", CondEq, _address)
                .Where("posttxid", CondEq, _post)
                .Where("block", CondLt, height),
            doubleScoreItm)
        .ok())
    {
        result = ANTIBOTRESULT::DoubleScore;
        return false;
    }

    // Check limit scores
    int scoresCount = 0;

    // Calculate in DB
    auto query = reindexer::Query("Scores").Where("address", CondEq, _address).Where("block", CondLt, height);
    if (checkTime_19_6) query = query.Where("time", CondGe, _time - 86400);
    else query = query.Where("block", CondGe, height - 1440);
    scoresCount += g_pocketdb->SelectCount(query);

    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Scores").Not().Where("txid", CondEq, _txid), res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Scores");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == _address)
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                            scoresCount += 1;

                        if (t_itm["posttxid"].As<string>() == _post)
                        {
                            result = ANTIBOTRESULT::DoubleScore;
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Scores"))
    {
        for (auto& mtx : blockVtx.Data["Scores"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address)
            {
                if (!checkTime_19_3 || mtx["time"].get_int64() <= _time)
                    scoresCount += 1;

                if (mtx["posttxid"].get_str() == _post)
                {
                    result = ANTIBOTRESULT::DoubleScore;
                    return false;
                }
            }
        }
    }

    ABMODE mode;
    getMode(_address, mode, height);
    int limit = getLimit(Score, mode, height);
    if (scoresCount >= limit)
    {
        result = ANTIBOTRESULT::ScoreLimit;
        return false;
    }

    // Check OP_RETURN
    std::vector <std::string> vasm;
    boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));

    // Check address and value in asm == reindexer data
    if (vasm.size() >= 4)
    {
        std::stringstream _op_return_data;
        _op_return_data << vasm[3];
        std::string _op_return_hex = _op_return_data.str();

        std::string _score_itm_val = _post_address + " " + std::to_string(_score_value);
        std::string _score_itm_hex = HexStr(_score_itm_val.begin(), _score_itm_val.end());

        if (_op_return_hex != _score_itm_hex)
        {
            result = ANTIBOTRESULT::OpReturnFailed;
            return false;
        }
    }

    return true;
}

bool AntiBot::check_complain(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _post = oitm["posttxid"].get_str();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Check score to self
    bool not_found = false;
    reindexer::Item postItm;
    if (g_pocketdb->SelectOne(
            reindexer::Query("Posts")
                .Where("txid", CondEq, _post)
                .Where("block", CondLt, height),
            postItm)
        .ok())
    {
        // Score to self post
        if (postItm["address"].As<string>() == _address)
        {
            result = ANTIBOTRESULT::SelfComplain;
            return false;
        }
    }
    else
    {
        // Post not found
        not_found = true;

        // Maybe in current block?
        if (blockVtx.Exists("Posts"))
        {
            for (auto& mtx : blockVtx.Data["Posts"])
            {
                if (mtx["txid"].get_str() == _post)
                {
                    not_found = false;
                    break;
                }
            }
        }

        if (not_found)
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Check double score to post
    reindexer::Item doubleComplainItm;
    if (g_pocketdb->SelectOne(
            reindexer::Query("Complains")
                .Where("address", CondEq, _address)
                .Where("posttxid", CondEq, _post)
                .Where("block", CondLt, height),
            doubleComplainItm)
        .ok())
    {
        result = ANTIBOTRESULT::DoubleComplain;
        return false;
    }

    ABMODE mode;
    int reputation = 0;
    int64_t balance = 0;
    getMode(_address, mode, reputation, balance, height);
    int64_t minRep = GetActualLimit(Limit::threshold_reputation_complains, height);
    if (reputation < minRep)
    {
        LogPrintf("%s - %s < %s\n", _address, reputation, minRep);
        result = ANTIBOTRESULT::LowReputation;
        return false;
    }

    // Check limits
    int complainCount = 0;

    auto query = reindexer::Query("Complains").Where("address", CondEq, _address).Where("block", CondLt, height);
    if (checkTime_19_6) query = query.Where("time", CondGe, _time - 86400);
    else query = query.Where("block", CondGe, height - 1440);
    complainCount += g_pocketdb->SelectCount(query);

    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Complains").Not().Where("txid", CondEq, _txid),
            res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Complains");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == _address)
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                            complainCount += 1;

                        if (t_itm["posttxid"].As<string>() == _post)
                        {
                            result = ANTIBOTRESULT::DoubleComplain;
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Complains"))
    {
        for (auto& mtx : blockVtx.Data["Complains"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address)
            {
                if (!checkTime_19_3 || mtx["time"].get_int64() <= _time)
                    complainCount += 1;

                if (mtx["posttxid"].get_str() == _post)
                {
                    result = ANTIBOTRESULT::DoubleComplain;
                    return false;
                }
            }
        }
    }

    int limit = getLimit(Complain, mode, height);
    if (complainCount >= limit)
    {
        result = ANTIBOTRESULT::ComplainLimit;
        return false;
    }

    return true;
}

bool AntiBot::check_changeInfo(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _address_referrer = oitm["referrer"].get_str();
    std::string _name = oitm["name"].get_str();
    int64_t _time = oitm["time"].get_int64();

    // Referrer to self? Seriously?;)
    if (_address == _address_referrer)
    {
        result = ANTIBOTRESULT::ReferrerSelf;
        return false;
    }

    // Get last updated item
    reindexer::Item userItm;
    if (g_pocketdb->SelectOne(
            reindexer::Query("UsersView")
                .Where("address", CondEq, _address)
                .Where("block", CondLt, height),
            userItm)
        .ok())
    {
        if (!_address_referrer.empty())
        {
            result = ANTIBOTRESULT::ReferrerAfterRegistration;
            return false;
        }

        auto depth = checkTime_19_6 ? (_time - userItm["time"].As<int64_t>()) : (height - userItm["block"].As<int>());
        if (depth <= GetActualLimit(Limit::change_info_timeout, height))
        {
            result = ANTIBOTRESULT::ChangeInfoLimit;
            return false;
        }
    }

    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Users").Not().Where("txid", CondEq, _txid), res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Users");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == _address)
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                        {
                            result = ANTIBOTRESULT::ChangeInfoLimit;
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Users"))
    {
        for (auto& mtx : blockVtx.Data["Users"])
        {
            if (mtx["address"].get_str() == _address && mtx["txid"].get_str() != _txid)
            {
                result = ANTIBOTRESULT::ChangeInfoLimit;
                return false;
            }
        }
    }

    // Check long nickname
    if (_name.size() < 1 && _name.size() > 35)
    {
        result = ANTIBOTRESULT::NicknameLong;
        return false;
    }

    // Check double nickname
    //if (height < Params().GetConsensus().checkpoint_non_unique_account_name) {
    if (g_pocketdb->SelectCount(
        reindexer::Query("UsersView").Where("name", CondEq, _name).Not().Where("address", CondEq, _address).Where(
            "block", CondLt, height)) > 0)
    {
        result = ANTIBOTRESULT::NicknameDouble;
        return false;
    }
    //}

    // TODO (brangr): block all spaces
    // Check spaces in begin and end
    if (boost::algorithm::ends_with(_name, "%20") || boost::algorithm::starts_with(_name, "%20"))
    {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    return true;
}

bool AntiBot::check_subscribe(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _address_to = oitm["address_to"].get_str();
    bool _private = oitm["private"].get_bool();
    bool _unsubscribe = oitm["unsubscribe"].get_bool();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    if (!CheckRegistration(oitm, _address_to, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Subscribes").Not().Where("txid", CondEq, _txid),
            res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Subscribes");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == _address && t_itm["address_to"].As<string>() == _address_to)
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                        {
                            result = ANTIBOTRESULT::ManyTransactions;
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Subscribes"))
    {
        for (auto& mtx : blockVtx.Data["Subscribes"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address &&
                mtx["address_to"].get_str() == _address_to)
            {
                result = ANTIBOTRESULT::ManyTransactions;
                return false;
            }
        }
    }

    if (_address == _address_to)
    {
        result = ANTIBOTRESULT::SelfSubscribe;
        return false;
    }

    reindexer::Item sItm;
    Error err = g_pocketdb->SelectOne(
        reindexer::Query("SubscribesView")
            .Where("address", CondEq, _address)
            .Where("address_to", CondEq, _address_to)
            .Where("block", CondLt, height),
        sItm);

    if (_unsubscribe && !err.ok())
    {
        result = ANTIBOTRESULT::InvalideSubscribe;
        return false;
    }

    if (!_unsubscribe && err.ok() && sItm["private"].As<bool>() == _private)
    {
        if (!IsCheckpointTransaction(_txid))
        {
            result = ANTIBOTRESULT::DoubleSubscribe;
            return false;
        }
        else
        {
            LogPrintf("Found checkpoint transaction %s\n", _txid);
        }
    }

    return true;
}

bool AntiBot::check_blocking(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _address_to = oitm["address_to"].get_str();
    bool _unblocking = oitm["unblocking"].get_bool();
    int64_t _time = oitm["time"].get_int64();

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    if (!CheckRegistration(oitm, _address_to, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    if (_address == _address_to)
    {
        result = ANTIBOTRESULT::SelfBlocking;
        return false;
    }

    //-----------------------
    // Also check mempool
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Blocking").Not().Where("txid", CondEq, _txid),
            res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Blocking");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["address"].As<string>() == _address && t_itm["address_to"].As<string>() == _address_to)
                    {
                        if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                        {
                            result = ANTIBOTRESULT::ManyTransactions;
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Check block
    if (blockVtx.Exists("Blocking"))
    {
        for (auto& mtx : blockVtx.Data["Blocking"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address &&
                mtx["address_to"].get_str() == _address_to)
            {
                result = ANTIBOTRESULT::ManyTransactions;
                return false;
            }
        }
    }

    reindexer::Item sItm;
    Error err = g_pocketdb->SelectOne(
        reindexer::Query("BlockingView")
            .Where("address", CondEq, _address)
            .Where("address_to", CondEq, _address_to)
            .Where("block", CondLt, height),
        sItm);

    if (_unblocking && !err.ok())
    {
        result = ANTIBOTRESULT::InvalidBlocking;
        return false;
    }

    if (!_unblocking && err.ok())
    {
        result = ANTIBOTRESULT::DoubleBlocking;
        return false;
    }

    return true;
}

bool AntiBot::check_comment(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    std::string _txid = oitm["txid"].get_str();
    int64_t _time = oitm["time"].get_int64();

    std::string _msg = oitm["msg"].get_str();
    std::string _otxid = oitm["otxid"].get_str();
    std::string _postid = oitm["postid"].get_str();
    std::string _parentid = oitm["parentid"].get_str();
    std::string _answerid = oitm["answerid"].get_str();

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Size message limit
    if (_msg == "" || UrlDecode(_msg).length() > GetActualLimit(Limit::comment_size_limit, height))
    {
        result = ANTIBOTRESULT::Size;
        return false;
    }

    // Parent comment
    if (_parentid != "" && !g_pocketdb->Exists(
        Query("Comment").Where("otxid", CondEq, _parentid).Where("last", CondEq, true).Not().Where("msg", CondEq,
            "").Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::InvalidParentComment;
        return false;
    }

    // Answer comment
    if (_answerid != "" && !g_pocketdb->Exists(
        Query("Comment").Where("otxid", CondEq, _answerid).Where("last", CondEq, true).Not().Where("msg", CondEq,
            "").Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::InvalidAnswerComment;
        return false;
    }

    Item post_itm;
    if (_postid == "" ||
        !g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _postid).Where("block", CondLt, height),
            post_itm).ok())
    {
        result = ANTIBOTRESULT::NotFound;
        return false;
    }

    // Blocking
    if (g_pocketdb->Exists(
        Query("BlockingView").Where("address", CondEq, post_itm["address"].As<string>()).Where("address_to", CondEq,
            _address).Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::Blocking;
        return false;
    }

    // Compute count of comments for last 24 hours
    {
        auto query = Query("Comment").Where("address", CondEq, _address).Where("last", CondEq, true).Where("block",
            CondLt, height);
        if (checkTime_19_6) query = query.Where("time", CondGe, _time - 86400);
        else query = query.Where("block", CondGe, height - 1440);
        int commentsCount = g_pocketdb->SelectCount(query);

        // Also check mempool
        if (checkMempool)
        {
            reindexer::QueryResults res;
            if (g_pocketdb->Select(
                reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid),
                res).ok())
            {
                for (auto& m : res)
                {
                    reindexer::Item mItm = m.GetItem();
                    std::string t_src = DecodeBase64(mItm["data"].As<string>());

                    reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
                    if (t_itm.FromJSON(t_src).ok())
                    {
                        if (t_itm["address"].As<string>() == _address &&
                            t_itm["otxid"].As<string>() == t_itm["txid"].As<string>())
                        {
                            if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                                commentsCount += 1;
                        }
                    }
                }
            }
        }

        // Check block
        if (blockVtx.Exists("Comment"))
        {
            for (auto& mtx : blockVtx.Data["Comment"])
            {
                if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address &&
                    mtx["otxid"].get_str() == mtx["txid"].get_str())
                {
                    if (!checkTime_19_3 || mtx["time"].get_int64() <= _time)
                        commentsCount += 1;
                }
            }
        }

        // Check limit
        ABMODE mode;
        getMode(_address, mode, height);
        int limit = getLimit(Comment, mode, height);
        if (commentsCount >= limit)
        {
            result = ANTIBOTRESULT::CommentLimit;
            return false;
        }
    }

    return true;
}

bool AntiBot::check_comment_edit(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    int64_t _time = oitm["time"].get_int64();

    std::string _msg = oitm["msg"].get_str();
    std::string _txid = oitm["txid"].get_str();
    std::string _otxid = oitm["otxid"].get_str();
    std::string _postid = oitm["postid"].get_str();
    std::string _parentid = oitm["parentid"].get_str();
    std::string _answerid = oitm["answerid"].get_str();

    // User registered?
    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Size message limit
    if (_msg == "" || UrlDecode(_msg).length() > GetActualLimit(Limit::comment_size_limit, height))
    {
        result = ANTIBOTRESULT::Size;
        return false;
    }

    // Original comment exists
    reindexer::Item _original_comment_itm;
    if (!g_pocketdb->SelectOne(
        Query("Comment").Where("otxid", CondEq, _otxid).Where("txid", CondEq, _otxid).Where("address", CondEq,
            _address).Where("block", CondLt, height), _original_comment_itm).ok())
    {
        result = ANTIBOTRESULT::NotFound;
        return false;
    }

    // Last comment not deleted
    if (!g_pocketdb->Exists(
        Query("Comment").Where("otxid", CondEq, _otxid).Where("last", CondEq, true).Where("address", CondEq,
            _address).Not().Where("msg", CondEq, "").Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::CommentDeletedEdit;
        return false;
    }

    // Parent comment
    if (_parentid != _original_comment_itm["parentid"].As<string>() || (_parentid != "" && !g_pocketdb->Exists(
        Query("Comment").Where("otxid", CondEq, _parentid).Where("last", CondEq, true).Not().Where("msg", CondEq,
            "").Where("block", CondLt, height))))
    {
        result = ANTIBOTRESULT::InvalidParentComment;
        return false;
    }

    // Answer comment
    if (_answerid != _original_comment_itm["answerid"].As<string>() || (_answerid != "" && !g_pocketdb->Exists(
        Query("Comment").Where("otxid", CondEq, _answerid).Where("last", CondEq, true).Not().Where("msg", CondEq,
            "").Where("block", CondLt, height))))
    {
        result = ANTIBOTRESULT::InvalidAnswerComment;
        return false;
    }

    // Original comment edit only 24 hours
    auto depth = checkTime_19_6 ? (_time - _original_comment_itm["time"].As<int64_t>()) : (height -
                                                                                           _original_comment_itm["block"].As<int>());
    if (depth > GetActualLimit(Limit::edit_comment_timeout, height))
    {
        result = ANTIBOTRESULT::CommentEditLimit;
        return false;
    }

    Item post_itm;
    if (_postid == "" ||
        !g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _postid).Where("block", CondLt, height),
            post_itm).ok())
    {
        result = ANTIBOTRESULT::NotFound;
        return false;
    }

    // Blocking
    if (g_pocketdb->Exists(
        Query("BlockingView").Where("address", CondEq, post_itm["address"].As<string>()).Where("address_to", CondEq,
            _address).Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::Blocking;
        return false;
    }

    // Double edit in block denied
    if (blockVtx.Exists("Comment"))
    {
        for (auto& mtx : blockVtx.Data["Comment"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid)
            {
                result = ANTIBOTRESULT::DoubleCommentEdit;
                return false;
            }
        }
    }

    // Double edit in mempool denied
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["otxid"].As<string>() == _otxid)
                    {
                        result = ANTIBOTRESULT::DoubleCommentEdit;
                        return false;
                    }
                }
            }
        }
    }

    // Check limit
    {
        size_t edit_count = g_pocketdb->SelectCount(
            Query("Comment").Where("otxid", CondEq, _otxid).Where("block", CondLt, height));

        ABMODE mode;
        getMode(_address, mode, height);
        int limit = getLimit(CommentEdit, mode, height);
        if (edit_count >= limit)
        {
            result = ANTIBOTRESULT::CommentEditLimit;
            return false;
        }
    }

    return true;
}

bool AntiBot::check_comment_delete(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _address = oitm["address"].get_str();
    int64_t _time = oitm["time"].get_int64();

    std::string _txid = oitm["txid"].get_str();
    std::string _otxid = oitm["otxid"].get_str();
    std::string _parentid = oitm["parentid"].get_str();
    std::string _answerid = oitm["answerid"].get_str();

    // User registered?
    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Original comment exists
    reindexer::Item _original_comment_itm;
    if (!g_pocketdb->SelectOne(
        Query("Comment").Where("otxid", CondEq, _otxid).Where("txid", CondEq, _otxid).Where("address", CondEq,
            _address).Where("block", CondLt, height), _original_comment_itm).ok())
    {
        result = ANTIBOTRESULT::NotFound;
        return false;
    }

    // Last comment not deleted
    if (!g_pocketdb->Exists(
        Query("Comment").Where("otxid", CondEq, _otxid).Where("last", CondEq, true).Where("address", CondEq,
            _address).Not().Where("msg", CondEq, "").Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::DoubleCommentDelete;
        return false;
    }

    // Parent comment
    if (_parentid != _original_comment_itm["parentid"].As<string>())
    {
        result = ANTIBOTRESULT::InvalidParentComment;
        return false;
    }

    // Answer comment
    if (_answerid != _original_comment_itm["answerid"].As<string>())
    {
        result = ANTIBOTRESULT::InvalidAnswerComment;
        return false;
    }

    // Double delete in block denied
    if (blockVtx.Exists("Comment"))
    {
        for (auto& mtx : blockVtx.Data["Comment"])
        {
            if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid)
            {
                result = ANTIBOTRESULT::DoubleCommentDelete;
                return false;
            }
        }
    }

    // Double delete in mempool denied
    if (checkMempool)
    {
        reindexer::QueryResults res;
        if (g_pocketdb->Select(
            reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok())
        {
            for (auto& m : res)
            {
                reindexer::Item mItm = m.GetItem();
                std::string t_src = DecodeBase64(mItm["data"].As<string>());

                reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
                if (t_itm.FromJSON(t_src).ok())
                {
                    if (t_itm["otxid"].As<string>() == _otxid)
                    {
                        result = ANTIBOTRESULT::DoubleCommentDelete;
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

bool AntiBot::check_comment_score(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3,
    bool checkTime_19_6, int height, ANTIBOTRESULT& result)
{
    std::string _txid = oitm["txid"].get_str();
    std::string _address = oitm["address"].get_str();
    std::string _comment_id = oitm["commentid"].get_str();
    int _score_value = oitm["value"].get_int();
    int64_t _time = oitm["time"].get_int64();

    if (_score_value != -1 && _score_value != 1)
    {
        result = ANTIBOTRESULT::Failed;
        return false;
    }

    if (!CheckRegistration(oitm, _address, checkMempool, checkTime_19_3, height, blockVtx, result))
    {
        return false;
    }

    // Check score to self
    bool not_found = false;
    std::string _comment_address;
    reindexer::Item commentItm;
    if (g_pocketdb->SelectOne(
        reindexer::Query("Comment").Where("otxid", CondEq, _comment_id).Where("last", CondEq, true).Not().Where("msg",
            CondEq, "").Where("block", CondLt, height), commentItm).ok())
    {
        _comment_address = commentItm["address"].As<string>();

        // Score to self comment
        if (_comment_address == _address)
        {
            result = ANTIBOTRESULT::SelfCommentScore;
            return false;
        }
    }
    else
    {
        // Comment not found
        not_found = true;

        // Maybe in current block?
        if (blockVtx.Exists("Comment"))
        {
            for (auto& mtx : blockVtx.Data["Comment"])
            {
                if (mtx["otxid"].get_str() == _comment_id && mtx["msg"].get_str() != "")
                {
                    _comment_address = mtx["address"].get_str();
                    not_found = false;
                    break;
                }
            }
        }

        if (not_found)
        {
            result = ANTIBOTRESULT::NotFound;
            return false;
        }
    }

    // Blocking
    if (height >= Params().GetConsensus().score_blocking_on && height < Params().GetConsensus().score_blocking_off &&
        g_pocketdb->Exists(Query("BlockingView").Where("address", CondEq, _comment_address).Where("address_to", CondEq,
            _address).Where("block", CondLt, height)))
    {
        result = ANTIBOTRESULT::Blocking;
        return false;
    }

    // Check double score to comment
    reindexer::Item doubleScoreItm;
    if (g_pocketdb->SelectOne(
            reindexer::Query("CommentScores")
                .Where("address", CondEq, _address)
                .Where("commentid", CondEq, _comment_id)
                .Where("block", CondLt, height),
            doubleScoreItm)
        .ok())
    {
        result = ANTIBOTRESULT::DoubleCommentScore;
        return false;
    }

    // Check limit scores
    {
        int scoresCount = 0;
        auto query = reindexer::Query("CommentScores").Where("address", CondEq, _address).Where("block", CondLt,
            height);
        if (checkTime_19_6) query = query.Where("time", CondGe, _time - 86400);
        else query = query.Where("block", CondGe, height - 1440);
        scoresCount += g_pocketdb->SelectCount(query);

        // Also check mempool
        if (checkMempool)
        {
            reindexer::QueryResults res;
            if (g_pocketdb->Select(
                reindexer::Query("Mempool").Where("table", CondEq, "CommentScores").Not().Where("txid", CondEq, _txid),
                res).ok())
            {
                for (auto& m : res)
                {
                    reindexer::Item mItm = m.GetItem();
                    std::string t_src = DecodeBase64(mItm["data"].As<string>());

                    reindexer::Item t_itm = g_pocketdb->DB()->NewItem("CommentScores");
                    if (t_itm.FromJSON(t_src).ok())
                    {
                        if (t_itm["address"].As<string>() == _address)
                        {
                            if (!checkTime_19_3 || t_itm["time"].As<int64_t>() <= _time)
                                scoresCount += 1;

                            if (t_itm["commentid"].As<string>() == _comment_id)
                            {
                                result = ANTIBOTRESULT::DoubleCommentScore;
                                return false;
                            }
                        }
                    }
                }
            }
        }

        // Check block
        if (blockVtx.Exists("CommentScores"))
        {
            for (auto& mtx : blockVtx.Data["CommentScores"])
            {
                if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address)
                {
                    if (!checkTime_19_3 || mtx["time"].get_int64() <= _time)
                        scoresCount += 1;

                    if (mtx["commentid"].get_str() == _comment_id)
                    {
                        result = ANTIBOTRESULT::DoubleCommentScore;
                        return false;
                    }
                }
            }
        }

        ABMODE mode;
        getMode(_address, mode, height);
        int limit = getLimit(CommentScore, mode, height);
        if (scoresCount >= limit)
        {
            result = ANTIBOTRESULT::CommentScoreLimit;
            return false;
        }
    }

    // Check OP_RETURN
    std::vector <std::string> vasm;
    boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));

    // Check address and value in asm == reindexer data
    if (vasm.size() >= 4)
    {
        std::stringstream _op_return_data;
        _op_return_data << vasm[3];
        std::string _op_return_hex = _op_return_data.str();

        std::string _score_itm_val = _comment_address + " " + std::to_string(_score_value);
        std::string _score_itm_hex = HexStr(_score_itm_val.begin(), _score_itm_val.end());

        if (_op_return_hex != _score_itm_hex)
        {
            result = ANTIBOTRESULT::OpReturnFailed;
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------

//-----------------------------------------------------
void AntiBot::CheckTransactionRIItem(UniValue oitm, int height, ANTIBOTRESULT& resultCode)
{
    BlockVTX blockVtx;
    CheckTransactionRIItem(oitm, blockVtx, true, height, resultCode);
}

void AntiBot::CheckTransactionRIItem(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, int height,
    ANTIBOTRESULT& resultCode)
{
    resultCode = ANTIBOTRESULT::Success;
    std::string table = oitm["table"].get_str();
    std::string tx_type = oitm["type"].get_str();
    bool checkTime_19_3 = height < Params().GetConsensus().checkpoint_0_19_3;
    bool checkTime_19_6 = height < Params().GetConsensus().checkpoint_0_19_6;
    bool splitContent = height >= Params().GetConsensus().checkpoint_split_content_video;

    // If `item` with `txid` already in reindexer db - skip checks
    std::string _txid_check_exists = oitm["txid"].get_str();
    if (table == "Posts" && oitm["txidEdit"].get_str() != "") _txid_check_exists = oitm["txidEdit"].get_str();
    if (g_addrindex->CheckRItemExists(table, _txid_check_exists)) return;

    // Check consistent transaction and reindexer::Item
    if (height >= Params().GetConsensus().opreturn_check)
    {
        std::vector <std::string> vasm;
        boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));
        if (vasm.size() < 3)
        {
            resultCode = ANTIBOTRESULT::FailedOpReturn;
            return;
        }

        if (vasm[2] != oitm["data_hash"].get_str())
        {
            if (table != "Users" || (table == "Users" && vasm[2] != oitm["data_hash_without_ref"].get_str()))
            {
                LogPrintf("FailedOpReturn vasm: %s - oitm: %s\n", vasm[2], oitm.write());
                resultCode = ANTIBOTRESULT::FailedOpReturn;
                return;
            }
        }
    }

    // Hard fork for old inconcistents antibot rules
    if (height <= Params().GetConsensus().nHeight_version_1_0_0_pre) return;

    if (table == "Posts")
    {
        if (!check_item_size(oitm, Post, height, resultCode)) return;

        if (splitContent)
        {
            if (tx_type == OR_VIDEO)
            {
                if (oitm["txidEdit"].get_str() != "")
                    check_video_edit(oitm, blockVtx, checkMempool, height, resultCode);
                else
                    check_video(oitm, blockVtx, checkMempool, height, resultCode);
            }
            else
            {
                if (oitm["txidEdit"].get_str() != "")
                    check_post_edit(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, true, height, resultCode);
                else
                    check_post(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, true, height, resultCode);
            }
        }
        else
        {
            if (oitm["txidEdit"].get_str() != "")
                check_post_edit(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, false, height, resultCode);
            else
                check_post(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, false, height, resultCode);
        }
    }
    else if (table == "Scores")
    {
        check_score(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else if (table == "Complains")
    {
        check_complain(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else if (table == "Subscribes")
    {
        check_subscribe(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else if (table == "Blocking")
    {
        check_blocking(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else if (table == "Users")
    {
        if (!check_item_size(oitm, User, height, resultCode)) return;
        check_changeInfo(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else if (table == "Comment")
    {
        if (tx_type == OR_COMMENT)
            check_comment(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
        else if (tx_type == OR_COMMENT_EDIT)
            check_comment_edit(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
        else if (tx_type == OR_COMMENT_DELETE)
            check_comment_delete(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else if (table == "CommentScores")
    {
        check_comment_score(oitm, blockVtx, checkMempool, checkTime_19_3, checkTime_19_6, height, resultCode);
    }
    else
    {
        resultCode = ANTIBOTRESULT::Unknown;
    }
}

bool AntiBot::CheckInputs(CTransactionRef& tx)
{
    for (auto& in : tx->vin)
    {
        if (!g_pocketdb->Exists(
            reindexer::Query("UTXO")
                .Where("txid", CondEq, in.prevout.hash.GetHex())
                .Where("txout", CondEq, (int) in.prevout.n)
                .Where("spent_block", CondEq, 0)))
        {
            return false;
        }
    }

    return true;
}

bool AntiBot::CheckBlock(BlockVTX& blockVtx, int height)
{
    for (auto& t : blockVtx.Data)
    {
        for (auto& mtx : t.second)
        {
            ANTIBOTRESULT resultCode = ANTIBOTRESULT::Success;
            CheckTransactionRIItem(mtx, blockVtx, false, height, resultCode);
            if (resultCode != ANTIBOTRESULT::Success)
            {
                LogPrintf("Transaction check with the AntiBot failed (%s) %s %s\n", mtx["txid"].get_str(), resultCode,
                    t.first);

                // Skip next transactions - already error
                return false;
            }
        }
    }

    return true;
}

bool AntiBot::GetUserState(std::string _address, UserStateItem& _state)
{
    _state = UserStateItem(_address);

    _state.user_registration_date = g_addrindex->GetUserRegistrationDate(_address);
    _state.address_registration_date = g_addrindex->GetAddressRegistrationDate(_address);
    _state.likers = g_addrindex->GetAddressLikers(_address, chainActive.Height() + 1);

    ABMODE mode;
    int reputation = 0;
    int64_t balance = 0;
    getMode(_address, mode, reputation, balance, chainActive.Height() + 1);

    _state.trial = (mode == Trial);
    _state.reputation = reputation;
    _state.balance = balance;

    int postsCount = getLimitsCount("Posts", _address);
    int postsLimit = getLimit(Post, mode, chainActive.Height() + 1);
    _state.post_spent = postsCount;
    _state.post_unspent = postsLimit - postsCount;

    int scoresCount = getLimitsCount("Scores", _address);
    int scoresLimit = getLimit(Score, mode, chainActive.Height() + 1);
    _state.score_spent = scoresCount;
    _state.score_unspent = scoresLimit - scoresCount;

    int complainsCount = getLimitsCount("Complains", _address);
    int complainsLimit = getLimit(Complain, mode, chainActive.Height() + 1);
    _state.complain_spent = complainsCount;
    _state.complain_unspent = complainsLimit - complainsCount;

    int commentsCount = getLimitsCount("Comment", _address);
    int commentsLimit = getLimit(Comment, mode, chainActive.Height() + 1);
    _state.comment_spent = commentsCount;
    _state.comment_unspent = commentsLimit - commentsCount;

    int commentScoresCount = getLimitsCount("CommentScores", _address);
    int commentScoresLimit = getLimit(CommentScore, mode, chainActive.Height() + 1);
    _state.comment_score_spent = commentScoresCount;
    _state.comment_score_unspent = commentScoresLimit - commentScoresCount;

    int allow_reputation_blocking = GetActualLimit(Limit::threshold_reputation_blocking, chainActive.Height() + 1);
    _state.number_of_blocking = g_pocketdb->SelectCount(
        reindexer::Query("BlockingView").Where("address_to", CondEq, _address).Where("address_reputation", CondGe,
            allow_reputation_blocking));

    return true;
}
//-----------------------------------------------------

//-----------------------------------------------------
bool AntiBot::AllowModifyReputation(std::string _score_address, int height)
{
    // Ignore scores from users with rating < Antibot::Limit::threshold_reputation_score
    int64_t _min_user_reputation = GetActualLimit(Limit::threshold_reputation_score, height);
    int _user_reputation = g_pocketdb->GetUserReputation(_score_address, height);
    if (_user_reputation < _min_user_reputation) return false;

    // Ignore scores from users with non verificated reputation
    auto[userId, userRegBlock] = g_pocketdb->GetUserData(_score_address);
    if (userId < 0 || userRegBlock < 0) return false;

    int64_t _min_likers = GetActualLimit(Limit::threshold_likers_count, height);

    if (height >= Params().GetConsensus().checkpoint_0_19_6)
        if (height - userRegBlock > 250'000) _min_likers = 30;

    int _user_likers = g_pocketdb->GetUserLikersCount(userId, height);
    if (_user_likers < _min_likers) return false;

    // All is OK
    return true;
}

bool AntiBot::AllowModifyReputationOverPost(std::string _score_address, std::string _post_address, int height,
    int64_t tx_time, std::string txid, bool lottery)
{
    // Check user reputation
    if (!AllowModifyReputation(_score_address, height)) return false;

    // Disable reputation increment if from one address to one address > 2 scores over day
    int64_t _max_scores_one_to_one = GetActualLimit(Limit::scores_one_to_one, height);
    int64_t _scores_one_to_one_depth = GetActualLimit(Limit::scores_one_to_one_depth, height);

    std::vector<int> values;
    if (lottery)
    {
        values.push_back(4);
        values.push_back(5);
    }
    else
    {
        values.push_back(1);
        values.push_back(2);
        values.push_back(3);
        values.push_back(4);
        values.push_back(5);
    }

    // For calculate ratings include current block
    // For check lottery not include current block (for reindex)
    int blockHeight = height + (lottery ? 0 : 1);

    // TODO (brangr): change time to blocks
    size_t scores_one_to_one_count = g_pocketdb->SelectCount(
        reindexer::Query("Scores")
            .Where("address", CondEq, _score_address)
            .Where("time", CondGe, tx_time - _scores_one_to_one_depth)
            .Where("time", CondLt, tx_time)
            .Where("block", CondLe, blockHeight)
            .Where("value", CondSet, values)
            .Not().Where("txid", CondEq, txid)
            .InnerJoin("posttxid", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, _post_address)));

    if (scores_one_to_one_count >= _max_scores_one_to_one) return false;

    // All is OK
    return true;
}
bool AntiBot::AllowModifyReputationOverPost(std::string _score_address, std::string _post_address, int height,
    const CTransactionRef& tx, bool lottery)
{
    return AllowModifyReputationOverPost(_score_address, _post_address, height, (int64_t) tx->nTime,
        tx->GetHash().GetHex(), lottery);
}

bool AntiBot::AllowModifyReputationOverComment(std::string _score_address, std::string _comment_address, int height,
    const CTransactionRef& tx, bool lottery)
{
    // Check user reputation
    if (!AllowModifyReputation(_score_address, height)) return false;

    // Disable reputation increment if from one address to one address > Limit::scores_one_to_one scores over Limit::scores_one_to_one_depth
    int64_t _max_scores_one_to_one = GetActualLimit(Limit::scores_one_to_one_over_comment, height);
    int64_t _scores_one_to_one_depth = GetActualLimit(Limit::scores_one_to_one_depth, height);

    std::vector<int> values;
    if (lottery)
    {
        values.push_back(1);
    }
    else
    {
        values.push_back(-1);
        values.push_back(1);
    }

    // For calculate ratings include current block
    // For check lottery not include current block (for reindex)
    int blockHeight = height + (lottery ? 0 : 1);

    // TODO (brangr): change time to blocks
    size_t scores_one_to_one_count = g_pocketdb->SelectCount(
        reindexer::Query("CommentScores")
            .Where("address", CondEq, _score_address)
            .Where("time", CondGe, (int64_t) tx->nTime - _scores_one_to_one_depth)
            .Where("time", CondLt, (int64_t) tx->nTime)
            .Where("block", CondLe, blockHeight)
            .Where("value", CondSet, values)
            .Not().Where("txid", CondEq, tx->GetHash().GetHex())
                // join by original id with txid, not otxid
            .InnerJoin("commentid", "txid", CondEq,
                reindexer::Query("Comment").Where("address", CondEq, _comment_address)));

    if (scores_one_to_one_count >= _max_scores_one_to_one) return false;

    // All is OK
    return true;
}