#ifndef CSV_FILE_LOADER_H
#define CSV_FILE_LOADER_H

#include "DataSeries.h"
#include "BaseFeed.h"
#include "BarFeed.h"
#include "csv_parser.hpp"

namespace xBacktest
{

class DataStream;
////////////////////////////////////////////////////////////////////////////////
class CsvFileLoader
{
public:
    class CsvBarFeed : public BarFeed
    {
        friend class CsvFileLoader;
    public:
        bool reset();
        bool getNextBar(Bar& outBar);
        bool isRealTime() { return false; }
        bool barsHaveAdjClose() { return true; }
        const DateTime peekDateTime() const;
        bool eof();
        CsvBarFeed* clone();

    private:        
        CsvBarFeed(int dataStreamId, int resolution);
        CsvBarFeed(const CsvBarFeed&);
        bool loadCsvTickData(const string& instrument, const string& file);
        bool loadCsvBarData(int resolution, const string& file);

    private:
        string m_instrument;
        int m_resolution;
        int m_readIdx;
        csv_parser* m_csvParser;
        shared_ptr< vector<Bar> > m_bars;
    };

    friend class CsvBarFeed;

    CsvFileLoader();
    ~CsvFileLoader();
    bool loadCsvFile(const string& instrument, int resolution, string& file, const Contract& contract, DataStream* stream);

private:   
    typedef struct {
        int                 id;
        string              name;
        int                 resolution;
        string              file;
        Contract            contract;
        vector<CsvBarFeed*> barFeeds;
    } CsvStreamDesc;

    vector<CsvStreamDesc> m_dataStreamDescs;
    
    int m_nextDataStreamId;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace xBacktest

#endif // CSV_FILE_LOADER_H
