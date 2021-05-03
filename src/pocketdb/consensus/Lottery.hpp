// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_LOTTERY_HPP
#define POCKETDB_LOTTERY_HPP

#include "pocketdb/consensus/Base.hpp"
#include "script/standard.h"
#include "chain.h"
#include "primitives/block.h"
#include "pocketdb/pocketnet.h"

namespace PocketConsensus
{
    using namespace PocketTx;
    using namespace PocketDb;

    using std::string;
    using std::shared_ptr;
    using std::make_shared;

    class Lottery : Base
    {
    private:


    protected:
        const int RATINGS_PAYOUT_MAX = 25;
        const int64_t _lottery_referral_depth; // todo (brangr): inherited in checkpoints

    public:
        Lottery() : Base() = default;

        bool Winners(const CBlock &block)
        {
            std::vector <std::string> vLotteryPost;
            std::map <std::string, std::string> mLotteryPostRef;
            std::map<std::string, int> allPostRatings;

            std::vector <std::string> vLotteryComment;
            std::map <std::string, std::string> mLotteryCommentRef;
            std::map<std::string, int> allCommentRatings;

            for (const auto& tx : blockPrev.vtx)
            {
                std::vector <std::string> vasm;
                if (!FindPocketNetAsmString(tx, vasm)) continue;
                if ((vasm[1] == OR_SCORE || vasm[1] == OR_COMMENT_SCORE) && vasm.size() >= 4)
                {
                    std::vector<unsigned char> _data_hex = ParseHex(vasm[3]);
                    std::string _data_str(_data_hex.begin(), _data_hex.end());
                    std::vector <std::string> _data;
                    boost::split(_data, _data_str, boost::is_any_of("\t "));
                    if (_data.size() >= 2)
                    {
                        std::string _address = _data[0];
                        int _value = std::stoi(_data[1]);

                        ...

                    }

                    ...
                }

                ...
            }

            ...
        }


    };

}

#endif // POCKETDB_LOTTERY_HPP
