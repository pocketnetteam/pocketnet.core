// Copyright (c) 2018 PocketNet developers
// Indexes for addresses and transactions
// + manupulations with Reindexer DB
//-----------------------------------------------------
#include "index/addrindex.h"
#include "html.h"
#include <consensus/consensus.h>
#include <validation.h>
//-----------------------------------------------------
std::unique_ptr<AddrIndex> g_addrindex;
//-----------------------------------------------------
AddrIndex::AddrIndex()
{
}
AddrIndex::~AddrIndex()
{
}
//-----------------------------------------------------
// LOCAL
//-----------------------------------------------------
bool string_is_valid(const std::string& str)
{
    return find_if(str.begin(), str.end(), [](char c) { return !(isalnum(c)); }) == str.end();
}
//-----------------------------------------------------
// PRIVATE
//-----------------------------------------------------
std::string AddrIndex::getOutAddress(const CTransactionRef& tx)
{
    for (int i = 0; i < tx->vout.size(); i++) {
        const CTxOut& txout = tx->vout[i];
        //-------------------------
        CTxDestination destAddress;
        bool fValidAddress = ExtractDestination(txout.scriptPubKey, destAddress);
        if (fValidAddress) {
            return EncodeDestination(destAddress);
        }
    }
    //-------------------------
    return "";
}

bool AddrIndex::insert_to_mempool(reindexer::Item& item, std::string table)
{
    reindexer::Item memItm = g_pocketdb->DB()->NewItem("Mempool");
    memItm["table"] = table;

    if (table == "Posts") {
        item["caption_"] = "";
        item["message_"] = "";

        // Mempool:
        //   txid - txid of post transaction
        //   txid_source - txid of original post transaction
        // Posts:
        //   txid - txid of original post transaction
        //   txidEdit - txid of post transaction

        std::string post_txid = item["txid"].As<string>();
        std::string post_txidEdit = item["txidEdit"].As<string>();
        if (post_txidEdit == "") {
            post_txidEdit = post_txid;
            post_txid = "";
        }

        memItm["txid"] = post_txidEdit;
        memItm["txid_source"] = post_txid;
    } else {
        memItm["txid"] = item["txid"].As<string>();
        memItm["txid_source"] = "";
    }
    
    memItm["data"] = EncodeBase64(item.GetJSON().ToString());

    return WriteMemRTransaction(memItm);
}

bool AddrIndex::indexUTXO(const CTransactionRef& tx, CBlockIndex* pindex)
{
    std::string txid = tx->GetHash().GetHex();

    // Get all addresses from tx outs
    for (int i = 0; i < tx->vout.size(); i++) {
        const CTxOut& txout = tx->vout[i];
        //-------------------------
        CTxDestination destAddress;
        if (!ExtractDestination(txout.scriptPubKey, destAddress)) continue;
        std::string encoded_address = EncodeDestination(destAddress);

        // Add Unspent transaction for this addresses
        reindexer::Item item = g_pocketdb->DB()->NewItem("UTXO");
        item["address"] = encoded_address;
        item["txid"] = txid;
        item["block"] = pindex->nHeight;
        item["txout"] = i;
        item["time"] = (int64_t)tx->nTime;
        item["amount"] = (int64_t)txout.nValue;
        item["spent_block"] = 0;

        if (!g_pocketdb->UpsertWithCommit("UTXO", item).ok()) return false;
    }

    // Get all addresses from tx ins
    if (!tx->IsCoinBase()) {
        for (int i = 0; i < tx->vin.size(); i++) {
            const CTxIn& txin = tx->vin[i];
            std::string txinid = txin.prevout.hash.GetHex();
            int txinout = (int)txin.prevout.n;

            // Mark UTXO item as deleted
            reindexer::QueryResults _res;
            reindexer::Error err = g_pocketdb->DB()->Select(reindexer::Query("UTXO", 0, 1).Where("txid", CondEq, txinid).Where("txout", CondEq, txinout), _res);
            if (!err.ok() || _res.Count() <= 0) {
                LogPrintf("Error get item: %s (%d)\n", err.code(), err.what());
                return false;
            }

            reindexer::Item item = _res[0].GetItem();
            item["spent_block"] = pindex->nHeight;

            if (!g_pocketdb->UpsertWithCommit("UTXO", item).ok()) return false;
        }
    }

    return true;
}

bool AddrIndex::indexAddress(const CTransactionRef& tx, CBlockIndex* pindex)
{
    std::string txid = tx->GetHash().GetHex();

    // Get all addresses from tx outs
    for (int i = 0; i < tx->vout.size(); i++) {
        const CTxOut& txout = tx->vout[i];

        // Check destinations
        CTxDestination destAddress;
        if (!ExtractDestination(txout.scriptPubKey, destAddress)) continue;
        std::string encoded_address = EncodeDestination(destAddress);

        // Check this address already registered
        if (g_pocketdb->Exists(reindexer::Query("Addresses").Where("address", CondEq, encoded_address))) continue;

        // New Address -> Save with transaction id and time
        reindexer::Item item = g_pocketdb->DB()->NewItem("Addresses");
        item["address"] = encoded_address;
        item["txid"] = txid;
        item["block"] = pindex->nHeight;
        item["time"] = (int64_t)tx->nTime;
        if (!g_pocketdb->UpsertWithCommit("Addresses", item).ok()) return false;
    }

    return true;
}

bool AddrIndex::indexTags(const CTransactionRef& tx, CBlockIndex* pindex)
{
    // Check this transaction contains `Post`
    std::string ri_table;
    if (!GetPocketnetTXType(tx, ri_table) || ri_table != "Posts") return true;

    // First get post with tags
    reindexer::Item postItm;
    if (!g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, tx->GetHash().GetHex()), postItm).ok()) return false;

    // Parse tags and check exists
    reindexer::VariantArray vaTags = postItm["tags"];
    std::vector<std::string> vTags;
    for (int i = 0; i < vaTags.size(); i++) {
        std::string _tag = vaTags[i].As<string>();
        if (_tag.size() > 0) {
            reindexer::QueryResults _res;
            g_pocketdb->Select(reindexer::Query("Tags").Where("tag", CondEq, _tag), _res);
            if (_res.Count() > 0) {
                for (auto& it : _res) {
                    reindexer::Item _tagItm = it.GetItem();
                    _tagItm["rating"] = _tagItm["rating"].As<int>() + 1;
                    g_pocketdb->Update("Tags", _tagItm);
                }
            } else {
                vTags.push_back(_tag);
            }
        }
    }

    // Save new
    if (vTags.size() > 0) {
        for (auto& it : vTags) {
            reindexer::Item _tagItm = g_pocketdb->DB()->NewItem("Tags");
            _tagItm["tag"] = it;
            _tagItm["rating"] = 1;
            g_pocketdb->Upsert("Tags", _tagItm);
        }
    }

    return true;
}

