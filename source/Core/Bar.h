#ifndef BAR_H
#define BAR_H

#include <vector>
#include <map>
#include "Defines.h"
#include "DateTime.h"

#define BAR_FIELD_SANITY_CHECK    (0)

namespace xBacktest
{

// A Bar is a summary of the trading activity for a security in a given period.
// Optimization to reduce memory footprint.
class DllExport Bar
{
public:
    enum Resolution {
        // It is important for frequency values to get bigger for bigger windows.
        UNKNOWN = 0,
        TICK    = -1,			    // The bar represents a single trade.
        SECOND  = 1,			    // The bar summarizes the trading activity during 1 second.
        MINUTE  = 60,			    // The bar summarizes the trading activity during 1 minute.
        HOUR    = 60 * 60,	        // The bar summarizes the trading activity during 1 hour.
        DAY     = 24 * 60 * 60,		// The bar summarizes the trading activity during 1 day.
        WEEK    = 24 * 60 * 60 * 7,	// The bar summarizes the trading activity during 1 week.
    };

    Bar();
	Bar(const char*     instrument,
        const DateTime& datetime, 
        double          open,
        double          high, 
        double          low, 
        double          close, 
        long long       volume, 
        long long       openInt, 
        int             resolution);

    const char*     getInstrument() const;     // Returns the instrument name.
    int             getSecurityType() const;
    void            setSecurityType(int type);
    const DateTime& getDateTime() const;       // Returns the datetime for this bar.
    double          getOpen() const;           // Returns the opening price.
    void            setOpen(double price);
    double          getHigh() const;           // Returns the highest price.
    void            setHigh(double price);
    double          getLow() const;            // Returns the lowest price.
    void            setLow(double price);
    double          getClose() const;          // Returns the closing price.
    void            setClose(double price);
    long long       getVolume() const;         // Returns the volume.
    void            setVolume(long long volume);
    long long       getOpenInt() const;        // Returns the open interest.
    void            setOpenInt(long long amount);
    double          getAmount() const;         // Returns the turnover.
    void            setAmount(double amount);
    double          getLastPrice() const;
    void            setLastPrice(double price);
    double          getBidPrice1() const;
    long long       getBidVolume1() const;
    double          getAskPrice1() const;
    long long       getAskVolume1() const;

    void      setTickField(double lastPrice, 
                           double bidPrice1, 
                           long long bidVolume1, 
                           double askPrice1, 
                           long long askVolume1);

    int       getResolution() const;           // Returns the bar's period.
    void      setResolution(int resolution);
    int       getInterval() const;
    void      setInterval(int interval);
    double    getTypicalPrice() const;         // Returns the typical price.
    bool      isValid() const;

private:
    int       m_securityType;
    string    m_instrument;
	DateTime  m_datetime;
	double    m_open;
	double    m_high;
	double    m_low;
	double    m_close;
    long long m_volume;
    long long m_openInt;
    double    m_amount;
	int       m_resolution;
    int       m_interval;

    double    m_lastPrice;
    double    m_bidPrice1;
    long long m_bidVolume1;
    double    m_askPrice1;
    long long m_askVolume1;
    bool      m_isValid;
};

} // namespace xBacktest

#endif // BAR_H