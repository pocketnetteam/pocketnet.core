// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETFRONTEND_HPP
#define SRC_POCKETFRONTEND_HPP

#include "rpc/protocol.h"

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

        tuple<bool, string> ReadFileFromDisk(const string& path)
        {
            try
            {
                ifstream file(path);
                string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

                return {true, content};
            }
            catch (const std::exception& e)
            {
                LogPrintf("Warning: failed read file %s with error %s\n", path, e.what());
                return {false, ""};
            }
        }

        tuple<bool, shared_ptr<StaticFile>> ReadFile(const string& path)
        {
            auto _path = path;
            auto _name = path;

            // Remove parameters - index.html?v2
            std::vector<std::string> pathPrms;
            boost::split(pathPrms, path, boost::is_any_of("?"));
            if (pathPrms.size() > 1)
                _path = pathPrms.front();

            // Split parts for detect name
            std::vector<std::string> pathParts;
            boost::split(pathParts, _path, boost::is_any_of("/"));
            if (pathParts.size() > 1)
                _name = pathParts.back();

            // Try read file from disk
            auto[readOk, content] = ReadFileFromDisk(_path);
            if (!readOk)
                return {false, nullptr};

            // Build file struct
            auto file = shared_ptr<StaticFile>(new StaticFile{
                _path,
                _name,
                DetectContentType(_name),
                content
            });

            return {true, file};
        }

        string DetectContentType(string fileName)
        {
            auto _extension = fileName;

            std::vector<std::string> nameParts;
            boost::split(nameParts, fileName, boost::is_any_of("."));
            if (nameParts.size() > 1)
                _extension = nameParts.back();

            if (MimeTypes.find(_extension) != MimeTypes.end())
                return MimeTypes[_extension];

            return MimeTypes["default"];
        }

    public:

        PocketFrontend()
        {
            auto testContent = shared_ptr<StaticFile>(new StaticFile{
                "/",
                "test.html",
                "",
                "<html><head><script src='main.js'></script></head><body>Hello World!</body></html>"
            });

            LOCK(CacheMutex);
            Cache.emplace(
                "/test.html",
                testContent
            );
        }

        void ClearCache()
        {
            LOCK(CacheMutex);
            Cache.clear();

            LogPrint(BCLog::RESTFRONTEND, "Cache cleared\n");
        }

        void CacheEmplace(const string& path, shared_ptr<StaticFile>& content)
        {
            // TODO (brangr): check content cacheable

            LOCK(CacheMutex);
            if (Cache.find(path) == Cache.end())
            {
                LogPrint(BCLog::RESTFRONTEND, "File '%s' emplaced in cache\n", path);
                Cache.emplace(path, content);
            }
        }

        tuple<bool, shared_ptr<StaticFile>> CacheGet(const string& path)
        {
            LOCK(CacheMutex);
            if (Cache.find(path) != Cache.end())
            {
                LogPrint(BCLog::RESTFRONTEND, "File '%s' found in cache\n", path);
                return {true, Cache.at(path)};
            }

            return {false, nullptr};
        }

        tuple<HTTPStatusCode, shared_ptr<StaticFile>> GetFile(const string& path)
        {
            if (path.empty())
                return {HTTP_BAD_REQUEST, nullptr};

            if (auto[ok, cacheContent] = CacheGet(path); ok)
                return {HTTP_OK, cacheContent};

            // Return HTTP_FORBIDDEN if file too large or in blocked
            // TODO (brangr): Check restrictions

            // Read file and return answer with HTTP_OK
            auto[readOk, fileContent] = ReadFile(path);
            if (!readOk)
                return {HTTP_NOT_FOUND, nullptr};

            // Save in cache for future
            CacheEmplace(path, fileContent);

            LogPrint(BCLog::RESTFRONTEND, "File '%s' readed from disk\n", path);

            return {HTTP_SERVICE_UNAVAILABLE, nullptr};
        }

    };

} // namespace PocketWeb


#endif //SRC_POCKETFRONTEND_HPP
