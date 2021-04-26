

#ifndef POCKETTX_TRANSACTIONINDEXER_HPP
#define POCKETTX_TRANSACTIONINDEXER_HPP

namespace PocketTx
{
    using namespace PocketDb;

    class BlockIndexer
    {
    public:
        BlockIndexer() = default;

        Index(block, pindex) {
            // for block.vtx
            // проставить транзакциям высоту и оут
            // расчитали рейтинги
            // проставить утхо чреез BlockRepository
            BlockRepository.InsertUtxoOuts()
            BlockRepository.UtxoSpent(height, vector<tuple<unit256, int>> txs)
        }

    protected:
    private:
    };
}

#endif //POCKETTX_TRANSACTIONINDEXER_HPP
