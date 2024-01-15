// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef WSHANDLERPROVIDER_H
#define WSHANDLERPROVIDER_H

#include <websocket/ws.h>

class WsHandlerProvider
{
public:
    static std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, std::shared_ptr<SimpleWeb::InMessage>)> on_message();
    static std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, int, const std::string &)> on_close();
    static std::function<void(std::shared_ptr<SimpleWeb::IWSConnection>, const SimpleWeb::error_code &)> on_error();
};

#endif // WSHANDLERPROVIDER_H
