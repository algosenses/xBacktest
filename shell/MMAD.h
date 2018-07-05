#ifndef MMAD_H
#define MMAD_H

#include <iostream>
#include "Framework.h"
#include "Config.h"
#include "Strategy.h"
#include "MA.h"
#include "Cross.h"

#define AVAILABLE_CASH    (1000000)

class MMAD : public Strategy
{
public:
    void onCreate()
    {
        memset(&m_params, 0, sizeof(PARAMS));
    }

    void onSetParameter(const string& name, int type, const string& value)
    {
        if (name == "openvol") {
            m_params.n_openvol = atoi(value.c_str());
        } else if (name == "short") {
            m_params.n_short = atoi(value.c_str());
        } else if (name == "long") {
            m_params.n_long = atoi(value.c_str());
        } else if (name == "delay") {
            m_params.n_delay = atoi(value.c_str());
        } else if (name == "lineGap") {
            m_params.lineGap = atof(value.c_str());
        } else if (name == "stopLossPct") {
            m_params.stopLossPct = atof(value.c_str());
        } else if (name == "stopProfitPct") {
            m_params.stopProfitPct = atof(value.c_str());
        } else if (name == "stopProfitDrawdown") {
            m_params.stopProfitDrawdown = atof(value.c_str());
        } else if (name == "stopProfitPct2") {
            m_params.stopProfitPct2 = atof(value.c_str());
        } else if (name == "stopProfitDrawdown2") {
            m_params.stopProfitDrawdown2 = atof(value.c_str());
        }
    }

    void onStart()
    {
        if (m_params.n_long > 0 && m_params.n_long < DataSeries::DEFAULT_MAX_LEN) {
            m_slowMA.init(getBarSeries().getCloseDataSeries(), m_params.n_long);
        }

        if (m_params.n_short > 0 && m_params.n_short < DataSeries::DEFAULT_MAX_LEN) {
            m_fastMA.init(getBarSeries().getCloseDataSeries(), m_params.n_short);
        }

        m_multiplier = getDefaultContract().multiplier;
        m_marginRatio = getDefaultContract().marginRatio;

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

        if (m_slowMA.length() < m_params.n_delay + 1 || isnan(m_slowMA[m_params.n_delay])) {
            return;
        }

        m_slowDelayMASeries.append(m_slowMA[m_params.n_delay]);

        if (m_calcMALineRange) {
            double slowMACurrValue = m_slowDelayMASeries[0];
            double fastMACurrValue = m_fastMA[0];

            printDebugMsg("%s: %f\n", bar.getDateTime().toString().c_str(), bar.getClose());
            printDebugMsg("MA%d: %f, MA%d: %f\n", m_params.n_short, fastMACurrValue, m_params.n_long, slowMACurrValue);

            initMALineOffset(fastMACurrValue, slowMACurrValue);

            printDebugMsg("UpperLineOffset:%d, CenterLineOffset:%d, LowerLineOffset:%d\n\n\n", m_upperMALineOffset, m_centerMALineOffset, m_lowerMALineOffset);

            m_calcMALineRange = false;

            return;
        }

        // We need 3 points to judge cross
        if (m_slowDelayMASeries.length() < 3) {
            return;
        }

        // Cross above upper line
        if (calcCrossAbovePosition(bar)) {
            printDebugMsg("enterLong\n\n\n");
            int openVol = AVAILABLE_CASH / (bar.getClose() * m_multiplier * m_marginRatio);
            enterLong(ExitSignal::Signal, openVol, bar.getClose());
        }

        // Cross below lower line
        if (calcCrossBelowPosition(bar)) {
            printDebugMsg("enterShort\n\n\n");
            int openVol = AVAILABLE_CASH / (bar.getClose() * m_multiplier * m_marginRatio);
            enterShort(ExitSignal::Signal, openVol, bar.getClose());
        }
    }

    void onPositionOpened(Position& position)
    {
        position.setStopLossPct(m_params.stopLossPct);
        position.setStopProfitPct(m_params.stopProfitPct, m_params.stopProfitDrawdown);
        position.setStopProfitPct(m_params.stopProfitPct2, m_params.stopProfitDrawdown2);
    }

private:
    void initMALineOffset(double currFastMAValue, double currSlowMAValue)
    {
        double slowMaValue = currSlowMAValue;
        int offset = 0;
        while (fabs(currFastMAValue - currSlowMAValue) > slowMaValue * m_params.lineGap) {
            if (currFastMAValue < currSlowMAValue) {
                currSlowMAValue -= slowMaValue * m_params.lineGap;
            } else {
                currSlowMAValue += slowMaValue * m_params.lineGap;
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

        m_lastCenterMALineOffset = m_centerMALineOffset;
    }

    bool crossAboveOffsetLine(int offset)
    {
        // Three points is enough to judge cross.
        // Append order is important, old data be append first.
        m_tempMASeries.clear();
        m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_params.lineGap);
        m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_params.lineGap);
        m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_params.lineGap);
        //        std::cout << "MATemp:" << m_tempMASeries[0] << ", " << m_tempMASeries[1] << ", " << m_tempMASeries[2] << std::endl;

