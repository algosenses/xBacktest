#ifndef STOCH_H
#define STOCH_H

#include "Technical.h"
#include "BarSeries.h"
#include "MA.h"

namespace xBacktest
{
    
class SOEventWindow : public EventWindow<Bar, double>
{
public:
    void init(int period);
    void onNewValue(const DateTime& datetime, const Bar& bar);
    double getValue() const;

private:
    SequenceDataSeries<Bar> m_barSeries;
};

// Stochastic Oscillator filter as described in http://stockcharts.com/school/doku.php?id=chart_school:technical_indicators:stochastic_oscillato.
// Note that the value returned by this filter is %K.To access %D use : meth : `getD`.
class DllExport StochasticOscillator : public EventBasedFilter<Bar, double>
{
public:
    // @param barSeries: The BarSeries instance being filtered.
    // @param period : The period.Must be > 1.
    // @param dSMAPeriod : The %D SMA period.Must be > 1.
    // @param useAdjustedValues : True to use adjusted Low / High / Close values.
    // @param maxLen : The maximum number of values to hold.
    // Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
    void init(BarSeries& barSeries, int period, int dSMAPeriod = 3, bool useAdjustedValues = false, int maxLen = DataSeries::DEFAULT_MAX_LEN);
    
    // Returns a `DataSeries` with the %D values.
    DataSeries& getD();

private:
    MA m_d;
    SOEventWindow m_evtWindow;
};

} // namespace xBacktest

#endif // STOCH_H
