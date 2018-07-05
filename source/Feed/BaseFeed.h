#ifndef BASE_FEED_H
#define BASE_FEED_H

#include <string>
#include <vector>
#include <map>
#include "Defines.h"
#include "Observer.h"
#include "DateTime.h"
#include "DataSeries.h"

namespace xBacktest
{

using std::map;

typedef struct _FeedValue
{
    DateTime datetime;
    void* data;

    _FeedValue() 
    {
        datetime.markInvalid();
        data = nullptr;
    }
} FeedValue;

// Base class for feeds.
// NOTE: This is a base class and should not be used directly.
// One `Feed` contains only one single data stream.
// data stream(series) should be updated when new values are available.
class BaseFeed : public Subject
{
public:
    BaseFeed(int maxLen);
	virtual ~BaseFeed();

    // Return True if this is a real-time feed.
    virtual bool isRealTime() = 0;

    // Subclass may implement this and update the given data series when new values are available.
    virtual void updateDataSeries(const DateTime& dateTime, const void* value);

    // Subclasses should implement this and return the next feed value.
    virtual bool getNextValue(FeedValue& outValue) = 0;

	// Returns the event that will be emitted when new values are available.
    Event& getNewValuesEvent();

	virtual bool dispatch();

protected:
    Event m_event;
    int m_maxLen;
};

////////////////////////////////////////////////////////////////////////////////
} // namespace xBacktest

#endif // BASE_FEED_H
