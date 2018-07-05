#ifndef XBACKTEST_COMPOSER_H
#define XBACKTEST_COMPOSER_H

#include "BarSeries.h"

namespace xBacktest
{

class BarComposer : public IEventHandler
{
public:
    BarComposer();
    void setCompositeParams(const TradingSession& session, 
                            Bar::Resolution inRes, 
                            Bar::Resolution outRes, 
                            int inInterval = 1, 
                            int outInterval = 1);
    void compisteBarSeries(BarSeries& input, BarSeries& output);
    bool pushNewBar(const Bar& bar);
    Bar& getCompositedBar();

    BarSeries* getInputBarSeries() const;
    BarSeries* getOutputBarSeris() const;

    void onEvent(int             type,
                 const DateTime& datetime,
                 const void *    context);

private:
    void covertPeriod(int open, int close, TradablePeriod& period);
    int  getSliceIndex(int currSecond);
    void computeSessionParams();
    bool composite(const DateTime& datetime, const Bar& bar);
    bool compositeAcrossDayBar(const DateTime& dt, const Bar& bar);

private:
    enum {
        MANUAL,
        BAR_SERIES,
    } CompositeMode;

    const int INVALID_SLICE_IDX = -1;

    int  m_compositeMode;
    bool m_compositeAcrossDayBar;

    TradingSession m_session;
    int m_tradingTotalSecs;

    int m_inputBarRes;
    int m_inputBarInterval;
    BarSeries* m_inputBarSeries;

    int m_outputBarRes;
    int m_outputBarInterval;
    BarSeries* m_outputBarSeries;

    int m_slicePeriod;     // in seconds
    int m_sliceTotalNum;
    int m_currSliceIdx;

    DateTime  m_lastInputBarDT;
    int       m_lastInputBarSec;
    DateTime  m_lastDateTime;
    double    m_lastOpen;
    double    m_lastHigh;
    double    m_lastLow;
    double    m_lastClose;
    long long m_lastVolume;
    long long m_lastOpenInt;
    
    Bar m_compositedBar;
};

} // xBacktest

#endif // XBACKTEST_COMPOSER_H