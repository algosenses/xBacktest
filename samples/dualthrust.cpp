#include <iostream>

#include "Strategy.h"

class DualThrust : public Strategy
{
public:
    void onCreate()
    {
        registerParameter("Ks", TypeDouble, &m_Ks);
        registerParameter("Kx", TypeDouble, &m_Kx);
        registerParameter("PastNDays", TypeInt, &m_pastNDays);
        m_range = 0;
        m_tradeAllowed = false;
    }

    void onStart()
    {
    }

    void onData(const Bar& bar, bool isCompleted)
    {
//        std::cout << bar.getDateTime().toString() << std::endl;
        m_historicalBars.push_back(bar);
    }

    void onPositionOpen(Position& position)
    {
        cout << position.getEntryDateTime().toString() << ": ";
        if (position.getShares() > 0) {
            cout << "EntryLong\t\t";
        } else {
            cout << "EntryShort\t\t";
        }

        position.setStopLossPct(0.05);
        position.setStopProfitPct(0.01, 0.5);

        if (position.getShares() > 0) {
            m_entryLongCount++;
        } else {
            m_entryShortCount++;
        }
    }

    void onPositionClosed(Position& position)
    {
        cout << position.getExitDateTime().toString() << ": ";
        if (position.getEntryOrder().isBuy()) {
            cout << "ExitLong" << endl;
        } else {
            cout << "ExitShort" << endl;
        }
    }

    void onBars(const Bars& bars)
    {
        DateTime currDate(bars.getDateTime().date());
        if (m_lastDate != currDate) {
            m_tradeAllowed = false;
            m_openPrice = bars.getBar().getOpen();

            DataRequest request;
            request.type = BarsBack;
            request.symbol = getDefaultInstrument();
            request.resolution = BaseBar::DAY;
            request.to = currDate;
            request.count = m_pastNDays;

            m_historicalBars.clear();

            if (request.count == loadHistoricalData(request)) {
                if (calcParameters()) {
                    m_tradeAllowed = true;
                }
            }
            m_lastDate = currDate;

            m_entryLongCount = 0;
            m_entryShortCount = 0;
        }

        if (!m_tradeAllowed) {
            return;
        }

        int mktPos = getPositionSize();
        double high = bars.getBar().getHigh();
        double low  = bars.getBar().getLow();
        double open = bars.getBar().getOpen();

        if (high > m_Ks * m_range + m_openPrice) {
            double entryPrx = m_Ks * m_range + m_openPrice;
            if (open > m_Ks * m_range + m_openPrice) {
                entryPrx = open;
            }

            int shares = getCash() / entryPrx / 300;

            if (getPositionSize() == 0 && m_entryLongCount == 0) {
                BuyLimit(getDefaultInstrument(), entryPrx, 1);
            }
        } else if (low < m_openPrice - m_Kx * m_range) {
            double entryPrx = m_openPrice - m_Kx * m_range;
            if (open < m_openPrice - m_Kx * m_range) {
                entryPrx = open;
            }

            int shares = getCash() / entryPrx / 300;

            if (getPositionSize() == 0 && m_entryShortCount == 0) {
                SellShortLimit(getDefaultInstrument(), entryPrx, 1);
            }
        }

        if (getPositionSize() != 0) {
            long secs = bars[getDefaultInstrument()].getDateTime().secs() % (3600 * 24);
            if (secs >= 15 * 3600 + 14 * 60) {
                getCurrentPosition().close();
            }
        }
    }

private:
    bool calcParameters()
    {
        if (m_pastNDays <= 0) {
            return false;
        }

        if (m_historicalBars.size() != m_pastNDays) {
            return false;
        }

        double HH = m_historicalBars[0].getHigh();
        double HC = m_historicalBars[0].getClose();
        double LC = m_historicalBars[0].getClose();
        double LL = m_historicalBars[0].getLow();

        for (size_t i = 0; i < m_historicalBars.size(); i++) {
            if (m_historicalBars[i].getHigh() > HH) {
                HH = m_historicalBars[i].getHigh();
            }

            if (m_historicalBars[i].getLow() < LL) {
                LL = m_historicalBars[i].getLow();
            }

            if (m_historicalBars[i].getClose() > HC) {
                HC = m_historicalBars[i].getClose();
            }

            if (m_historicalBars[i].getClose() < LC) {
                LC = m_historicalBars[i].getClose();
            }
        }

        m_range = max((HH - LC), (HC - LL));

        return true;
    }

private:
    double m_Ks;
    double m_Kx;
    int m_pastNDays;
    DateTime m_lastDate;
    double m_range;
    double m_openPrice;
    bool m_tradeAllowed;
    vector<Bar> m_historicalBars;
    int m_entryLongCount;
    int m_entryShortCount;
};

//EXPORT_STRATEGY(DualThrust)
