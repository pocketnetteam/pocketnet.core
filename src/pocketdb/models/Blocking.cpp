//
// Created by Andrey Brunenko on 21.04.2021.
//

#include "Blocking.h"

void Blocking::Deserialize(const UniValue &src)
{
    Transaction::Deserialize(src);

    if (src.exists("lang"))
        SetLang(src["lang"].get_str());

    if (src.exists("txidEdit"))
        SetRootTxId(src["txidEdit"].get_str());

    if (src.exists("txidRepost"))
        SetRelayTxId(src["txidRepost"].get_str());
}

Blocking::~Blocking() = default;