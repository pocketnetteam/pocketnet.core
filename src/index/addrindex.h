// Copyright (c) 2018 PocketNet developers
// Indexes for addresses and transactions
//-----------------------------------------------------
#ifndef ADDRINDEX_H
#define ADDRINDEX_H
//-----------------------------------------------------
#include "pocketdb/pocketdb.h"
#include "pocketdb/pocketnet.h"
#include "antibot/antibot.h"
#include "primitives/block.h"
#include "script/standard.h"
#include "key_io.h"
#include "core_io.h"
#include "chain.h"
#include <univalue.h>
#include <coins.h>
#include <boost/algorithm/string.hpp>
#include <consensus/merkle.h>
//-----------------------------------------------------
using namespace reindexer;
//-----------------------------------------------------
struct AddressRegistrationItem {
	std::string address;
	std::string txid;
	time_t time;

	AddressRegistrationItem(
		std::string _address,
		std::string _txid,
		time_t _time
	) {
		address = _address;
		txid = _txid;
		time = _time;
	}
};
//-----------------------------------------------------
struct AddressUnspentTransactionItem {
	std::string address;
	std::string txid;
	int txout;

	AddressUnspentTransactionItem(
		std::string _address,
		std::string _txid,
		int _txout
	) {
		address = _address;
		txid = _txid;
		txout = _txout;
	}

	friend bool operator==(const AddressUnspentTransactionItem& lhs, const AddressUnspentTransactionItem& rhs)
	{
		return lhs.txid == rhs.txid && lhs.txout == rhs.txout;
	}
};
//-----------------------------------------------------
class AddrIndex
{
private:
	bool insert_to_mempool(reindexer::Item& item, std::string table);
	/*
		Calculate rating for one score.
		After this method need COMMIT!!!
	*/
	bool calculate_ratings(std::string posttxid, int64_t time, int value, bool rollback=false);
	std::string getOutAddress(const CTransactionRef& tx);
    
    /*
		Indexing block transactions.
		OUTs to Unspent
		INs to Spent
	*/
    bool indexUTXO(const CTransactionRef& tx, CBlockIndex* pindex);
	/*
		Indexing block transactions for collect rating of User.
		OP_RETURN can contains `OR_SCORE` value - its Score for Post
	*/
	bool indexRating(const CTransactionRef& tx, 
        std::map<std::string, double>& userReputations,
        std::map<std::string, std::pair<int, int>>& postRatings,
        std::map<std::string, int>& postReputations,
        CBlockIndex* pindex);
    /*
		Indexing block transactions for collect rating of User.
		OP_RETURN can contains `OR_COMMENT_SCORE` value - its Score for Comment
	*/
	bool indexCommentRating(const CTransactionRef& tx, 
        std::map<std::string, double>& userReputations,
        std::map<std::string, std::pair<int, int>>& commentRatings,
        std::map<std::string, int>& commentReputations,
        CBlockIndex* pindex);
	/*
		Indexing block transactions for collect tags.
		OP_RETURN can contains `OR_POST` value - its Post
	*/
	bool indexTags(const CTransactionRef& tx, CBlockIndex* pindex);
	/*
		Save first occurrence of address.
		Essentially the first mention in out of transactions.
		ONLY FIRST!
	*/
	bool indexAddress(const CTransactionRef& tx, CBlockIndex* pindex);
	/*
		Aggregate table UserRatings for all users
	*/
	bool computeUsersRatings(CBlockIndex* pindex, std::map<std::string, double>& userReputations);
	/*
		Increment rating of Post
	*/
	bool computePostsRatings(CBlockIndex* pindex, std::map<std::string, std::pair<int, int>>& postRatings, std::map<std::string, int>& postReputations);
    /*
		Increment rating of Comment
	*/
	bool computeCommentRatings(CBlockIndex* pindex, std::map<std::string, std::pair<int, int>>& commentRatings, std::map<std::string, int>& commentReputations);
    /*
        Indexing posts data
    */
    bool indexPost(const CTransactionRef& tx, CBlockIndex* pindex);

public:
    explicit AddrIndex();
    ~AddrIndex();

