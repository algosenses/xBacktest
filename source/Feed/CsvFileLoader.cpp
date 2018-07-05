#include <iostream>
#include "Utils.h"
#include "DataStorage.h"
#include "CsvFileLoader.h"
#include "Bar.h"
#include "Logger.h"
#include "Errors.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
CsvFileLoader::CsvBarFeed::CsvBarFeed(int dataStreamId, int resolution)
    : BarFeed(resolution)
{
    m_readIdx = 0;
    m_instrument = "";
    // default resolution
    m_resolution = Bar::MINUTE;
    setId(DataStorage::getNextBarFeedId());
    setDataStreamId(dataStreamId);
    m_bars = make_shared< vector<Bar> >();
    m_csvParser = nullptr;
}

CsvFileLoader::CsvBarFeed::CsvBarFeed(const CsvBarFeed& other)
    : BarFeed(other.getResolution())
{
    m_instrument = other.m_instrument;
    m_resolution = other.m_resolution;
    m_bars = other.m_bars;
    m_readIdx = 0;
    m_csvParser = nullptr;

    setId(DataStorage::getNextBarFeedId());
    setDataStreamId(other.getDataStreamId());
    setName(other.getName());
    setInstrument(other.getInstrument());
    setLength(other.getLength());
    setBeginDateTime(other.getBeginDateTime());
    setEndDateTime(other.getEndDateTime());
}

CsvFileLoader::CsvBarFeed* CsvFileLoader::CsvBarFeed::clone()
{
    return new CsvFileLoader::CsvBarFeed(*this);
}

bool CsvFileLoader::CsvBarFeed::reset()
{
    m_readIdx = 0;
    return true;
}

bool CsvFileLoader::CsvBarFeed::getNextBar(Bar& outBar)
{
    if (m_readIdx < getLength()) {
        outBar = (*m_bars)[m_readIdx];
        m_readIdx++;
        return true;
    }

    return false;
}

const DateTime CsvFileLoader::CsvBarFeed::peekDateTime() const
{
    return (*m_bars)[m_readIdx].getDateTime();
}

bool CsvFileLoader::CsvBarFeed::eof()
{
    return m_readIdx >= getLength();
}

bool CsvFileLoader::CsvBarFeed::loadCsvBarData(int resolution, const string& file)
{
    Logger_Info() << "Read bars from CSV file...";

    /* Declare the variables to be used */
    const char *filename = file.c_str();
    const char field_terminator = ',';
    const char line_terminator = '\n';
    const char enclosure_char = '"';

    if (filename == nullptr) {
        return false;
    }

    if (m_csvParser == nullptr) {
        m_csvParser = new csv_parser();
    }

    if (m_csvParser == nullptr) {
        return false;
    }

    /* Define how many records we're gonna skip. This could be used to skip the column definitions. */
    m_csvParser->set_skip_lines(1);

    /* Specify the file to parse */
    if (!m_csvParser->init(filename)) {
        return false;
    }

    /* Here we tell the parser how to parse the file */
    m_csvParser->set_enclosed_char(enclosure_char, ENCLOSURE_OPTIONAL);

    m_csvParser->set_field_term_char(field_terminator);

    m_csvParser->set_line_term_char(line_terminator);

    unsigned int row_count = 1U;

    vector<Bar>& bars = *m_bars;
    /* Check to see if there are more records, then grab each row one at a time */
    while (m_csvParser->has_more_rows())
    {
        unsigned int i = 0;

        /* Get the record */
        csv_row row = m_csvParser->get_row();

#if 1
        if (row.size() == 8) {
            char* p = (char *)strchr(row[1].c_str(), ' ');
            if (p) {
                *p = '\0';
            }

            vector<string> result;
            Utils::split(row[1], "/", result);
            if (result.size() != 3) {
                continue;
            }

            DateTime datetime(atoi(result[0].c_str()), atoi(result[1].c_str()), atoi(result[2].c_str()));

            string instrument(row[0]);

            if (instrument.empty()) {
                throw std::invalid_argument("Instrument is empty.");
            }

            if (m_instrument.empty()) {
                m_instrument = instrument;
                setInstrument(m_instrument);
            }

            
            double open = atof(row[2].c_str());
            double high = atof(row[3].c_str());
            double low = atof(row[4].c_str());
            double close = atof(row[5].c_str());
            long long volume = atoi(row[6].c_str());
            double amount = atof(row[7].c_str());

            try {
                Bar bar(instrument.c_str(), datetime, open, high, low, close, volume, 0, resolution);
                bar.setAmount(amount);
                bars.push_back(bar);
            }
            catch (std::invalid_argument e) {
                return false;
            }
        }
#else  
        // MyTrader's minute bar format:
        // date,time,open,high,low,close,avg,volume,openint
        // Example:
        // 2014-09-29,14:33,12100,12140,12100,12120,12125,4610,229270
        if (row.size() == 9) {
            string dtStr;
            dtStr.append(row[0]);
            dtStr.append("T");
            dtStr.append(row[1]);
            dtStr.append(":00");
            DateTime datetime(dtStr);
            double open = atof(row[2].c_str());
            double high = atof(row[3].c_str());
            double low = atof(row[4].c_str());
            double close = atof(row[5].c_str());
            int volume = atoi(row[7].c_str());
            int openint = atoi(row[8].c_str());

            try {
                Bar bar(instrument, datetime, open, high, low, close, volume, openint, BaseBar::MINUTE);
                m_barVector.push_back(bar);
            }
            catch (std::invalid_argument e) {
                return false;
            }
        }
#endif

        row_count++;
    }

    // reverse the order
    std::reverse(bars.begin(), bars.end());

    m_readIdx = 0;
    setLength(bars.size());
    if (bars.size() > 0) {
        setBeginDateTime(bars.front().getDateTime());
        setEndDateTime(bars.back().getDateTime());
    }

    delete m_csvParser;
    m_csvParser = nullptr;

    return true;
}

