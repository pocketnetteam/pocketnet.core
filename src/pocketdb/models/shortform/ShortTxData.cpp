// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/shortform/ShortTxData.h"

#include "pocketdb/helpers/ShortFormHelper.h"


PocketDb::ShortTxData::ShortTxData(std::string hash, PocketTx::TxType txType, std::optional<std::string> address, std::optional<int64_t> height,
                                    std::optional<int64_t> blockNum, std::optional<ShortAccount> account, std::optional<int> val,
                                    std::optional<std::string> description, std::optional<std::string> commentParentId,
                                    std::optional<std::string> commentAnswerId, std::optional<std::string> rootTxHash,
                                    std::optional<std::vector<std::pair<std::string, std::optional<ShortAccount>>>> multipleAddresses
                                  )
    : m_hash(std::move(hash)),
      m_txType(txType),
      m_height(std::move(height)),
      m_blockNum(std::move(blockNum)),
      m_address(std::move(address)),
      m_account(std::move(account)),
      m_val(std::move(val)),
      m_description(std::move(description)),
      m_commentParentId(std::move(commentParentId)),
      m_commentAnswerId(std::move(commentAnswerId)),
      m_rootTxHash(std::move(rootTxHash)),
      m_multipleAddresses(std::move(multipleAddresses))
{}

PocketDb::ShortTxData::ShortTxData(std::string hash, PocketTx::TxType txType)
    : m_hash(std::move(hash)),
      m_txType(txType)
{}

UniValue PocketDb::ShortTxData::Serialize() const
{
    UniValue data(UniValue::VOBJ);

    data.pushKV("hash", m_hash);
    data.pushKV("txType", (int)m_txType);
    if (m_height) data.pushKV("height", m_height.value());
    if (m_blockNum) data.pushKV("blockNum", m_blockNum.value());
    if (m_address) data.pushKV("address", m_address.value());
    if (m_account) data.pushKV("account", m_account->Serialize());
    if (m_val) data.pushKV("val", m_val.value());
    if (m_description) data.pushKV("description", m_description.value());
    if (m_commentParentId) data.pushKV("commentParentId", m_commentParentId.value());
    if (m_commentAnswerId) data.pushKV("commentAnswerId", m_commentAnswerId.value());
    if (m_rootTxHash) data.pushKV("rootTxHash", *m_rootTxHash);
    if (m_inputs) {
        UniValue inputs (UniValue::VARR);
        std::vector<UniValue> tmp;
        for (const auto& input: *m_inputs) {
            tmp.emplace_back(std::move(input.Serialize()));
        }
        inputs.push_backV(tmp);
        data.pushKV("inputs", inputs);
    }
    if (m_outputs) {
        UniValue outputs (UniValue::VARR);
        std::vector<UniValue> tmp;
        for (const auto& output: *m_outputs) {
            tmp.emplace_back(std::move(output.Serialize()));
        }
        outputs.push_backV(tmp);
        data.pushKV("outputs", outputs);
    }
    if (m_multipleAddresses) {
        UniValue multipleAddresses(UniValue::VOBJ);
        multipleAddresses.reserveKVSize(m_multipleAddresses->size());
        for (const auto& addressData: *m_multipleAddresses) {
            multipleAddresses.pushKV(addressData.first, std::move(addressData.second->Serialize()), false);
        }
        data.pushKV("multipleAddresses", multipleAddresses);
    }

    return data;
}

const std::string& PocketDb::ShortTxData::GetHash() const { return m_hash; }

const std::optional<std::string>& PocketDb::ShortTxData::GetAddress() const { return m_address; }

void PocketDb::ShortTxData::SetAddress(const std::optional<std::string>& address) { m_address = address; }

PocketTx::TxType PocketDb::ShortTxData::GetTxType() const { return m_txType; }

const std::optional<int64_t>& PocketDb::ShortTxData::GetHeight() const { return m_height; }

void PocketDb::ShortTxData::SetHeight(const std::optional<int64_t>& height) { m_height = height; }

const std::optional<int64_t>& PocketDb::ShortTxData::GetBlockNum() const { return m_blockNum; }

void PocketDb::ShortTxData::SetBlockNum(const std::optional<int64_t>& blockNum) { m_blockNum = blockNum; }

const std::optional<PocketDb::ShortAccount>& PocketDb::ShortTxData::GetAccount() const { return m_account; }

void PocketDb::ShortTxData::SetAccount(const std::optional<ShortAccount>& account) { m_account = account; }

const std::optional<int>& PocketDb::ShortTxData::GetVal() const { return m_val; }

void PocketDb::ShortTxData::SetVal(const std::optional<int>& val) { m_val = val; }

const std::optional<std::string>& PocketDb::ShortTxData::GetDescription() const { return m_description; }

void PocketDb::ShortTxData::SetDescription(const std::optional<std::string>& description) { m_description = description; }

void PocketDb::ShortTxData::SetCommentParentId(const std::optional<std::string>& commentParentId) { m_commentParentId = commentParentId; }

const std::optional<std::string>& PocketDb::ShortTxData::GetCommentParentId() const { return m_commentParentId; }

void PocketDb::ShortTxData::SetCommentAnswerId(const std::optional<std::string>& commentAnswerId) { m_commentAnswerId = commentAnswerId; }

const std::optional<std::string>& PocketDb::ShortTxData::GetCommentAnswerId() const { return m_commentAnswerId; }

void PocketDb::ShortTxData::SetRootTxHash(const std::optional<std::string>& rootTxHash) { m_rootTxHash = rootTxHash; }

const std::optional<std::string>& PocketDb::ShortTxData::GetRootTxHash() const { return m_rootTxHash; }

void PocketDb::ShortTxData::SetMultipleAddresses(const std::optional<std::vector<std::pair<std::string, std::optional<ShortAccount>>>>& multipleAddresses) { m_multipleAddresses = multipleAddresses; }

const std::optional<std::vector<std::pair<std::string, std::optional<PocketDb::ShortAccount>>>>& PocketDb::ShortTxData::GetMultipleAddresses() { return m_multipleAddresses; }

void PocketDb::ShortTxData::SetOutputs(const std::optional<std::vector<ShortTxOutput>>& outputs) { m_outputs = outputs; }

const std::optional<std::vector<PocketDb::ShortTxOutput>>& PocketDb::ShortTxData::GetOutputs() const { return m_outputs; }

void PocketDb::ShortTxData::SetInputs(const std::optional<std::vector<ShortTxOutput>>& inputs) { m_inputs = inputs; }

const std::optional<std::vector<PocketDb::ShortTxOutput>>& PocketDb::ShortTxData::GetInputs() const { return m_inputs; }
