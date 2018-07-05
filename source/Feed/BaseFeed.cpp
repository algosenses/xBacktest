#include <iostream>
#include "Bar.h"
#include "BaseFeed.h"

namespace xBacktest
{
////////////////////////////////////////////////////////////////////////////////
BaseFeed::BaseFeed(int maxLen)
{
    m_maxLen = maxLen;
}

BaseFeed::~BaseFeed()
{

}

void BaseFeed::updateDataSeries(const DateTime& dateTime, const void* value)
{

}

bool BaseFeed::dispatch()
{
    bool ret = false;
    FeedValue value;
    if (getNextValue(value)) {
        updateDataSeries(value.datetime, value.data);
        m_event.emit(value.datetime, value.data);
        return true;
    }

    return false;
}

Event& BaseFeed::getNewValuesEvent()
{
	return m_event;
}

} // namespace xBacktest
////////////////////////////////////////////////////////////////////////////////
