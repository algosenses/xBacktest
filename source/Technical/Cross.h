#ifndef XBACKTEST_CROSS_H
#define XBACKTEST_CROSS_H

#include "BarSeries.h"

namespace xBacktest
{

class DllExport Cross
{
public:
    // Checks for a cross above conditions over the specified period between two DataSeries objects.
    // It returns the number of times values1 crossed above values2 during the given period.
    // @param values1: The DataSeries that crosses.
    // @param values2: The DataSeries being crossed.
    // @param start: The start of the range.
    // @param end: The end of the range.
    // NOTE:
    // The default start and end values check for cross above conditions over the last 2 values.
    static int crossAbove(const DataSeries& values1, const DataSeries& values2, int start = 0, int end = 1);

    // Checks for a cross below conditions over the specified period between two DataSeries objects.
    // It returns the number of times values1 crossed below values2 during the given period.
    // @param values1: The DataSeries that crosses.
    // @param values2: The DataSeries being crossed.
    // @param start: The start of the range.
    // @param end: The end of the range.
    // NOTE:
    // The default start and end values check for cross below conditions over the last 2 values.
    static int crossBelow(const DataSeries& values1, const DataSeries& values2, int start = 0, int end = 1);

};

} // namespace xBacktest

#endif // XBACKTEST_CROSS_H