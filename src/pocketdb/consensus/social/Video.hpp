// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_VIDEO_HPP
#define POCKETCONSENSUS_VIDEO_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
//#include "pocketdb/models/dto/Video.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  User consensus base class
    *
    *******************************************************************************************************************/
    class VideoConsensus : public SocialBaseConsensus
    {
    public:
        VideoConsensus(int height) : SocialBaseConsensus(height) {}
        VideoConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Validate(tx, block); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Validate(static_pointer_cast<Blocking>(tx), block); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Check(static_pointer_cast<Blocking>(tx)); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

    protected:

        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<User> tx, PocketBlock& block)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }

    private:
    
        tuple<bool, SocialConsensusResult> Check(shared_ptr<CommentEdit> tx)
        {
            
        }

    
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class VideoConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<VideoConsensus*(int height)>> m_rules =
        {
            {0, [](int height) { return new VideoConsensus(height); }},
        };

    public:
        shared_ptr<VideoConsensus> Instance(int height)
        {
            return shared_ptr<VideoConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_VIDEO_HPP
