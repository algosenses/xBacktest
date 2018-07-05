#include <iostream>
#include <iomanip>
#include "Strategy.h"
#include "MA.h"
#include "Cross.h"
#include "HighLow.h"

class STFT : public Strategy
{
public:
    void onCreate()
    {
        registerParameter("LongMALength",     PARAM_TYPE_INT,    &m_longMALength);
        registerParameter("ShortMALength",    PARAM_TYPE_INT,    &m_shortMALength);
        registerParameter("SlidingWndLength", PARAM_TYPE_INT,    &m_slidingWndLength);
        registerParameter("UpperBandRatio",   PARAM_TYPE_DOUBLE, &m_upperBandRatio);
        registerParameter("LowerBandRatio",   PARAM_TYPE_DOUBLE, &m_lowerBandRatio);
        registerParameter("StopLossPct",      PARAM_TYPE_DOUBLE, &m_stopLossPct);
        registerParameter("StopProfitPct",    PARAM_TYPE_DOUBLE, &m_stopProfitRtns);
        registerParameter("DrawdownPct",      PARAM_TYPE_DOUBLE, &m_stopProfitDrawdown);
    }

    void onStart()
    {
         m_shortMA.init(getBarSeries().getCloseDataSeries(), m_shortMALength);
         m_longMA.init(getBarSeries().getCloseDataSeries(), m_longMALength);
         m_maxPrice.init(m_longMA, m_slidingWndLength);
         m_minPrice.init(m_longMA,  m_slidingWndLength);

         m_lastCrossState = 0;
         m_lastCrossLine = 0;
    }

    void onPositionOpen(Position& position)
    {
        position.setStopLossPct(m_stopLossPct);
        position.setStopProfitPct(m_stopProfitRtns, m_stopProfitDrawdown);
    }

    void onBar(const Bar& bar)
    {
        if (isnan(m_longMA[0]) || isnan(m_shortMA[0]) || isnan(m_maxPrice[0]) || isnan(m_minPrice[0])) {
            return;
        }

        m_upperBandPrice.append(m_maxPrice[0] * (1 - m_upperBandRatio));
        m_lowerBandPrice.append(m_minPrice[0] * (1 + m_lowerBandRatio));

        int pos = getPositionSize();
        int lots = 10;// = (getCash() * 0.5) / bar.getClose();

        if (Cross::crossAbove(m_shortMA, m_maxPrice)) {
            if (pos < 0) {
                getCurrentPosition().close();
                Buy(getDefaultInstrument(), lots);
            } else if (pos == 0) {
                Buy(getDefaultInstrument(), lots);
            }

            std::cout << "enterLong" << std::endl;

            m_lastCrossLine = LineMax;
            m_lastCrossState = Cross_Above;
            output(bar);
        }

        if (Cross::crossBelow(m_shortMA, m_minPrice)) {
            if (pos > 0) {
                getCurrentPosition().close();
                SellShort(getDefaultInstrument(), lots);
            } else if (pos == 0) {
                SellShort(getDefaultInstrument(), lots);
            }

            std::cout << "enterShort" << std::endl;
            m_lastCrossLine = LineMin;
            m_lastCrossState = Cross_Below;
            output(bar);
        } 

        if (Cross::crossBelow(m_shortMA, m_upperBandPrice)) {
            if (m_lastCrossLine == LineMax && m_lastCrossState == Cross_Below) {
                if (pos > 0) {
                    getCurrentPosition().close();
                    SellShort(getDefaultInstrument(), lots);
                } else if (pos == 0) {
                    SellShort(getDefaultInstrument(), lots);
                }
                std::cout << "enterShort" << std::endl;
            }
            m_lastCrossLine = LineUpperBand;
            m_lastCrossState = Cross_Below;
            output(bar);
        }

        if (Cross::crossAbove(m_shortMA, m_lowerBandPrice)) {
            if (m_lastCrossLine == LineMin && m_lastCrossState == Cross_Above) {
                if (pos < 0) {
                    getCurrentPosition().close();
                    Buy(getDefaultInstrument(), lots);
                } else if (pos == 0) {
                    Buy(getDefaultInstrument(), lots);
                }
                std::cout << "enterLong" << std::endl;
            }
            m_lastCrossLine = LineLowerBand;
            m_lastCrossState = Cross_Above;
            output(bar);
        } 

        if (Cross::crossBelow(m_shortMA, m_maxPrice)) {
            m_lastCrossLine = LineMax;
            m_lastCrossState = Cross_Below;
        } 

        if (Cross::crossAbove(m_shortMA, m_minPrice)) {
            m_lastCrossLine = LineMin;
            m_lastCrossState = Cross_Above;
        }
    }

private:
    void output(const Bar& bar)
    {
        std::cout << bar.getDateTime().toString() << ",";
        std::cout << bar.getOpen() << "," << bar.getHigh() << "," << bar.getLow() << "," << bar.getClose() << std::endl;
        std::cout << std::fixed << std::setprecision(2) << "MA" << m_shortMALength << ":" << m_shortMA[0] << ", MAmax:" << m_maxPrice[0] << ", MAmin:" << m_minPrice[0];
        std::cout << ", upb:" << m_upperBandPrice[0] << ", downb:" << m_lowerBandPrice[0] << std::endl;
        std::cout << "STATE: ";
        switch (m_lastCrossState) {
        case Cross_Above:
            std::cout << "Cross_Above";
            break;
        case Cross_Below:
            std::cout << "Cross_Below";
            break;
        }

        std::cout << " ";

        switch (m_lastCrossLine) {
        case LineMax:
            std::cout << "LineMax";
            break;
        case LineMin:
            std::cout << "LineMin";
            break;
        case LineUpperBand:
            std::cout << "LineUpperBand";
            break;
        case LineLowerBand:
            std::cout << "LineLowerBand";
            break;
        }
        std::cout << "\n\n";
    }

private:
    int    m_longMALength;
    int    m_shortMALength;
    int    m_slidingWndLength;
    double m_upperBandRatio;
    double m_lowerBandRatio;
    double m_stopLossPct;
    double m_stopProfitRtns;
    double m_stopProfitDrawdown;

    SMA  m_longMA;
    SMA  m_shortMA;
    High m_maxPrice;
    Low  m_minPrice;

    enum {
        Cross_Above = 1,
        Cross_Below,
    };

    enum {
        LineMax = 1,
        LineMin,
        LineUpperBand,   // Line below max
        LineLowerBand,   // Line above min
    };

    int m_lastCrossState;
    int m_lastCrossLine;

    SequenceDataSeries<double> m_upperBandPrice;
    SequenceDataSeries<double> m_lowerBandPrice;
};

//EXPORT_STRATEGY(STFT)