bool AddrIndex::indexPost(const CTransactionRef& tx, CBlockIndex* pindex)
{
    // First get post
    reindexer::Item postItm;
    if (!g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, tx->GetHash().GetHex()).Where("txidEdit", CondEq, ""), postItm).ok()) {
        if (!g_pocketdb->SelectOne(reindexer::Query("PostsHistory").Where("txid", CondEq, tx->GetHash().GetHex()).Where("txidEdit", CondEq, ""), postItm).ok()) {
            if (!g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txidEdit", CondEq, tx->GetHash().GetHex()), postItm).ok()) {
                if (!g_pocketdb->SelectOne(reindexer::Query("PostsHistory").Where("txidEdit", CondEq, tx->GetHash().GetHex()), postItm).ok()) {
                    return false;
                }
            }
        }
    }










    return true;
}
//-----------------------------------------------------
// RATINGS
//-----------------------------------------------------
bool AddrIndex::indexRating(const CTransactionRef& tx,
    std::map<std::string, double>& userReputations,
    std::map<std::string, std::pair<int, int>>& postRatings,
    std::map<std::string, int>& postReputations,
    CBlockIndex* pindex)
{
    std::string txid = tx->GetHash().GetHex();

    // Find this Score in DB for get upvote value
    Item scoreItm;
    if (!g_pocketdb->SelectOne(reindexer::Query("Scores").Where("txid", CondEq, txid), scoreItm).ok()) return false;
    std::string score_address = scoreItm["address"].As<string>();
    std::string posttxid = scoreItm["posttxid"].As<string>();
    int scoreVal = scoreItm["value"].As<int>();

    // Find post for get author address
    Item postItm;
    if (!g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, posttxid), postItm).ok()) return false;
    std::string post_address = postItm["address"].As<string>();


    // Save rating for post in any case
    if (postRatings.find(posttxid) == postRatings.end()) postRatings.insert(std::make_pair(posttxid, std::make_pair(0, 0)));
    postRatings[posttxid].first += scoreVal;
    postRatings[posttxid].second += 1;


    // Modify reputation for user and post
    std::string _check_score_address = score_address;
    if (pindex->nHeight < Params().GetConsensus().nHeight_fix_ratings) {
        _check_score_address = post_address;
    }

    // Scores with small reputation and scores one to one
    bool modify_by_user_reputation = g_antibot->AllowModifyReputationOverPost(_check_score_address, post_address, pindex->nHeight - 1, tx, false);

    // Scores to old posts not modify reputation
    bool modify_block_old_post = (tx->nTime - postItm["time"].As<int64_t>()) < GetActualLimit(Limit::scores_depth_modify_reputation, pindex->nHeight - 1);

    // USER & POST reputation
    if (modify_by_user_reputation && modify_block_old_post) {

        // User reputation
        if (userReputations.find(post_address) == userReputations.end()) userReputations.insert(std::make_pair(post_address, 0));
        userReputations[post_address] += scoreVal - 3; // Reputation between -2 and 2

        // Post reputation
        if (postReputations.find(posttxid) == postReputations.end()) postReputations.insert(std::make_pair(posttxid, 0));
        postReputations[posttxid] += scoreVal - 3; // Reputation between -2 and 2

    }

    return true;
}

bool AddrIndex::indexCommentRating(const CTransactionRef& tx,
    std::map<std::string, double>& userReputations,
    std::map<std::string, std::pair<int, int>>& commentRatings,
    std::map<std::string, int>& commentReputations,
    CBlockIndex* pindex)
{
    std::string txid = tx->GetHash().GetHex();

    // Find this Score in DB for get upvote value
    Item scoreCommentItm;
    if (!g_pocketdb->SelectOne(reindexer::Query("CommentScores").Where("txid", CondEq, txid), scoreCommentItm).ok()) return false;
    std::string score_address = scoreCommentItm["address"].As<string>();
    std::string commentid = scoreCommentItm["commentid"].As<string>();
    int scoreVal = scoreCommentItm["value"].As<int>();

    // Find comment for get author address
    Item commentItm;
    if (!g_pocketdb->SelectOne(reindexer::Query("Comment").Where("otxid", CondEq, commentid).Where("last", CondEq, true), commentItm).ok()) return false;
    std::string comment_address = commentItm["address"].As<string>();


    // Save rating for comment in any case
    if (commentRatings.find(commentid) == commentRatings.end()) commentRatings.insert(std::make_pair(commentid, std::make_pair(0, 0)));
    if (scoreVal > 0) commentRatings[commentid].first += 1;
    else commentRatings[commentid].second += 1;


    // Modify reputation for comment
    bool allow_modify_reputation = g_antibot->AllowModifyReputationOverComment(score_address, comment_address, pindex->nHeight - 1, tx, false);

    // USER & COMMENT reputation
    if (allow_modify_reputation) {

        // User reputation
        if (userReputations.find(comment_address) == userReputations.end()) userReputations.insert(std::make_pair(comment_address, 0));
        userReputations[comment_address] += scoreVal / 10.0; // Reputation equals -0.1 or 0.1

        // Comment reputation        
        if (commentReputations.find(commentid) == commentReputations.end()) commentReputations.insert(std::make_pair(commentid, 0));
        commentReputations[commentid] += scoreVal;

    }

    return true;
}

bool AddrIndex::computeUsersRatings(CBlockIndex* pindex, std::map<std::string, double>& userReputations)
{
    for (auto& ur : userReputations) {

        double rep = g_pocketdb->GetUserReputation(ur.first, pindex->nHeight - 1);
        rep += ur.second;

        // Create new item with this height - new accumulating rating
        reindexer::Item _itm_rating_new = g_pocketdb->DB()->NewItem("UserRatings");
        _itm_rating_new["address"] = ur.first;
        _itm_rating_new["block"] = pindex->nHeight;
        _itm_rating_new["reputation"] = rep;
        if (!g_pocketdb->UpsertWithCommit("UserRatings", _itm_rating_new).ok()) return false;

        // Update user reputation
        if (!g_pocketdb->UpdateUserReputation(ur.first, rep)) return false;
    }

    return true;
}

bool AddrIndex::computePostsRatings(CBlockIndex* pindex, std::map<std::string, std::pair<int, int>>& postRatings, std::map<std::string, int>& postReputations)
{
    for (auto& pr : postRatings) {
        int sum = 0;
        int cnt = 0;
        int rep = 0;

        // First get existed ratings
        // Ratings without this connected index
        g_pocketdb->GetPostRating(pr.first, sum, cnt, rep, pindex->nHeight - 1);

        if (postReputations.find(pr.first) != postReputations.end()) {
            rep += postReputations[pr.first];
        }

        // Increment rating and count
        sum += pr.second.first;
        cnt += pr.second.second;

        // Create new item with this height - new accumulating rating
        reindexer::Item _itm_rating_new = g_pocketdb->DB()->NewItem("PostRatings");
        _itm_rating_new["posttxid"] = pr.first;
        _itm_rating_new["block"] = pindex->nHeight;
        _itm_rating_new["scoreSum"] = sum;
        _itm_rating_new["scoreCnt"] = cnt;
        _itm_rating_new["reputation"] = rep;

        if (!g_pocketdb->UpsertWithCommit("PostRatings", _itm_rating_new).ok()) return false;

        // Update post rating
        if (!g_pocketdb->UpdatePostRating(pr.first, sum, cnt, rep)) return false;
    }

    return true;
}

bool AddrIndex::computeCommentRatings(CBlockIndex* pindex, std::map<std::string, std::pair<int, int>>& commentRatings, std::map<std::string, int>& commentReputations)
{
    for (auto& pr : commentRatings) {
        int up = 0;
        int down = 0;
        int rep = 0;

        // First get existed ratings
        // Ratings without this connected index
        g_pocketdb->GetCommentRating(pr.first, up, down, rep, pindex->nHeight - 1);

        if (commentReputations.find(pr.first) != commentReputations.end()) {
            rep += commentReputations[pr.first];
        }

        // Increment rating and count
        up += pr.second.first;
        down += pr.second.second;

        // Create new item with this height - new accumulating rating
        reindexer::Item _itm_rating_new = g_pocketdb->DB()->NewItem("CommentRatings");
        _itm_rating_new["commentid"] = pr.first;
        _itm_rating_new["block"] = pindex->nHeight;
        _itm_rating_new["scoreUp"] = up;
        _itm_rating_new["scoreDown"] = down;
        _itm_rating_new["reputation"] = rep;

        if (!g_pocketdb->UpsertWithCommit("CommentRatings", _itm_rating_new).ok()) return false;

        // Update Comment rating
        if (!g_pocketdb->UpdateCommentRating(pr.first, up, down, rep)) return false;
    }

    return true;
}
//-----------------------------------------------------
// PUBLIC
//-----------------------------------------------------
bool AddrIndex::IndexBlock(const CBlock& block, CBlockIndex* pindex)
{
    // User reputations map for this block
    // <address, rep>
    std::map<std::string, double> userReputations;

    // Post ratings map for this block
    // <posttxid, <sum, cnt>>
    std::map<std::string, std::pair<int, int>> postRatings;
    
    // Post reputations map for this block
    // <posttxid, rep>
    std::map<std::string, int> postReputations;
    
    // Comment ratings map for this block
    // <commentid, <up, down>>
    std::map<std::string, std::pair<int, int>> commentRatings;

    // Comment reputations map for this block
    // <commentid, rep>
    std::map<std::string, int> commentReputations;

    for (const auto& tx : block.vtx) {
        // Indexing UTXOs
        if (!indexUTXO(tx, pindex)) {
            LogPrintf("(AddrIndex::IndexBlock) indexUTXO - tx (%s)\n", tx->GetHash().GetHex());
            return false;
        }

        // Indexing addresses
        if (!indexAddress(tx, pindex)) {
            LogPrintf("(AddrIndex::IndexBlock) indexAddress - tx (%s)\n", tx->GetHash().GetHex());
            return false;
        }

        std::string ri_table;
        if (!GetPocketnetTXType(tx, ri_table)) continue;

        // Indexing ratings
        if (ri_table == "Scores" && !indexRating(tx, userReputations, postRatings, postReputations, pindex)) {
            LogPrintf("(AddrIndex::IndexBlock) indexRating - tx (%s)\n", tx->GetHash().GetHex());
            return false;
        }

        // Indexing ratings
        if (ri_table == "CommentScores" && !indexCommentRating(tx, userReputations, commentRatings, commentReputations, pindex)) {
            LogPrintf("(AddrIndex::IndexBlock) indexCommentRating - tx (%s)\n", tx->GetHash().GetHex());
            return false;
        }
        
        if (ri_table == "Posts" && !indexPost(tx, pindex)) {
            LogPrintf("(AddrIndex::IndexBlock) indexPost - tx (%s)\n", tx->GetHash().GetHex());
            return false;
        }
    }

    // Save ratings for users
    if (!computeUsersRatings(pindex, userReputations)) {
        LogPrintf("(AddrIndex::IndexBlock) computeUsersRatings - block (%s)\n", block.GetHash().GetHex());
        return false;
    }

    // Save ratings for posts
    if (!computePostsRatings(pindex, postRatings, postReputations)) {
        LogPrintf("(AddrIndex::IndexBlock) computePostsRatings - block (%s)\n", block.GetHash().GetHex());
        return false;
    }

    // Save ratings for comment
    if (!computeCommentRatings(pindex, commentRatings, commentReputations)) {
        LogPrintf("(AddrIndex::IndexBlock) computeCommentRatings - block (%s)\n", block.GetHash().GetHex());
        return false;
    }

    return true;
}