bool CsvFileLoader::CsvBarFeed::loadCsvTickData(const string& instrument, const string& file)
{
    if (m_instrument != "" && m_instrument.compare(instrument)) {
        return false;
    }

    if (instrument.empty()) {
        throw std::invalid_argument("Instrument is empty.");
    }

    m_instrument = instrument;
    setInstrument(m_instrument);

    /* Declare the variables to be used */
    const char *filename = file.c_str();
    const char field_terminator = ',';
    const char line_terminator = '\n';
    const char enclosure_char = '"';

    if (filename == nullptr) {
        return false;
    }

    if (m_csvParser == nullptr) {
        m_csvParser = new csv_parser();
    }

    if (m_csvParser == nullptr) {
        return false;
    }

    /* Define how many records we're gonna skip. This could be used to skip the column definitions. */
//    m_csvParser->set_skip_lines(1);

    /* Specify the file to parse */
    if (!m_csvParser->init(filename)) {
        return false;
    }

    /* Here we tell the parser how to parse the file */
    m_csvParser->set_enclosed_char(enclosure_char, ENCLOSURE_OPTIONAL);

    m_csvParser->set_field_term_char(field_terminator);

    m_csvParser->set_line_term_char(line_terminator);

    unsigned int row_count = 1U;

    vector<Bar>& bars = *m_bars;

    typedef struct {
        const char* name;
        int offset;
    } ColumnIndex;

    // date,time,ms,open,high,low,close,volume,openint, askPrice4,askVolume4 ~ askPrice0,askVolume0, bidPrice0,bidVolume0 ~ bidPirce4,bidVolume4
    ColumnIndex col_idx[] = {
        /* 0  */{ "date", -1 },
        /* 1  */{ "time", -1 },
        /* 2  */{ "ms", -1 },
        /* 3  */{ "open", -1 },
        /* 4  */{ "high", -1 },
        /* 5  */{ "low", -1 },
        /* 6  */{ "close", -1 },
        /* 7  */{ "volume", -1 },
        /* 8  */{ "openint", -1 },
        /* 9  */{ "askPrice0", -1 },
        /* 10 */{ "askVolume0", -1 },
        /* 11 */{ "bidPrice0", -1 },
        /* 12 */{ "bidVolume0", -1 },
    };

    /* Check to see if there are more records, then grab each row one at a time */
    while (m_csvParser->has_more_rows())
    {
        unsigned int i = 0;

        /* Get the record */
        csv_row row = m_csvParser->get_row();

        if (row_count == 1) {
            bool has_title = false;
            for (size_t i = 0; i < row.size(); i++) {
                for (size_t j = 0; j < sizeof(col_idx) / sizeof(col_idx[0]); j++) {
                    if (_stricmp(row[i].c_str(), col_idx[j].name) == 0) {
                        col_idx[j].offset = i;
                        has_title = true;
                    }
                }
            }

            if (!has_title) {
                ASSERT(false, "CSV file has no title!");
                return false;
            }

            if (col_idx[0].offset < 0 || 
                col_idx[1].offset < 0 ||
                col_idx[6].offset < 0 ||
                col_idx[7].offset < 0 ||
                col_idx[8].offset < 0 ||
                col_idx[9].offset < 0 ||
                col_idx[10].offset < 0 ||
                col_idx[11].offset < 0 ||
                col_idx[12].offset < 0) {
                ASSERT(false, "Parse CSV title (first row) error!");
                return false;
            }

            row_count++;
            continue;
        }

        int dateIdx = col_idx[0].offset;
        int timeIdx = col_idx[1].offset;
        

        char temp[16] = { 0 };
        memcpy(temp, row[dateIdx].c_str(), 4);        // Year
        int year = atoi(temp);

        memset(temp, 0, sizeof(temp));
        memcpy(temp, row[dateIdx].c_str() + 4, 2);    // Month
        int month = atoi(temp);

        memset(temp, 0, sizeof(temp));
        memcpy(temp, row[dateIdx].c_str() + 6, 2);    // Day
        int day = atoi(temp);

        memset(temp, 0, sizeof(temp));
        memcpy(temp, row[timeIdx].c_str(), 2);        // Hour
        int hour = atoi(temp);

        memset(temp, 0, sizeof(temp));
        memcpy(temp, row[timeIdx].c_str() + 2, 2);
        int minute = atoi(temp);

        memset(temp, 0, sizeof(temp));
        memcpy(temp, row[timeIdx].c_str() + 4, 2);
        int second = atoi(temp);

        int msIdx = col_idx[2].offset;
        int ms = 0;
        if (msIdx > 0) {
            ms = atoi(row[msIdx].c_str());     // UpdateMs
        }

        DateTime datetime(year, month, day, hour, minute, second, ms);

        double open = 0;
        int openIdx = col_idx[3].offset;
        if (openIdx > 0) {
            open = atof(row[openIdx].c_str());
        }
        
        double high = 0;
        int highIdx = col_idx[4].offset;
        if (highIdx > 0) {
            high = atof(row[highIdx].c_str());
        }
        
        double low = 0;
        int lowIdx = col_idx[5].offset;
        if (lowIdx > 0) {
            low = atof(row[lowIdx].c_str());
        }
        
        double close = 0;
        int closeIdx = col_idx[6].offset;
        if (closeIdx > 0) {
            close = atof(row[closeIdx].c_str());
        }

        long volume = 0;
        int volIdx = col_idx[7].offset;
        if (volIdx > 0) {
            volume = atoi(row[volIdx].c_str());
        }

        long long openInt = 0;
        int openIntIdx = col_idx[8].offset;
        if (openIntIdx > 0) {
            openInt = atoi(row[openIntIdx].c_str());
        }

        double askprice = 0;
        int apIdx = col_idx[9].offset;
        if (apIdx > 0) {
            askprice = atof(row[apIdx].c_str());
        }

        long long askvolume = 0;
        int avIdx = col_idx[10].offset;
        if (avIdx > 0) {
            askvolume = atoi(row[avIdx].c_str());
        }

        double bidprice = 0;
        int bpIdx = col_idx[11].offset;
        if (bpIdx > 0) {
            bidprice = atof(row[bpIdx].c_str());
        }

        long long bidvolume = 0;
        int bvIdx = col_idx[12].offset;
        if (bvIdx > 0) {
            bidvolume = atoi(row[bvIdx].c_str());
        }

        try {
            Bar bar(instrument.c_str(), datetime, open, high, low, close, volume, openInt, Bar::TICK);
            bar.setTickField(close, bidprice, bidvolume, askprice, askvolume);
            bars.push_back(bar);
        }
        catch (std::invalid_argument e) {
            return false;
        }

        row_count++;
    }

//    std::reverse(bars.begin(), bars.end());

    m_readIdx = 0;
    setLength(bars.size());
    if (bars.size() > 0) {
        setBeginDateTime(bars.front().getDateTime());
        setEndDateTime(bars.back().getDateTime());
    }

    delete m_csvParser;
    m_csvParser = nullptr;

    return true;
}


