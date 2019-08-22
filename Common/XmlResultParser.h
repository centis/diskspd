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

#pragma once

#include "Common.h"

class XmlResultParser : public IResultParser
{
public:
    XmlResultParser(const std::string& millisecondsFormatString, const std::string& secondsFormatString, const std::string& percentFormatString);
    string ParseResults(Profile& profile, const SystemInformation& system, vector<Results> vResults);

private:

    void _Output(const char* format, ...);
    void _OutputValue(const std::string& name, const std::string& value);
    
    void _OutputValue(const std::string& name, int value);
    void _OutputValue(const std::string& name, unsigned int value);
    void _OutputValue(const std::string& name, long value);
    void _OutputValue(const std::string& name, unsigned long value);
    void _OutputValue(const std::string& name, unsigned long long value);

    void _OutputValue(const std::string& name, double value, const std::string& formatString);

    void _OutputValueInPercent(const std::string& name, double value);
    void _OutputValueInSeconds(const std::string& name, double value);
    void _OutputValueMilliseconds(const std::string& name, double value);
    void _OutputValueInMilliseconds(const std::string& name, double value);
    void _OutputLatencyInMilliseconds(const std::string& name, double value);

    void _OutputCpuUtilization(const Results& results, const SystemInformation& system);
    void _OutputETW(struct ETWMask ETWMask, struct ETWEventCounters EtwEventCounters);
    void _OutputETWSessionInfo(struct ETWSessionInfo sessionInfo);
    void _OutputLatencyPercentiles(const Histogram<float>& readLatencyHistogram, const Histogram<float>& writeLatencyHistogram,
        const Histogram<float>& totalLatencyHistogram);
    void _OutputLatencyBuckets(const Histogram<float>& readLatencyHistogram, const Histogram<float>& writeLatencyHistogram,
        const Histogram<float>& totalLatencyHistogram, ConstHistogramBucketListPtr histogramBucketList, double fTestDurationInSeconds);
    void _OutputTargetResults(const TargetResults& results, bool fMeasureLatency, ConstHistogramBucketListPtr histogramBucketList,
        double fTestDurationInSeconds, bool fCalculateIopsStdDev, UINT32 ulIoBucketDurationInMilliseconds);
    void _OutputLatencySummary(const Histogram<float>& readLatencyHistogram, const Histogram<float>& writeLatencyHistogram,
        const Histogram<float>& totalLatencyHistogram, ConstHistogramBucketListPtr histogramBucketList, double fTestDurationInSeconds);
    void _OutputLatencySummary(const Histogram<float>& latencyHistogram, const std::string& latencyHistogramName);
    void _OutputTargetIops(const IoBucketizer& readBucketizer, const IoBucketizer& writeBucketizer, UINT32 bucketTimeInMs);
    void _OutputOverallIops(const Results& results, UINT32 bucketTimeInMs);
    void _OutputIops(const IoBucketizer& readBucketizer, const IoBucketizer& writeBucketizer, UINT32 bucketTimeInMs);

    std::string _sResult;
    std::string _sMillisecondsFormatString;
    std::string _sSecondsFormatString;
    std::string _sPercentFormatString;
};
