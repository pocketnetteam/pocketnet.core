#pragma once

#include "logging.h"
#include "rpc/blockchain.h"
#include "univalue.h"
#include "util/time.h"

#include "chainparams.h"
#include "validation.h"
#include "util/ref.h"
#include <boost/thread.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <net.h>
#include <numeric>
#include <set>

namespace Statistic
{

    using RequestKey = std::string;
    using RequestTime = std::chrono::milliseconds;
    using RequestIP = std::string;
    using RequestPayloadSize = std::size_t;

    struct RequestSample
    {
        RequestKey Key;
        RequestTime TimestampBegin;
        RequestTime TimestampEnd;
        RequestIP SourceIP;
        RequestPayloadSize InputSize;
        RequestPayloadSize OutputSize;
    };

    class RequestStatEngine
    {
    public:
        RequestStatEngine() = default;

        void AddSample(const RequestSample& sample)
        {
            if (sample.TimestampEnd < sample.TimestampBegin)
                return;

            LOCK(_samplesLock);
            _samples.push_back(sample);
        }

        std::size_t GetNumSamples()
        {
            LOCK(_samplesLock);
            return _samples.size();
        }

        std::size_t GetNumSamplesBetween(RequestTime begin, RequestTime end)
        {
            LOCK(_samplesLock);
            return std::count_if(
                _samples.begin(),
                _samples.end(),
                [=](const RequestSample& sample)
                {
                    return sample.TimestampBegin >= begin && sample.TimestampEnd <= end;
                });
        }

        std::size_t GetNumSamplesBefore(RequestTime time)
        {
            return GetNumSamplesBetween(RequestTime::min(), time);
        }

        std::size_t GetNumSamplesSince(RequestTime time)
        {
            return GetNumSamplesBetween(time, RequestTime::max());
        }

        RequestTime GetAvgRequestTimeSince(RequestTime since)
        {
            LOCK(_samplesLock);

            if (_samples.empty())
                return {};

            RequestTime sum{};
            std::size_t count{};

            for (auto& sample : _samples)
            {
                if (sample.TimestampBegin >= since && sample.Key != "WorkQueue::Enqueue")
                {
                    sum += sample.TimestampEnd - sample.TimestampBegin;
                    count++;
                }
            }

            if (count <= 0) return {};
            return sum / count;
        }

        int GetWorkQueueAvgCount(RequestTime since)
        {
            LOCK(_samplesLock);

            if (_samples.empty())
                return {};

            int count = 0;
            for (auto& sample : _samples)
            {
                if (sample.TimestampBegin >= since && sample.Key == "WorkQueue::Enqueue")
                    count++;
            }

            return count;
        }

        RequestTime GetAvgRequestTime()
        {
            return GetAvgRequestTimeSince(RequestTime::min());
        }

        std::vector<RequestSample> GetTopHeavyTimeSamplesSince(std::size_t limit, RequestTime since)
        {
            return GetTopTimeSamplesImpl(limit, since);
        }

        std::vector<RequestSample> GetTopHeavyTimeSamples(std::size_t limit)
        {
            return GetTopHeavyTimeSamplesSince(limit, RequestTime::min());
        }

        std::vector<RequestSample> GetTopHeavyInputSamplesSince(std::size_t limit, RequestTime since)
        {
            return GetTopSizeSamplesImpl(limit, &RequestSample::InputSize, since);
        }

        std::vector<RequestSample> GetTopHeavyInputSamples(std::size_t limit)
        {
            return GetTopHeavyInputSamplesSince(limit, RequestTime::min());
        }

        std::vector<RequestSample> GetTopHeavyOutputSamplesSince(std::size_t limit, RequestTime since)
        {
            return GetTopSizeSamplesImpl(limit, &RequestSample::OutputSize, since);
        }

