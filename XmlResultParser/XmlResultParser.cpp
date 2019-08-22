/*

DISKSPD

Copyright(c) Microsoft Corporation
All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "xmlresultparser.h"

XmlResultParser::XmlResultParser(const std::string& millisecondsFormatString,
                                 const std::string& secondsFormatString,
                                 const std::string& percentFormatString) :
    _sMillisecondsFormatString(millisecondsFormatString), _sSecondsFormatString(secondsFormatString), _sPercentFormatString(percentFormatString)
{
}

// TODO: refactor to a single function shared with the ResultParser
void XmlResultParser::_Output(const char* format, ...)
{
    assert(nullptr != format);
    va_list listArg;
    va_start(listArg, format);
    char buffer[4096] = {};
    vsprintf_s(buffer, _countof(buffer), format, listArg);
    va_end(listArg);
    _sResult += buffer;
}

void XmlResultParser::_OutputValue(const std::string& name, const std::string& value)
{
    _Output("<%s>%s</%s>\n", name.c_str(), value.c_str(), name.c_str());
}

void XmlResultParser::_OutputValue(const std::string& name, int value)
{
    _OutputValue(name, std::to_string(value));
}

void XmlResultParser::_OutputValue(const std::string& name, unsigned int value)
{
    _OutputValue(name, std::to_string(value));
}

void XmlResultParser::_OutputValue(const std::string& name, long value)
{
    _OutputValue(name, std::to_string(value));
}

void XmlResultParser::_OutputValue(const std::string& name, unsigned long value)
{
    _OutputValue(name, std::to_string(value));
}

void XmlResultParser::_OutputValue(const std::string& name, unsigned long long value)
{
    _OutputValue(name, std::to_string(value));
}

void XmlResultParser::_OutputValue(const std::string& name, double value, const std::string& formatString)
{
    _OutputValue(name, Util::DoubleToStringHelper(value, formatString.c_str()));
}

void XmlResultParser::_OutputValueInPercent(const std::string& name, double value)
{
    _OutputValue(name + "Percent", value, _sPercentFormatString);
}

void XmlResultParser::_OutputValueInSeconds(const std::string& name, double value)
{
    _OutputValue(name + "Seconds", value, _sSecondsFormatString);
}

void XmlResultParser::_OutputValueMilliseconds(const std::string& name, double value)
{
    _OutputValue(name, value / 1000.0, _sMillisecondsFormatString);
}

void XmlResultParser::_OutputValueInMilliseconds(const std::string& name, double value)
{
    _OutputValueMilliseconds(name + "Milliseconds", value);
}

void XmlResultParser::_OutputLatencyInMilliseconds(const std::string& name, double value)
{
    _OutputValueInMilliseconds(name + "Latency", value);
}

void XmlResultParser::_OutputTargetResults(const TargetResults& results,
                                           bool fMeasureLatency,
                                           ConstHistogramBucketListPtr histogramBucketList,
                                           double fTestDurationInSeconds,
                                           bool fCalculateIopsStdDev,
                                           UINT32 _ulIoBucketDurationInMilliseconds)
{
    // TODO: results.readBucketizer;
    // TODO: results.writeBucketizer;

    _OutputValue("Path", results.sPath.c_str());
    _OutputValue("BytesCount", results.ullBytesCount);
    _OutputValue("FileSize", results.ullFileSize);
    _OutputValue("IOCount", results.ullIOCount);
    _OutputValue("ReadBytes", results.ullReadBytesCount);
    _OutputValue("ReadCount", results.ullReadIOCount);
    _OutputValue("WriteBytes", results.ullWriteBytesCount);
    _OutputValue("WriteCount", results.ullWriteIOCount);
    if (fMeasureLatency)
    {
        /************************************************************************************************************************
         *   CBTODO: Move this totalLatencyHistogram Merge into targetResults and cache it there.
         ***********************************************************************************************************************/
        Histogram<float> totalLatencyHistogram;
        totalLatencyHistogram.Merge(results.writeLatencyHistogram);
        totalLatencyHistogram.Merge(results.readLatencyHistogram);

        _OutputLatencySummary(results.readLatencyHistogram, results.writeLatencyHistogram, totalLatencyHistogram, histogramBucketList, fTestDurationInSeconds);
    }

    if (fCalculateIopsStdDev)
    {
        _OutputTargetIops(results.readBucketizer, results.writeBucketizer, _ulIoBucketDurationInMilliseconds);
    }
}

