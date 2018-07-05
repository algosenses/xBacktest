#include <algorithm>
#include "Defines.h"
#include "BarFeed.h"
#include "BarSeries.h"

namespace xBacktest
{

BarFeed::BarFeed(int resolution, int maxLen)
{
	m_event.setType(Event::EvtNewBar);
    m_maxLen = maxLen;

    m_id = 0;
    m_dataStreamId = 0;
    m_instrument.clear();
    m_resolution = (Bar::Resolution)resolution;
    m_interval = 1;
}

void BarFeed::setId(int id)
{
    m_id = id;
}

int BarFeed::getId() const
{
    return m_id;
}

void BarFeed::setDataStreamId(int id)
{
    m_dataStreamId = id;
}

int BarFeed::getDataStreamId() const
{
    return m_dataStreamId;
}

void BarFeed::setContract(const Contract& contract)
{
    m_contract = contract;
}

const Contract& BarFeed::getContract() const
{
    return m_contract;
}

void BarFeed::setName(const string& name)
{
    m_name = name;
}

const string& BarFeed::getName() const
{
    return m_name;
}

const string& BarFeed::getInstrument() const
{
    return m_instrument;
}

void BarFeed::setInstrument(const string& instrument)
{
    m_instrument = instrument;
}

void BarFeed::emitNewBar(const Bar& bar)
{
    BarEventCtx ctx;
    ctx.dataStreamId = m_dataStreamId;
    ctx.barFeedId    = m_id;
    ctx.bar          = bar;
    m_event.emit(m_lastBar.getDateTime(), &ctx);
}

bool BarFeed::enableComposer(const TradingSession& session, Bar::Resolution res, int interval)
{
    m_barComposer.setCompositeParams(session, m_resolution, res, m_interval, interval);

    m_composerEnabled = true;

    return true;
}

bool BarFeed::dispatch()
{
    if (getNextBar(m_lastBar)) {

        m_lastBar.setResolution(getResolution());
        m_lastBar.setInterval(getInterval());

        if (!m_composerEnabled) {
            // Emit new bar directly.
            emitNewBar(m_lastBar);
        } else {
            // Ask composer to do composition.
            m_barComposer.pushNewBar(m_lastBar);
        }
        return true;
    }

    return false;
}

void BarFeed::setLength(int length)
{
    m_length = length;
}

int BarFeed::getLength() const
{
    return m_length;
}

void BarFeed::addTradablePeriod(const DateTime& begin, const DateTime& end)
{
    m_tradablePeriods.push_back({ begin, end });
}

const vector<BarFeed::TradablePeriod>& BarFeed::getTradablePeriods() const
{
    return m_tradablePeriods;
}

void BarFeed::setBeginDateTime(const DateTime& datetime)
{
    m_beginDateTime = datetime;
}

const DateTime& BarFeed::getBeginDateTime() const
{
    return m_beginDateTime;
}

void BarFeed::setEndDateTime(const DateTime& datetime)
{
    m_endDateTime = datetime;
}

const DateTime& BarFeed::getEndDateTime() const
{
    return m_endDateTime;
}

int BarFeed::getResolution() const
{
    return m_resolution;
}

void BarFeed::setResolution(Bar::Resolution resolution)
{
    m_resolution = resolution;
}

void BarFeed::setInterval(int interval)
{
    m_interval = interval;
}

int BarFeed::getInterval() const
{
    return m_interval;
}

Event& BarFeed::getNewBarEvent()
{
	return m_event;
}

const Bar& BarFeed::getCurrentBar()
{
    return m_lastBar;
}

Bar BarFeed::getLastBar() const
{
    return m_lastBar;
}

int BarFeed::loadData(
    int   reqId,
    const DataRequest& request,
    void* object,
    void (*callback)(const DateTime& datetime, void* context))
{
    return 0;
}

} // namespace xBacktest