bool AddrIndex::CheckRItemExists(std::string table, std::string txid)
{
    if (table == "Posts")
    {
        if (g_pocketdb->Exists(reindexer::Query("Posts").Where("txid", CondEq, txid).Where("txidEdit", CondEq, ""))) return true;
        if (g_pocketdb->Exists(reindexer::Query("Posts").Where("txidEdit", CondEq, txid))) return true;
        if (g_pocketdb->Exists(reindexer::Query("PostsHistory").Where("txid", CondEq, txid).Where("txidEdit", CondEq, ""))) return true;
        if (g_pocketdb->Exists(reindexer::Query("PostsHistory").Where("txidEdit", CondEq, txid))) return true;
        return false;
    }
    else
    {
        return g_pocketdb->Exists(reindexer::Query(table).Where("txid", CondEq, txid));
    }
}

bool AddrIndex::WriteMemRTransaction(reindexer::Item& item)
{
    return WriteRTransaction("Mempool", item, -1);
}

bool AddrIndex::WriteRTransaction(std::string table, reindexer::Item& item, int height)
{
    std::string _txid_check_exists = item["txid"].As<string>();

    // If new mempool transaction
    if (table == "Mempool") {
        // If received transaction is mempool
        // need check general table - maybe data already received and worked?
        if (CheckRItemExists(item["table"].As<string>(), _txid_check_exists)) return true;
        if (g_pocketdb->UpsertWithCommit("Mempool", item).ok()) return true;
        return false;
    }
    // Or post transaction?
    else if (table == "Posts" && item["txidEdit"].As<string>() != "") {
        _txid_check_exists = item["txidEdit"].As<string>();
    }

    if (CheckRItemExists(table, _txid_check_exists)) return true;

    // For all tables, except Mempool, need set `block` number
    item["block"] = height;

    // Change User Profile
    if (table == "Users") {
        std::string _address = item["address"].As<string>();

        reindexer::Item user_cur;
        if (g_pocketdb->SelectOne(reindexer::Query("UsersView").Where("address", CondEq, _address), user_cur).ok()) {
            item["id"] = user_cur["id"].As<int>();
            item["regdate"] = user_cur["regdate"].As<int64_t>();
            item["referrer"] = user_cur["referrer"].As<string>();
        } else {
            item["id"] = (int)g_pocketdb->SelectTotalCount("UsersView");
            item["regdate"] = item["time"].As<int64_t>();
        }

        if (!g_pocketdb->UpsertWithCommit("Users", item).ok()) return false;
        if (!g_pocketdb->UpdateUsersView(_address, height).ok()) return false;
    }

    // New Post
    if (table == "Posts") {
        std::string caption_decoded = UrlDecode(item["caption"].As<string>());
        item["caption_"] = ClearHtmlTags(caption_decoded);

        std::string message_decoded = UrlDecode(item["message"].As<string>());
        item["message_"] = ClearHtmlTags(message_decoded);

        if (!g_pocketdb->CommitPostItem(item).ok()) return false;
    }

    // Score for post
    if (table == "Scores") {
        if (!g_pocketdb->UpsertWithCommit("Scores", item).ok()) return false;
    }

    // Complaine for post
    if (table == "Complains") {
        if (!g_pocketdb->UpsertWithCommit("Complains", item).ok()) return false;
    }

    // New subscribe or unsubscribe
    if (table == "Subscribes") {
        if (!g_pocketdb->UpsertWithCommit("Subscribes", item).ok()) return false;
        if (!g_pocketdb->UpdateSubscribesView(item["address"].As<string>(), item["address_to"].As<string>()).ok()) return false;
    }

    // New blocking or unblocking
    if (table == "Blocking") {
        if (!g_pocketdb->UpsertWithCommit("Blocking", item).ok()) return false;
        if (!g_pocketdb->UpdateBlockingView(item["address"].As<string>(), item["address_to"].As<string>()).ok()) return false;
    }

    // New Comment
    if (table == "Comment") {
        if (!g_pocketdb->CommitLastItem("Comment", item).ok()) return false;
    }

    // Comment score
    if (table == "CommentScores") {
        if (!g_pocketdb->UpsertWithCommit("CommentScores", item).ok()) return false;
    }

    return true;
}

bool AddrIndex::FindPocketNetAsmString(const CTransactionRef& tx, std::vector<std::string>& vasm)
{
    std::string asmStr;
    if (!FindPocketNetAsmString(tx, asmStr)) return false;
    boost::split(vasm, asmStr, boost::is_any_of("\t "));
    //-------------------------
    return true;
}
bool AddrIndex::FindPocketNetAsmString(const CTransactionRef& tx, std::string& asmStr)
{
    const CTxOut& txout = tx->vout[0];
    if (txout.scriptPubKey[0] == OP_RETURN) {
        asmStr = ScriptToAsmStr(txout.scriptPubKey);
        return true;
    }

    return false;
}

bool AddrIndex::GetPocketnetTXType(const CTransactionRef& tx, std::string& ri_table)
{
    std::vector<std::string> vasm;
    if (!FindPocketNetAsmString(tx, vasm)) return false;
    return ConvertOPToTableName(vasm[1], ri_table);
}
bool AddrIndex::IsPocketnetTransaction(const CTransactionRef& tx)
{
    std::string _ri_table = "";
    return GetPocketnetTXType(tx, _ri_table);
}
bool AddrIndex::IsPocketnetTransaction(const CTransaction& tx)
{
    return IsPocketnetTransaction(MakeTransactionRef(tx));
}