        std::vector<RequestSample> GetTopHeavyOutputSamples(std::size_t limit)
        {
            return GetTopHeavyOutputSamplesSince(limit, RequestTime::min());
        }

        std::set<RequestIP> GetUniqueSourceIPsSince(RequestTime since)
        {
            std::set<RequestIP> result{};

            LOCK(_samplesLock);
            for (auto& sample : _samples)
                if (sample.TimestampBegin >= since)
                    result.insert(sample.SourceIP);

            return result;
        }

        std::set<RequestIP> GetUniqueSourceIPs()
        {
            return GetUniqueSourceIPsSince(RequestTime::min());
        }

        UniValue CompileStatsAsJsonSince(RequestTime since, const util::Ref& context)
        {
            UniValue result{UniValue::VOBJ};
            const auto& node = EnsureNodeContext(context);

            constexpr auto top_limit = 1;
            const auto sample_to_json = [](const RequestSample& sample)
            {
                UniValue value{UniValue::VOBJ};

                value.pushKV("Key", sample.Key);
                value.pushKV("TimestampBegin", sample.TimestampBegin.count());
                value.pushKV("TimestampEnd", sample.TimestampEnd.count());
                value.pushKV("TimestampProcess", sample.TimestampEnd.count() - sample.TimestampBegin.count());
                value.pushKV("SourceIP", sample.SourceIP);
                value.pushKV("InputSize", (int) sample.InputSize);
                value.pushKV("OutputSize", (int) sample.OutputSize);

                return value;
            };

            UniValue unique_ips_json{UniValue::VARR};
            UniValue top_tm_json{UniValue::VARR};
            UniValue top_in_json{UniValue::VARR};
            UniValue top_out_json{UniValue::VARR};

            auto unique_ips = GetUniqueSourceIPsSince(since);
            auto unique_ips_count = unique_ips.size();
            if (LogInstance().WillLogCategory(BCLog::STATDETAIL))
            {
                auto top_tm = GetTopHeavyTimeSamplesSince(top_limit, since);
                auto top_in = GetTopHeavyInputSamplesSince(top_limit, since);
                auto top_out = GetTopHeavyOutputSamplesSince(top_limit, since);

                for (auto& ip : unique_ips)
                    unique_ips_json.push_back(ip);

                for (auto& sample : top_tm)
                    top_tm_json.push_back(sample_to_json(sample));

                for (auto& sample : top_in)
                    top_in_json.push_back(sample_to_json(sample));

                for (auto& sample : top_out)
                    top_out_json.push_back(sample_to_json(sample));
            }

            UniValue chainStat(UniValue::VOBJ);
            chainStat.pushKV("Chain", Params().NetworkIDString());
            chainStat.pushKV("Height", ChainActive().Height());
            chainStat.pushKV("LastBlock", ChainActive().Tip()->GetBlockHash().GetHex());
            chainStat.pushKV("PeersALL", (int) node.connman->GetNodeCount(CConnman::NumConnections::CONNECTIONS_OUT));
            chainStat.pushKV("PeersIN", (int) node.connman->GetNodeCount(CConnman::NumConnections::CONNECTIONS_IN));
            chainStat.pushKV("PeersOUT", (int) node.connman->GetNodeCount(CConnman::NumConnections::CONNECTIONS_OUT));
            result.pushKV("General", chainStat);

            UniValue rpcStat(UniValue::VOBJ);
            rpcStat.pushKV("Requests", (int) GetNumSamplesSince(since));
            rpcStat.pushKV("AvgReqTime", GetAvgRequestTimeSince(since).count());
            rpcStat.pushKV("UniqueIPs", (int) unique_ips_count);
            if (LogInstance().WillLogCategory(BCLog::STATDETAIL))
            {
                rpcStat.pushKV("UniqueIps", unique_ips_json);
                rpcStat.pushKV("TopTime", top_tm_json);
                rpcStat.pushKV("TopInputSize", top_in_json);
                rpcStat.pushKV("TopOutputSize", top_out_json);
            }
            result.pushKV("RPC", rpcStat);

            return result;
        }

