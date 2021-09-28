// Copyright (c) 2014-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_CHAINPARAMSBASE_H
#define POCKETCOIN_CHAINPARAMSBASE_H

#include <memory>
#include <string>
#include <vector>

/**
 * CBaseChainParams defines the base parameters (shared between pocketcoin-cli and pocketcoind)
 * of a given instance of the Pocketcoin system.
 */
class CBaseChainParams
{
public:
    /** BIP70 chain name strings (main, test or regtest) */
    static const std::string MAIN;
    static const std::string TESTNET;
    static const std::string REGTEST;

    const std::string& DataDir() const { return strDataDir; }
    int RPCPort() const { return nRPCPort; }
    int PublicRPCPort() const { return nPublicRPCPort; }
    int StaticRPCPort() const { return nStaticRPCPort; }
    int RestPort() const { return nRestPort; }

    CBaseChainParams() = delete;
    CBaseChainParams(const std::string& data_dir, int rpc_port, int rpc_pub_port, int rpc_static_port, int rest_port) :
                                                                                    nRPCPort(rpc_port),
                                                                                    nPublicRPCPort(rpc_pub_port),
                                                                                    nStaticRPCPort(rpc_static_port),
                                                                                    nRestPort(rest_port),
                                                                                    strDataDir(data_dir) {}

private:
    int nRPCPort;
    int nPublicRPCPort;
    int nStaticRPCPort;
    int nRestPort;
    std::string strDataDir;
};

/**
 * Creates and returns a std::unique_ptr<CBaseChainParams> of the chosen chain.
 * @returns a CBaseChainParams* of the chosen chain.
 * @throws a std::runtime_error if the chain is not supported.
 */
std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const std::string& chain);

/**
 *Set the arguments for chainparams
 */
void SetupChainParamsBaseOptions();

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CBaseChainParams& BaseParams();

/** Sets the params returned by Params() to those for the given network. */
void SelectBaseParams(const std::string& chain);

#endif // POCKETCOIN_CHAINPARAMSBASE_H