////////////////////////////////////////////////////////////////////////////////
CsvFileLoader::CsvFileLoader()
{
    // Arbitrary number greater than 0.
    m_nextDataStreamId = 1;
}

bool CsvFileLoader::loadCsvFile(const string& instrument, int resolution, string& file, const Contract& contract, DataStream* stream)
{
    Logger_Info() << "Loading CSV file '" << file << "'...";

    if (instrument.empty()) {
        ASSERT(false, "Symbol(name) is empty.");
    }

    for (size_t i = 0; i < m_dataStreamDescs.size(); i++) {
        if (m_dataStreamDescs[i].name == instrument &&
            m_dataStreamDescs[i].resolution == resolution) {
            return false;
        }
    }

    CsvStreamDesc desc;
    desc.id = m_nextDataStreamId++;
    desc.name = instrument;
    desc.resolution = resolution;
    desc.file = file;
    desc.contract = contract;

    CsvBarFeed* feed = new CsvBarFeed(desc.id, (Bar::Resolution)resolution);
    Contract c = contract;
    strncpy(c.instrument, instrument.c_str(), sizeof(c.instrument)-1);
    feed->setContract(c);

    if (resolution != Bar::TICK) {
        feed->loadCsvBarData(resolution, file);
    } else {
        feed->loadCsvTickData(instrument, file);
    }

    m_dataStreamDescs.push_back(desc);
    m_dataStreamDescs.back().barFeeds.push_back(feed);

    stream->setCommonContract(contract);
    stream->setId(desc.id);
    stream->setName(instrument);
    stream->setResolution(resolution);
    stream->insertBarFeed(feed);

    return true;
}

CsvFileLoader::~CsvFileLoader()
{
}

} // namespace xBacktest