        UniValue CompileStatsAsJson(const util::Ref& context)
        {
            return CompileStatsAsJsonSince(RequestTime::min(), context);
        }

        // Just a helper to prevent copypasta
        RequestTime GetCurrentSystemTime()
        {
            return std::chrono::duration_cast<RequestTime>(std::chrono::system_clock::now().time_since_epoch());
        }

        void Run(boost::thread_group& threadGroup, const util::Ref& context)
        {
            shutdown = false;
            threadGroup.create_thread(boost::bind(&RequestStatEngine::PeriodicStatLogger, this, boost::cref(context)));
        }

        void Stop()
        {
            shutdown = true;
        }

        void PeriodicStatLogger(const util::Ref& context)
        {
            auto statLoggerSleep = gArgs.GetArg("-statdepth", 60) * 1000;
            std::string msg = "Statistic for last %lds:\n%s\n";

            while (!shutdown)
            {
                auto chunkSize = GetCurrentSystemTime() - std::chrono::milliseconds(statLoggerSleep);
                LogPrint(BCLog::STAT, msg.c_str(), statLoggerSleep / 1000, CompileStatsAsJsonSince(chunkSize, context).write(1));
                LogPrint(BCLog::STATDETAIL, msg.c_str(), statLoggerSleep / 1000,
                    CompileStatsAsJsonSince(chunkSize, context).write(1));

                RemoveSamplesBefore(chunkSize);
                UninterruptibleSleep(std::chrono::milliseconds{statLoggerSleep});
            }
        }

    private:
        std::vector<RequestSample> _samples;
        Mutex _samplesLock;
        bool shutdown = false;

        void RemoveSamplesBefore(RequestTime time)
        {
            LOCK(_samplesLock);
            int64_t sizeBefore = _samples.size();

            _samples.erase(
                std::remove_if(
                    _samples.begin(),
                    _samples.end(),
                    [time](const RequestSample& sample)
                    {
                        return sample.TimestampBegin < time;
                    }),
                _samples.end());

            LogPrint(BCLog::STAT, "Clear statistic cache: %d -> %d items after.\n", sizeBefore, _samples.size());
        }

        std::vector<RequestSample>
        GetTopSizeSamplesImpl(std::size_t limit, RequestPayloadSize RequestSample::*size_field, RequestTime since)
        {
            LOCK(_samplesLock);
            auto samples_copy = _samples;

            samples_copy.erase(
                std::remove_if(
                    samples_copy.begin(),
                    samples_copy.end(),
                    [since](const RequestSample& sample)
                    {
                        return sample.TimestampBegin < since;
                    }),
                samples_copy.end());

            std::sort(
                samples_copy.begin(),
                samples_copy.end(),
                [size_field](const RequestSample& left, const RequestSample& right)
                {
                    return left.*size_field > right.*size_field;
                });

            if (samples_copy.size() > limit)
                samples_copy.resize(limit);

            return samples_copy;
        }

        std::vector<RequestSample> GetTopTimeSamplesImpl(std::size_t limit, RequestTime since)
        {
            LOCK(_samplesLock);
            auto samples_copy = _samples;

            samples_copy.erase(
                std::remove_if(
                    samples_copy.begin(),
                    samples_copy.end(),
                    [since](const RequestSample& sample)
                    {
                        return sample.TimestampBegin < since;
                    }),
                samples_copy.end());

            std::sort(
                samples_copy.begin(),
                samples_copy.end(),
                [](const RequestSample& left, const RequestSample& right)
                {
                    return left.TimestampEnd - left.TimestampBegin > right.TimestampEnd - right.TimestampBegin;
                });

            if (samples_copy.size() > limit)
                samples_copy.resize(limit);

            return samples_copy;
        }
    };

} // namespace Statistic