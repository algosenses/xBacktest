#include <iostream>
#include <iomanip>
#include "Strategy.h"
#include "MA.h"
#include "Cross.h"

class MMAD : public Strategy
{
public:
    void onCreate()
    {
        registerParameter("SlowMaLength",  PARAM_TYPE_INT,    &m_slowMALength);
        registerParameter("FastMaLength",  PARAM_TYPE_INT,    &m_fastMALength);
        registerParameter("SlowMaDelay",   PARAM_TYPE_INT,    &m_slowMaDelayValue);
        registerParameter("MALineGap",     PARAM_TYPE_DOUBLE, &m_MALineGap);
        registerParameter("StopLossPct",   PARAM_TYPE_DOUBLE, &m_stopLossPct);
        registerParameter("StopProfitPct", PARAM_TYPE_DOUBLE, &m_stopProfitRtns);
        registerParameter("DrawdownPct",   PARAM_TYPE_DOUBLE, &m_stopProfitDrawdown);
    }

    void onStart()
    {
        m_fastMA.init(getBarSeries().getCloseDataSeries(), m_fastMALength);
        m_slowMA.init(getBarSeries().getCloseDataSeries(), m_slowMALength);

        m_currCenterMASeries.clear();
        m_tempMASeries.clear();

        m_calcMALineRange = true;
        m_initState = true;
    }

    void onBar(const Bar& bar)
    {
        if (isnan(m_fastMA[0]) || isnan(m_slowMA[0]))
        {
            return;
        }

        if (m_slowMA.length() < m_slowMaDelayValue + 1 || isnan(m_slowMA[m_slowMaDelayValue])) {
            return;
        }

        m_slowDelayMASeries.append(m_slowMA[m_slowMaDelayValue]);

        if (m_calcMALineRange) {
            double slowMACurrValue = m_slowDelayMASeries[0];
            double fastMACurrValue = m_fastMA[0];

            std::cout << bar.getDateTime().toString() << ": " << bar.getClose() << std::endl;
            std::cout << "MA" << m_fastMALength << ":" << m_fastMA[0];
            std::cout << ", MA" << m_slowMALength << ":" << m_slowDelayMASeries[0] << std::endl;

            initMALineOffset(fastMACurrValue, slowMACurrValue);

            std::cout << "UpperLineOffset:" << m_upperMALineOffset
                << ", CenterLineOffset:" << m_centerMALineOffset
                << ", LowerLineOffset:" << m_lowerMALineOffset << "\n\n\n";

            m_calcMALineRange = false;

            return;
        }

        // We need 3 points to judge cross
        if (m_slowDelayMASeries.length() < 3) {
            return;
        }


        // Cross above upper line
        if (calcCrossAbovePosition()) {
            std::cout << std::endl << bar.getDateTime().toString() << ": " << bar.getClose() << std::endl;
            std::cout << "MA" << m_fastMALength << ":" << m_fastMA[0] << ", " << m_fastMA[1] << ", " << m_fastMA[2] << std::endl;
            std::cout << "MA" << m_slowMALength << ":" << m_slowDelayMASeries[0] << ", " << m_slowDelayMASeries[1] << ", " << m_slowDelayMASeries[2] << std::endl;
            std::cout << "CrossAbove" << std::endl;
            std::cout << "UpperLineOffset:" << m_upperMALineOffset
                << ", CenterLineOffset:" << m_centerMALineOffset
                << ", LowerLineOffset:" << m_lowerMALineOffset << "\n\n\n";
        }

        // Cross below lower line
        if (calcCrossBelowPosition()) {
            std::cout << bar.getDateTime().toString() << ": " << bar.getClose() << std::endl;
            std::cout << "MA" << m_fastMALength << ":" << m_fastMA[0];
            std::cout << ", MA" << m_slowMALength << ":" << m_slowDelayMASeries[0] << std::endl;
            std::cout << "CrossBelow" << std::endl;
            std::cout << "UpperLineOffset:" << m_upperMALineOffset
                << ", CenterLineOffset:" << m_centerMALineOffset
                << ", LowerLineOffset:" << m_lowerMALineOffset << "\n\n\n";
        }
    }

private:
    void initMALineOffset(double currFastMAValue, double currSlowMAValue)
    {
        int offset = 0;
        while (fabs(currFastMAValue - currSlowMAValue) > currSlowMAValue * m_MALineGap) {
            if (currFastMAValue < currSlowMAValue) {
                currSlowMAValue -= currSlowMAValue * m_MALineGap;
            } else {
                currSlowMAValue += currSlowMAValue * m_MALineGap;
            }
            offset++;
        }

        m_upperMALineOffset = 0;
        m_lowerMALineOffset = 0;
        if (currSlowMAValue > currFastMAValue) {
            m_upperMALineOffset = -offset;
            m_centerMALineOffset = -offset - 1;
            m_lowerMALineOffset = -offset - 2;
        } else {
            m_upperMALineOffset = offset + 2;
            m_centerMALineOffset = offset + 1;
            m_lowerMALineOffset = offset;
        }
    }

