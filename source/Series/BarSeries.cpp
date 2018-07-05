#include "BarSeries.h"

namespace xBacktest
{

BarSeries::BarSeries(int maxLen)
	: SequenceDataSeries(maxLen)
    , m_openDSAccessor(BarSeries::Accessor<double>::DS_OPEN, *this)
    , m_highDSAccessor(BarSeries::Accessor<double>::DS_HIGH, *this)
    , m_lowDSAccessor(BarSeries::Accessor<double>::DS_LOW, *this)
    , m_closeDSAccessor(BarSeries::Accessor<double>::DS_CLOSE, *this)
    , m_volumeDSAccessor(BarSeries::Accessor<long long>::DS_VOLUME, *this)
    , m_openIntDSAccessor(BarSeries::Accessor<long long>::DS_OPENINT, *this)
    , m_lastPriceDSAccessor(BarSeries::Accessor<double>::DS_LAST_PRICE, *this)
    , m_bidPrice1DSAccessor(BarSeries::Accessor<double>::DS_BID_PRICE1, *this)
    , m_bidVolume1DSAccessor(BarSeries::Accessor<long long>::DS_BID_VOLUME1, *this)
    , m_askPrice1DSAccessor(BarSeries::Accessor<double>::DS_ASK_PRICE1, *this)
    , m_askVolume1DSAccessor(BarSeries::Accessor<long long>::DS_ASK_VOLUME1, *this)
{
    m_resolution = Bar::MINUTE;
    m_interval = 1;
}

void BarSeries::setResolution(int resolution)
{
    m_resolution = resolution;
}

int BarSeries::getResolution() const
{
    return m_resolution;
}

void BarSeries::setInterval(int interval)
{
    m_interval = interval;
}

int BarSeries::getInterval() const
{
    return m_interval;
}

void BarSeries::append(Bar& bar)
{
    appendWithDateTime(bar.getDateTime(), &bar);
}

void BarSeries::appendWithDateTime(const DateTime& datetime, const void* value)
{
    SequenceDataSeries<Bar>::appendWithDateTime(datetime, value);

    const Bar& bar = *((const Bar*) value);
    double open       = bar.getOpen();
    double high       = bar.getHigh();
    double low        = bar.getLow();
    double close      = bar.getClose();
    long long volume  = bar.getVolume();
    long long openInt = bar.getOpenInt();

    Event::Context ctx;
    ctx.datetime = datetime;

    ctx.data = (void*)(&open);    m_openDSAccessor.getNewValueEvent().emit(datetime, &ctx);
    ctx.data = (void*)(&high);    m_highDSAccessor.getNewValueEvent().emit(datetime, &ctx);
    ctx.data = (void*)(&low);     m_lowDSAccessor.getNewValueEvent().emit(datetime, &ctx);
    ctx.data = (void*)(&close);   m_closeDSAccessor.getNewValueEvent().emit(datetime, &ctx);
    ctx.data = (void*)(&volume);  m_volumeDSAccessor.getNewValueEvent().emit(datetime, &ctx);
    ctx.data = (void*)(&openInt); m_openIntDSAccessor.getNewValueEvent().emit(datetime, &ctx);

    if (bar.getResolution() == Bar::TICK) {
        double lastPrice  = bar.getLastPrice();
        double askPrice1  = bar.getAskPrice1();
        double askVolume1 = bar.getAskVolume1();
        double bidPrice1  = bar.getBidPrice1();
        double bidVolume1 = bar.getBidVolume1();

        ctx.data = (void*)(&lastPrice);  m_lastPriceDSAccessor.getNewValueEvent().emit(datetime, &ctx);
        ctx.data = (void*)(&askPrice1);  m_askPrice1DSAccessor.getNewValueEvent().emit(datetime, &ctx);
        ctx.data = (void*)(&askVolume1); m_askVolume1DSAccessor.getNewValueEvent().emit(datetime, &ctx);
        ctx.data = (void*)(&bidPrice1);  m_bidPrice1DSAccessor.getNewValueEvent().emit(datetime, &ctx);
        ctx.data = (void*)(&bidVolume1); m_bidVolume1DSAccessor.getNewValueEvent().emit(datetime, &ctx);
    }
}

DataSeries& BarSeries::getOpenDataSeries()
{
	return m_openDSAccessor;
}

DataSeries& BarSeries::getCloseDataSeries()
{
	return m_closeDSAccessor;
}

DataSeries& BarSeries::getHighDataSeries()
{
	return m_highDSAccessor;
}

DataSeries& BarSeries::getLowDataSeries()
{
	return m_lowDSAccessor;
}

DataSeries& BarSeries::getVolumeDataSeries()
{
	return m_volumeDSAccessor;
}

DataSeries& BarSeries::getOpenIntDataSeries()
{
    return m_openIntDSAccessor;
}

DataSeries& BarSeries::getPriceDataSeries()
{
    return m_closeDSAccessor;
}

DataSeries& BarSeries::getLastPriceDataSeries()
{
    return m_lastPriceDSAccessor;
}

DataSeries& BarSeries::getBidPrice1DataSeries()
{
    return m_bidPrice1DSAccessor;
}

DataSeries& BarSeries::getBidVolume1DataSeries()
{
    return m_bidVolume1DSAccessor;
}

DataSeries& BarSeries::getAskPrice1DataSeries()
{
    return m_askPrice1DSAccessor;
}

DataSeries& BarSeries::getAskVolume1DataSeries()
{
    return m_askVolume1DSAccessor;
}

} // namespace xBacktest