bool AddrIndex::RollbackDB(int blockHeight, bool back_to_mempool)
{
    // Deleting Scores
    {
        if (back_to_mempool) {
            reindexer::QueryResults _scores_res;
            if (g_pocketdb->DB()->Select(reindexer::Query("Scores").Where("block", CondGt, blockHeight), _scores_res).ok()) {
                for (auto& it : _scores_res) {
                    reindexer::Item _score_itm = it.GetItem();
                    if (back_to_mempool && !insert_to_mempool(_score_itm, "Scores")) return false;
                }
            }
        }

        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("Scores").Where("block", CondGt, blockHeight)).ok()) return false;
    }

    // Rollback Posts
    {
        reindexer::QueryResults _posts_res;
        if (!g_pocketdb->DB()->Select(reindexer::Query("Posts").Where("block", CondGt, blockHeight), _posts_res).ok()) return false;
        for (auto& it : _posts_res) {
            reindexer::Item _delete_post_itm = it.GetItem();
            std::string _post_txid = _delete_post_itm["txid"].As<string>();

            // TODO (brangr): Temporaly remove tags
            {
                // reindexer::VariantArray vaTags = _post_itm["tags"];
                // std::vector<std::string> vTags;
                // for (int i = 0; i < vaTags.size(); i++) {
                //     std::string _tag = vaTags[i].As<string>();
                //     if (_tag.size() > 0) {
                //         vTags.push_back(_tag);
                //     }
                // }

                // if (vTags.size() > 0) {
                //     reindexer::QueryResults _res;
                //     g_pocketdb->Select(reindexer::Query("Tags").Where("tag", CondSet, vTags), _res);
                //     for (auto& it : _res) {
                //         reindexer::Item _tagItm = it.GetItem();
                //         _tagItm["rating"] = _tagItm["rating"].As<int>() - 1;
                //         g_pocketdb->Update("Tags", _tagItm);
                //     }
                // }
            }

            if (back_to_mempool && !insert_to_mempool(_delete_post_itm, "Posts")) return false;
            if (!g_pocketdb->RestorePostItem(_post_txid, blockHeight).ok()) return false;
        }
    }

    // Rollback Reposts
    {

    }

    // Rollback Complains
    {
        if (back_to_mempool) {
            reindexer::QueryResults _complains_res;
            if (g_pocketdb->DB()->Select(reindexer::Query("Complains").Where("block", CondGt, blockHeight), _complains_res).ok()) {
                for (auto& it : _complains_res) {
                    reindexer::Item _post_itm = it.GetItem();
                    if (back_to_mempool && !insert_to_mempool(_post_itm, "Complains")) return false;
                }
            }
        }

        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("Complains").Where("block", CondGt, blockHeight)).ok()) return false;
    }

    // Rollback UTXO
    {
        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("UTXO").Where("block", CondGt, blockHeight)).ok()) return false;

        reindexer::QueryResults _utxo_res;
        if (g_pocketdb->DB()->Select(reindexer::Query("UTXO").Where("spent_block", CondGt, blockHeight), _utxo_res).ok()) {
            for (auto& it : _utxo_res) {
                reindexer::Item _utxo_itm = it.GetItem();
                _utxo_itm["spent_block"] = 0;
                if (!g_pocketdb->UpsertWithCommit("UTXO", _utxo_itm).ok()) return false;
            }
        }
    }

    // Rollback Addresses
    {
        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("Addresses").Where("block", CondGt, blockHeight)).ok()) return false;
    }

    // Cleaning Users with restore from UsersHistory
    {
        reindexer::QueryResults _users_res;
        if (!g_pocketdb->DB()->Select(reindexer::Query("Users").Where("block", CondGt, blockHeight), _users_res).ok()) return false;
        for (auto& it : _users_res) {
            reindexer::Item _user_itm = it.GetItem();
            std::string _user_txid = _user_itm["txid"].As<string>();
            std::string _user_address = _user_itm["address"].As<string>();

            // First remove current profile
            if (back_to_mempool && !insert_to_mempool(_user_itm, "Users")) return false;
            if (!g_pocketdb->DeleteWithCommit(reindexer::Query("Users").Where("txid", CondEq, _user_txid)).ok()) return false;
            if (!g_pocketdb->UpdateUsersView(_user_address, blockHeight).ok()) return false;
        }
    }

    // Cleaning Subscribes with restore from SubscribesHistory
    {
        reindexer::QueryResults _subs_res;
        if (!g_pocketdb->DB()->Select(reindexer::Query("Subscribes").Where("block", CondGt, blockHeight), _subs_res).ok()) return false;
        for (auto& it : _subs_res) {
            reindexer::Item _subs_itm = it.GetItem();
            std::string _subs_txid = _subs_itm["txid"].As<string>();
            std::string _subs_address = _subs_itm["address"].As<string>();
            std::string _subs_address_to = _subs_itm["address_to"].As<string>();

            if (back_to_mempool && !insert_to_mempool(_subs_itm, "Subscribes")) return false;
            if (!g_pocketdb->DeleteWithCommit(reindexer::Query("Subscribes").Where("txid", CondEq, _subs_txid)).ok()) return false;
            if (!g_pocketdb->UpdateSubscribesView(_subs_address, _subs_address_to).ok()) return false;
        }
    }

    // Cleaning Blocking with restore from BlockingHistory
    {
        reindexer::QueryResults _bl_res;
        if (!g_pocketdb->DB()->Select(reindexer::Query("Blocking").Where("block", CondGt, blockHeight), _bl_res).ok()) return false;
        for (auto& it : _bl_res) {
            reindexer::Item _bl_itm = it.GetItem();
            std::string _bl_txid = _bl_itm["txid"].As<string>();
            std::string _bl_address = _bl_itm["address"].As<string>();
            std::string _bl_address_to = _bl_itm["address_to"].As<string>();

            if (back_to_mempool && !insert_to_mempool(_bl_itm, "Blocking")) return false;
            if (!g_pocketdb->DeleteWithCommit(reindexer::Query("Blocking").Where("txid", CondEq, _bl_txid)).ok()) return false;
            if (!g_pocketdb->UpdateBlockingView(_bl_address, _bl_address_to).ok()) return false;
        }
    }
    
    // Rollback CommentScores
    {
        if (back_to_mempool) {
            reindexer::QueryResults _comment_scores_res;
            if (g_pocketdb->DB()->Select(reindexer::Query("CommentScores").Where("block", CondGt, blockHeight), _comment_scores_res).ok()) {
                for (auto& it : _comment_scores_res) {
                    reindexer::Item _comment_score_itm = it.GetItem();
                    if (back_to_mempool && !insert_to_mempool(_comment_score_itm, "CommentScores")) return false;
                }
            }
        }

        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("CommentScores").Where("block", CondGt, blockHeight)).ok()) return false;
    }

    // Rollback Comments
    {
        reindexer::QueryResults _comment_res;
        if (!g_pocketdb->DB()->Select(reindexer::Query("Comment").Where("block", CondGt, blockHeight), _comment_res).ok()) return false;
        for (auto& it : _comment_res) {
            reindexer::Item _delete_comment_itm = it.GetItem();
            std::string _comment_txid = _delete_comment_itm["txid"].As<string>();
            std::string _comment_otxid = _delete_comment_itm["otxid"].As<string>();

            if (back_to_mempool && !insert_to_mempool(_delete_comment_itm, "Comment")) return false;
            if (!g_pocketdb->RestoreLastItem("Comment", _comment_txid, _comment_otxid, blockHeight).ok()) return false;
        }
    }

    // Rollback Users Ratings
    {
        std::vector<std::string> vUsersRatingRefresh;

        // First save all users for update rating after rollback ratings blocks
        reindexer::QueryResults usersRatingsRes;
        if (!g_pocketdb->DB()->Select(reindexer::Query("UserRatings").Where("block", CondGt, blockHeight), usersRatingsRes).ok()) return false;
        for (auto& it : usersRatingsRes) {
            reindexer::Item userRatingItm = it.GetItem();
            std::string _user_address = userRatingItm["address"].As<string>();

            if (std::find(vUsersRatingRefresh.begin(), vUsersRatingRefresh.end(), _user_address) == vUsersRatingRefresh.end()) {
                vUsersRatingRefresh.push_back(_user_address);
            }
        }

        // Rollback ratings blocks
        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("UserRatings").Where("block", CondGt, blockHeight)).ok()) return false;

        // Update users ratings
        for (auto& _user_address : vUsersRatingRefresh) {
            if (!g_pocketdb->UpdateUserReputation(_user_address, blockHeight)) return false;
        }
    }

    // Rollback Posts Ratings
    {
        std::vector<std::string> vPostsRatingRefresh;

        // First save all users for update rating after rollback ratings blocks
        reindexer::QueryResults postsRatingsRes;
        if (!g_pocketdb->DB()->Select(reindexer::Query("PostRatings").Where("block", CondGt, blockHeight), postsRatingsRes).ok()) return false;
        for (auto& it : postsRatingsRes) {
            reindexer::Item postRatingRes = it.GetItem();
            std::string _posttxid = postRatingRes["posttxid"].As<string>();

            if (std::find(vPostsRatingRefresh.begin(), vPostsRatingRefresh.end(), _posttxid) == vPostsRatingRefresh.end()) {
                vPostsRatingRefresh.push_back(_posttxid);
            }
        }

        // Rollback ratings blocks
        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("PostRatings").Where("block", CondGt, blockHeight)).ok()) return false;

        // Update posts ratings
        for (auto& _posttxid : vPostsRatingRefresh) {
            if (!g_pocketdb->UpdatePostRating(_posttxid, blockHeight)) return false;
        }
    }

    // Rollback Comment Ratings
    {
        std::vector<std::string> vCommentRatingsRefresh;

        // First save all comments for update rating after rollback ratings blocks
        reindexer::QueryResults commentRatingsRes;
        if (!g_pocketdb->DB()->Select(reindexer::Query("CommentRatings").Where("block", CondGt, blockHeight), commentRatingsRes).ok()) return false;
        for (auto& it : commentRatingsRes) {
            reindexer::Item commentRatingRes = it.GetItem();
            std::string _commentoid = commentRatingRes["commentid"].As<string>();

            if (std::find(vCommentRatingsRefresh.begin(), vCommentRatingsRefresh.end(), _commentoid) == vCommentRatingsRefresh.end()) {
                vCommentRatingsRefresh.push_back(_commentoid);
            }
        }

        // Rollback ratings blocks
        if (!g_pocketdb->DeleteWithCommit(reindexer::Query("CommentRatings").Where("block", CondGt, blockHeight)).ok()) return false;

        // Update posts ratings
        for (auto& _commentoid : vCommentRatingsRefresh) {
            if (!g_pocketdb->UpdateCommentRating(_commentoid, blockHeight)) return false;
        }
    }

    return true;
}