void XmlResultParser::_OutputLatencySummary(const Histogram<float>& readLatencyHistogram,
                                            const Histogram<float>& writeLatencyHistogram,
                                            const Histogram<float>& totalLatencyHistogram,
                                            ConstHistogramBucketListPtr histogramBucketList,
                                            double fTestDurationInSeconds)
{
    if (readLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputLatencySummary(readLatencyHistogram, "Read");
    }

    if (writeLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputLatencySummary(writeLatencyHistogram, "Write");
    }

    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputLatencySummary(totalLatencyHistogram, std::string());
    }

    _OutputLatencyPercentiles(readLatencyHistogram, writeLatencyHistogram, totalLatencyHistogram);

    if (histogramBucketList)
    {
        _OutputLatencyBuckets(readLatencyHistogram, writeLatencyHistogram, totalLatencyHistogram, histogramBucketList, fTestDurationInSeconds);
    }
}

void XmlResultParser::_OutputLatencySummary(const Histogram<float>& latencyHistogram,
                                            const std::string& latencyHistogramName)
{
    _OutputValue(latencyHistogramName + "LatencyHistogramBins", latencyHistogram.GetBucketCount());
    _OutputLatencyInMilliseconds(latencyHistogramName + "Average", latencyHistogram.GetAvg());
    _OutputLatencyInMilliseconds(latencyHistogramName + "Stdev", latencyHistogram.GetStandardDeviation());
}

void XmlResultParser::_OutputTargetIops(const IoBucketizer& readBucketizer,
                                        const IoBucketizer& writeBucketizer,
                                        UINT32 bucketTimeInMs)
{
    _Output("<Iops>\n");

    IoBucketizer totalIoBucketizer;
    totalIoBucketizer.Merge(readBucketizer);
    totalIoBucketizer.Merge(writeBucketizer);

    if (readBucketizer.GetNumberOfValidBuckets() > 0)
    {
        _OutputValueMilliseconds("ReadIopsStdDev", readBucketizer.GetStandardDeviationIOPS() / (bucketTimeInMs / 1000.0));
    }
    if (writeBucketizer.GetNumberOfValidBuckets() > 0)
    {
        _OutputValueMilliseconds("WriteIopsStdDev", writeBucketizer.GetStandardDeviationIOPS() / (bucketTimeInMs / 1000.0));
    }
    if (totalIoBucketizer.GetNumberOfValidBuckets() > 0)
    {
        _OutputValueMilliseconds("IopsStdDev", totalIoBucketizer.GetStandardDeviationIOPS() / (bucketTimeInMs / 1000.0));
    }
    _OutputIops(readBucketizer, writeBucketizer, bucketTimeInMs);
    _Output("</Iops>\n");
}

void XmlResultParser::_OutputETWSessionInfo(struct ETWSessionInfo sessionInfo)
{
    _Output("<ETWSessionInfo>\n");
    _OutputValue("BufferSizeKB", sessionInfo.ulBufferSize);
    _OutputValue("MinimimBuffers", sessionInfo.ulMinimumBuffers);
    _OutputValue("MaximumBuffers", sessionInfo.ulMaximumBuffers);
    _OutputValue("FreeBuffers", sessionInfo.ulFreeBuffers);
    _OutputValue("BuffersWritten", sessionInfo.ulBuffersWritten);
    _OutputValue("FlushTimerSeconds", sessionInfo.ulFlushTimer);
    _OutputValue("AgeLimitMinutes", sessionInfo.lAgeLimit);

    _OutputValue("AllocatedBuffers", sessionInfo.ulNumberOfBuffers);
    _OutputValue("LostEvents", sessionInfo.ulEventsLost);
    _OutputValue("LostLogBuffers", sessionInfo.ulLogBuffersLost);
    _OutputValue("LostRealTimeBuffers", sessionInfo.ulRealTimeBuffersLost);
    _Output("</ETWSessionInfo>\n");
}

