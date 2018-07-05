#include "Composer.h"
#include "BarFeed.h"
#include "Event.h"

namespace xBacktest
{

BarComposer::BarComposer()
    : INVALID_SLICE_IDX(-1)
{
    m_compositeMode = MANUAL;
    m_compositeAcrossDayBar = false;

    m_lastInputBarSec = 0;
    // output 1-minute bars by default.
    m_slicePeriod = 60;

    m_inputBarSeries = nullptr;
    m_outputBarSeries = nullptr;
}

void BarComposer::setCompositeParams(const TradingSession& session,
                                    Bar::Resolution inRes,
                                    Bar::Resolution outRes,
                                    int inInterval,
                                    int outInterval)
{
    m_compositeMode     = MANUAL;
    m_session           = session;
    m_inputBarRes       = inRes;
    m_outputBarRes      = outRes;
    m_inputBarInterval  = inInterval;
    m_outputBarInterval = outInterval;

    for (size_t i = 0; i < m_session.size(); i++) {
        covertPeriod(m_session[i].begin, m_session[i].end, m_session[i]);
    }

    computeSessionParams();

    if (outRes >= Bar::DAY) {
        m_compositeAcrossDayBar = true;
    }

    m_lastInputBarDT.markInvalid();
}

void BarComposer::compisteBarSeries(BarSeries& input, BarSeries& output)
{
    m_compositeMode   = BAR_SERIES;
    m_inputBarSeries  = &input;
    m_outputBarSeries = &output;

    m_inputBarSeries->getNewValueEvent().subscribe(this);

    return;
}

void BarComposer::covertPeriod(int open, int close, TradablePeriod& period)
{
    period.begin = (open / 10000) * 3600 + ((open % 10000)) / 100 * 60 + (open % 100);
    period.end = (close / 10000) * 3600 + ((close % 10000)) / 100 * 60 + (close % 100);
}

bool BarComposer::pushNewBar(const Bar& bar)
{
    if (m_inputBarRes >= m_outputBarRes) {
        return false;
    }

    if (m_compositeAcrossDayBar) {
        return compositeAcrossDayBar(bar.getDateTime(), bar);
    } else {
        return composite(bar.getDateTime(), bar);
    }
}

Bar& BarComposer::getCompositedBar()
{
    return m_compositedBar;
}

BarSeries* BarComposer::getInputBarSeries() const
{
    return m_inputBarSeries;
}

BarSeries* BarComposer::getOutputBarSeris() const
{
    return m_outputBarSeries;
}

void BarComposer::onEvent(int type, const DateTime& datetime, const void* context)
{
    if (type == Event::EvtDataSeriesNewValue) {
        Event::Context& ctx = *((Event::Context *)context);
        Bar& bar = *((Bar *)ctx.data);
        if (m_compositeAcrossDayBar) {
            compositeAcrossDayBar(datetime, bar);
        } else {
            composite(datetime, bar);
        }
    }
}

void BarComposer::computeSessionParams()
{
    m_tradingTotalSecs = 0;
    for (size_t i = 0; i < m_session.size(); i++) {
        m_tradingTotalSecs += (m_session[i].end - m_session[i].begin + 1);
    }

    m_sliceTotalNum = m_tradingTotalSecs / m_slicePeriod;
    m_currSliceIdx = INVALID_SLICE_IDX;
}

int BarComposer::getSliceIndex(int currSec)
{
    // index start from 0
    int idx = -1;
    int prevSliceCount = 0;

    for (size_t i = 0; i < m_session.size(); i++) {
        if (currSec >= m_session[i].begin && currSec < m_session[i].end) {
            idx = (currSec - m_session[i].begin) / m_slicePeriod;
            idx += prevSliceCount;
        }
        prevSliceCount += (m_session[i].end - m_session[i].begin) / m_slicePeriod;
    }

    return idx;
}

// Cut trading time into equalized slices, fill them with input bars according their timestamp.
// Once a slice is full, this slice is used to generate new bar.
bool BarComposer::composite(const DateTime& datetime, const Bar& bar)
{
    bool hasNewBar = false;

    if (m_inputBarRes >= m_outputBarRes) {
        return false;
    }

    int secs = datetime.secs() % (24 * 60 * 60);

    if (secs < m_lastInputBarSec) {
        return false;
    }

    assert(m_session.size() > 0);

    int sliceIdx = getSliceIndex(secs);
    if (sliceIdx < m_currSliceIdx || sliceIdx <= INVALID_SLICE_IDX) {
        return false;
    }

    double price = bar.getClose();

    if (m_currSliceIdx == INVALID_SLICE_IDX) {
        m_currSliceIdx = sliceIdx;
        if (m_inputBarRes == Bar::TICK) {
            m_lastOpen = price;
            m_lastHigh = price;
            m_lastLow = price;
            m_lastOpen = price;
            m_lastVolume = bar.getVolume();
            m_lastOpenInt = bar.getOpenInt();
        }
    } else {
        if (sliceIdx > m_currSliceIdx) {
            // push new bar into output bar series.
            Bar outBar(bar.getInstrument(),
                m_lastInputBarDT,
                m_lastOpen,
                m_lastHigh,
                m_lastLow,
                m_lastClose,
                m_lastVolume,
                m_lastOpenInt,
                m_outputBarRes);

            // fill tick fields with latest value
            if (m_inputBarRes == Bar::TICK) {
                outBar.setTickField(
                    bar.getLastPrice(),
                    bar.getBidPrice1(),
                    bar.getBidVolume1(),
                    bar.getAskPrice1(),
                    bar.getAskVolume1());
            }

            if (m_compositeMode == BAR_SERIES && m_outputBarSeries != nullptr) {
                m_outputBarSeries->appendWithDateTime(datetime, &outBar);
            }

            hasNewBar = true;
            m_compositedBar = outBar;

            m_lastOpen = 0;
            m_lastHigh = 0;
            m_lastLow = 0;
            m_lastClose = 0;
            m_lastVolume = 0;
            m_lastOpenInt = 0;

            if (m_inputBarRes == Bar::TICK) {
                m_lastOpen = price;
                m_lastHigh = price;
                m_lastLow = price;
                m_lastOpen = price;
                m_lastVolume = bar.getVolume();
                m_lastOpenInt = bar.getOpenInt();
            }

            m_currSliceIdx = sliceIdx;
        } else {
            if (price > m_lastHigh) {
                m_lastHigh = price;
            }

            if (price < m_lastLow) {
                m_lastLow = price;
            }
            m_lastClose = price;
            m_lastVolume = bar.getVolume();
            m_lastOpenInt = bar.getOpenInt();
        }
    }

    m_lastInputBarSec = secs;
    m_lastInputBarDT  = datetime;

    return hasNewBar;
}

bool BarComposer::compositeAcrossDayBar(const DateTime& dt, const Bar& bar)
{
    bool hasNewBar = false;

    if (!m_lastInputBarDT.isValid()) {
        m_lastInputBarDT = dt;
        m_lastDateTime = bar.getDateTime();
        m_lastOpen = bar.getOpen();
        m_lastHigh = bar.getHigh();
        m_lastLow = bar.getLow();
        m_lastClose = bar.getClose();
        m_lastVolume = bar.getVolume();
        m_lastOpenInt = bar.getOpenInt();

        Bar outBar(bar.getInstrument(),
            m_lastDateTime,
            m_lastOpen,
            m_lastHigh,
            m_lastLow,
            m_lastClose,
            m_lastVolume,
            m_lastOpenInt,
            m_outputBarRes);

        // We have a new bar, at least the open price is available at this point.
        // Other fields will be updated once this bar is closed.
        if (m_compositeMode == BAR_SERIES && m_outputBarSeries != nullptr) {
            m_outputBarSeries->appendWithDateTime(m_lastDateTime, &outBar);
        }

        return false;
    }

    bool across = false;
    if (m_outputBarRes == Bar::DAY) {
        if (m_lastInputBarDT.days() < dt.days()) {
            across = true;
        }
    } else if (m_outputBarRes == Bar::WEEK) {
        
    }

    if (!across) {
        if (bar.getHigh() > m_lastHigh) {
            m_lastHigh = bar.getHigh();
        }

        if (bar.getLow() < m_lastLow) {
            m_lastLow = bar.getLow();
        }

        m_lastClose = bar.getClose();
        m_lastVolume += bar.getVolume();
        m_lastOpenInt = bar.getOpenInt();

        if (m_compositeMode == BAR_SERIES && m_outputBarSeries != nullptr) {
            Bar& dest = (Bar&)m_outputBarSeries->at(0);
            dest.setHigh(m_lastHigh);
            dest.setLow(m_lastLow);
        }

    } else {
        Bar updatedBar(bar.getInstrument(),
            m_lastDateTime,
            m_lastOpen,
            m_lastHigh,
            m_lastLow,
            m_lastClose,
            m_lastVolume,
            m_lastOpenInt,
            m_outputBarRes);

        if (m_compositeMode == BAR_SERIES && m_outputBarSeries != nullptr) {
            // update all fields.
            Bar& dest = (Bar&)m_outputBarSeries->at(0);
            dest = updatedBar;
        }
       
        hasNewBar = true;
        m_compositedBar = updatedBar;

        m_lastDateTime = bar.getDateTime();
        m_lastOpen = bar.getOpen();
        m_lastHigh = bar.getHigh();
        m_lastLow = bar.getLow();
        m_lastClose = bar.getClose();
        m_lastVolume = bar.getVolume();
        m_lastOpenInt = bar.getOpenInt();

        Bar outBar(bar.getInstrument(),
            m_lastDateTime,
            m_lastOpen,
            m_lastHigh,
            m_lastLow,
            m_lastClose,
            m_lastVolume,
            m_lastOpenInt,
            m_outputBarRes);

        // Append new bar.
        if (m_compositeMode == BAR_SERIES && m_outputBarSeries != nullptr) {
            m_outputBarSeries->appendWithDateTime(m_lastDateTime, &outBar);
        }
    }

    m_lastInputBarDT = dt;

    return hasNewBar;
}

} // namespace xBacktest