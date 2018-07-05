#include <iostream>
#include <boost/filesystem.hpp>
#include "Logger.h"
#include "BinFileLoader.h"
#include "DataStorage.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
BinFileLoader::BinFileBarFeed::BinFileBarFeed(
        const string& instrument, 
        int resolution,
        BinFileItem* begin,
        BinFileItem* end)
    : BarFeed(resolution)
{
    setInstrument(instrument);
    m_readIdx = 0;
    m_begin = begin;
    m_end = end;
}

bool BinFileLoader::BinFileBarFeed::reset()
{
    m_readIdx = 0;
    return true;
}

bool BinFileLoader::BinFileBarFeed::getNextBar(Bar& outBar)
{
    if (m_readIdx < getLength()) {
        DateTime dt = BinFileLoader::getDateTime(m_begin[m_readIdx].date, m_begin[m_readIdx].time);

        Bar bar(m_begin[m_readIdx].instrument,
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

int BinFileLoader::BinFileBarFeed::loadData(
    int   reqId,
    const DataRequest& request,
    void* object,
    void (*callback)(const DateTime& datetime, void* ctx))
{
    if (request.instrument != getInstrument()) {
        return 0;
    }

    int ret = 0;
    
    BinFileItem* item = m_begin;
    for ( ;
        item != m_end;
        item++) {
        DateTime dt = BinFileLoader::getDateTime(item->date, item->time);
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

        BinFileItem* from = item - request.count;
        while (from != item) {
            DateTime dt = BinFileLoader::getDateTime(from->date, from->time);
            Bar bar(item->instrument, 
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

BinFileLoader::BinFileBarFeed* BinFileLoader::BinFileBarFeed::clone()
{
    BinFileBarFeed* feed = new BinFileLoader::BinFileBarFeed(*this);
    feed->reset();
    feed->setId(DataStorage::getNextBarFeedId());

    return feed;
}

const DateTime BinFileLoader::BinFileBarFeed::peekDateTime() const
{
    return BinFileLoader::getDateTime(m_begin[m_readIdx].date, m_begin[m_readIdx].time);
}

bool BinFileLoader::BinFileBarFeed::eof()
{
    return m_readIdx >= getLength();
}

////////////////////////////////////////////////////////////////////////////////
BinFileLoader::BinFileLoader()
{
    m_dataStreamDescs.clear();
}

BinFileLoader::~BinFileLoader()
{
    for (size_t i = 0; i < m_barFeeds.size(); i++) {
        delete m_barFeeds[i];
    }
    m_barFeeds.clear();

    for (size_t i = 0; i < m_dataStreamDescs.size(); i++) {
        m_dataStreamDescs[i].mappedFile->close();
        delete m_dataStreamDescs[i].mappedFile;
    }
    m_dataStreamDescs.clear();
}

bool BinFileLoader::loadBinFile(const string& name, int resolution, int interval, const string& file, const Contract& contract, DataStream* stream)
{
    Logger_Info() << "Loading binary file '" << file << "'...";

    if (name.empty()) {
        ASSERT(false, "Symbol(name) is empty.");
    }

    for (size_t i = 0; i < m_dataStreamDescs.size(); i++) {
        if (m_dataStreamDescs[i].name == name && 
            m_dataStreamDescs[i].resolution == resolution &&
            m_dataStreamDescs[i].interval == interval) {
            return false;
        }
    }
    
    BinStreamDesc desc;
    desc.id         = DataStorage::getNextDataStreamId();
    desc.name       = name;
    desc.resolution = resolution;
    desc.interval   = interval;
    desc.file       = file;
    desc.contract   = contract;

    try {
        boost::iostreams::mapped_file_source* mappedFile = new boost::iostreams::mapped_file_source();
        mappedFile->open(file);

        if (mappedFile->is_open()) {
            desc.mappedFile = mappedFile;
            desc.begin = (BinFileItem*) mappedFile->data();
            int fileLen = mappedFile->size();
            int itemNum = fileLen / sizeof(BinFileItem);
            desc.end = (BinFileItem*)((char*)desc.begin + fileLen);
        } 

    } catch (exception &e) {
        ASSERT(false, e.what());
        return false;
    }

    m_dataStreamDescs.push_back(desc);

    vector<BinFileBarFeed*> barFeeds;
    scanBarFeeds(barFeeds, desc, contract);

    stream->setCommonContract(contract);
    stream->setId(desc.id);
    stream->setName(name);
    stream->setResolution(resolution);
    stream->setInterval(interval);
    for (auto& feed : barFeeds) {
        stream->insertBarFeed(feed);
    }

    m_barFeeds.insert(m_barFeeds.end(), barFeeds.begin(), barFeeds.end());

    return (barFeeds.size() > 0);
}

bool BinFileLoader::checkTimeline(BinFileBarFeed* feed)
{
    BinFileItem* begin = feed->m_begin;
    BinFileItem* end = feed->m_end;
    DateTime lastDt = BinFileLoader::getDateTime(begin->date, begin->time);

    int idx = 0;
    // Ending item is also available.
    while (begin <= end) {
        // check timeline
        DateTime dt = BinFileLoader::getDateTime(begin->date, begin->time);
        if (dt < lastDt) {
            char str[256];
            sprintf(str, "\r\nTimeline wrap back, please verify the correctness of your input data."
                         "\r\nFeed:     %s"
                         "\r\nIndex:    %d"
                         "\r\nPrevious: %s" 
                         "\r\nCurrent:  %s",
                feed->getInstrument().c_str(),
                idx,
                lastDt.toString().c_str(),
                dt.toString().c_str());
            Logger_Err() << str;
            exit(-1);
            return false;
        }

        // check price
        if (begin->open <= 0.0 || 
            begin->high <= 0.0 || 
            begin->low <= 0.0 ||
            begin->close <= 0.0 || 
            begin->high < begin->low) {
            char str[256];
            sprintf(str, "\r\nAbnormal price, please verify the correctness of your input data."
                "\r\nFeed:     %s"
                "\r\nIndex:    %d"
                "\r\nDateTime: %s"
                "\r\nPrice:    %0.2f, %0.2f, %0.2f, %0.2f",
                feed->getInstrument().c_str(),
                idx,
                dt.toString().c_str(),
                begin->open, begin->high, begin->low, begin->close);
            Logger_Err() << str;
            exit(-1);
            return false;
        }

        lastDt = dt;
        begin++;
        idx++;
    }

    return true;
}

int BinFileLoader::scanTradablePeriod(BinFileBarFeed* feed)
{
    BinFileItem* begin = feed->m_begin;
    BinFileItem* end = feed->m_end;
    DateTime startDT = BinFileLoader::getDateTime(begin->date, begin->time);
    DateTime endDT = startDT;
    int closeTime = feed->getContract().closeTime;
    // tradable period end at contract's close time.
    if (begin->time > closeTime) {
        endDT = BinFileLoader::getDateTime(begin->date, closeTime);
    }
    
    int lastHotFlag = begin->hot;
    int count = 0;

    while (begin != end) {
        if (begin->hot < 0) {
            if (lastHotFlag >= 0) {
                feed->addTradablePeriod(startDT, endDT);
                count++;
            }
            startDT = BinFileLoader::getDateTime(begin->date, begin->time);
        } else {
            if (lastHotFlag < 0) {
                startDT = BinFileLoader::getDateTime(begin->date, begin->time);
            }
            if (begin->time > closeTime) {
                endDT = BinFileLoader::getDateTime(begin->date, closeTime);
            } else {
                endDT = BinFileLoader::getDateTime(begin->date, begin->time);
            }
        }

        lastHotFlag = begin->hot;
        begin++;
    }

    if (begin->hot >= 0) {
        if (begin->time > closeTime) {
            endDT = BinFileLoader::getDateTime(begin->date, closeTime);
        } else {
            endDT = BinFileLoader::getDateTime(begin->date, begin->time);
        }
        feed->addTradablePeriod(startDT, endDT);
        count++;
    }

    return count;
}

int BinFileLoader::scanBarFeeds(vector<BinFileBarFeed*>& feeds, BinStreamDesc& desc, const Contract& contract)
{
    Logger_Info() << "Scan bar feeds...";

    feeds.clear();
    
    if (m_dataStreamDescs.size() == 0) {
        return 0;
    }

    string instrument;
    
    int currPos = 0;
    int length = 0;
    BinFileBarFeed* barFeed = nullptr;
    DateTime startDT;
    DateTime endDT;
    instrument.clear();

    BinFileItem* item      = desc.begin;
    BinFileItem* lastBegin = item;

    for (; item != desc.end; item++) {
        if (instrument.empty()) {
            instrument = item->instrument;
            startDT    = BinFileLoader::getDateTime(item->date, item->time);
            endDT      = startDT;
        }

        if (item->instrument != instrument) {
            barFeed = new BinFileLoader::BinFileBarFeed(
                                                instrument,
                                                desc.resolution,
                                                lastBegin,
                                                lastBegin + length - 1
                                                );
            barFeed->setId(DataStorage::getNextBarFeedId());
            Contract c = contract;
            strncpy(c.instrument, instrument.c_str(), sizeof(c.instrument));
            barFeed->setContract(c);
            barFeed->setDataStreamId(desc.id);
            barFeed->setName(desc.name);
            barFeed->setLength(length);
            barFeed->setBeginDateTime(startDT);
            barFeed->setEndDateTime(endDT);
            barFeed->setResolution((Bar::Resolution)desc.resolution);
            barFeed->setInterval(desc.interval);
            feeds.push_back(barFeed);

            instrument = item->instrument;
            startDT = BinFileLoader::getDateTime(item->date, item->time);
            endDT = startDT;
            lastBegin = item;
            length = 1;
        } else {
            endDT = BinFileLoader::getDateTime(item->date, item->time);
            length++;
        }
        currPos++;
    }

    // push last one
    barFeed = new BinFileLoader::BinFileBarFeed(
        instrument,
        desc.resolution,
        lastBegin,
        lastBegin + length - 1
        );
    barFeed->setId(DataStorage::getNextBarFeedId());
    Contract c = contract;
    strncpy(c.instrument, instrument.c_str(), sizeof(c.instrument));
    barFeed->setContract(c);
    barFeed->setDataStreamId(desc.id);
    barFeed->setName(desc.name);
    barFeed->setLength(length);
    barFeed->setBeginDateTime(startDT);
    barFeed->setEndDateTime(endDT);
    barFeed->setResolution((Bar::Resolution)desc.resolution);
    barFeed->setInterval(desc.interval);

    feeds.push_back(barFeed);

    Logger_Info() << "Check time line correctness...";
    for (size_t i = 0; i < feeds.size(); i++) {
        if (!checkTimeline((BinFileBarFeed*)feeds[i])) {
            return 0;
        }
    }

    int periods = 0;
    for (size_t i = 0; i < feeds.size(); i++) {
        periods += scanTradablePeriod((BinFileBarFeed*)feeds[i]);
    }

    int size = feeds.size();

    Logger_Info() << "Construct " << size << (size <= 1 ? " barfeed, " : " barfeeds, ") << periods << " tradable " << ( periods > 1 ? "periods." : "period.");

    return size;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace xBacktest