void XmlResultParser::_OutputETW(struct ETWMask ETWMask, struct ETWEventCounters EtwEventCounters)
{
    _Output("<ETW>\n");
    if (ETWMask.bDiskIO)
    {
        _Output("<DiskIO>\n");
        _OutputValue("Read", EtwEventCounters.ullIORead);
        _OutputValue("Write", EtwEventCounters.ullIOWrite);
        _Output("</DiskIO>\n");
    }
    if (ETWMask.bImageLoad)
    {
        _OutputValue("LoadImage", EtwEventCounters.ullImageLoad);
    }
    if (ETWMask.bMemoryPageFaults)
    {
        _Output("<MemoryPageFaults>\n");
        _OutputValue("CopyOnWrite", EtwEventCounters.ullMMCopyOnWrite);
        _OutputValue("DemandZeroFault", EtwEventCounters.ullMMDemandZeroFault);
        _OutputValue("GuardPageFault", EtwEventCounters.ullMMGuardPageFault);
        _OutputValue("HardPageFault", EtwEventCounters.ullMMHardPageFault);
        _OutputValue("TransitionFault", EtwEventCounters.ullMMTransitionFault);
        _Output("</MemoryPageFaults>\n");
    }
    if (ETWMask.bMemoryHardFaults && !ETWMask.bMemoryPageFaults)
    {
        _OutputValue("HardPageFault", EtwEventCounters.ullMMHardPageFault);
    }
    if (ETWMask.bNetwork)
    {
        _Output("<Network>\n");
        _OutputValue("Accept", EtwEventCounters.ullNetAccept);
        _OutputValue("Connect", EtwEventCounters.ullNetConnect);
        _OutputValue("Disconnect", EtwEventCounters.ullNetDisconnect);
        _OutputValue("Reconnect", EtwEventCounters.ullNetReconnect);
        _OutputValue("Retransmit", EtwEventCounters.ullNetRetransmit);
        _OutputValue("TCPIPSend", EtwEventCounters.ullNetTcpSend);
        _OutputValue("TCPIPReceive", EtwEventCounters.ullNetTcpReceive);
        _OutputValue("UDPIPSend", EtwEventCounters.ullNetUdpSend);
        _OutputValue("UDPIPReceive", EtwEventCounters.ullNetUdpReceive);
        _Output("</Network>\n");
    }
    if (ETWMask.bProcess)
    {
        _Output("<Process>\n");
        _OutputValue("Start", EtwEventCounters.ullProcessStart);
        _OutputValue("End", EtwEventCounters.ullProcessEnd);
        _Output("</Process>\n");
    }
    if (ETWMask.bRegistry)
    {
        _Output("<Registry>\n");
        _OutputValue("NtCreateKey", EtwEventCounters.ullRegCreate);
        _OutputValue("NtDeleteKey", EtwEventCounters.ullRegDelete);
        _OutputValue("NtDeleteValueKey", EtwEventCounters.ullRegDeleteValue);
        _OutputValue("NtEnumerateKey", EtwEventCounters.ullRegEnumerateKey);
        _OutputValue("NtEnumerateValueKey", EtwEventCounters.ullRegEnumerateValueKey);
        _OutputValue("NtFlushKey", EtwEventCounters.ullRegFlush);
        _OutputValue("NtOpenKey", EtwEventCounters.ullRegOpen);
        _OutputValue("NtQueryKey", EtwEventCounters.ullRegQuery);
        _OutputValue("NtQueryMultipleValueKey", EtwEventCounters.ullRegQueryMultipleValue);
        _OutputValue("NtQueryValueKey", EtwEventCounters.ullRegQueryValue);
        _OutputValue("NtSetInformationKey", EtwEventCounters.ullRegSetInformation);
        _OutputValue("NtSetValueKey", EtwEventCounters.ullRegSetValue);
        _Output("</Registry>\n");
    }
    if (ETWMask.bThread)
    {
        _Output("<Thread>\n");
        _OutputValue("Start", EtwEventCounters.ullThreadStart);
        _OutputValue("End", EtwEventCounters.ullThreadEnd);
        _Output("</Thread>\n");
    }
    _Output("</ETW>\n");
}

