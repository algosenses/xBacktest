#ifndef KDJ_H
#define KDJ_H

#include "BarSeries.h"
#include "Technical.h"
#include "MA.h"
#include "HighLow.h"

namespace xBacktest
{

/*  
 *  KDJ formula:
 *  Parameters: 
 *  N: period, M1: slowLen, M2: smoothLen
 *
 *  RSV:(CLOSE-LLV(LOW,N))/(HHV(HIGH,N)-LLV(LOW,N))*100;
 *  K:SMA(RSV,M1,1);
 *  D:SMA(K,M2,1);
 *  J:3*K-2*D;
 */
class DllExport KDJ : public SequenceDataSeries<Bar>, public IEventHandler
{
public:
    void init(BarSeries& barSeries, int period = 9, int slowLen = 3, int smoothLen = 3, int maxLen = DataSeries::DEFAULT_MAX_LEN);
    DataSeries& getK();
    DataSeries& getD();
    DataSeries& getJ();

private:
    void onEvent(int type, const DateTime& datetime, const void *context);
    void onNewValue(const DateTime& datetime, Bar& value);

private:
    int m_period;
    int m_slowLen;
    int m_smoothLen;

    SequenceDataSeries<Bar> m_barSeries;
    SequenceDataSeries<double> m_k;
    SequenceDataSeries<double> m_d;
    SequenceDataSeries<double> m_j;
};

} // namespace xBacktest

#endif // KDJ_H