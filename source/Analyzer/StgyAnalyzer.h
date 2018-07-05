#ifndef STGY_ANALYZER_H
#define STGY_ANALYZER_H

#include <vector>
#include "DateTime.h"

namespace xBacktest
{

class BacktestingBroker;
class Bar;

// Base class for strategy analyzers.
// NOTE: This is a base class and should not be used directly.
class StgyAnalyzer
{
public:
    virtual void beforeAttach(BacktestingBroker& broker) { }
    virtual void attached(BacktestingBroker& broker) { }
    virtual void beforeOnBar(BacktestingBroker& broker, const Bar& bar) { }
};

} // namespace xBacktest

#endif // STGY_ANALYZER_H