void XmlResultParser::_OutputCpuUtilization(const Results& results,
                                            const SystemInformation& system)
{
    size_t ulProcCount = results.vSystemProcessorPerfInfo.size();
    size_t ulBaseProc = 0;
    size_t ulActiveProcCount = 0;
    size_t ulNumGroups = system.processorTopology._vProcessorGroupInformation.size();

    _Output("<CpuUtilization>\n");

    double busyTime = 0;
    double totalIdleTime = 0;
    double totalUserTime = 0;
    double totalKrnlTime = 0;

    for (unsigned int ulGroup = 0; ulGroup < ulNumGroups; ulGroup++) {
        const ProcessorGroupInformation *pGroup = &system.processorTopology._vProcessorGroupInformation[ulGroup];

        // System has multiple groups but we only have counters for the first one
        if (ulBaseProc >= ulProcCount) {
            break;
        }
        
        for (unsigned int ulProcessor = 0; ulProcessor < pGroup->_maximumProcessorCount; ulProcessor++) {
            double idleTime;
            double userTime;
            double krnlTime;
            double thisTime;

            if (!pGroup->IsProcessorActive((BYTE)ulProcessor)) {
                continue;
            }

            long long fTime = results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].KernelTime.QuadPart +
                              results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].UserTime.QuadPart;

            idleTime = 100.0 * results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].IdleTime.QuadPart / fTime;
            krnlTime = 100.0 * results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].KernelTime.QuadPart / fTime;
            userTime = 100.0 * results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].UserTime.QuadPart / fTime;

            thisTime = (krnlTime + userTime) - idleTime;

            _Output("<CPU>\n");
            _OutputValue("Group", ulGroup);
            _OutputValue("Id", ulProcessor);
            _OutputValueInPercent("Usage", thisTime);
            _OutputValueInPercent("User", userTime);
            _OutputValueInPercent("Kernel", krnlTime - idleTime);
            _OutputValueInPercent("Idle", idleTime);
            _Output("</CPU>\n");

            busyTime += thisTime;
            totalIdleTime += idleTime;
            totalUserTime += userTime;
            totalKrnlTime += krnlTime;

            ulActiveProcCount++;
        }

        ulBaseProc += pGroup->_maximumProcessorCount;
    }

    if (ulActiveProcCount == 0) {
        ulActiveProcCount = 1;
    }

    _Output("<Average>\n");
    _OutputValueInPercent("Usage", busyTime / ulActiveProcCount);
    _OutputValueInPercent("User", totalUserTime / ulActiveProcCount);
    _OutputValueInPercent("Kernel", (totalKrnlTime - totalIdleTime) / ulActiveProcCount);
    _OutputValueInPercent("Idle", totalIdleTime / ulActiveProcCount);
    _Output("</Average>\n");

    _Output("</CpuUtilization>\n");
}

