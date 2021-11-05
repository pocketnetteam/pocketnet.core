// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_POCKETNET_HELPER_H
#define POCKETHELPERS_POCKETNET_HELPER_H

#include <map>
#include <string>
#include <vector>

#include "chainparams.h"

namespace PocketHelpers
{
    using namespace std;

    static map<string, vector<string>> PocketnetDevelopers{
        {"main", {
            "P9EkPPJPPRYxmK541WJkmH8yBM4GuWDn2m",
            "PUGBtfmivvcg1afnEt9vqVi3yZ7v6S9BcC",
            "PDtuJDVXaq82HH7fafgwBmcoxbqqWdJ9u9",
            "PDCNrwP1i8BJQWh2bctuJyAaXxozgMcRYT",
            "PD4us1zniwrJv64xhPyhT2mgNrTvPur9YN",
            "PVJ1rRdS9y9sUpaBJc8quiSTJGrC3xW8EH",
            "PAF1BvWEH7pA24QbbEvCEasViC2Pw9BVj3",
            "PSADH5AY5M9RaWrDVdaMrR2C2s6dCGfNK4"
        }},
        {"test", {
            "TG69Jioc81PiwMAJtRanfZqUmRY4TUG7nt",
            "TLnfXcFNxxrpEUUzrzZvbW7b9gWFtAcc8x",
            "TYMo5HRFpc7tqzccaVifx7s2x2ZDqMikCR",
        }}
    };

    static inline bool IsDeveloper(const string& address)
    {
        auto net = Params().NetworkIDString();
        return find(PocketnetDevelopers[net].begin(), PocketnetDevelopers[net].end(), address) != PocketnetDevelopers[net].end();
    }
}

#endif // POCKETHELPERS_POCKETNET_HELPER_H
