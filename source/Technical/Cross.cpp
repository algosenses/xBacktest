#include <cmath>
#include "Cross.h"

namespace xBacktest
{

// The following definitions are from <<The art of computer programming>> by Knuth.
static bool approximatelyEqual(float a, float b, float epsilon)
{
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

static bool essentiallyEqual(float a, float b, float epsilon)
{
    return fabs(a - b) <= ((fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

static bool definitelyGreaterThan(float a, float b, float epsilon)
{
    return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

static bool definitelyLessThan(float a, float b, float epsilon)
{
    return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

template<typename T>
static int crossImpl(const DataSeries& values1, const DataSeries& values2, int start, int end, bool signCheck)
{
    assert(start >= 0 && end >= 0 && end >= start);

    if (values1.length() < end + 1 || values2.length() < end + 1) {
        return 0;
    }

    const SequenceDataSeries<T>& v1 = static_cast<const SequenceDataSeries<T>&>(values1);
    const SequenceDataSeries<T>& v2 = static_cast<const SequenceDataSeries<T>&>(values2);

#if 1
    // Performance optimization code.
    int count = 0;
    T diffs[3];
    diffs[0] = 0;
    diffs[1] = 0;
    diffs[2] = 0;
        
    // Latest value placed at the front of data series.
    // So values[0] is the newest one.
    for (int i = start; i <= end; i++) {
#ifdef _MSC_VER
        if (_isnan(v1[i]) || _isnan(v2[i])) {
#else
        if (std::isnan(v1[i]) || std::isnan(v2[i])) {
#endif
            return 0;
        }

        if (fabs(double(v1[i] - v2[i])) < 0.00000001) {
            diffs[count++] = 0;
        } else if (v1[i] > v2[i]) {
            diffs[count++] = 1;
        } else {
            diffs[count++] = -1;
        }
    }

    int ret = 0;
    if (count < 2) {
        return 0;
    }

    if (count == 2 && diffs[1] == 0) {
        if (fabs(double(v1[end+1] - v2[end+1])) < 0.00000001) {
            diffs[count++] = 0;
        } else if (v1[end+1] > v2[end+1]) {
            diffs[count++] = 1;
        } else {
            diffs[count++] = -1;
        }
    }

    // Cross above
    if (signCheck) {
        if (diffs[0] > 0) {
            if (diffs[1] < 0) {
                ret += 1;
            } else if (diffs[1] == 0) {
                if (diffs[2] < 0) {
                    ret += 1;
                }
            }
        }
    } else {
        if (diffs[0] < 0) {
            if (diffs[1] > 0) {
                ret += 1;
            } else if (diffs[1] == 0) {
                if (diffs[2] > 0) {
                    ret += 1;
                }
            }
        }
    }
#else
    vector<T> diffs;

    // Latest value placed at the front of data series.
    // So values[0] is the newest one.
    for (int i = start; i <= end; i++) {
        if (_isnan(v1[i]) || _isnan(v2[i])) {
            return 0;
        }

        if (fabs(v1[i] - v2[i]) < 0.00000001) {
            diffs.push_back(0);
        } else if (v1[i] > v2[i]) {
            diffs.push_back(1);
        } else {
            diffs.push_back(-1);
        }
    }

    int ret = 0;
    size_t size = diffs.size();
    if (size < 2) {
        return 0;
    }

    if (size == 2 && diffs[1] == 0) {
        if (fabs(v1[end+1] - v2[end+1]) < 0.00000001) {
            diffs.push_back(0);
        } else if (v1[end+1] > v2[end+1]) {
            diffs.push_back(1);
        } else {
            diffs.push_back(-1);
        }
    }

    int idx = 0;
    size = diffs.size();
    for (size_t i = 0; i < (size - 1) && idx < (size - 1); i++) {
        // Cross above
        if (signCheck) {
            if (diffs[idx] > 0) {
                if (diffs[idx + 1] < 0) {
                    ret += 1;
                    idx += 1;
                } else if (diffs[idx + 1] == 0) {
                    if (idx < size - 2) {
                        if (diffs[idx + 2] < 0) {
                            ret += 1;
                            idx += 2;
                        }
                    }
                }
            }
        } else {
            if (diffs[idx] < 0) {
                if (diffs[idx + 1] > 0) {
                    ret += 1;
                    idx += 1;
                } else if (diffs[idx + 1] == 0) {
                    if (idx < size - 2) {
                        if (diffs[idx + 2] > 0) {
                            ret += 1;
                            idx += 2;
                        }
                    }
                }
            }
        }
    }
#endif

    return ret;
}

int Cross::crossAbove(const DataSeries& values1, const DataSeries& values2, int start, int end)
{
    if (values1.getType() != values2.getType()) {
        return 0;
    }

    switch (values1.getType()) {
    case DataSeries::DSTypeInt:
        return crossImpl<int>(values1, values2, start, end, true);
        break;
    case DataSeries::DSTypeFloat:
        return crossImpl<double>(values1, values2, start, end, true);
        break;
    case DataSeries::DSTypeInt64:
        return crossImpl<long long>(values1, values2, start, end, true);
        break;

    default:
        return 0;
        break;
    }
}

int Cross::crossBelow(const DataSeries& values1, const DataSeries& values2, int start, int end)
{
    if (values1.getType() != values2.getType()) {
        return 0;
    }

    switch (values1.getType()) {
    case DataSeries::DSTypeInt:
        return crossImpl<int>(values1, values2, start, end, false);
        break;
    case DataSeries::DSTypeFloat:
        return crossImpl<double>(values1, values2, start, end, false);
        break;
    case DataSeries::DSTypeInt64:
        return crossImpl<long long>(values1, values2, start, end, false);
        break;

    default:
        return 0;
        break;
    }
}

} // namespace xBacktest