bool AddrIndex::GetAddressRegistrationDate(std::vector<std::string> addresses,
    std::vector<AddressRegistrationItem>& registrations)
{
    //db->AddIndex("Addresses", { "address", "hash", "string", IndexOpts().PK() });
    //db->AddIndex("Addresses", { "txid", "", "string", IndexOpts() });
    //db->AddIndex("Addresses", { "time", "tree", "int64", IndexOpts() });
    //-------------------------
    reindexer::QueryResults queryRes;
    reindexer::Error err = g_pocketdb->DB()->Select(
        reindexer::Query("Addresses").Where("address", CondSet, addresses),
        queryRes);
    //-------------------------
    if (err.ok()) {
        for (auto it : queryRes) {
            reindexer::Item itm(it.GetItem());
            registrations.push_back(
                AddressRegistrationItem(
                    itm["address"].As<string>(),
                    itm["txid"].As<string>(),
                    itm["time"].As<time_t>()));
        }
    }
    //-------------------------
    return true;
}

int64_t AddrIndex::GetAddressRegistrationDate(std::string _address)
{
    //db->AddIndex("Addresses", { "address", "hash", "string", IndexOpts().PK() });
    //db->AddIndex("Addresses", { "txid", "", "string", IndexOpts() });
    //db->AddIndex("Addresses", { "time", "tree", "int64", IndexOpts() });
    //-------------------------
    reindexer::Item addrRes;
    reindexer::Error err = g_pocketdb->SelectOne(
        reindexer::Query("Addresses").Where("address", CondEq, _address),
        addrRes);

    if (err.ok()) {
        return addrRes["time"].As<int64_t>();
    } else {
        return -1;
    }
}

bool AddrIndex::GetUnspentTransactions(std::vector<std::string> addresses,
    std::vector<AddressUnspentTransactionItem>& transactions)
{
    // Get unspent transactions for addresses list from DB
    reindexer::QueryResults queryRes;
    reindexer::Error err = g_pocketdb->DB()->Select(
        reindexer::Query("UTXO")
            .Where("address", CondSet, addresses)
            .Where("spent_block", CondEq, 0),
        queryRes);
    //-------------------------
    if (err.ok()) {
        for (auto it : queryRes) {
            reindexer::Item itm(it.GetItem());
            transactions.emplace_back(
                AddressUnspentTransactionItem(
                    itm["address"].As<string>(),
                    itm["txid"].As<string>(),
                    itm["txout"].As<int>()));
        }
    }
    //-------------------------
    return true;
}

int64_t AddrIndex::GetUserRegistrationDate(std::string _address)
{
    reindexer::Item userItm;
    reindexer::Error err = g_pocketdb->SelectOne(reindexer::Query("UsersView").Where("address", CondEq, _address), userItm);

    if (err.ok()) {
        return userItm["regdate"].As<int64_t>();
    } else {
        return -1;
    }
}

bool AddrIndex::GetRecomendedSubscriptions(std::string _address, int count, std::vector<string>& recommendedSubscriptions)
{
    int sampleSize = 1000; // size of representative sample

    std::vector<std::string> subscriptions;
    std::vector<std::string> fellowSubscribers;

    reindexer::QueryResults queryRes1;
    // Get address Subscriptions
    reindexer::Error err = g_pocketdb->DB()->Select(
        reindexer::Query("SubscribesView").Where("address", CondEq, _address).Where("private", CondEq, false),
        queryRes1);
    //-------------------------
    if (err.ok() && queryRes1.Count() > 0) {
        for (auto it : queryRes1) {
            reindexer::Item itm(it.GetItem());
            std::string _addr = itm["address_to"].As<string>();
            if (std::find(subscriptions.begin(), subscriptions.end(), _addr) != subscriptions.end()) continue;
            subscriptions.push_back(_addr);
        }

        // Get all subscribers to those addresses
        reindexer::QueryResults queryRes2;
        err = g_pocketdb->DB()->Select(
            reindexer::Query("SubscribesView").Where("address_to", CondSet, subscriptions).Limit(sampleSize),
            queryRes2);
        if (err.ok() && queryRes2.Count() > 0) {
            for (auto it : queryRes2) {
                reindexer::Item itm(it.GetItem());
                std::string _addr = itm["address"].As<string>();
                if (_address == _addr) continue;
                if (std::find(fellowSubscribers.begin(), fellowSubscribers.end(), _addr) != fellowSubscribers.end()) continue;
                fellowSubscribers.push_back(_addr);
            }

            reindexer::AggregationResult aggRes;
            err = g_pocketdb->SelectAggr(
                reindexer::Query("SubscribesView").Where("address", CondSet, fellowSubscribers).Where("private", CondEq, false).Aggregate("address_to", AggFacet),
                "address_to",
                aggRes);

            if (err.ok() && aggRes.facets.size()) {
                // region TF - IDF tuning < cmath >
                int freqInDoc = 0;
                int freqInCorpus = 0;
                // Get all adresses from aggRes.facets into vector
                std::vector<std::string> popularSubscribtions;
                std::map<std::string, int> popularSubscribtionsRate;

                // Get SUM of all "count" in aggRes.facets - freqInDoc
                for (const auto& f : aggRes.facets) {
                    freqInDoc += f.count;
                    popularSubscribtions.push_back(f.value);
                }

                // Find CondSet those addresses in UserReputations
                reindexer::QueryResults queryResUserReputations;
                err = g_pocketdb->DB()->Select(
                    reindexer::Query("UserRatings", 0, 1).Where("address", CondSet, popularSubscribtions).Sort("block", true),
                    queryResUserReputations);

                if (err.ok() && queryResUserReputations.Count() > 0) {
                    for (auto it : queryResUserReputations) {
                        reindexer::Item UserReputations(it.GetItem());
                        std::string _addr = UserReputations["address"].As<string>();
                        double reputation = UserReputations["reputation"].As<double>();

                        // Get "reputation" in result - freqInCorpus
                        freqInCorpus = freqInCorpus + reputation;

                        // UserReputations to map
                        popularSubscribtionsRate.emplace(_addr, reputation);
                    }
                }

                if (freqInDoc == 0) freqInDoc = 1;

                std::vector<std::pair<std::string, double>> mapPopularSubscriptions;
                for (const auto& f : aggRes.facets) {
                    double val = 0;
                    if (freqInCorpus > 0 && popularSubscribtionsRate[f.value] != 0) val = f.count / freqInDoc * std::log(freqInCorpus / popularSubscribtionsRate[f.value]); // TF - IDF
                    if (freqInCorpus == 0) val = f.count;                                                                                                                   // Else - sort as is - without TF - IDF tuning
                    mapPopularSubscriptions.push_back(std::pair<std::string, double>(f.value, val));
                }
                // ---------------------
                struct IntCmp {
                    bool operator()(const std::pair<std::string, double>& lhs, const std::pair<std::string, double>& rhs)
                    {
                        return lhs.second > rhs.second; // Changed  < to > since we need DESC order
                    }
                };
                // ---------------------
                int limit = mapPopularSubscriptions.size() < count ? mapPopularSubscriptions.size() : count;
                std::partial_sort(mapPopularSubscriptions.begin(), mapPopularSubscriptions.begin() + limit, mapPopularSubscriptions.end(), IntCmp());

                for (int i = 0; i < limit; ++i) {
                    if (std::find(subscriptions.begin(), subscriptions.end(), mapPopularSubscriptions[i].first) == subscriptions.end()) {
                        recommendedSubscriptions.push_back(mapPopularSubscriptions[i].first);
                        // TODO we'll skip own Subscriptions so there can be less than 10
                    }
                }
            }
        }
    }
    //-------------------------
    return true;
}

