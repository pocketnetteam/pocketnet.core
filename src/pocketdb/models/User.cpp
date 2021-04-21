#include "User.h"

using namespace PocketTx;

void User::Deserialize(const UniValue& src) {
    Transaction::Deserialize(src);

    if (src.exists("id"))
        SetId(src["id"].get_int64());

    if (src.exists("regdate"))
        SetRegistration(src["regdate"].get_int64());

    if (src.exists("lang"))
        SetLang(src["lang"].get_str());

    if (src.exists("name"))
        SetName(src["name"].get_str());

    if (src.exists("referrer"))
        SetReferrer(src["referrer"].get_str());
}


User::~User() = default;