    // Check reindexer table for exist item with txid
    bool CheckRItemExists(std::string table, std::string txid);

    /* Write new transaction to mempool */
    bool WriteMemRTransaction(reindexer::Item& item);
    bool WriteRTransaction(std::string table, reindexer::Item& item, int height);
	/*
		Find asm string with OP_RETURN for check type transaction
	*/
	bool FindPocketNetAsmString(const CTransactionRef& tx, std::vector<std::string>& vasm);
    bool FindPocketNetAsmString(const CTransactionRef& tx, std::string& asmStr);
	/*
		Return table name for transaction if is PocketNET transaction
	*/
	bool GetPocketnetTXType(const CTransactionRef& tx, std::string& ri_table);
	bool IsPocketnetTransaction(const CTransactionRef& tx);
    bool IsPocketnetTransaction(const CTransaction& tx);
	/*
		Indexing block transactions.
	*/
    bool IndexBlock(const CBlock& block, CBlockIndex* pindex);
	/*
		Fix tables data.
		New current best block is `bestBlock`
		Need delete all data from RIDB above this block height
		Also need recalculating ratings
	*/
	bool RollbackDB(int blockHeight, bool back_to_mempool=false);
	/*
		Get all unspent transactions for array of addresses.
		Function fill array `std::map<std::string, int>& transactions`.
		Where:
			key: txid in string
			value: out number
	*/
    bool GetUnspentTransactions(std::vector<std::string> addresses,
		std::vector<AddressUnspentTransactionItem>& transactions);
	/*
		Input list of addresses.
		Output list of addresses with first transaction.
	*/
	int64_t GetAddressRegistrationDate(std::string _address);
	/*
		Return registration date for one address
	*/
	bool GetAddressRegistrationDate(std::vector<std::string> addresses,
		std::vector<AddressRegistrationItem>& registrations);
	/*
		Returns date of registration user.
		If not exists - return `-1`
	*/
	int64_t GetUserRegistrationDate(std::string _address);
	/*
		Functions for calculate recommendations for user
	*/
	bool GetRecomendedSubscriptions(std::string _address, int count, std::vector<string>& recommendedSubscriptions);
    bool GetRecommendedPostsBySubscriptions(std::string _address, int count, std::set<string>& recommendedPosts);
    bool GetRecommendedPostsByScores(std::string _address, int count, std::set<string>& recommendedPosts);
	/*
		Get RI data for block transactions for send to another node.
	*/
	bool GetBlockRIData(CBlock block, std::string& data);
	/*
		Write transaction for block received from another node
	*/
	bool SetBlockRIData(std::string& data, int height);
	/*
		Get RI data for transaction for send to another node.
		Check transaction is PocketNet type transaction
		and get json data from reindexer DB by OP_RETURN type.
		* First check RIMempool
		* Second check general tables
	*/
	bool GetTXRIData(CTransactionRef& tx, std::string& data);
	/*
		Write PocketNet data for this transaction
	*/
	bool SetTXRIData(std::string& data, int height);
	/*
		Write RI Mempool data to general tables
	*/
	bool CommitRIMempool(const CBlock& block, int height);
	/*
		Delete item from RIMempool
	*/
	bool ClearMempool(std::string txid);
	/*
		Compute state of Reindexer DB
	*/
    bool ComputeRHash(CBlockIndex* pindexPrev, std::string& hash);
    /*
        Check new block RHash
    */
	bool CheckRHash(const CBlock& block, CBlockIndex* pindexPrev);
	/*
		Write ReindexerDB current state to coinbase transaction
	*/
	bool WriteRHash(CBlock& block, CBlockIndex* pindexPrev);
    /*
        Present reindexer::Item as UniValue for antibot check
    */
    UniValue GetUniValue(const CTransactionRef& tx, Item& item, std::string table);
    
};
//-----------------------------------------------------
extern std::unique_ptr<AddrIndex> g_addrindex;
//-----------------------------------------------------
#endif // ADDRINDEX_H