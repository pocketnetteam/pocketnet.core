// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_WEB_ICONNECTION_H
#define POCKETNET_CORE_WEB_ICONNECTION_H

#include <string>

// TODO (losty): a very common to IReplier from RPC. Consider generalize

/**
 * Class that is needed to generalize nitification functional
 * between webrtc's datachannel and websocket
 */
class IConnection
{
public:
    virtual void Send(const std::string& msg) = 0;
    virtual void Close(int code) = 0;
    virtual const std::string& GetRemoteEndpointAddress() const = 0;
};

#endif // POCKETNET_CORE_WEB_ICONNECTION_H