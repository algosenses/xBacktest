#include <iostream>
#include "MA.h"
#include "Cross.h"
#include "Strategy.h"
#include "Utils.h"

class MACrossDelay : public Strategy
{
    void onCreate()
    {
        registerParameter("MA1Period", TypeInt, &m_ma1Period);
        registerParameter("MA2Period", TypeInt, &m_ma2Period);
        registerParameter("Delay",     TypeInt, &m_delay);
    }

    void onStart()
    {
        m_sma1.init(getBarSeries().getOpenDataSeries(), m_ma1Period);
        m_sma2.init(getBarSeries().getOpenDataSeries(), m_ma2Period);
    }

    void onPositionOpen(Position& position)
    {
#if 0
        cout << position.getEntryDateTime().toString() << ": ";
        if (position.getShares() > 0) {
            cout << "EntryLong\t\t";
        } else {
            cout << "EntryShort\t\t";
        }
#endif
        position.setStopLossPct(0.006);
        position.setStopProfitPct(0.02, 0.7);
    }

    void onPositionClosed(Position& position)
    {
#if 0
        cout << position.getExitDateTime().toString() << ": ";
        if (position.getEntryOrder().isBuy()) {
            cout << "ExitLong" << endl;
        } else {
            cout << "ExitShort" << endl;
        }
#endif
    }

    void onBars(const Bars& bars)
    {
        if (isnan(m_sma1[0])) {
            return;
        }
        
        if (m_sma2.length() < m_delay + 1 || isnan(m_sma2[m_delay])) {
            return;
        }

        m_ma1Series.append(m_sma1[0]);
        m_ma2Series.append(m_sma2[m_delay]);

        if (Cross::crossAbove(m_ma1Series, m_ma2Series)) {
            if (getPositionSize() < 0) {
                getCurrentPosition().close();
            }

            if (getPositionSize() == 0) {
                int shares = getCash() / bars.getBar().getOpen();
                BuyLimit(getDefaultInstrument(), bars.getBar().getOpen(), shares);
            }
        } else if (Cross::crossBelow(m_ma1Series, m_ma2Series)) {
            if (getPositionSize() > 0) {
                getCurrentPosition().close();
            }

            if (getPositionSize() == 0) {
                int shares = getCash() / bars.getBar().getOpen();
                SellShortLimit(getDefaultInstrument(), bars.getBar().getOpen(), shares);
            }
        }
    }

private:
    int m_ma1Period;
    SMA m_sma1;
    int m_ma2Period;
    SMA m_sma2;
    int m_delay;

    SequenceDataSeries<double> m_ma1Series;
    SequenceDataSeries<double> m_ma2Series;
};

//EXPORT_STRATEGY(MACrossDelay)