        if (Cross::crossAbove(m_fastMA, m_tempMASeries)) {
            return true;
        }

        return false;
    }

    bool calcCrossAbovePosition(const Bar& bar)
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
            m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_params.lineGap);
            m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_params.lineGap);
            m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_params.lineGap);
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

        printDebugMsg("%s: %f\n", bar.getDateTime().toString().c_str(), bar.getClose());
        printDebugMsg("MA%d: %f, %f, %f\n", m_params.n_short, m_fastMA[0], m_fastMA[1], m_fastMA[2]);
        printDebugMsg("MA%d: %f, %f, %f\n", m_params.n_long, m_slowDelayMASeries[0], m_slowDelayMASeries[1], m_slowDelayMASeries[2]);
        printDebugMsg("CrossAbove: %d\n", m_centerMALineOffset);
        printDebugMsg("UpperLineOffset:%d, CenterLineOffset:%d, LowerLineOffset:%d, lastCenterLineOffset:%d\n",
            m_upperMALineOffset,
            m_centerMALineOffset,
            m_lowerMALineOffset,
            m_lastCenterMALineOffset);

        if (m_lastCenterMALineOffset != m_centerMALineOffset) {
            m_lastCenterMALineOffset = m_centerMALineOffset;
            return true;
        }

        return false;
    }

    bool crossBelowOffsetLine(int offset)
    {
        m_tempMASeries.clear();
        m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_params.lineGap);
        m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_params.lineGap);
        m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_params.lineGap);
        // std::cout << "MATemp:" << m_tempMASeries[0] << ", " << m_tempMASeries[1] << ", " << m_tempMASeries[2] << std::endl;

        if (Cross::crossBelow(m_fastMA, m_tempMASeries)) {
            return true;
        }

        return false;
    }

    bool calcCrossBelowPosition(const Bar& bar)
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
            m_tempMASeries.append(m_slowDelayMASeries[2] + m_slowDelayMASeries[2] * offset * m_params.lineGap);
            m_tempMASeries.append(m_slowDelayMASeries[1] + m_slowDelayMASeries[1] * offset * m_params.lineGap);
            m_tempMASeries.append(m_slowDelayMASeries[0] + m_slowDelayMASeries[0] * offset * m_params.lineGap);
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

        printDebugMsg("%s: %f\n", bar.getDateTime().toString().c_str(), bar.getClose());
        printDebugMsg("MA%d: %f, MA%d: %f\n", m_params.n_short, m_fastMA[0], m_params.n_long, m_slowDelayMASeries[0]);
        printDebugMsg("CrossBelow: %d", m_centerMALineOffset);
        printDebugMsg("UpperLineOffset:%d, CenterLineOffset:%d, LowerLineOffset:%d, lastCenterLineOffset:%d\n",
            m_upperMALineOffset,
            m_centerMALineOffset,
            m_lowerMALineOffset,
            m_lastCenterMALineOffset);

        if (m_lastCenterMALineOffset != m_centerMALineOffset) {
            m_lastCenterMALineOffset = m_centerMALineOffset;
            return true;
        }

        return false;
    }

private:
    typedef struct {
        int    n_openvol;
        int    n_short;
        int    n_long;
        int    n_delay;
        double lineGap;
        double stopLossPct;
        double stopProfitPct;
        double stopProfitDrawdown;
        double stopProfitPct2;
        double stopProfitDrawdown2;
    } PARAMS;

    PARAMS m_params;

    SMA m_fastMA;
    SMA m_slowMA;

    bool m_initState;
    bool m_calcMALineRange;
    int m_upperMALineOffset;
    // current center MA line's distance from the slow MA
    int m_centerMALineOffset;
    int m_lowerMALineOffset;

    int m_lastCenterMALineOffset;

    SequenceDataSeries<double> m_slowDelayMASeries;
    SequenceDataSeries<double> m_tempMASeries;

    int    m_multiplier;
    double m_marginRatio;
};

#endif // MMAD_H