    bool crossAboveOffsetLine(int offset)
    {
        // Three points is enough to judge cross.
        // Append order is important, old data be append first.
        m_tempMASeries.clear();
        m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_MALineGap);
        m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_MALineGap);
        m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_MALineGap);
//        std::cout << "MATemp:" << m_tempMASeries[0] << ", " << m_tempMASeries[1] << ", " << m_tempMASeries[2] << std::endl;

        if (Cross::crossAbove(m_fastMA, m_tempMASeries)) {
            return true;
        }

        return false;
    }

    bool calcCrossAbovePosition()
    {
        int offset = m_lowerMALineOffset;
        // Judge cross at least 3 times, from lower line to upper line
        int minJudgeTime = 3;
        int i = 0;
        for (i = 0; i < minJudgeTime; i++) {
            if (crossAboveOffsetLine(offset + i)) {
                break;
            }
        }

        if (i == minJudgeTime) {
            return false;
        }

        offset += i;


        while (1) {
            // Three points is enough to judge cross.
            m_tempMASeries.clear();
            m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_MALineGap);
            m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_MALineGap);
            m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_MALineGap);
            if (Cross::crossAbove(m_fastMA, m_tempMASeries)) {
                offset++;
            } else {
                break;
            }
        }

        offset -= 1;

//        std::cout << "Cross Above offset:" << offset << std::endl;

        m_centerMALineOffset = offset;
        m_lowerMALineOffset = m_centerMALineOffset - 1;
        m_upperMALineOffset = m_centerMALineOffset + 1;

        return true;
    }

    bool crossBelowOffsetLine(int offset)
    {
        m_tempMASeries.clear();
        m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_MALineGap);
        m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_MALineGap);
        m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_MALineGap);
        // std::cout << "MATemp:" << m_tempMASeries[0] << ", " << m_tempMASeries[1] << ", " << m_tempMASeries[2] << std::endl;

        if (Cross::crossBelow(m_fastMA, m_tempMASeries)) {
            return true;
        }

        return false;
    }

    bool calcCrossBelowPosition()
    {
        int offset = m_upperMALineOffset;
        // Judge cross at least 3 times, from upper line to lower line
        int minJudgeTime = 3;
        int i = 0;
        for (i = 0; i < minJudgeTime; i++) {
            if (crossBelowOffsetLine(offset - i)) {
                break;
            }
        }

        if (i == minJudgeTime) {
            return false;
        }

        offset -= i;


        while (1) {
            m_tempMASeries.clear();
            m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_MALineGap);
            m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_MALineGap);
            m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_MALineGap);
            if (Cross::crossBelow(m_fastMA, m_tempMASeries)) {
                offset--;
            } else {
                break;
            }
        }

        offset += 1;

//        std::cout << "Cross Below offset:" << offset << std::endl;
        m_centerMALineOffset = offset;
        m_lowerMALineOffset = m_centerMALineOffset - 1;
        m_upperMALineOffset = m_centerMALineOffset + 1;

        return true;
    }

private:
    int m_slowMALength;
    int m_fastMALength;
    int m_slowMaDelayValue;
    double m_MALineGap;
    double m_stopLossPct;
    double m_stopProfitRtns;
    double m_stopProfitDrawdown;

    SMA m_fastMA;
    SMA m_slowMA;

    bool m_initState;
    bool m_calcMALineRange;
    int m_upperMALineOffset;
    int m_centerMALineOffset;
    int m_lowerMALineOffset;

    SequenceDataSeries<double> m_slowDelayMASeries;
    SequenceDataSeries<double> m_currCenterMASeries;
    SequenceDataSeries<double> m_tempMASeries;
};

//EXPORT_STRATEGY(MMAD)