// emit the iops time series (this obviates needing perfmon counters, in common cases, and provides file level data)
void XmlResultParser::_OutputIops(const IoBucketizer& readBucketizer,
                                  const IoBucketizer& writeBucketizer,
                                  UINT32 bucketTimeInMs)
{
    bool done = false;
    for (size_t i = 0; !done; i++)
    {
        done = true;

        double r = 0.0;
        double r_min = 0.0;
        double r_max = 0.0;
        double r_avg = 0.0;
        double r_stddev = 0.0;

        double w = 0.0;
        double w_min = 0.0;
        double w_max = 0.0;
        double w_avg = 0.0;
        double w_stddev = 0.0;

        if (readBucketizer.GetNumberOfValidBuckets() > i)
        {
            r = readBucketizer.GetIoBucketCount(i) / (bucketTimeInMs / 1000.0);
            r_min = readBucketizer.GetIoBucketMinDurationUsec(i) / 1000.0;
            r_max = readBucketizer.GetIoBucketMaxDurationUsec(i) / 1000.0;
            r_avg = readBucketizer.GetIoBucketAvgDurationUsec(i) / 1000.0;
            r_stddev = readBucketizer.GetIoBucketDurationStdDevUsec(i) / 1000.0;
            done = false;
        }
        if (writeBucketizer.GetNumberOfValidBuckets() > i)
        {
            w = writeBucketizer.GetIoBucketCount(i) / (bucketTimeInMs / 1000.0);
            w_min = writeBucketizer.GetIoBucketMinDurationUsec(i) / 1000.0;
            w_max = writeBucketizer.GetIoBucketMaxDurationUsec(i) / 1000.0;
            w_avg = writeBucketizer.GetIoBucketAvgDurationUsec(i) / 1000.0;
            w_stddev = writeBucketizer.GetIoBucketDurationStdDevUsec(i) / 1000.0;
            done = false;
        }
        if (!done)
        {
            _Output("<Bucket SampleMillisecond=\"%lu\" Read=\"%.0f\" Write=\"%.0f\" Total=\"%.0f\" "
                    "ReadMinLatencyMilliseconds=\"%.3f\" ReadMaxLatencyMilliseconds=\"%.3f\" ReadAvgLatencyMilliseconds=\"%.3f\" ReadLatencyStdDev=\"%.3f\" "
                    "WriteMinLatencyMilliseconds=\"%.3f\" WriteMaxLatencyMilliseconds=\"%.3f\" WriteAvgLatencyMilliseconds=\"%.3f\" WriteLatencyStdDev=\"%.3f\"/>\n", 
                    bucketTimeInMs*(i + 1), r, w, r + w,
                    r_min, r_max, r_avg, r_stddev,
                    w_min, w_max, w_avg, w_stddev);
        }
    }
}

void XmlResultParser::_OutputOverallIops(const Results& results,
                                         UINT32 bucketTimeInMs)
{
    IoBucketizer readBucketizer;
    IoBucketizer writeBucketizer;

    for (const auto& thread : results.vThreadResults)
    {
        for (const auto& target : thread.vTargetResults)
        {
            readBucketizer.Merge(target.readBucketizer);
            writeBucketizer.Merge(target.writeBucketizer);
        }
    }

    _OutputTargetIops(readBucketizer, writeBucketizer, bucketTimeInMs);
}

void XmlResultParser::_OutputLatencyPercentiles(const Histogram<float>& readLatencyHistogram,
                                                const Histogram<float>& writeLatencyHistogram,
                                                const Histogram<float>& totalLatencyHistogram)
{
    _Output("<Latency>\n");
    _Output("<Bucket>\n");
    _OutputValue("Percentile", 0);
    if (readLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputValueInMilliseconds("Read", readLatencyHistogram.GetMin());
    }
    if (writeLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputValueInMilliseconds("Write", writeLatencyHistogram.GetMin());
    }
    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputValueInMilliseconds("Total", totalLatencyHistogram.GetMin());
    }
    _Output("</Bucket>\n");

    //  Construct vector of percentiles and decimal precision to squelch trailing zeroes.  This is more
    //  detailed than summary text output, and does not contain the decorated names (15th, etc.)

    vector<pair<int, double>> vPercentiles;
    for (int p = 1; p <= 99; p++)
    {
        vPercentiles.push_back(make_pair(0, p));
    }

    vPercentiles.push_back(make_pair(1, 99.9));
    vPercentiles.push_back(make_pair(2, 99.99));
    vPercentiles.push_back(make_pair(3, 99.999));
    vPercentiles.push_back(make_pair(4, 99.9999));
    vPercentiles.push_back(make_pair(5, 99.99999));
    vPercentiles.push_back(make_pair(6, 99.999999));
    vPercentiles.push_back(make_pair(7, 99.9999999));

    for (auto p : vPercentiles)
    {
        _Output("<Bucket>\n");
        _Output("<Percentile>%.*f</Percentile>\n", p.first, p.second);
        if (readLatencyHistogram.GetSampleSize() > 0)
        {
            _OutputValueInMilliseconds("Read", readLatencyHistogram.GetPercentile(p.second / 100));
        }
        if (writeLatencyHistogram.GetSampleSize() > 0)
        {
            _OutputValueInMilliseconds("Write", writeLatencyHistogram.GetPercentile(p.second / 100));
        }
        if (totalLatencyHistogram.GetSampleSize() > 0)
        {
            _OutputValueInMilliseconds("Total", totalLatencyHistogram.GetPercentile(p.second / 100));
        }
        _Output("</Bucket>\n");
    }

    _Output("<Bucket>\n");
    _OutputValue("Percentile", 100);
    if (readLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputValueInMilliseconds("Read", readLatencyHistogram.GetMax());
    }
    if (writeLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputValueInMilliseconds("Write", writeLatencyHistogram.GetMax());
    }
    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _OutputValueInMilliseconds("Total", totalLatencyHistogram.GetMax());
    }
    _Output("</Bucket>\n");

    _Output("</Latency>\n");
}

