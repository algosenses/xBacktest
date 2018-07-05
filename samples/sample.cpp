#include "Strategy.h"

class Sample : public Strategy
{
public:
    void onCreate()
    {
    }

    void onStart()
    {

    }

    void onPositionOpen(Position& position) 
    {

    }

    void onPositionClosed(Position& position) 
    {

    }

    void onBar(const Bar& bar)
    {

    }

private:
    SMA m_sma;
    MACD m_macd;
};

//EXPORT_STRATEGY(Sample)
