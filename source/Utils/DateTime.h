#ifndef DATETIME_H
#define DATETIME_H

#include <time.h>
#include <string>
#include <ostream>
#ifdef _WIN32
#include <Windows.h>
#endif
#include "Date.h"
#include "Export.h"

using namespace std;

namespace xBacktest
{

/*
 * Represents a date convertable among various standard
 * date represenations including RFC 3339 used by JSON encodings.
 */

class DllExport DateTime
{
public:
    DateTime();
    DateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int ms = 0);
    // DateTime String Format: yyyy-mm-ddTHH:MM:SS.ms or yyyy-mm-dd
    // For example: 2014-03-21T11:03:59.3455 or 2014-10-09
	explicit DateTime(const string& date);
    // ticks: elapsed milliseconds since the Epoch
    explicit DateTime(long long ticks);
	DateTime(const DateTime& date);
	~DateTime();
	string toString() const;

	bool isValid() const { return t_.tv_sec != kInvalidEpoch_; }
    /*
	 * Marks this date as being invalid.
     */
	void markInvalid();

	int compare(const DateTime& date) const {
		return t_.tv_sec == date.t_.tv_sec
			? t_.tv_usec - date.t_.tv_usec
			: t_.tv_sec - date.t_.tv_sec;
	}

	bool operator ==(const DateTime& date) const {
		return compare(date) == 0 && date.isValid();
	}

	bool operator <(const DateTime& date) const {
		return compare(date) < 0 && date.isValid();
	}

	bool operator >(const DateTime& date) const {
		return compare(date) > 0 && date.isValid();
	}

	bool operator !=(const DateTime& date) const {
		return compare(date) != 0 && isValid();
	}

	bool operator <=(const DateTime& date) const {
		return compare(date) <= 0 && isValid();
	}

	bool operator >=(const DateTime& date) const {
		return compare(date) >= 0 && date.isValid();
	}

    operator std::string() 
    {
        return toString();
    }

	DateTime& operator=(const DateTime& date) {
		t_ = date.t_;
		return *this;
	}

    DateTime operator -(const DateTime& date) const {
        DateTime dt;
        dt.t_.tv_sec  = t_.tv_sec - date.t_.tv_sec;
        dt.t_.tv_usec = t_.tv_usec - date.t_.tv_usec;
        return dt;
    }

    DateTime date() const {
        DateTime dt;
        dt.t_.tv_usec = 0;
        dt.t_.tv_sec  = t_.tv_sec / (60*60*24) * (60*60*24);
        return dt;
    }

    // Unix time (epoch time) is defined as the number of seconds after 1-1-1970, 00h:00.
    // date = (unix_time/86400 + datenum(1970,1,1))
    // if unix_time is given in seconds, unix_time / 86400 will give the number 
    // of days since Jan. 1st 1970. Add to that the offset used by Matlab's datenum
    // (datenum(0000,1,1) == 1), and you have the amount of days since Jan. 1st, 0000.
    // If you have milliseconds, just use: 
    // date = unix_time/86400/1000 + datenum(1970,1,1)
    #define ELAPSED_DAYS_FROM_0000_TO_1970      (719529)
    #define ELAPSED_DAYS_FROM_0000_TO_1900      (693960)
    #define ELAPSED_DAYS_FROM_1900_TO_1970      (25567)

    Date toDate() const
    {
//        return Date(t_.tv_sec/86400 + 719529);
        // Because our Date class use 1900 as it's start point.
        return Date(t_.tv_sec / 86400 + ELAPSED_DAYS_FROM_1900_TO_1970);
    }

    DateTime yesterday() const {
        DateTime dt;
        dt.t_.tv_usec = 0;
        dt.t_.tv_sec = t_.tv_sec / (60 * 60 * 24) * (60 * 60 * 24);
        dt.t_.tv_sec -= (60 * 60 * 24);
        return dt;
    }

    long secs() const {
        return t_.tv_sec;
    }

    int dateNum() const;

    int timeNum() const {
        int sec = secs() % (24 * 3600);
        if (sec == 0) {
            return 0;
        }

        int hour = sec / (3600);
        int minute = 0;
        if (hour != 0) {
            minute = (sec % (hour * 3600)) / 60;
        } else {
            minute = sec / 60;
        }

        return (hour * 10000 + minute * 100 + secs() % 60);
    }

    int days() const {
        return t_.tv_sec / (60*60*24);
    }

    int year() const;

    int month() const;

    int dayOfMonth() const;

    // milliseconds
    long long ticks() const {
        return ((long long)t_.tv_sec) * 1000 + (t_.tv_usec / 1000);
    }

protected:
	struct timeval t_;
	static const time_t kInvalidEpoch_;
};

DllExport std::ostream& operator<<(std::ostream&, const DateTime&);

} // namespace xBacktest

#endif // DATETIME_H
