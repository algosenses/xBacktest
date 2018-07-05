#include <iostream>
#include "Strategy.h"
#include "MA.h"
#include "Cross.h"

class SMACross : public Strategy
{
public:
    void onCreate()
    {
        registerParameter("Period",         PARAM_TYPE_INT,    &m_smaPeriod);
        registerParameter("StopLossPct",    PARAM_TYPE_DOUBLE, &m_stopLossPct);
        registerParameter("StopProfitPct",  PARAM_TYPE_DOUBLE, &m_returnsPct);
        registerParameter("DrawdownPct",    PARAM_TYPE_DOUBLE, &m_drawdownPct);
    }

    void onStart()
    {
        // We'll use adjusted close values instead of regular close values.
        setUseAdjustedValues(true);
        m_sma.init(getBarSeries().getAdjCloseDataSeries(), m_smaPeriod);
    }

    void onPositionOpen(Position& position) 
    {
//        position.setStopLossPct(m_stopLossPct);
//        position.setStopProfitPct(m_returnsPct, m_drawdownPct);
    }

    void onBar(const Bar& bar)
    {   
        int posSize = getPositionSize();
        if (posSize == 0) {
            // If a position was not opened, check if we should enter a long position.
            if (Cross::crossAbove(getBarSeries().getAdjCloseDataSeries(), m_sma)) {
                // Enter a buy market order for 10 shares. The order is good till canceled.
                int shares = (int)(getCash() * 0.7 / bar.getClose(true));
                Buy(getDefaultInstrument(), 10, true);
            }
        } else if (Cross::crossBelow(getBarSeries().getAdjCloseDataSeries(), m_sma)) {
            if (posSize > 0) {
                Sell(getDefaultInstrument(), posSize, true);
            }
        }
    }

private:
    SMA m_sma;
    int m_smaPeriod;
    double m_stopLossPct;
    double m_returnsPct;
    double m_drawdownPct;
};

EXPORT_STRATEGY(SMACross)
