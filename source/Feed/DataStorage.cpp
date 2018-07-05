#include "DataStorage.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
unsigned long DataStream::m_nextId = 0;

DataStream::DataStream()
{
    m_id = m_nextId++;
}

DataStream::~DataStream()
{
}

void DataStream::setId(int id)
{
    m_id = id;
}

int DataStream::getId() const
{
    return m_id;
}

void DataStream::setName(const string& name)
{
    m_name = name;
}

const string& DataStream::getName() const
{
    return m_name;
}

void DataStream::setResolution(int resolution)
{
    m_resolution = resolution;
}

void DataStream::setInterval(int interval)
{
    m_interval = interval;
}

void DataStream::setCommonContract(const Contract& contract)
{
    m_commContract = contract;
}

const Contract& DataStream::getCommonContract() const
{
    return m_commContract;
}

void DataStream::insertBarFeed(BarFeed* feed)
{
    m_barFeeds.push_back(feed);
}

vector<BarFeed*>& DataStream::getBarFeeds()
{
    return m_barFeeds;
}

int DataStream::cloneSharedBarFeed(vector<BarFeed*>& feeds)
{
    feeds.clear();

    for (size_t i = 0; i < m_barFeeds.size(); i++) {
        BarFeed* feed = m_barFeeds[i]->clone();
        feeds.push_back(feed);
    }

    return feeds.size();
}

BarFeed* DataStream::cloneSharedBarFeed(const string& instrument)
{
    BarFeed* barFeed = nullptr;
    for (size_t i = 0; i < m_barFeeds.size(); i++) {
        if (m_barFeeds[i]->getInstrument() == instrument) {
            barFeed = m_barFeeds[i]->clone();
            break;
        }
    }

    return barFeed;
}

////////////////////////////////////////////////////////////////////////////////
unsigned long DataStorage::m_nextBarFeedId = 0;
unsigned long DataStorage::m_nextDataStreamId = 0;

DataStorage::DataStorage()
{

}

DataStorage::~DataStorage()
{
    for (auto& stream : m_dataStreams) {
        delete stream.second;
    }

    m_dataStreams.clear();
}

unsigned long DataStorage::getNextBarFeedId()
{
    return ++m_nextBarFeedId;
}

unsigned long DataStorage::getNextDataStreamId()
{
    return ++m_nextDataStreamId;
}

bool DataStorage::loadCsvFile(const string& name, const string& filename, int resolution, int interval, const Contract& contract)
{
    return false;
}

bool DataStorage::loadTsFile(const string& name, const string& filename, int resolution, int interval, const Contract& contract)
{
#if 0
    DataStream* stream = new DataStream();
    bool ret = m_tsFileLoader.loadTsFile(name, resolution, filename, contract, stream);
    if (!ret) {
        delete stream;
        return false;
    }
    
    m_dataStreams.insert(std::make_pair(name, stream));

    return true;
#endif
	return false;
}

bool DataStorage::loadBinFile(const string& name, const string& filename, int resolution, int interval, const Contract& contract)
{
    DataStream* stream = new DataStream();
    bool ret = m_binFileLoader.loadBinFile(name, resolution, interval, filename, contract, stream);
    if (!ret) {
        delete stream;
        return false;
    }

    m_dataStreams.insert(std::make_pair(name, stream));

    return true;
}

bool DataStorage::loadDataStreamFile(const string& name, const string& filename, int format, int resolution, int interval, const Contract& contract)
{
    if (format == DATA_FILE_FORMAT_BIN) {
        return loadBinFile(name,filename, resolution, interval, contract);
    } else if (format == DATA_FILE_FORMAT_CSV) {
        return loadCsvFile(name,filename, resolution, interval, contract);
    } else if (format == DATA_FILE_FORMAT_TS) {
        return loadTsFile(name,filename, resolution, interval, contract);
    }

    return false;
}

DataStream* DataStorage::getDataStream(const string& name)
{
    const auto& itor = m_dataStreams.find(name);
    if (itor != m_dataStreams.end()) {
        return itor->second;
    }

    return nullptr;
}

DataStream* DataStorage::getDataStream(int id)
{
    for (auto& stream : m_dataStreams) {
        if (stream.second->getId() == id) {
            return stream.second;
        }
    }

    return nullptr;
}

int DataStorage::getDataStreamId(const string& name)
{
    const auto& itor = m_dataStreams.find(name);
    if (itor != m_dataStreams.end()) {
        return itor->second->getId();
    }

    return 0;
}

int DataStorage::getAllDataStream(vector<DataStream*>& streams)
{
    streams.clear();
    for (auto& stream : m_dataStreams) {
        streams.push_back(stream.second);
    }

    return streams.size();
}

BarFeed* DataStorage::createSharedBarFeed(const string& instrument, int resolution, int interval)
{
    for (auto& stream : m_dataStreams) {
        for (auto& feed : stream.second->getBarFeeds()) {
            if (feed->getInstrument() == instrument &&
                feed->getResolution() == resolution &&
                feed->getInterval() == interval) {
                return feed->clone();
            }
        }
    }

    return nullptr;
}

} // namespace xBacktest