// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "web/WSConnection.h"

WSConnection::WSConnection(std::shared_ptr<SimpleWeb::IWSConnection> wsConnection)
    : m_rawWsConnection(std::move(wsConnection))
{
    m_remote_endpoint_address = m_rawWsConnection->remote_endpoint_address();
}

void WSConnection::Send(const std::string &msg)
{
    m_rawWsConnection->send(msg);
}

void WSConnection::Close(int code)
{
    m_rawWsConnection->send_close(code);
}

const std::string& WSConnection::GetRemoteEndpointAddress() const
{
    return m_remote_endpoint_address;    
}
