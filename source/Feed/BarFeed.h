#ifndef BAR_FEED_H
#define BAR_FEED_H

#include <vector>
#include "BaseFeed.h"
#include "Bar.h"
#include "DataSeries.h"
#include "BarSeries.h"
#include "Composer.h"

namespace xBacktest
{

// Subclasses should implement:
// - getNextBar
// - Remaining Subject methods
//
// THIS IS A VERY BASIC CLASS AND IT WON'T DO ANY VERIFICATIONS OVER THE BAR RETURNED.

// Note: This is a base class and should not be used directly.
class BarFeed : public Subject
{
public:
    typedef struct {
        int dataStreamId;
        int barFeedId;
        Bar bar;
    } BarEventCtx;

    typedef struct {
        int   reqId;
        int   dataStreamId;
        int   barFeedId;
        void* object;
        Bar   bar;
    } HistoricalDataContext;

    typedef struct {
        DateTime begin;
        DateTime end;
    } TradablePeriod;

    typedef enum {

    } Category;

	// This is a base class and should not be used directly.
	BarFeed(int resolution, int maxLen = DataSeries::DEFAULT_MAX_LEN);

    virtual ~BarFeed() {}
    virtual BarFeed* clone()  { return nullptr; }
    virtual BarFeed* create() { return nullptr; }

    void setId(int id);

    int getId() const;

    void setDataStreamId(int id);

    int  getDataStreamId() const;

    void setContract(const Contract& contract);

    const Contract& getContract() const;

    void setName(const string& name);

    const string& getName() const;

    virtual bool reset() = 0;

    // Subclasses should implement this and return a Bar or None if there are no bars.
    // This is for BarFeed subclasses and it should not be called directly.
    // Override to return the next Bar in the feed or None if there are no bars.
    virtual bool getNextBar(Bar& outBar) = 0;

    void setLength(int length);

    int  getLength() const;

    void addTradablePeriod(const DateTime& begin, const DateTime& end);

    const vector<TradablePeriod>& getTradablePeriods() const;

    void setBeginDateTime(const DateTime& datetime);

    const DateTime& getBeginDateTime() const;

    void setEndDateTime(const DateTime& datetime);

    const DateTime& getEndDateTime() const;

    int  getResolution() const;

    void setResolution(Bar::Resolution resolution);

    void setInterval(int interval);
    int  getInterval() const;

    // Returns the current bar.
    const Bar& getCurrentBar();

	// Returns the last Bar or None
	Bar getLastBar() const;

	Event& getNewBarEvent();

    // Returns the instrument.
    const string& getInstrument() const;

    void setInstrument(const string& instrument);

    // Returns the Bar DataSeries.
    void getDataSeries();

    bool dispatch();

    void emitNewBar(const Bar& bar);

    bool enableComposer(const TradingSession& session, Bar::Resolution res, int interval = 1);

    // Return a array of feed values in the specified date time range.
    virtual int loadData(
        int   reqId,
        const DataRequest& request,
        void* object,
        void (*callback)(const DateTime& datetime, void* context));

private:
    Event m_event;
    int   m_maxLen;

    int      m_id;    
    
    // Data stream which this bar feed located in.
    int      m_dataStreamId;

    // Name of this bar feed.
    string   m_name;
    // Instrument contained in this bar feed.
    string   m_instrument;
    Bar::Resolution m_resolution;
    int      m_interval;
    
    Contract m_contract;

    vector<TradablePeriod> m_tradablePeriods;

    DateTime m_beginDateTime;
    DateTime m_endDateTime;
    DateTime m_prevDateTime;
    DateTime m_currDateTime;
    Bar      m_lastBar;
    int      m_length;

    bool     m_composerEnabled;
    BarComposer m_barComposer;
};

} // namespace xBacktest

#endif // BAR_FEED_H