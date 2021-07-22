// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETFRONTEND_H
#define SRC_POCKETFRONTEND_H

#include "sync.h"
#include "util.h"
#include "logging.h"
#include "rpc/protocol.h"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

namespace PocketWeb
{
    using namespace std;

    struct StaticFile
    {
        string Path;
        string Name;
        string ContentType;
        string Content;
    };

    class PocketFrontend
    {
    protected:

        boost::filesystem::path _rootPath;

        Mutex CacheMutex;
        map<string, shared_ptr<StaticFile>> Cache;

        map<string, string> MimeTypes{
            {"default", "application/octet-stream"},
            {"svg",     "image/svg+xml"},
            {"js",      "application/javascript"},
            {"css",     "text/css"},
            {"json",    "application/json"},
            {"png",     "image/png"},
            {"woff2",   "application/font-woff2"},
            {"html",    "text/html"},
            {"jpg",     "image/jpeg"},
        };

        tuple<bool, string> ReadFileFromDisk(const string& path);

        tuple<bool, shared_ptr<StaticFile>> ReadFile(const string& path);

        string DetectContentType(string fileName);

    public:

        PocketFrontend() = default;

        void Init();

        void ClearCache();

        void CacheEmplace(const string& path, shared_ptr<StaticFile>& content);

        tuple<bool, shared_ptr<StaticFile>> CacheGet(const string& path);

        tuple<HTTPStatusCode, shared_ptr<StaticFile>> GetFile(const string& path, bool stopRecurse = false);

    };

} // namespace PocketWeb


#endif //SRC_POCKETFRONTEND_H
