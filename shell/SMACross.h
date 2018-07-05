#ifndef SMA_CROSS_H
#define SMA_CROSS_H

#include <iostream>
#include "Framework.h"
#include "Config.h"
#include "Strategy.h"
#include "MA.h"
#include "Cross.h"
//#include "vld.h"

class SMACross : public Strategy
{
public:
    void onCreate()
    {
        registerParameter("Period",        PARAM_TYPE_INT,    &m_smaPeriod);
        registerParameter("StopLossPct",   PARAM_TYPE_DOUBLE, &m_stopLossPct);
        registerParameter("StopProfitPct", PARAM_TYPE_DOUBLE, &m_returnsPct);
        registerParameter("DrawdownPct",   PARAM_TYPE_DOUBLE, &m_drawdownPct);
    }

    void onStart()
    {
        m_sma.init(getBarSeries().getCloseDataSeries(), m_smaPeriod);
    }

    void onBar(const Bar& bar)
    {
//        std::cout << bar.getDateTime().toString() << std::endl;
//        std::cout << bar.getClose() << std::endl;

//        DateTime dt(2011, 2, 24, 13, 35);
//         if (dt <= bar.getDateTime()) {
//             std::cout << bar.getDateTime().toString() << ", ";
//             std::cout << bar.getClose() << ", " << m_sma[0] << std::endl;
//         }

        int posSize = getPositionSize();

        // If a position was not opened, check if we should enter a long position.
        if (Cross::crossAbove(getBarSeries().getCloseDataSeries(), m_sma)) {
            if (posSize < 0) {
                BuyToCovert(getMainInstrument(), -posSize, true);
                // Enter a buy market order for 10 shares. The order is good till canceled.
                int shares = (int)(getAvailableCash() * 0.7 / bar.getClose());
//                Buy(getDefaultInstrument(), 1, true);
            }

            if (posSize == 0) {
//                Buy(getDefaultInstrument(), 1, true);
            }
        }

        if (Cross::crossBelow(getBarSeries().getCloseDataSeries(), m_sma)) {
            if (posSize > 0) {
//                Sell(getDefaultInstrument(), posSize, true);
//                SellShort(getDefaultInstrument(), 1, true);
            }

            if (posSize == 0) {
                SellShort(getMainInstrument(), 1, true);
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

#endif // SMA_CROSS_H