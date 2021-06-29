// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_NFTFILESERVICE_H
#define SRC_NFTFILESERVICE_H

#include <string>
#include <fs.h>

class NftFileService
{
public:
    void AppendPartToTempFile(std::string& id, std::string& payload);
    void MoveToPersistence(std::string& id);

private:
    fs::path GetTempFilesDir();
    fs::path GetPersistenceFileDir(std::string& id);
    std::string GetFileSubFolder(std::string& id);
};


#endif //SRC_NFTFILESERVICE_H
