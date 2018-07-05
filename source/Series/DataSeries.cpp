#include "DataSeries.h"

namespace xBacktest
{

unsigned int DataSeries::DEFAULT_MAX_LEN = 1024 * 2;

DataSeries::DataSeries()
{
    m_type = DSTypeUnknown;
}

int DataSeries::getType() const
{
    return m_type;
}

void DataSeries::setType(int type)
{
    m_type = type;
}

DataSeries::~DataSeries()
{
}

template DllExport class SequenceDataSeries<int>;
template DllExport class SequenceDataSeries<double>;
template DllExport class SequenceDataSeries<long long>;

} // namespace xBacktest
