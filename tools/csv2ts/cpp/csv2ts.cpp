#include <iostream>
#include <TeaFiles/File.h>
#include <TeaFiles/IO.h>
#include "DateTime.h"
#include "Utils.h"
#include "csv_parser.hpp"

using teatime::TeaFile;

#define CSV_FIELD_NUM           (9)
#define CSV_FIELD_TERMINATOR    ','
#define SKIP_LINES              (2)

typedef unsigned long long int64;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    char   symbol[32];
    int64  datetime;
    double open;
    double high;
    double low;
    double close;
    int64  volume;
    int64  openint;
    //} __attribute__((__packed__)) Bar;
} Bar;
#pragma pack(pop)

static void split(const std::string& s, const std::string& delim, std::vector<std::string>& result)
{
    size_t last = 0;
    size_t index = s.find_first_of(delim, last);
    while (index != std::string::npos) {
        result.push_back(s.substr(last, index - last));
        last = index + 1;
        index = s.find_first_of(delim, last);
    }

    if (index - last > 0) {
        result.push_back(s.substr(last, index - last));
    }
}

bool loadCsvFile(const std::string& file, std::vector<Bar>& result)
{
    const char field_terminator = CSV_FIELD_TERMINATOR;
    const char line_terminator = '\n';
    const char enclosure_char = '"';

    csv_parser parser;

    parser.set_skip_lines(SKIP_LINES);
    if (!parser.init(file.c_str())) {
        std::cout << "Open file " << file << " failed!" << std::endl;
        return false;
    }

    parser.set_enclosed_char(enclosure_char, ENCLOSURE_OPTIONAL);
    parser.set_field_term_char(field_terminator);
    parser.set_line_term_char(line_terminator);

    const string instrument = Utils::getFileBaseName(file);

    int line = 0;

    xBacktest::DateTime start(2005, 1, 1);

    while (parser.has_more_rows()) {
        csv_row row = parser.get_row();
        if (row.size() == 7) {
            xBacktest::DateTime dt(row[0]);
            if (dt <= start) {
                continue;
            }

            double open = atof(row[1].c_str());
            double high = atof(row[2].c_str());
            double low = atof(row[3].c_str());
            double close = atof(row[4].c_str());
            long long volume = atoi(row[5].c_str());
            double amount = atof(row[6].c_str());

            if (open <= 0 || high <= 0 || low <= 0 || close <= 0 || volume < 0 || amount < 0) {
                std::cout << "Invalid data, skip file '" << file << "'..." << std::endl;
                return false;
            }

            Bar bar = { 0 };
            bar.datetime = dt.ticks();
            bar.open = open;
            bar.high = high;
            bar.low = low;
            bar.close = close;
            bar.volume = volume;
            bar.openint = amount;
            strcpy(bar.symbol, instrument.c_str());
            
            result.push_back(bar);
        }
    }

    // XXXX, drop last bar.
    if (result.size() > 0) {
        result.pop_back();
    }

    std::cout << "Load CSV file '" << file << "', read " << result.size() << " bars." << std::endl;

    return true;
}

bool writeBars2TsFile(const std::string& file, std::vector<Bar>& bars)
{
    if (bars.size() == 0) {
        return false;
    }

    std::cout << "Write Bars into TS file '" << file << "'..." << std::endl;

    try {
        auto tf = TeaFile<Bar>::Create(file.c_str());
        for (int i = 0; i < bars.size(); i++) {
            tf->Write(bars[i]);
        }
    }
    catch (exception& e) {
        std::cout << e.what() << std::endl;
        return false;
    }
    
    return true;
}

int directory2ts(const string& path, const string& tsfile)
{
    unsigned long long count = 0;

    vector<string> list;
    Utils::getAllFilesNamesWithinFolder(path, list);
    int size = list.size();
    if (size == 0) {
        return 0;
    }

    vector<Bar> bars;
    try {
        auto tf = TeaFile<Bar>::Create(tsfile.c_str());

        for (size_t i = 0; i < list.size(); i++) {
            const string fullpath = path + '\\' + list[i];

            bars.clear();
            if (loadCsvFile(fullpath, bars)) {
                count += bars.size();
                for (int j = 0; j < bars.size(); j++) {
                    tf->Write(bars[j]);
                }
            }
        }
    }
    catch (exception& e) {
        std::cout << e.what() << std::endl;
        return 0;
    }

    return count;
}

int main_(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input CSV file> <output TS file>." << std::endl;
        return 0;
    }

    vector<Bar> bars;

    if (loadCsvFile(argv[1], bars)) {
        if (!writeBars2TsFile(argv[2], bars)) {
            std::cout << "Write TS file failed!" << std::endl;
            return -1;
        }
    } else {
        std::cout << "Load CSV file failed." << std::endl;
        return -1;
    }

    std::cout << "Write " << bars.size() << " bars into TS file '" << argv[2] << "' done." << std::endl;

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input CSV path> <output TS file>." << std::endl;
        return 0;
    }

    unsigned long long count = directory2ts(argv[1], argv[2]);

    std::cout << "Write " << count << " bars into TS file '" << argv[2] << "' done." << std::endl;

    return 0;
}