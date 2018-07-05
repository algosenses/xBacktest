#ifndef BIN_FILE_LOADER_H
#define BIN_FILE_LOADER_H

#include <boost/iostreams/device/mapped_file.hpp>

#include "BarFeed.h"

/*
typedef struct {
    long   datenum;    // YYYYMMDD
    long   date;
    int    time;
    double open;
    double high;
    double low;
    double close;
    double cj;
    double cc;
    char   instru[8];
    int    iMain;
} Bar;
*/

namespace xBacktest
{

class DataStream;

class BinFileLoader
{
public:
#pragma pack(push)
#pragma pack(4)
    typedef struct {
        char        instrument[8];
        uint32_t    date;    // YYYYMMDD
        uint32_t    time;
        double      open;
        double      high;
        double      low;
        double      close;
        double      volume;
        double      openInt;
        uint32_t    hot;     // hot contract
    } BinFileItem;
#pragma pack(pop)

    static DateTime getDateTime(long date, int time)
    {
        // Example: 20150818, 161039
        int year = date / 10000;
        int month = (date % 10000) / 100;
        int day = date % 100;
        int hour = time / 10000;
        int min = (time % 10000) / 100;
        int sec = time % 100;

        return DateTime(year, month, day, hour, min, sec);
    }

    class BinFileBarFeed : public BarFeed
    {
        friend class BinFileLoader;
    public:
        bool reset();
        bool getNextBar(Bar& outBar);
        bool isRealTime() { return false; }
        bool barsHaveAdjClose() { return true; }
        const DateTime peekDateTime() const;
        bool eof();
        int loadData(int   reqId,
            const DataRequest& request,
            void* object,
            void(*callback)(const DateTime& datetime, void* ctx));
        BinFileBarFeed* clone();

    private:
        BinFileBarFeed(const string& instrument,
            int resolution,
            BinFileItem* begin,
            BinFileItem* end);
//        BinFileBarFeed(const BinFileBarFeed&);

    private:
        int m_readIdx;
        BinFileItem* m_begin;
        BinFileItem* m_end; // Ending item is also available.
    };

    BinFileLoader();
    ~BinFileLoader();
    bool loadBinFile(const string& symbol, int resolution, int interval, const string& file, const Contract& contract, DataStream* stream);

private:
    typedef struct {
        int         id;
        string      name;
        int         resolution;
        int         interval;
        string      file;
        Contract    contract;

        boost::iostreams::mapped_file_source* mappedFile;
        BinFileItem*                          begin;
        BinFileItem*                          end;
    } BinStreamDesc;

    bool checkTimeline(BinFileBarFeed* feed);
    int  scanTradablePeriod(BinFileBarFeed* feed);
    int  scanBarFeeds(vector<BinFileBarFeed*>& feeds, BinStreamDesc& desc, const Contract& contract);

private:
    vector<BinStreamDesc> m_dataStreamDescs;
    vector<BinFileBarFeed*> m_barFeeds;
};

} // namespace xBacktest

#endif // BIN_FILE_LOADER_H