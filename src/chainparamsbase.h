// Copyright (c) 2014-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_CHAINPARAMSBASE_H
#define POCKETCOIN_CHAINPARAMSBASE_H

#include <memory>
#include <string>

class ArgsManager;

/**
 * CBaseChainParams defines the base parameters (shared between pocketcoin-cli and pocketcoind)
 * of a given instance of the Pocketcoin system.
 */
class CBaseChainParams
{
public:
    ///@{
    /** Chain name strings */
    static const std::string MAIN;
    static const std::string TESTNET;
    static const std::string SIGNET;
    static const std::string REGTEST;
    ///@}

    const std::string& DataDir() const { return strDataDir; }
    uint16_t RPCPort() const { return m_rpc_port; }
    uint16_t PublicRPCPort() const { return m_public_rpc_port; }
    uint16_t StaticRPCPort() const { return m_static_rpc_port; }
    uint16_t RestPort() const { return m_rest_port; }
    uint16_t OnionServiceTargetPort() const { return m_onion_service_target_port; }

    CBaseChainParams() = delete;
    CBaseChainParams(const std::string& data_dir, uint16_t rpc_port, uint16_t rpc_pub_port, uint16_t rpc_static_port, uint16_t rest_port, uint16_t onion_service_target_port) :
                                                                                    m_rpc_port(rpc_port),
                                                                                    m_public_rpc_port(rpc_pub_port),
                                                                                    m_static_rpc_port(rpc_static_port),
                                                                                    m_rest_port(rest_port),
                                                                                    m_onion_service_target_port(onion_service_target_port),
                                                                                    strDataDir(data_dir) {}

private:
    const uint16_t m_rpc_port;
    const uint16_t m_public_rpc_port;
    const uint16_t m_static_rpc_port;
    const uint16_t m_rest_port;
    const uint16_t m_onion_service_target_port;
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
void SetupChainParamsBaseOptions(ArgsManager& argsman);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CBaseChainParams& BaseParams();

/** Sets the params returned by Params() to those for the given network. */
void SelectBaseParams(const std::string& chain);

#endif // POCKETCOIN_CHAINPARAMSBASE_H
