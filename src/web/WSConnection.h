// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEB_WSCONNECTION_H
#define POCKETNET_CORE_WEB_WSCONNECTION_H

#include "web/IConnection.h"
#include "websocket/ws.h"

#include <string>

/**
 * Simple wrapper around SimpleWeb's websocket to provide
 * generalizing with wih datachannel connection
 * 
 */
class WSConnection : public IConnection
{
public:
    explicit WSConnection(std::shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> wsConnection);
    void Send(const std::string& msg) override;
    void Close(int code) override;
    const std::string& GetRemoteEndpointAddress() const override;
private:
    std::string m_remote_endpoint_address;
    std::shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> m_rawWsConnection;
};

#endif // POCKETNET_CORE_WEB_WSCONNECTION_H