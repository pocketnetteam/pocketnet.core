// Copyright (c) 2016-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <chainparams.h>
#include <validation.h>
#include <streams.h>
#include <consensus/validation.h>

namespace block_bench {
#include <bench/data/block1533073.h>
} // namespace block_bench

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    CDataStream stream((const char*)block_bench::block1533073,
            (const char*)&block_bench::block1533073[sizeof(block_bench::block1533073)],
            SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    bench.unit("block").run([&] {
        CBlock block;
        stream >> block;
        bool rewound = stream.Rewind(sizeof(block_bench::block1533073));
        assert(rewound);
    });
}

static void DeserializeAndCheckBlockTest(benchmark::Bench& bench)
{
    CDataStream stream((const char*)block_bench::block1533073,
            (const char*)&block_bench::block1533073[sizeof(block_bench::block1533073)],
            SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);

    bench.unit("block").run([&] {
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> block;
        bool rewound = stream.Rewind(sizeof(block_bench::block1533073));
        assert(rewound);

        CValidationState validationState;
        bool checked = CheckBlock(block, validationState, chainParams->GetConsensus());
        assert(checked);
    });
}

BENCHMARK(DeserializeBlockTest);
BENCHMARK(DeserializeAndCheckBlockTest);
