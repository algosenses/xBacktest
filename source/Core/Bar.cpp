#include <stdexcept>
#include "DateTime.h"
#include "Bar.h"

namespace xBacktest
{

Bar::Bar()
{
    m_open = 0;
    m_high = 0;
    m_low = 0;
    m_close = 0;
    m_volume = 0;
    m_openInt = 0;
    m_amount = 0;

    m_lastPrice = 0;
    m_askPrice1 = 0;
    m_askVolume1 = 0;
    m_bidPrice1 = 0;
    m_bidVolume1 = 0;

    m_resolution = 0;
    m_interval = 1;
    m_isValid = false;
}

Bar::Bar(
    const char* instrument,
	const DateTime& datetime, 
	double open, 
	double high, 
	double low, 
	double close, 
    long long volume, 
    long long openInt, 
	int resolution)
{
#if BAR_FIELD_SANITY_CHECK
	if (high < open) {
        if (high != 0) {
		    throw std::invalid_argument(instrument + " " + string("high < open on") + datetime.toString());
        }
	}

	if (high < low) {
        if (high != 0) {
		    throw std::invalid_argument(instrument + " " + string("high < low on") + datetime.toString());
        }
	}

	if (high < close) {
        if (high != 0) {
		    throw std::invalid_argument(instrument + " " + string("high < close on") + datetime.toString());
        }
	}

	if (low > open) {
        if (open != 0) {
		    throw std::invalid_argument(instrument + " " + string("low > open on") + datetime.toString());
        }
	}

	if (low > high) {
        if (high != 0) {
		    throw std::invalid_argument(instrument + " " + string("low > high on") + datetime.toString());
        }
	}

	if (low > close) {
        if (close != 0) {
		    throw std::invalid_argument(instrument + " " + string("low > close on") + datetime.toString());
        }
	}
#endif
    m_securityType = SecurityType::Unknown;
    m_instrument   = instrument;
	m_datetime     = datetime;
	m_open         = open;
	m_high         = high;
	m_low          = low;
    m_close        = close;
	m_volume       = volume;
    m_openInt      = openInt;
	m_resolution   = resolution;
    m_isValid      = true;
}

int Bar::getSecurityType() const
{
    return m_securityType;
}

void Bar::setSecurityType(int type)
{
    m_securityType = type;
}

const DateTime& Bar::getDateTime() const
{
	return m_datetime;
}

double Bar::getOpen() const
{
    return m_open;
}

void Bar::setOpen(double price)
{
    m_open = price;
}

double Bar::getHigh() const
{
    return m_high;
}

void Bar::setHigh(double price)
{
    m_high = price;
}

double Bar::getLow() const
{
	return m_low;
}

void Bar::setLow(double price)
{
    m_low = price;
}

double Bar::getClose() const
{
    return m_close;
}

void Bar::setClose(double price)
{
    m_close = price;
}

double Bar::getTypicalPrice() const
{
    return (getHigh() + getLow() + getClose()) / 3.0;
}

long long Bar::getVolume() const
{
	return m_volume;
}

void Bar::setVolume(long long volume)
{
    m_volume = volume;
}

long long Bar::getOpenInt() const
{
    return m_openInt;
}

void Bar::setOpenInt(long long openint)
{
    m_openInt = openint;
}

double Bar::getAmount() const
{
    return m_amount;
}

void Bar::setAmount(double amount)
{
    m_amount = amount;
}

double Bar::getLastPrice() const
{
    return m_lastPrice;
}

void Bar::setLastPrice(double price)
{
    m_lastPrice = price;
}

double Bar::getBidPrice1() const
{
    return m_bidPrice1;
}

long long Bar::getBidVolume1() const
{
    return m_bidVolume1;
}

double Bar::getAskPrice1() const
{
    return m_askPrice1;
}

long long Bar::getAskVolume1() const
{
    return m_askVolume1;
}

void Bar::setTickField(
    double lastPrice,
    double bidPrice1,
    long long bidVolume1,
    double askPrice1,
    long long askVolume1)
{
    m_lastPrice  = lastPrice;
    m_bidPrice1  = bidPrice1;
    m_bidVolume1 = bidVolume1;
    m_askPrice1  = askPrice1;
    m_askVolume1 = askVolume1;
}

int Bar::getResolution() const
{
	return m_resolution;
}

void Bar::setResolution(int resolution)
{
    m_resolution = resolution;
}

int Bar::getInterval() const
{
    return m_interval;
}

void Bar::setInterval(int interval)
{
    m_interval = interval;
}

const char* Bar::getInstrument() const
{
    return m_instrument.c_str();
}

bool Bar::isValid() const
{
    return m_isValid;
}

}  // namespace xBacktest