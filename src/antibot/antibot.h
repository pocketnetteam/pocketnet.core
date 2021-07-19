// brangr
// Collect all transactions pars <sender>-<recipient>
// for antibot module
//-----------------------------------------------------
#ifndef ANTIBOT_H
#define ANTIBOT_H
//-----------------------------------------------------
#include "html.h"
#include "pocketdb/pocketdb.h"
#include "pocketdb/pocketnet.h"
#include "validation.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <timedata.h>
//-----------------------------------------------------
struct UserStateItem {
    std::string address;
    int64_t user_registration_date;
    int64_t address_registration_date;
    int reputation;
    int64_t balance;
    bool trial;
    int64_t likers;

    int post_unspent;
    int post_spent;

    int score_unspent;
    int score_spent;

    int complain_unspent;
    int complain_spent;

    int comment_unspent;
    int comment_spent;

    int comment_score_unspent;
    int comment_score_spent;

    int number_of_blocking;

    UserStateItem(std::string _address)
    {
        address = _address;
    }

    UniValue Serialize()
    {
        UniValue result(UniValue::VOBJ);

        result.pushKV("address", address);
        result.pushKV("user_reg_date", user_registration_date);
        result.pushKV("addr_reg_date", address_registration_date);
        result.pushKV("reputation", reputation / 10.0);
        result.pushKV("balance", balance);
        result.pushKV("trial", trial);
        result.pushKV("likers", likers);
        result.pushKV("post_unspent", post_unspent);
        result.pushKV("post_spent", post_spent);
        result.pushKV("score_unspent", score_unspent);
        result.pushKV("score_spent", score_spent);
        result.pushKV("complain_unspent", complain_unspent);
        result.pushKV("complain_spent", complain_spent);
        result.pushKV("number_of_blocking", number_of_blocking);

        result.pushKV("comment_spent", comment_spent);
        result.pushKV("comment_unspent", comment_unspent);
        result.pushKV("comment_score_spent", comment_score_spent);
        result.pushKV("comment_score_unspent", comment_score_unspent);

        return result;
    }
};
//-----------------------------------------------------
enum CHECKTYPE {
    User,
    Post,
    PostEdit,
    Score,
    Complain,
    Comment,
    CommentEdit,
    CommentScore,
    CheckType_ContentVideo,
    CheckType_ContentVideoEdit
};
//-----------------------------------------------------
enum ABMODE {
    Trial,
    Full
};
//-----------------------------------------------------
enum ANTIBOTRESULT {
    Success = 0,
    NotRegistered = 1,
    PostLimit = 2,
    ScoreLimit = 3,
    DoubleScore = 4,
    SelfScore = 5,
    ChangeInfoLimit = 6,
    InvalideSubscribe = 7,
    DoubleSubscribe = 8,
    SelfSubscribe = 9,
    Unknown = 10,
    Failed = 11,
    NotFound = 12,
    DoubleComplain = 13,
    SelfComplain = 14,
    ComplainLimit = 15,
    LowReputation = 16,
    ContentSizeLimit = 17,
    NicknameDouble = 18,
    NicknameLong = 19,
    ReferrerSelf = 20,
    FailedOpReturn = 21,
    InvalidBlocking = 22,
    DoubleBlocking = 23,
    SelfBlocking = 24,
    DoublePostEdit = 25,
    PostEditLimit = 26,
    PostEditUnauthorized = 27,
    ManyTransactions = 28,
    CommentLimit = 29,
    CommentEditLimit = 30,
    CommentScoreLimit = 31,
    Blocking = 32,
    Size = 33,
    InvalidParentComment = 34,
    InvalidAnswerComment = 35,
    DoubleCommentEdit = 37,
    SelfCommentScore = 38,
    DoubleCommentDelete = 39,
    DoubleCommentScore = 40,
    OpReturnFailed = 41,
    CommentDeletedEdit = 42,
    ReferrerAfterRegistration = 43,
    NotAllowed = 44,
    ChangeTxType = 45
};
//-----------------------------------------------------
struct BlockVTX {
    std::map<std::string, std::vector<UniValue>> Data;

