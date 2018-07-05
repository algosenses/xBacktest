#ifndef BAR_DATA_SERIES_H
#define BAR_DATA_SERIES_H

#include "Bar.h"
#include "DataSeries.h"
#include "Errors.h"

namespace xBacktest
{

// A DataSeries of Bar instances.
class DllExport BarSeries : public SequenceDataSeries<Bar>
{
public:
    template<typename T>
    class Accessor : public SequenceDataSeries<T>
    {
    public:
        enum {
            DS_UNKNOWN = 0,
            DS_OPEN,
            DS_HIGH,
            DS_LOW,
            DS_CLOSE,
            DS_VOLUME,
            DS_OPENINT,
            DS_AMOUNT,
            DS_LAST_PRICE,
            DS_BID_PRICE1,
            DS_BID_VOLUME1,
            DS_ASK_PRICE1,
            DS_ASK_VOLUME1
        };

        Accessor(int type, BarSeries& barSeries) 
            : m_type(type)
            , m_barSeries(barSeries)
            , m_newValueEvt(Event::EvtDataSeriesNewValue)
        {
        }

        ~Accessor() { }

        Event& getNewValueEvent()
        {
            return m_newValueEvt;
        }

        int length() const
        {
            return m_barSeries.length();
        }
       
        const T operator[](int idx) const
        {
            const Bar& bar = m_barSeries[idx];
            switch (m_type) {
            case DS_OPEN:
                return bar.getOpen();
            case DS_HIGH:
                return bar.getHigh();
            case DS_LOW:
                return bar.getLow();
            case DS_CLOSE:
                return bar.getClose();
            case DS_VOLUME:
                return bar.getVolume();
            case DS_OPENINT:
                return bar.getOpenInt();
            case DS_AMOUNT:
                return bar.getAmount();
            case DS_UNKNOWN:
            default:
//                ASSERT(false, "Unknown accessor type.");
                assert(false);
            }

            assert(false);
            return T(0);
        }

        void appendWithDateTime(const DateTime& dateTime, const void* value)
        {
        }

    private:
        int        m_type;
        Event      m_newValueEvt;
        BarSeries& m_barSeries;
    };

    // @param: maxLen: The maximum number of values to hold.
    // Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
    BarSeries(int maxLen = DataSeries::DEFAULT_MAX_LEN);
    void setResolution(int resolution);
    int  getResolution() const;
    void setInterval(int interval);
    int  getInterval() const;
    void append(Bar& bar);
    void appendWithDateTime(const DateTime& datetime, const void* value);

    // Returns a `DataSeries` with the open prices.
    DataSeries& getOpenDataSeries();
    // Returns a `DataSeries` with the close prices.
    DataSeries& getCloseDataSeries();
    // Returns a `DataSeries` with the high prices.
    DataSeries& getHighDataSeries();
    // Returns a `DataSeries` with the low prices.
    DataSeries& getLowDataSeries();
    // Returns a `DataSeries` with the volume.
    DataSeries& getVolumeDataSeries();
    // Returns a `DataSeries` with the open interest.
    DataSeries& getOpenIntDataSeries();
    // Returns a `DataSeries` with the close or adjusted close prices.
    DataSeries& getPriceDataSeries();

    DataSeries& getLastPriceDataSeries();
    DataSeries& getBidPrice1DataSeries();
    DataSeries& getBidVolume1DataSeries();
    DataSeries& getAskPrice1DataSeries();
    DataSeries& getAskVolume1DataSeries();

private:
    int m_resolution;
    int m_interval;

    Accessor<double>    m_openDSAccessor;
    Accessor<double>    m_highDSAccessor;
    Accessor<double>    m_lowDSAccessor;
    Accessor<double>    m_closeDSAccessor;
    Accessor<long long> m_volumeDSAccessor;
    Accessor<long long> m_openIntDSAccessor;

    Accessor<double>    m_lastPriceDSAccessor;
    Accessor<double>    m_bidPrice1DSAccessor;
    Accessor<long long> m_bidVolume1DSAccessor;
    Accessor<double>    m_askPrice1DSAccessor;
    Accessor<long long> m_askVolume1DSAccessor;
};

} // namespace xBacktest

#endif // BAR_DATA_SERIES_H