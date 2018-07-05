#ifndef TS_FILE_LOADER_H
#define TS_FILE_LOADER_H

#include "TeaFiles/File.h"
#include "TeaFiles/IO.h"
#include "BarFeed.h"

using namespace teatime;

namespace xBacktest
{

class DataStream;

// Binary time-series file feed.
class TsFileLoader
{
public:
#pragma pack(push)
#pragma pack(1)
    typedef struct {
        char   name[32];
        int64  datetime;
        double open;
        double high;
        double low;
        double close;
        int64  volume;
        int64  openInt;
    } TsFileItem;
#pragma pack(pop)

    class TsFileBarFeed : public BarFeed
    {
        friend class TsFileLoader;
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
                     void (*callback)(const DateTime& datetime, void* ctx));
        TsFileBarFeed* clone();

    private:        
        TsFileBarFeed(const string& instrument, 
                      int resolution,
                      TsFileItem* begin,
                      TsFileItem* end);
        TsFileBarFeed(const TsFileBarFeed&);

    private:
        int m_readIdx;
        TsFileItem* m_begin;
        TsFileItem* m_end;
    };

    TsFileLoader();
    ~TsFileLoader();
    bool loadTsFile(const string& symbol, int resolution, const string& file, const Contract& contract, DataStream* stream);

private:
    int scanBarFeeds(vector<TsFileBarFeed*>& feeds, const Contract& contract);

private:
    typedef struct {
        int         id;
        string      name;
        int         resolution;
        string      file;
        Contract    contract;

        shared_ptr<FileMapping<TsFileItem>> base;
        TsFileItem*                         begin;
        TsFileItem*                         end;
    } TsStreamDesc;

    vector<TsStreamDesc> m_dataStreamDescs;
    vector<TsFileBarFeed*> m_barFeeds;
};

} // namespace xBacktest

#endif // TS_FILE_LOADER_H