#include "TeaFiles/File.h"
#include "TeaFiles/IO.h"
#include "TeaFiles/header/ReadContext.h"
#include "TeaFiles/description/TeaFileDescription.h"
#include "TeaFiles/os/FileIO.h"
#include "TeaFiles/file/FormattedReader.h"

#include "Defines.h"
#include "TsFileLoader.h"
#include "DataStorage.h"
#include "Logger.h"
#include "Errors.h"

using namespace teatime;

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
TsFileLoader::TsFileBarFeed::TsFileBarFeed(
        const string& instrument, 
        int resolution,
        TsFileItem* begin,
        TsFileItem* end)
    : BarFeed(resolution)
{
    setInstrument(instrument);
    m_readIdx = 0;
    m_begin     = begin;
    m_end       = end;
}

TsFileLoader::TsFileBarFeed::TsFileBarFeed(const TsFileBarFeed& feed)
    : BarFeed(feed.getResolution())
{
    m_readIdx  = 0;
    m_begin   = feed.m_begin;
    m_end     = feed.m_end;

    setId(DataStorage::getNextBarFeedId());
    setDataStreamId(feed.getDataStreamId());
    setName(feed.getName());
    setContract(feed.getContract());
    setInstrument(feed.getInstrument());
    setLength(feed.getLength());
    setBeginDateTime(feed.getBeginDateTime());
    setEndDateTime(feed.getEndDateTime());
}

TsFileLoader::TsFileBarFeed* TsFileLoader::TsFileBarFeed::clone()
{
    return new TsFileLoader::TsFileBarFeed(*this);
}

bool TsFileLoader::TsFileBarFeed::reset()
{
    m_readIdx = 0;
    return true;
}

bool TsFileLoader::TsFileBarFeed::getNextBar(Bar& outBar)
{
    if (m_readIdx < getLength()) {
        DateTime dt(m_begin[m_readIdx].datetime);

        Bar bar(m_begin[m_readIdx].name,
            dt,
            m_begin[m_readIdx].open,
            m_begin[m_readIdx].high,
            m_begin[m_readIdx].low,
            m_begin[m_readIdx].close,
            m_begin[m_readIdx].volume,
            m_begin[m_readIdx].openInt,
            getResolution());

        m_readIdx++;

        outBar = bar;

        return true;
    }

    return false;
}

int TsFileLoader::TsFileBarFeed::loadData(
    int   reqId,
    const DataRequest& request,
    void* object,
    void (*callback)(const DateTime& datetime, void* ctx))
{
    if (request.instrument != getInstrument()) {
        return 0;
    }

    int ret = 0;
    
    TsFileItem* item = m_begin;
    for ( ;
        item != m_end;
        item++) {
        DateTime dt(item->datetime);
        if (dt == request.to) {
            break;
        }
    }
    if (item == m_end) {
        return 0;
    }

    if (request.type == BarsBack) {
        if (item - m_begin <= request.count - 1) {
            return 0;
        }

        TsFileItem* from = item - request.count;
        while (from != item) {
            DateTime dt(from->datetime);
            Bar bar(item->name, 
                dt,
                item->open,
                item->high,
                item->low,
                item->close,
                item->volume,
                item->openInt,
                getResolution());

            HistoricalDataContext ctx;
            ctx.dataStreamId = getDataStreamId();
            ctx.barFeedId    = getId();
            ctx.object       = object;
            ctx.bar          = bar;
            callback(dt, &ctx);
            from++;
        }
        return request.count;
    } else {
        // Not supported yet.
        return 0;
    }

    return 0;
}

const DateTime TsFileLoader::TsFileBarFeed::peekDateTime() const
{
    return DateTime(m_begin[m_readIdx].datetime);
}

bool TsFileLoader::TsFileBarFeed::eof()
{
    return m_readIdx >= getLength();
}

////////////////////////////////////////////////////////////////////////////////
TsFileLoader::TsFileLoader()
{

}

TsFileLoader::~TsFileLoader()
{
    for (size_t i = 0; i < m_barFeeds.size(); i++) {
        delete m_barFeeds[i];
    }

    m_barFeeds.clear();
}

