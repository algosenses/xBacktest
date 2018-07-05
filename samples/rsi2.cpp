#include <iostream>
#include <iomanip>
#include "Strategy.h"
#include "MA.h"
#include "RSI.h"
#include "Cross.h"
#include "Utils.h"

class RSI2 : public Strategy
{
public:
    void onCreate()
    {
        registerParameter("EntrySmaPeriod",      TypeInt,    &m_entrySmaPeriod);
        registerParameter("ExitSmaPeriod",       TypeInt,    &m_exitSmaPeriod);
        registerParameter("RsiPeriod",           TypeInt,    &m_rsiPeriod);
        registerParameter("OverBoughtThreshold", TypeInt,    &m_overBoughtThreshold);
        registerParameter("OverSoldThreshold",   TypeInt,    &m_overSoldThreshold);
        registerParameter("StopProfitPct",       TypeDouble, &m_stopProfitPct);
        registerParameter("DrawdownPct",         TypeDouble, &m_drawdownPct);
    }

    void onStart()
    {
        setUseAdjustedValues(true);
        DataSeries& ds = getBarSeries().getPriceDataSeries();
        m_entrySMA.init(ds, m_entrySmaPeriod);
        m_exitSMA.init(ds, m_exitSmaPeriod);
        m_rsi.init(ds, m_rsiPeriod);
    }

    void onPositionOpen(Position& position) 
    {
//        std::cout << "StrategyId: " << getId() << std::endl;
//        std::cout << "Position: " << position.getId() << ", " << position.getInstrument() << ", " << position.getShares() << std::endl;
    }

    void onPositionClosed(Position& position)
    {
//        std::cout << __FUNCTION__ << ":" << position.getId() << std::endl;
    }

    void onBars(const Bars& bars)
    {
        // Wait for enough bars to be available to calculate SMA and RSI.
        if (isnan(m_exitSMA[0]) || isnan(m_entrySMA[0]) || isnan(m_rsi[0])) {
            return;
        }

        int posSize = getPositionSize();

        if (posSize > 0) {
            if (exitLongSignal(bars.getBar())) {
                Sell(getDefaultInstrument(), posSize);
            }
        } else if (posSize < 0) {
            if (exitShortSignal(bars.getBar())) {
                BuyToCovert(getDefaultInstrument(), -posSize);
            }
        } else {
            if (enterLongSignal(bars.getBar())) {
                int shares = (int)(getCash() * 0.9 / bars.getBar().getClose(true));
                Buy(getDefaultInstrument(), 1, true);
            } else if (enterShortSignal(bars.getBar())) {
                int shares = (int)(getCash() * 0.9 / bars.getBar().getClose(true));
                SellShort(getDefaultInstrument(), 1, true);
//                std::cout << "enter short@  " << bars.getDateTime().toString() << ", shares: " << setprecision(10) << getCash() << ", " << bars.getBar().getClose(true) << std::endl;
            }
        }
    }

private:
    bool enterLongSignal(const Bar& bar)
    {
        return bar.getClose(true) > m_entrySMA[0] && m_rsi[0] <= m_overSoldThreshold;
    }

    bool exitLongSignal(const Bar& bar)
    {
        return Cross::crossAbove(getBarSeries().getPriceDataSeries(), m_exitSMA);
    }

    bool enterShortSignal(const Bar& bar)
    {
        return bar.getClose(true) < m_entrySMA[0] && m_rsi[0] >= m_overBoughtThreshold;
    }

    bool exitShortSignal(const Bar& bar)
    {
        return Cross::crossBelow(getBarSeries().getPriceDataSeries(), m_exitSMA);
    }

private:
    int m_entrySmaPeriod;
    int m_exitSmaPeriod;
    int m_rsiPeriod;
    int m_overBoughtThreshold;
    int m_overSoldThreshold;
    double m_stopProfitPct;
    double m_drawdownPct;

    SMA m_entrySMA;
    SMA m_exitSMA;
    RSI m_rsi;
};

//EXPORT_STRATEGY(RSI2)