    size_t Size()
    {
        return Data.size();
    }

    void Add(std::string table, UniValue itm)
    {
        if (Data.find(table) == Data.end()) {
            std::vector<UniValue> vtxri;
            Data.insert_or_assign(table, vtxri);
        }

        Data[table].push_back(itm);
    }

    bool Exists(std::string table)
    {
        return Data.find(table) != Data.end();
    }

    void RemoveLast(std::string table) {
        if (Data.find(table) != Data.end()) {
            Data[table].pop_back();
        }
    }
};
//-----------------------------------------------------
class AntiBot
{
private:
    void getMode(std::string _address, ABMODE& mode, int& reputation, int64_t& balance, int height);
    void getMode(std::string _address, ABMODE& mode, int height);
    int getLimit(CHECKTYPE _type, ABMODE _mode, int height);

    // Maximum size for reindexer item with switch for type
    bool check_item_size(UniValue oitm, CHECKTYPE _type, int height, ANTIBOTRESULT& result);

    // Check new post and edited post from address
    bool check_post(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, bool splitContent, int height, ANTIBOTRESULT& result);
    bool check_post_edit(const UniValue& oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, bool splitContent, int height, ANTIBOTRESULT& result);

    bool check_video(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, int height, ANTIBOTRESULT& result);
    bool check_video_edit(const UniValue& oitm, BlockVTX& blockVtx, bool checkMempool, int height, ANTIBOTRESULT& result);

    // Check new score to post from address
    bool check_score(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

    // Check new complain to post from address
    bool check_complain(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

    // Check change profile
    bool check_changeInfo(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

    // Check subscribe/unsubscribe
    bool check_subscribe(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

    // Check blocking/unblocking
    bool check_blocking(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

    // Check new comment
    bool check_comment(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);
    bool check_comment_edit(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);
    bool check_comment_delete(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

    // Check new score to comment
    bool check_comment_score(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkTime_19_3, bool checkTime_19_6, int height, ANTIBOTRESULT& result);

public:
    explicit AntiBot();
    ~AntiBot();

    // Check user is a registration. Need one record in DB Users
    bool CheckRegistration(UniValue oitm, std::string address, bool checkMempool, bool checkTime_19_3, int height, BlockVTX& blockVtx, ANTIBOTRESULT& result);

    /*
		Check conditions for new transaction.
		PocketNET data must be in RIMempool
	*/
    void CheckTransactionRIItem(UniValue oitm, BlockVTX& blockVtx, bool checkMempool, int height, ANTIBOTRESULT& resultCode);
    void CheckTransactionRIItem(UniValue oitm, int height, ANTIBOTRESULT& resultCode);
    /*
        Check inputs for exists utxo
    */
    bool CheckInputs(CTransactionRef& tx);
    /*
        Check all transactions in block
        Include this transactions as parents
    */
    bool CheckBlock(BlockVTX& blockVtx, int height);
    /*
		Return array of user states.
		Contains info about spent and unspent posts and scores. Also current reputation value
	*/
    bool GetUserState(std::string _address, UserStateItem& _state);
    /*
        to test the possibility of changing that reputation
    */
    bool AllowModifyReputation(std::string _score_address, int height);
    bool AllowModifyReputationOverPost(std::string _score_address, std::string _post_address, int height, int64_t tx_time, std::string txid, bool lottery);
    bool AllowModifyReputationOverPost(std::string _score_address, std::string _post_address, int height, const CTransactionRef& tx, bool lottery);
    bool AllowModifyReputationOverComment(std::string _score_address, std::string _comment_address, int height, const CTransactionRef& tx, bool lottery);
};
//-----------------------------------------------------
extern std::unique_ptr<AntiBot> g_antibot;
//-----------------------------------------------------
#endif // ADDRINDEX_H