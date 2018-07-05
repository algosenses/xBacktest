#ifndef MA_H
#define MA_H

#include "Technical.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
// This is the formula I'm using to calculate the averages based on previous ones.
// 1 2 3 4
// x x x
//   x x x
//
// avg0 = (a + b + c) / 3
// avg1 = (b + c + d) / 3
//
// avg0 = avg1 + x
// (a + b + c) / 3 = (b + c + d) / 3 + x
// a/3 + b/3 + c/3 = b/3 + c/3 + d/3 + x
// a/3 = d/3 + x
// x = a/3 - d/3
//
// avg1 = avg0 - x
// avg1 = avg0 + d/3 - a/3
class MAEventWindow : public EventWindow<double, double>
{
public:
	void init(int period);
	void onNewValue(const DateTime& datetime, const double& value);
	double getValue() const;

private:
	double m_value;
};

// Simple Moving Average filter.
class DllExport MA : public EventBasedFilter<double, double>
{
public:
    MA();
	// @param: dataSeries: The DataSeries instance being filtered.
	// @param: period: The number of values to use to calculate the SMA.
	// @param: maxLen: The maximum number of values to hold.
	// Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
	void init(DataSeries& dataSeries, int period, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
	MAEventWindow m_smaEventWnd;
};

////////////////////////////////////////////////////////////////////////////////
class EMAEventWindow : public EventWindow<double, double>
{
public:
    void init(int period);
    void onNewValue(const DateTime& datetime, const double& value);
    double getValue() const;

private:
    double m_multiplier;
    double m_value;
};

class DllExport EMA : public EventBasedFilter<double, double>
{
public:
    // Exponential Moving Average filter.
    // @param dataSeries: The DataSeries instance being filtered.
    // @param period: The number of values to use to calculate the EMA. Must be an integer greater than 1.
    // @param maxLen: The maximum number of values to hold.
    //  Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
    void init(DataSeries& dataSeries, int period, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    EMAEventWindow m_emaEventWnd;
};

////////////////////////////////////////////////////////////////////////////////
class SMAEventWindow : public EventWindow<double, double>
{
public:
    void init(int period, double weight);
    void onNewValue(const DateTime& datetime, const double& value);
    double getValue() const;

private:
    double m_weight;
    double m_multiplier;
    double m_value;
};

class DllExport SMA : public EventBasedFilter<double, double>
{
public:
    void init(DataSeries& dataSeries, int period, double weight = 1, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    SMAEventWindow m_smaEventWnd;
};

////////////////////////////////////////////////////////////////////////////////
class AMAEventWindow : public EventWindow<double, double>
{
public:
    void init(int effRatioLen, int fastAvgLen, int slowAvgLen);
    void onNewValue(const DateTime& datetime, const double& value);
    double getValue() const;

private:
    double m_value;
    int m_effRatioLen;
    int m_fastAvgLen;
    int m_slowAvgLen;

    // Scaled smoothing constant
    double m_fastSc;
    double m_slowSc;
};

// Kaufman Adaptive Moving Average filter.
class DllExport AMA : public EventBasedFilter<double, double>
{
public:
    AMA();
    // @param: dataSeries: The DataSeries instance being filtered.
    // @param: effRatioLen: The number of values to use to calculate the AMA.
    // @param: maxLen: The maximum number of values to hold.
    // Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
    void init(DataSeries& dataSeries, int effRatioLen, int fastAvgLen = 2, int slowAvgLen = 30, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    AMAEventWindow m_amaEventWnd;
};

} // namespace xBacktest

#endif // MA_H