void XmlResultParser::_OutputLatencyBuckets(const Histogram<float>& readLatencyHistogram,
                                            const Histogram<float>& writeLatencyHistogram,
                                            const Histogram<float>& totalLatencyHistogram,
                                            ConstHistogramBucketListPtr histogramBucketList,
                                            double fTestDurationInSeconds)
{
    _Output("<FixedBucketLatency>\n");

    unsigned totalReadLatencyHistogramBins = 0;
    unsigned totalWriteLatencyHistogramBins = 0;
    unsigned totalReadWriteLatencyHistogramBins = 0;

    unsigned totalReadOpsCount = 0;
    unsigned totalWriteOpsCount = 0;
    unsigned totalReadWriteOpsCount = 0;

    float rangeMinUsec = 0.0;
    for (auto rangeMaxMilliSeconds : *histogramBucketList)
    {
        //	Histogram data is stored in microseconds but histogramBucketList is in milliseconds, so convert it here.
        float rangeMinMilliSeconds = rangeMinUsec / 1000.0f;
        float rangeMaxUsec = rangeMaxMilliSeconds * 1000.0f;

        unsigned readBucketCount = readLatencyHistogram.GetHitCount(rangeMinUsec, rangeMaxUsec);
        if (readBucketCount > 0)
        {
            totalReadLatencyHistogramBins++;
            totalReadOpsCount += readBucketCount;
        }

        unsigned writeBucketCount = writeLatencyHistogram.GetHitCount(rangeMinUsec, rangeMaxUsec);
        if (writeBucketCount > 0)
        {
            totalWriteLatencyHistogramBins++;
            totalWriteOpsCount += writeBucketCount;
        }

        unsigned readWriteBucketCount = totalLatencyHistogram.GetHitCount(rangeMinUsec, rangeMaxUsec);
        if (readWriteBucketCount > 0)
        {
            totalReadWriteLatencyHistogramBins++;
            totalReadWriteOpsCount += readWriteBucketCount;
        }

        _Output("<Bucket>\n");

        if ((rangeMaxMilliSeconds == std::numeric_limits<float>::max()))
        {
            _OutputValue("Time", "Max");
        }
        else
        {
            _OutputValue("Time", rangeMaxMilliSeconds, "%.1f");
        }

        if (readBucketCount > 0)
        {
            _OutputValue("ReadCount", readBucketCount);
        }

        if (writeBucketCount > 0)
        {
            _OutputValue("WriteCount", writeBucketCount);
        }

        if (readWriteBucketCount > 0)
        {
            _OutputValue("ReadWriteCount", readWriteBucketCount);
        }

        _Output("</Bucket>\n");

        rangeMinUsec = rangeMaxUsec;
    }

    //_OutputValueInSeconds("TestDuration", fTestDurationInSeconds);
    //_OutputTag("CalculatedIOPS", (totalReadWriteOpsCount / fTestDurationInSeconds));
    _Output("</FixedBucketLatency>\n");
}