bool AddrIndex::GetRecommendedPostsBySubscriptions(std::string _address, int count, std::set<string>& recommendedPosts)
{
    std::vector<std::string> subscriptions;
    GetRecomendedSubscriptions(_address, count, subscriptions);

    for (auto itsub : subscriptions) {
        reindexer::Item queryItm;
        reindexer::Error err = g_pocketdb->SelectOne(
            reindexer::Query("Posts").Where("address", CondEq, itsub).Sort("time", true),
            queryItm);
        if (err.ok()) {
            recommendedPosts.emplace(queryItm["txid"].As<string>());
        }
    }

    return true;
}

bool AddrIndex::GetRecommendedPostsByScores(std::string _address, int count, std::set<string>& recommendedPosts)
{
    int sampleSize = 1000; // size of representative sample

    std::vector<std::string> userLikedPosts;
    std::vector<std::string> fellowLikers;

    reindexer::QueryResults queryRes1;
    reindexer::Error err = g_pocketdb->DB()->Select(
        reindexer::Query("Scores").Where("address", CondEq, _address).Where("value", CondSet, {4, 5}).Sort("time", true).Limit(50),
        queryRes1);
    //-------------------------
    if (err.ok() && queryRes1.Count() > 0) {
        for (auto it : queryRes1) {
            reindexer::Item itm(it.GetItem());
            userLikedPosts.push_back(itm["posttxid"].As<string>());
        }

        reindexer::QueryResults queryRes2;
        err = g_pocketdb->DB()->Select(
            reindexer::Query("Scores").Where("posttxid", CondSet, userLikedPosts).Where("value", CondSet, {4, 5}).Limit(sampleSize),
            queryRes2);
        if (err.ok() && queryRes2.Count() > 0) {
            for (auto it : queryRes2) {
                reindexer::Item itm(it.GetItem());
                std::string _addr = itm["address"].As<string>();
                if (_address == _addr) continue;
                if (std::find(fellowLikers.begin(), fellowLikers.end(), _addr) != fellowLikers.end()) continue;
                fellowLikers.push_back(itm["address"].As<string>());
            }

            reindexer::AggregationResult aggRes;
            err = g_pocketdb->SelectAggr(
                reindexer::Query("Scores").Where("address", CondSet, fellowLikers).Where("value", CondSet, {4, 5}).Aggregate("posttxid", AggFacet),
                "posttxid",
                aggRes);

            if (err.ok() && aggRes.facets.size()) {
                struct IntCmp {
                    bool operator()(const reindexer::FacetResult& lhs, const reindexer::FacetResult& rhs)
                    {
                        return lhs.count > rhs.count; // Changed  < to > since we need DESC order
                    }
                };
                // ---------------------
                std::vector<reindexer::FacetResult> vecFacets(aggRes.facets.begin(), aggRes.facets.end());

                int limit = vecFacets.size() < count ? vecFacets.size() : count;
                std::partial_sort(vecFacets.begin(), vecFacets.begin() + limit, vecFacets.end(), IntCmp());

                for (int i = 0; i < limit; ++i) {
                    if (std::find(userLikedPosts.begin(), userLikedPosts.end(), vecFacets[i].value) == userLikedPosts.end()) {
                        recommendedPosts.emplace(vecFacets[i].value);
                        // TODO we'll skip own liked posts so there can be less than 10
                    }
                }
            }
        }
    }

    return true;
}

bool AddrIndex::GetBlockRIData(CBlock block, std::string& data)
{
    UniValue ret_data(UniValue::VOBJ);
    uint256 blockhash = block.GetHash();

    // Maybe reindexer part data received from another node?
    // .. then relay from global POCKETNET_DATA
    if (POCKETNET_DATA.find(blockhash) != POCKETNET_DATA.end()) {
        data = POCKETNET_DATA[blockhash];
        return true;
    }

    // .. or data already in reindexer DB
    for (CTransactionRef& tr : block.vtx) {
        std::string d;
        if (IsPocketTX(tr)) {
            if (!GetTXRIData(tr, d)) return false;
            ret_data.pushKV(tr->GetHash().GetHex(), d);
        }
    }

    if (ret_data.size()) {
        data = ret_data.write();
    }

    return true;
}

bool AddrIndex::SetBlockRIData(std::string& data, int height)
{
    UniValue _data(UniValue::VOBJ);
    _data.read(data);
    //----------------------
    for (unsigned int i = 0; i < _data.size(); i++) {
        std::string _d = _data[i].get_str();
        SetTXRIData(_d, height);
    }
    //----------------------
    return true;
}

bool AddrIndex::GetTXRIData(CTransactionRef& tx, std::string& data)
{
    std::string ri_table = "";

    // First check this transaction not money
    if (!GetPocketnetTXType(tx, ri_table)) return true;
    //----------------------
    std::string txid = tx->GetHash().GetHex();
    UniValue ret_data(UniValue::VOBJ);

    // Type of transaction is "pocketnet"
    // First check RIMempool for transactions from mempool
    // If RIMempool empty -> Check general tables
    reindexer::Item itm;
    bool mempool = true;

    if (!g_pocketdb->SelectOne(reindexer::Query("Mempool").Where("txid", CondEq, txid), itm).ok()) {
        mempool = false;
        
        Error err;
        if (ri_table == "Posts") {
            err = g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, txid).Where("txidEdit", CondEq, ""), itm);
            if (!err.ok()) err = g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txidEdit", CondEq, txid), itm);
            if (!err.ok()) {
                reindexer::Item hist_item;
                err = g_pocketdb->SelectOne(reindexer::Query("PostsHistory").Where("txid", CondEq, txid).Where("txidEdit", CondEq, ""), hist_item);
                if (!err.ok()) err = g_pocketdb->SelectOne(reindexer::Query("PostsHistory").Where("txidEdit", CondEq, txid), hist_item);
                if (err.ok()) {
                    itm = g_pocketdb->DB()->NewItem("Posts");
                    itm["txid"] = hist_item["txid"].As<string>();
                    itm["txidEdit"] = hist_item["txidEdit"].As<string>();
                    itm["block"] = hist_item["block"].As<int>();
                    itm["time"] = hist_item["time"].As<int64_t>();
                    itm["address"] = hist_item["address"].As<string>();
                    itm["lang"] = hist_item["lang"].As<string>();
                    itm["caption"] = hist_item["caption"].As<string>();
                    itm["message"] = hist_item["message"].As<string>();
                    itm["tags"] = hist_item["tags"];
                    itm["url"] = hist_item["url"].As<string>();
                    itm["images"] = hist_item["images"];
                    itm["settings"] = hist_item["settings"].As<string>();
                }
            }

            if (err.ok()) {
                itm["caption_"] = "";
                itm["message_"] = "";
                itm["scoreSum"] = 0;
                itm["scoreCnt"] = 0;
                itm["reputation"] = 0;
            }
        } else {
            err = g_pocketdb->SelectOne(reindexer::Query(ri_table).Where("txid", CondEq, txid), itm);
        }
        
        if (!err.ok()) {
            LogPrintf("DEBUG!!! GetTXRIData: ridata not found %s\n", txid);
            return false;
        }
    }

    ret_data.pushKV("t", (mempool ? "Mempool" : ri_table));
    ret_data.pushKV("d", EncodeBase64(itm.GetJSON().ToString()));
    data = ret_data.write();
    return true;
}

