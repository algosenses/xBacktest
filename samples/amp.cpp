#include <iostream>
#include "Strategy.h"
#include "Position.h"

class AMP : public Strategy
{
public:
    void onCreate()
    {
        m_range = 0;
        m_daytrade = false;
        m_lastDate.markInvalid();

        registerParameter("upper", PARAM_TYPE_DOUBLE, &m_upperRatio);
        registerParameter("lower", PARAM_TYPE_DOUBLE, &m_lowerRatio);
    }

    void onPositionOpen(Position& position) 
    {
//        cout << position.getEntryDateTime().toString() << ": ";
        if (position.getShares() > 0) {
//            cout << "EntryLong\t\t";
        } else {
//            cout << "EntryShort\t\t";
        }

        position.setStopLossPct(0.006);
        position.setStopProfitPct(0.015, 0.6);
        
        if (position.getShares() > 0) {
            m_entryLongCount++;
        } else {
            m_entryShortCount++;
        }
    }

    void onPositionClosed(Position& position) 
    {
//        cout << position.getExitDateTime().toString() << ": ";
        if (position.getEntryOrder().isBuy()) {
//            cout << "ExitLong" << endl;
        } else {
//            cout << "ExitShort" << endl;
        }
    }

    void onBar(const Bar& bar)
    {
        DateTime currDate(bar.getDateTime().date());
        if (!m_lastDate.isValid()) {
            m_lastDate = currDate;
            double open = bar.getOpen();
            m_lastDayOpenPrice = open;
            m_todayOpenPrice   = open;
            m_entryLongCount = 0;
            m_entryShortCount = 0;
        }

        if (m_lastDate == currDate) {
            if (m_highPrxSeries.length() == 0) {
                m_lastDayOpenPrice = bar.getOpen();
            }
            m_highPrxSeries.append(bar.getHigh());
            m_lowPrxSeries.append(bar.getLow());
        } else {
            m_range = m_highPrxSeries.maximum() - m_lowPrxSeries.minimum();

            m_daytrade = true;
            m_highPrxSeries.clear();
            m_lowPrxSeries.clear();

            // Save the first bar of this day
            m_highPrxSeries.append(bar.getHigh());
            m_lowPrxSeries.append(bar.getLow());
            
            m_lastDate = currDate;

            m_todayOpenPrice = bar.getOpen();
            m_entryLongCount = 0;
            m_entryShortCount = 0;

            if (m_range / m_lastDayOpenPrice <= 0.01) {
                m_range = m_lastDayOpenPrice * 0.01;
            } else if (m_range / m_lastDayOpenPrice >= 0.04) {
                m_range = m_lastDayOpenPrice * 0.04;
            }


            m_upperBand = m_todayOpenPrice + m_range * m_upperRatio;
            m_lowerBand = m_todayOpenPrice - m_range * m_lowerRatio;
        }

        if (m_daytrade) {
            double open = bar.getOpen();
            double high = bar.getHigh();
            double low  = bar.getLow();

            if (high > m_upperBand) {
                double entryPx = m_upperBand;
                if (open > m_upperBand) {
                    entryPx = open;
                }

                int shares = getCash() / entryPx;

                // Enter long
                if (getPositionSize() == 0 && m_entryLongCount == 0) {
//                    cout << "Buy...@ " << bars[getDefaultInstrument()].getDateTime().toString() << endl;
                    BuyLimit(getDefaultInstrument(), entryPx, shares);
                }

            } else if (low < m_lowerBand) {
                double entryPrx = m_lowerBand;
                if (open < m_lowerBand) {
                    entryPrx = open;
                }

                int shares = getCash() / entryPrx;

                // Enter short
                if (getPositionSize() == 0 && m_entryShortCount == 0) {
//                    cout << "SellShort@ " << entryPrx << "@" << bars[getDefaultInstrument()].getDateTime().toString() << endl;
                    SellShortLimit(getDefaultInstrument(), entryPrx, shares);
                }
            }

            if (getPositionSize() != 0) {
                long secs = bar.getDateTime().secs() % (3600 * 24);
                if (secs >= 14 * 3600 + 59 * 60) {
                    getCurrentPosition().close();
                }
            }
        }
    }

private:
    DateTime m_lastDate;
    double m_range;
    bool m_daytrade;
    double m_lastDayOpenPrice;
    double m_todayOpenPrice;
    double m_upperRatio;
    double m_lowerRatio;
    double m_upperBand;
    double m_lowerBand;
    int m_entryLongCount;
    int m_entryShortCount;
    SequenceDataSeries<double> m_highPrxSeries;
    SequenceDataSeries<double> m_lowPrxSeries;
};

//EXPORT_STRATEGY(AMP)
