// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/Payload.h"

namespace PocketTx
{
    Payload::Payload() {}

    shared_ptr <string> Payload::GetTxHash() const { return m_txHash; }
    void Payload::SetTxHash(string value) { m_txHash = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString1() const { return m_string1; }
    void Payload::SetString1(string value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString2() const { return m_string2; }
    void Payload::SetString2(string value) { m_string2 = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString3() const { return m_string3; }
    void Payload::SetString3(string value) { m_string3 = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString4() const { return m_string4; }
    void Payload::SetString4(string value) { m_string4 = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString5() const { return m_string5; }
    void Payload::SetString5(string value) { m_string5 = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString6() const { return m_string6; }
    void Payload::SetString6(string value) { m_string6 = make_shared<string>(value); }

    shared_ptr <string> Payload::GetString7() const { return m_string7; }
    void Payload::SetString7(string value) { m_string7 = make_shared<string>(value); }

} // namespace PocketTx