bool TsFileLoader::loadTsFile(const string& name, int resolution, const string& file, const Contract& contract, DataStream* stream)
{
    Logger_Info() << "Loading TS file '" << file << "'...";

    if (name.empty()) {
        ASSERT(false, "Symbol(name) is empty.");
    }

    for (size_t i = 0; i < m_dataStreamDescs.size(); i++) {
        if (m_dataStreamDescs[i].name == name && 
            m_dataStreamDescs[i].resolution == resolution) {
            return false;
        }
    }
    
    TsStreamDesc desc;
    desc.id         = DataStorage::getNextDataStreamId();
    desc.name       = name;
    desc.resolution = resolution;
    desc.file       = file;
    desc.contract   = contract;

    try {
        auto& tf = TeaFile<TsFileItem>::OpenRead(file.c_str());

//        cout << tf->Description() << endl;
        desc.base  = tf->OpenReadableMapping();
        desc.begin = desc.base->begin();
        desc.end   = desc.base->end();

    } catch (exception &e) {
        ASSERT(false, e.what());
        return false;
    }

    m_dataStreamDescs.push_back(desc);

    scanBarFeeds(m_barFeeds, contract);

    stream->setCommonContract(contract);
    stream->setId(desc.id);
    stream->setName(name);
    stream->setResolution(resolution);
    for (auto feed : m_barFeeds) {
        stream->insertBarFeed(feed);
    }

    return (m_barFeeds.size() > 0);
}

int TsFileLoader::scanBarFeeds(vector<TsFileBarFeed*>& feeds, const Contract& contract)
{
    Logger_Info() << "Scan instruments...";

    feeds.clear();
    
    if (m_dataStreamDescs.size() == 0) {
        return 0;
    }

    string instrument;
    
    for (size_t i = 0; i < m_dataStreamDescs.size(); i++) {
        int currPos = 0;
        int length = 0;
        TsFileBarFeed* barFeed = nullptr;
        DateTime begineDateTime;
        DateTime endDateTime;
        instrument.clear();

        TsFileItem* item      = m_dataStreamDescs[i].begin;
        TsFileItem* lastBegin = item;
        for (; item != m_dataStreamDescs[i].end; item++) {
            if (instrument.empty()) {
                instrument     = item->name;
                begineDateTime = DateTime(item->datetime);
                endDateTime    = begineDateTime;
            }

            if (item->name != instrument) {
                barFeed = new TsFileLoader::TsFileBarFeed(
                                                    instrument,
                                                    m_dataStreamDescs[i].resolution,
                                                    lastBegin,
                                                    lastBegin + length - 1
                                                    );
                barFeed->setId(DataStorage::getNextBarFeedId());
                Contract c = contract;
                c.instrument = instrument;
                barFeed->setContract(c);
                barFeed->setDataStreamId(m_dataStreamDescs[i].id);
                barFeed->setName(m_dataStreamDescs[i].name);
                barFeed->setLength(length);
                barFeed->setBeginDateTime(begineDateTime);
                barFeed->setEndDateTime(endDateTime);
                feeds.push_back(barFeed);

                instrument = item->name;
                begineDateTime = DateTime(item->datetime);
                endDateTime = begineDateTime;
                lastBegin = item;
                length = 1;
            } else {
                endDateTime = DateTime(item->datetime);
                length++;
            }
            currPos++;
        }

        // push last one
        barFeed = new TsFileLoader::TsFileBarFeed(
            instrument,
            m_dataStreamDescs[i].resolution,
            lastBegin,
            lastBegin + length - 1
            );
        barFeed->setId(DataStorage::getNextBarFeedId());
        Contract c = contract;
        c.instrument = instrument;
        barFeed->setContract(c);
        barFeed->setDataStreamId(m_dataStreamDescs[i].id);
        barFeed->setName(m_dataStreamDescs[i].name);
        barFeed->setLength(length);
        barFeed->setBeginDateTime(begineDateTime);
        barFeed->setEndDateTime(endDateTime);
        feeds.push_back(barFeed);
    }

    int size = feeds.size();

    Logger_Info() << "Construct " << size << (size <= 1 ? " barfeed." : " barfeeds.");

    return size;
}

} // namespace xBacktest