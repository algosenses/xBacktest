#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include "CsvFileLoader.h"
#include "BinFileLoader.h"
//#include "TsFileLoader.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
// One data stream may contains many bar feeds, but those feeds must have the
// same contract, resolution, interval and other properties except instrument id.
class DataStream
{
    friend class DataStorage;

public:
    void              setId(int id);
    int               getId() const;
    void              setName(const string& name);
    const string&     getName() const;
    void              setResolution(int resolution);
    void              setInterval(int interval);
    void              setCommonContract(const Contract& contract);
    const Contract&   getCommonContract() const;
    void              insertBarFeed(BarFeed* feed);
    vector<BarFeed*>& getBarFeeds();

    // Create a new shared bar feed.
    // Read index will be reset to point to the start of bar stream.
    // This allows many run-times access a memory space concurrently.
    BarFeed*          cloneSharedBarFeed(const string& instrument);
    int               cloneSharedBarFeed(vector<BarFeed*>& feeds);

private:
    DataStream();
    ~DataStream();
    DataStream(const DataStream &);
    DataStream &operator=(const DataStream &);

private:
    int              m_id;
    string           m_name;
    int              m_resolution;
    int              m_interval;
    vector<BarFeed*> m_barFeeds;
    Contract         m_commContract;

    static unsigned long m_nextId;
};

////////////////////////////////////////////////////////////////////////////////
// One Data Storage may contains many data streams.
class DataStorage
{
public:
    DataStorage();
    ~DataStorage();

    bool loadDataStreamFile(const string& name, const string& filename, int format, int resolution, int interval, const Contract& contract);

    DataStream* getDataStream(const string& name);
    DataStream* getDataStream(int id);
    int getDataStreamId(const string& name);
    int getAllDataStream(vector<DataStream*>& streams);
    BarFeed* createSharedBarFeed(const string& instrument, int resolution, int interval = 0);

    static unsigned long getNextDataStreamId();
    static unsigned long getNextBarFeedId();

private:
    bool loadCsvFile(const string& name, const string& filename, int resolution, int interval, const Contract& contract);
    bool loadTsFile(const string& name, const string& filename, int resolution, int interval, const Contract& contract);
    bool loadBinFile(const string& name, const string& filename, int resolution, int interval, const Contract& contract);

private:
    static unsigned long m_nextBarFeedId;
    static unsigned long m_nextDataStreamId;

    CsvFileLoader m_csvFileLoader;
    BinFileLoader m_binFileLoader;
//    TsFileLoader  m_tsFileLoader;

    unordered_map<string, DataStream*> m_dataStreams;
};

}

#endif // DATA_STORAGE_H