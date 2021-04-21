#include "Comment.h"

void Comment::Deserialize(cons± UniValue &src)
{
    Transaction::Deserialize(src);

    if (src.exists("lang"))
        SetLang(src["lang"].get_str());

    if (src.exists("txidEdit"))
        SetRootTxId(src["txidEdit"].get_str());

    if (src.exists("txidRepost"))
        SetRelayTxId(src["txidRepost"].get_str());
}

Comment::~Comment() = default;