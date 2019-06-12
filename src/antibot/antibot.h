// brangr
// Collect all transactions pars <sender>-<recipient>
// for antibot module
//-----------------------------------------------------
#ifndef ANTIBOT_H
#define ANTIBOT_H
//-----------------------------------------------------
#include "pocketdb/pocketdb.h"
#include "pocketdb/pocketnet.h"
#include "validation.h"
#include <timedata.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
//-----------------------------------------------------
struct UserStateItem {
	std::string address;
	int64_t user_registration_date;
	int64_t address_registration_date;
	int reputation;
	int64_t balance;
	bool trial;

	int post_unspent;
	int post_spent;

	int score_unspent;
	int score_spent;

	int complain_unspent;
	int complain_spent;
    
	int number_of_blocking;

	UserStateItem(std::string _address) {
		address = _address;
	}
};
//-----------------------------------------------------
enum CHECKTYPE {
	User,
	Post,
    PostEdit,
	Score,
	Complain
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
    InvalideBlocking = 22,
    DoubleBlocking = 23,
    SelfBlocking = 24,
    DoublePostEdit = 25,
    PostEditLimit = 26,
    PostEditUnauthorized = 27,
    ManyTransactions = 28
};
//-----------------------------------------------------
struct BlockVTX {
    std::map<std::string, std::vector<UniValue>> Data;

    size_t Size() {
        return Data.size();
    }

    void Add(std::string table, UniValue itm) {
        if (Data.find(table) == Data.end()) {
            std::vector<UniValue> vtxri;
            Data.insert_or_assign(table, vtxri);
        }
        
        Data[table].push_back(itm);
    }

    bool Exists(std::string table) {
        return Data.find(table) != Data.end();
    }
};
//-----------------------------------------------------
class AntiBot
{
private:
	void getMode(std::string _address, ABMODE &mode, int &reputation, int64_t &balance, int height);
	void getMode(std::string _address, ABMODE &mode, int height);
	int getLimit(CHECKTYPE _type, ABMODE _mode, int height);


    // Check user is a registration. Need one record in DB Users
    bool CheckRegistration(std::string _address, std::string _txid, BlockVTX blockVtx);

	// Check new post and edited post from address
	bool check_post(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);
    bool check_post_edit(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);

	// Check new score to post from address
	bool check_score(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);

	// Check new complain to post from address
	bool check_complain(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);

	// Check change profile
	bool check_changeInfo(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);

	// Check subscribe/unsubscribe
	bool check_subscribe(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);

	// Check blocking/unblocking
	bool check_blocking(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT &result);

	// Maximum size for reindexer item with switch for type
	bool check_item_size(UniValue oitm, CHECKTYPE _type, ANTIBOTRESULT &result, int height);

public:
    explicit AntiBot();
    ~AntiBot();

	/*
		Check conditions for new transaction.
		PocketNET data must be in RIMempool
	*/
	void CheckTransactionRIItem(UniValue oitm, BlockVTX blockVtx, bool checkMempool, ANTIBOTRESULT& resultCode);
    void CheckTransactionRIItem(UniValue oitm, ANTIBOTRESULT& resultCode);
    /*
        Check inputs for exists utxo
    */
    bool CheckInputs(CTransactionRef& tx);
    /*
        Check all transactions in block
        Include this transactions as parents
    */
    bool CheckBlock(BlockVTX blockVtx);
	/*
		Return array of user states.
		Contains info about spent and unspent posts and scores. Also current reputation value
	*/
	bool GetUserState(std::string _address, int64_t _time, UserStateItem& _state);
};
//-----------------------------------------------------
extern std::unique_ptr<AntiBot> g_antibot;
//-----------------------------------------------------
#endif // ADDRINDEX_H