string XmlResultParser::ParseResults(Profile& profile,
                                     const SystemInformation& system,
                                     vector<Results> vResults)
{
    _sResult.clear();

    _Output("<Results>\n");
    _sResult += system.GetXml();
    _sResult += profile.GetXml();
    for (size_t iResults = 0; iResults < vResults.size(); iResults++)
    {
        const Results& results = vResults[iResults];
        const TimeSpan& timeSpan = profile.GetTimeSpans()[iResults];

        _Output("<TimeSpan>\n");
        double fTime = PerfTimer::PerfTimeToSeconds(results.ullTimeCount); //test duration
        if (fTime >= 0.0000001)
        {
            // There either is a fixed number of threads for all files to share (GetThreadCount() > 0) or a number of threads per file.
            // In the latter case vThreadResults.size() == number of threads per file * file count
            size_t ulThreadCnt = (timeSpan.GetThreadCount() > 0) ? timeSpan.GetThreadCount() : results.vThreadResults.size();
            unsigned int ulProcCount = system.processorTopology._ulActiveProcCount;

            _OutputValueInSeconds("TestTime", fTime);
            _OutputValue("ThreadCount", ulThreadCnt);
            _OutputValue("RequestCount", timeSpan.GetRequestCount());
            _OutputValue("ProcCount", ulProcCount);

            _OutputCpuUtilization(results, system);

            if (timeSpan.GetMeasureLatency())
            {
                Histogram<float> readLatencyHistogram;
                Histogram<float> writeLatencyHistogram;
                Histogram<float> totalLatencyHistogram;

                for (const auto& thread : results.vThreadResults)
                {
                    for (const auto& target : thread.vTargetResults)
                    {
                        readLatencyHistogram.Merge(target.readLatencyHistogram);
                        writeLatencyHistogram.Merge(target.writeLatencyHistogram);
                        totalLatencyHistogram.Merge(target.writeLatencyHistogram);
                        totalLatencyHistogram.Merge(target.readLatencyHistogram);
                    }
                }

                _OutputLatencySummary(readLatencyHistogram, writeLatencyHistogram, totalLatencyHistogram, profile.GetHistogramBucketList(), fTime);

                ConstHistogramBucketListPtr histogramBucketList = profile.GetHistogramBucketList();
                if (histogramBucketList)
                {
                    _OutputLatencyBuckets(readLatencyHistogram, writeLatencyHistogram, totalLatencyHistogram, histogramBucketList, fTime);
                }
            }

            if (timeSpan.GetCalculateIopsStdDev())
            {
                _OutputOverallIops(results, timeSpan.GetIoBucketDurationInMilliseconds());
            }

            if (results.fUseETW)
            {
                _OutputETW(results.EtwMask, results.EtwEventCounters);
                _OutputETWSessionInfo(results.EtwSessionInfo);
            }

            for (size_t iThread = 0; iThread < results.vThreadResults.size(); iThread++)
            {
                const ThreadResults& threadResults = results.vThreadResults[iThread];

                _Output("<Thread>\n");
                _OutputValue("Id", iThread);

                for (const auto& targetResults : threadResults.vTargetResults)
                {
                    _Output("<Target>\n");
                    _OutputTargetResults(targetResults, timeSpan.GetMeasureLatency(), profile.GetHistogramBucketList(), fTime, timeSpan.GetCalculateIopsStdDev(),
                        timeSpan.GetIoBucketDurationInMilliseconds());
                    _Output("</Target>\n");
                }
                _Output("</Thread>\n");
            }
        }
        else
        {
            _Output("<Error>The test was interrupted before the measurements began. No results are displayed.</Error>\n");
        }
        _Output("</TimeSpan>\n");
    }
    _Output("</Results>");
    return _sResult;
}
