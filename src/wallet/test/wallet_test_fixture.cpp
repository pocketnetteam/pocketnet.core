// Copyright (c) 2016-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/wallet_test_fixture.h>

#include <rpc/server.h>
#include <wallet/db.h>
#include "httpserver.h"

WalletTestingSetup::WalletTestingSetup(const std::string& chainName):
    TestingSetup(chainName), m_wallet("mock", WalletDatabase::CreateMock())
{
    bool fFirstRun;
    m_wallet.LoadWallet(fFirstRun);
    RegisterValidationInterface(&m_wallet);

    RegisterWalletRPCCommands(g_socket->m_table_rpc);
}

WalletTestingSetup::~WalletTestingSetup()
{
    UnregisterValidationInterface(&m_wallet);
}