bool AddrIndex::SetTXRIData(std::string& data, int height)
{
    UniValue _data(UniValue::VOBJ);
    if (!_data.read(data)) return false;
    //----------------------
    std::string table = _data["t"].get_str();
    std::string itm_src = DecodeBase64(_data["d"].get_str());

    reindexer::Item itm = g_pocketdb->DB()->NewItem(table);
    if (!itm.FromJSON(itm_src).ok()) return false;
    if (!WriteRTransaction(table, itm, height)) return false;
    //----------------------
    return true;
}

bool AddrIndex::CommitRIMempool(const CBlock& block, int height)
{
    reindexer::Error err;
    //------------------------
    for (const auto& tx : block.vtx) {
        std::string rTable;
        if (!GetPocketnetTXType(tx, rTable)) continue;

        std::string txid = (*tx).GetHash().GetHex();

        // Get data from RIMempool
        reindexer::Item mpItm;
        if (!g_pocketdb->SelectOne( reindexer::Query("Mempool").Where("txid", CondEq, txid), mpItm ).ok()) {
            if (!CheckRItemExists(rTable, txid)) {
                LogPrintf("--- AddrIndex::CommitRIMempool Mempool not found: %s\n", txid);
                return false;
            } else {
                continue;
            }
        }

        //------------------------
        if (gArgs.GetBoolArg("-debug_pocketnet_tx", false)) {
            std::ofstream outfile;
            outfile.open("debug_pocketnet_tx.txt", std::ios_base::app);
            outfile << txid + ": " + mpItm["data"].As<string>() << std::endl; 
        }
        //------------------------

        // Parse mempool data
        std::string ri_table = mpItm["table"].As<string>();
        reindexer::Item new_item = g_pocketdb->DB()->NewItem(ri_table);
        new_item.FromJSON(DecodeBase64(mpItm["data"].As<string>()));

        if (!WriteRTransaction(ri_table, new_item, height)) return false;
        if (!ClearMempool(txid)) return false;
        if (!CheckRItemExists(rTable, txid)) {
            LogPrintf("--- AddrIndex::CommitRIMempool Error after write: %s\n", txid);
            return false;
        }
    }

    return true;
}

bool AddrIndex::ClearMempool(std::string txid)
{
    return g_pocketdb->DeleteWithCommit(reindexer::Query("Mempool").Where("txid", CondEq, txid)).ok();
}

std::string ComputeHash(std::string src)
{
    unsigned char _hash[32] = {};
    CSHA256().Write((const unsigned char*)src.data(), src.size()).Finalize(_hash);
    std::vector<unsigned char> vec(_hash, _hash + sizeof(_hash));
    return HexStr(vec);
}

bool GetBlockRHash(const CBlock& block, std::string& blockRHash)
{
    std::string asmstr = ScriptToAsmStr(block.vtx[0]->vin[0].scriptSig);
    std::vector<std::string> vasm;
    boost::split(vasm, asmstr, boost::is_any_of("\t "));
    if (vasm.size() < 3) return false;

    blockRHash = vasm[2];
    return true;
}

bool AddrIndex::ComputeRHash(CBlockIndex* pindexPrev, std::string& hash)
{
    std::string data = "";

    // Compute hashes for reindexer tables
    {
        // Get hash of Users
        {
            std::string usersData = "";
            reindexer::QueryResults usersRes;
            g_pocketdb->Select(reindexer::Query("Users").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), usersRes);
            for (auto& u : usersRes) {
                std::string _usersData = "";
                reindexer::Item userItm(u.GetItem());

                _usersData += userItm["txid"].As<string>();
                _usersData += userItm["block"].As<string>();
                _usersData += userItm["time"].As<string>();
                _usersData += userItm["address"].As<string>();
                _usersData += userItm["name"].As<string>();
                _usersData += userItm["birthday"].As<string>();
                _usersData += userItm["gender"].As<string>();
                _usersData += userItm["regdate"].As<string>();
                _usersData += userItm["avatar"].As<string>();
                _usersData += userItm["about"].As<string>();
                _usersData += userItm["lang"].As<string>();
                _usersData += userItm["url"].As<string>();
                _usersData += userItm["pubkey"].As<string>();
                _usersData += userItm["donations"].As<string>();
                _usersData += userItm["referrer"].As<string>();
                _usersData += userItm["id"].As<string>();

                usersData += ComputeHash(_usersData);
            }

            if (usersData != "") data += ComputeHash(usersData);
        }

        // Get hash of Posts
        {
            std::string postsData = "";
            reindexer::QueryResults postsRes;
            g_pocketdb->Select(reindexer::Query("Posts").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), postsRes);
            for (auto& postIt : postsRes) {
                std::string _postsData = "";
                reindexer::Item postItm(postIt.GetItem());

                _postsData += postItm["txid"].As<string>();
                _postsData += postItm["block"].As<string>();
                _postsData += postItm["time"].As<string>();
                _postsData += postItm["address"].As<string>();
                _postsData += postItm["lang"].As<string>();
                _postsData += postItm["caption"].As<string>();
                _postsData += postItm["message"].As<string>();
                _postsData += postItm["settings"].As<string>();
                _postsData += postItm["url"].As<string>();

                reindexer::VariantArray postTags = postItm["tags"];
                for (unsigned int i = 0; i < postTags.size(); i++) { _postsData += postTags[i].As<string>(); }

                reindexer::VariantArray postImages = postItm["images"];
                for (unsigned int i = 0; i < postImages.size(); i++) { _postsData += postImages[i].As<string>(); }

                postsData += ComputeHash(_postsData);
            }

            if (postsData != "") data += ComputeHash(postsData);
        }

        // Get hash of Scores
        {
            std::string scoresData = "";
            reindexer::QueryResults scoresRes;
            g_pocketdb->Select(reindexer::Query("Scores").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), scoresRes);
            for (auto& scoreIt : scoresRes) {
                std::string _scoresData = "";
                reindexer::Item scoreItm(scoreIt.GetItem());

                _scoresData += scoreItm["txid"].As<string>();
                _scoresData += scoreItm["block"].As<string>();
                _scoresData += scoreItm["time"].As<string>();
                _scoresData += scoreItm["posttxid"].As<string>();
                _scoresData += scoreItm["address"].As<string>();
                _scoresData += scoreItm["value"].As<string>();

                scoresData += ComputeHash(_scoresData);
            }

            if (scoresData != "") data += ComputeHash(scoresData);
        }

        // Get hash of Subscribes
        {
            std::string subscribesData = "";
            reindexer::QueryResults subscribesRes;
            g_pocketdb->Select(reindexer::Query("Subscribes").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), subscribesRes);
            for (auto& subscribeIt : subscribesRes) {
                std::string _subscribesData = "";
                reindexer::Item subscribeItm(subscribeIt.GetItem());

                _subscribesData += subscribeItm["txid"].As<string>();
                _subscribesData += subscribeItm["block"].As<string>();
                _subscribesData += subscribeItm["time"].As<string>();
                _subscribesData += subscribeItm["address"].As<string>();
                _subscribesData += subscribeItm["address_to"].As<string>();
                _subscribesData += subscribeItm["private"].As<string>();
                _subscribesData += subscribeItm["unsubscribe"].As<string>();

                subscribesData += ComputeHash(_subscribesData);
            }

            if (subscribesData != "") data += ComputeHash(subscribesData);
        }

        // Get hash of Blocking
        {
            std::string blockingsData = "";
            reindexer::QueryResults blockingsRes;
            g_pocketdb->Select(reindexer::Query("Blocking").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), blockingsRes);
            for (auto& blockingIt : blockingsRes) {
                std::string _blockingData = "";
                reindexer::Item blockingItm(blockingIt.GetItem());

                _blockingData += blockingItm["txid"].As<string>();
                _blockingData += blockingItm["block"].As<string>();
                _blockingData += blockingItm["time"].As<string>();
                _blockingData += blockingItm["address"].As<string>();
                _blockingData += blockingItm["address_to"].As<string>();
                _blockingData += blockingItm["unblocking"].As<string>();

                blockingsData += ComputeHash(_blockingData);
            }

            if (blockingsData != "") data += ComputeHash(blockingsData);
        }

        // Get hash of Complains
        {
            std::string complainsData = "";
            reindexer::QueryResults complainsRes;
            g_pocketdb->Select(reindexer::Query("Complains").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), complainsRes);
            for (auto& complainIt : complainsRes) {
                std::string _complainsData = "";
                reindexer::Item complainItm(complainIt.GetItem());

                _complainsData += complainItm["txid"].As<string>();
                _complainsData += complainItm["block"].As<string>();
                _complainsData += complainItm["time"].As<string>();
                _complainsData += complainItm["posttxid"].As<string>();
                _complainsData += complainItm["address"].As<string>();
                _complainsData += complainItm["reason"].As<string>();

                complainsData += ComputeHash(_complainsData);
            }

            if (complainsData != "") data += ComputeHash(complainsData);
        }

        // Get hash of UTXO
        {
            std::string utxosData = "";
            reindexer::QueryResults utxosRes;
            g_pocketdb->Select(reindexer::Query("UTXO").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), utxosRes);
            for (auto& utxoIt : utxosRes) {
                std::string _utxosData = "";
                reindexer::Item utxoItm(utxoIt.GetItem());

                _utxosData += utxoItm["txid"].As<string>();
                _utxosData += utxoItm["txout"].As<string>();
                _utxosData += utxoItm["time"].As<string>();
                _utxosData += utxoItm["block"].As<string>();
                _utxosData += utxoItm["address"].As<string>();
                _utxosData += utxoItm["amount"].As<string>();
                _utxosData += utxoItm["spent_block"].As<string>();

                utxosData += ComputeHash(_utxosData);
            }

            if (utxosData != "") data += ComputeHash(utxosData);
        }

        // Get hash of Addresses
        {
            std::string addressesData = "";
            reindexer::QueryResults addressesRes;
            g_pocketdb->Select(reindexer::Query("Addresses").Where("block", CondEq, pindexPrev->nHeight).Sort("txid", false), addressesRes);
            for (auto& addressIt : addressesRes) {
                std::string _addressesData = "";
                reindexer::Item addressItm(addressIt.GetItem());

                _addressesData += addressItm["txid"].As<string>();
                _addressesData += addressItm["block"].As<string>();
                _addressesData += addressItm["address"].As<string>();
                _addressesData += addressItm["time"].As<string>();

                addressesData += ComputeHash(_addressesData);
            }

            if (addressesData != "") data += ComputeHash(addressesData);
        }

        // Get hash of UserRatings
        {
            std::string userRatingsData = "";
            reindexer::QueryResults userRatingsRes;
            g_pocketdb->Select(reindexer::Query("UserRatings").Where("block", CondEq, pindexPrev->nHeight).Sort("address", false), userRatingsRes);
            for (auto& userRatingIt : userRatingsRes) {
                std::string _userRatingsData = "";
                reindexer::Item userRatingItm(userRatingIt.GetItem());

                _userRatingsData += userRatingItm["block"].As<string>();
                _userRatingsData += userRatingItm["address"].As<string>();
                _userRatingsData += userRatingItm["scoreSum"].As<string>();
                _userRatingsData += userRatingItm["scoreCnt"].As<string>();

                userRatingsData += ComputeHash(_userRatingsData);
            }

            if (userRatingsData != "") data += ComputeHash(userRatingsData);
        }

        // Get hash of PostRatings
        {
            std::string postRatingsData = "";
            reindexer::QueryResults postRatingsRes;
            g_pocketdb->Select(reindexer::Query("PostRatings").Where("block", CondEq, pindexPrev->nHeight).Sort("posttxid", false), postRatingsRes);
            for (auto& postRatingIt : postRatingsRes) {
                std::string _postRatingsData = "";
                reindexer::Item postRatingItm(postRatingIt.GetItem());

                _postRatingsData += postRatingItm["block"].As<string>();
                _postRatingsData += postRatingItm["posttxid"].As<string>();
                _postRatingsData += postRatingItm["scoreSum"].As<string>();
                _postRatingsData += postRatingItm["scoreCnt"].As<string>();
                _postRatingsData += postRatingItm["reputation"].As<string>();

                postRatingsData += ComputeHash(_postRatingsData);
            }

            if (postRatingsData != "") data += ComputeHash(postRatingsData);
        }
    }

    // Get previous block data hash
    {
        if (pindexPrev->nHeight > 0) {
            CBlock prevBlock;
            if (!ReadBlockFromDisk(prevBlock, pindexPrev, Params().GetConsensus())) return false;

            std::string prevBlockRHash;
            if (GetBlockRHash(prevBlock, prevBlockRHash)) {
                data += prevBlockRHash;
            }
        }
    }

    hash = ComputeHash(data);
    
    return true;
}

