#include <iostream>

#include "../../Source/Feed/TsFileFeed.h"

using namespace teatime;
using namespace xBacktest;

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input file> <output file>." << std::endl;
        return -1;
    }

    vector<TsFileStorage::TsFileItem> outItems;

    try {
        auto tf = TeaFile<TsFileStorage::TsFileItem>::OpenRead(argv[1]);
        shared_ptr<teatime::FileMapping<TsFileStorage::TsFileItem>> itemArea = tf->OpenReadableMapping();
        if (tf->ItemCount() == 0) {
            return -1;
        }

        TsFileStorage::TsFileItem *item = itemArea->begin();
        teatime::Time currDate = item->Time.Date();
        double open = item->Open;
        double high = item->High;
        double low = item->Low;
        double close = item->Close;
        double volume = item->Volume;

        for (; item != itemArea->end(); ++item) {
            if (currDate != item->Time.Date()) {

                TsFileStorage::TsFileItem fileItem;
                fileItem.Time = currDate;
                fileItem.Open = open;
                fileItem.Close = close;
                fileItem.High = high;
                fileItem.Low = low;
                fileItem.Volume = volume;
                fileItem.AdjClose = 0;
                outItems.push_back(fileItem);

                currDate = item->Time.Date();
                open = item->Open;
                high = item->High;
                low = item->Low;
                close = item->Close;
            } else {
                if (item->High > high) {
                    high = item->High;
                }
                if (item->Low < low) {
                    low = item->Low;
                }
                close = item->Close;
                volume += item->Volume;
            }
        }

        // save last one.
        TsFileStorage::TsFileItem fileItem;
        fileItem.Time = currDate;
        fileItem.Open = open;
        fileItem.Close = close;
        fileItem.High = high;
        fileItem.Low = low;
        fileItem.Volume = volume;
        fileItem.AdjClose = 0;
        outItems.push_back(fileItem);

    } catch (exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    if (outItems.size() > 0) {
        try {
            auto tf = TeaFile<TsFileStorage::TsFileItem>::Create(argv[2]);
            for (int i = 0; i < outItems.size(); i++) {
                tf->Write(outItems[i]);
            }
        } catch (exception &e) {
            std::cout << e.what() << std::endl;
            return -1;
        }
    }

    return 0;
}