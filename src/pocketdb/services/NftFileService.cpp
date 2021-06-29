// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "NftFileService.h"
#include "util.h"

void NftFileService::AppendPartToTempFile(std::string& id, std::string& payload)
{
    std::ofstream file;
    fs::path tempFileDir = GetTempFilesDir();
    fs::path tempFilePath = tempFileDir;
    tempFilePath += id;

    if (!fs::exists(tempFileDir))
    {
        if (!fs::create_directories(tempFileDir))
        {
            LogPrintf("Unable to create directory for temp nft files %s\n", tempFileDir.string());
            throw std::runtime_error("Unable to create directory for temp nft files");
        }
    }

    file.open(tempFilePath.string().c_str(), std::_S_out | std::_S_app);
    if (!file.is_open()) {
        LogPrintf("Unable to open temp nft file %s for writing\n", tempFilePath.string());
        throw std::runtime_error("Unable to open temp nft file %s for writing");
    }
    file << payload;
    file.close();
}

void NftFileService::MoveToPersistence(std::string& id)
{
    auto tempFilePath = GetTempFilesDir() / id;
    auto persistenceFileDir = GetPersistenceFileDir(id);
    auto persistenceFilePath = persistenceFileDir / id;

    if (!fs::exists(tempFilePath))
    {
        LogPrintf("Temp nft file doesn't exists %s\n", tempFilePath.string());
        throw std::runtime_error("Temp nft file doesn't exists");
    }

    if (!fs::exists(persistenceFileDir))
    {
        if (!fs::create_directories(persistenceFileDir))
        {
            LogPrintf("Unable to create directory for persistence nft files %s\n", persistenceFileDir.string());
            throw std::runtime_error("Unable to create directory for persistence nft files");
        }
    }

    fs::copy_file(tempFilePath, persistenceFilePath);
    fs::remove(tempFilePath);
}

fs::path NftFileService::GetTempFilesDir()
{
    auto result = GetDataDir();
    result += "/nft/temp/";

    return result;
}

fs::path NftFileService::GetPersistenceFileDir(std::string& id)
{
    return GetDataDir() / "nft/persistence" / GetFileSubFolder(id);
}

std::string NftFileService::GetFileSubFolder(std::string& id)
{
    return id.substr(0, 2);
}