bool AddrIndex::CheckRHash(const CBlock& block, CBlockIndex* pindexPrev)
{
    std::string blockRHash;
    if (!GetBlockRHash(block, blockRHash)) return false;

    std::string currentRHash;
    if (!ComputeRHash(pindexPrev, currentRHash)) return false;

    // LogPrintf("--- %s = %s\n", blockRHash, currentRHash);
    return blockRHash == currentRHash;
}

// TODO (brangr): Enable
bool AddrIndex::WriteRHash(CBlock& block, CBlockIndex* pindexPrev)
{
    // std::string currentRHash;
    // if (!ComputeRHash(pindexPrev, currentRHash)) return false;

    // CMutableTransaction txCoinbase(*block.vtx[0]);
    // txCoinbase.vin[0].scriptSig = (CScript(txCoinbase.vin[0].scriptSig) << ParseHex(currentRHash)) + COINBASE_FLAGS;
    // block.vtx[0] = MakeTransactionRef(std::move(txCoinbase));

    // block.hashMerkleRoot = BlockMerkleRoot(block);
    return true;
}

UniValue AddrIndex::GetUniValue(const CTransactionRef& tx, Item& item, std::string table)
{
    UniValue oitm(UniValue::VOBJ);

    oitm.pushKV("type", PocketTXType(tx));
    oitm.pushKV("table", table);
    oitm.pushKV("txid", item["txid"].As<string>());
    oitm.pushKV("address", item["address"].As<string>());
    oitm.pushKV("size", (int)(item.GetJSON().ToString().size()));
    oitm.pushKV("time", (int64_t)tx->nTime);

    std::string itm_hash;
    g_pocketdb->GetHashItem(item, table, true, itm_hash);
    oitm.pushKV("data_hash", itm_hash);

    std::string asmStr;
    g_addrindex->FindPocketNetAsmString(tx, asmStr);
    oitm.pushKV("asm", asmStr);

    if (table == "Posts") {
        oitm.pushKV("txidEdit", item["txidEdit"].As<string>());
    }
    
    if (table == "Scores") {
        oitm.pushKV("posttxid", item["posttxid"].As<string>());
        oitm.pushKV("value", item["value"].As<int>());
    }
    
    if (table == "Complains") {
        oitm.pushKV("posttxid", item["posttxid"].As<string>());
    }
    
    if (table == "Subscribes") {
        oitm.pushKV("address_to", item["address_to"].As<string>());
        oitm.pushKV("private", item["private"].As<bool>());
        oitm.pushKV("unsubscribe", item["unsubscribe"].As<bool>());
    }

    if (table == "Blocking") {
        oitm.pushKV("address_to", item["address_to"].As<string>());
        oitm.pushKV("unblocking", item["unblocking"].As<bool>());
    }
    
    if (table == "Users") {
        oitm.pushKV("referrer", item["referrer"].As<string>());
        oitm.pushKV("name", item["name"].As<string>());

        //if (chainActive.Height() < Params().GetConsensus().nHeight_version_1_0_0) {
            std::string itm_hash_ref;
            g_pocketdb->GetHashItem(item, table, false, itm_hash_ref);
            oitm.pushKV("data_hash_without_ref", itm_hash_ref);
        //}
    }

    if (table == "Comment") {
        oitm.pushKV("msg", item["msg"].As<string>());
        oitm.pushKV("otxid", item["otxid"].As<string>());
        oitm.pushKV("postid", item["postid"].As<string>());
        oitm.pushKV("parentid", item["parentid"].As<string>());
        oitm.pushKV("answerid", item["answerid"].As<string>());
    }

    if (table == "CommentScores") {
        oitm.pushKV("commentid", item["commentid"].As<string>());
        oitm.pushKV("value", item["value"].As<int>());
    }

    return oitm;
}
