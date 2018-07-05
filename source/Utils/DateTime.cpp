#include <iostream>
#include <string.h>
#include <time.h>
//#include "boost/date_time.hpp"
#include "DateTime.h"
#ifdef __GNUC__ 
#include <sys/time.h>
#endif

namespace xBacktest
{

/*
using namespace boost::posix_time;
using namespace boost::gregorian;

const std::locale formats[] = {
    std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S")),
    std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y/%m/%d %H:%M:%S")),
    std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%d.%m.%Y %H:%M:%S")),
    std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d"))
};
const size_t formats_n = sizeof(formats) / sizeof(formats[0]);

boost::int64_t milliseconds_from_epoch(const string& s)
{
    boost::posix_time::ptime pt;
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));
    try {
        for (size_t i = 0; i < formats_n; i++) {
            std::istringstream is(s);
            is.imbue(formats[i]);
            is >> pt;
            if (pt != boost::posix_time::ptime()) {
                break;
            }
        }
    } catch (...) {
        return 0;
    }

    return (pt - epoch).total_milliseconds();
}
*/
// Helper function for parsing time string where strptime isnt available.
// We're hardcoding the calls for what we expect to parse
// Params:
//    skip is a string literal we expect to see before the number (e.g. ':')
//    num_digits is the number of digits we expect to have
//          and the next character after num_digits should not be a digit.
//    *from is the string to parse, updated to point to the first unconsumed
//          character after we've parsed the component
//    value is the value that we've extracted
// Returns true if the string conforms to expectations, false otherwise.
bool ParseIntComponent(
	const char* skip, int num_digits, const char** from, int* value) {
		const char* input = *from;
		for (; *skip && *skip == *input; ++skip, ++input) {
		}
		if (*skip) return false;
		*value = 0;
		for (int i = 0; i < num_digits; ++i) {
			if (!isdigit(input[i])) return false;
			*value = 10 * *value + (input[i] - '0');
		}
		if (isdigit(input[num_digits])) return false;
		*from = input + num_digits;
		return true;
}

#ifdef _MSC_VER
#include <Windows.h>
void SystemTimeToTimeval(
	const SYSTEMTIME& systime, struct timeval* t) {
		FILETIME filetime;
		SystemTimeToFileTime(&systime, &filetime);
		__int64 low = filetime.dwLowDateTime;
		__int64 high = filetime.dwHighDateTime;

		// SystemTime is in 100-nanos. So this is 1/10th of a microsecond.
		const __int64 kMicrosPerSecond = 1000000;
		const __int64 kNanos100PerMicro = 10;
		const __int64 kWindowsEpochDifference = 11644473600LL;  // 1/1/1970 - 1/1/1601
		__int64 nanos100SinceWindowsEpoch = (high << 32 | low);
		__int64 microsSinceWindowsEpoch = nanos100SinceWindowsEpoch / kNanos100PerMicro;
		__int64 secsSinceWindowsEpoch = microsSinceWindowsEpoch / kMicrosPerSecond;
		__int64 secs = secsSinceWindowsEpoch - kWindowsEpochDifference;
		t->tv_sec = secs;
		t->tv_usec = microsSinceWindowsEpoch % kMicrosPerSecond;
}

void gettimeofday(struct timeval* t, void *ignore) {
	SYSTEMTIME systime;
	::GetSystemTime(&systime);
	SystemTimeToTimeval(systime, t);
}

// tm is local time
time_t timegm(struct tm* tm) {
	SYSTEMTIME systime;
	systime.wYear = tm->tm_year + 1900;
	systime.wMonth = tm->tm_mon + 1;
	systime.wDayOfWeek = tm->tm_wday;
	systime.wDay = tm->tm_mday;
	systime.wHour = tm->tm_hour;
	systime.wMinute = tm->tm_min;
	systime.wSecond = tm->tm_sec;
	systime.wMilliseconds = 0;

	struct timeval t;
	SystemTimeToTimeval(systime, &t);
	return t.tv_sec;
}
#endif

typedef unsigned uint;
typedef unsigned long long uint64;

struct tm* SecondsSinceEpochToDateTime(struct tm* pTm, uint64 SecondsSinceEpoch)
{
  uint64 sec;
  uint quadricentennials, centennials, quadrennials, annuals/*1-ennial?*/;
  uint year, leap;
  uint yday, hour, min;
  uint month, mday, wday;
  static const uint daysSinceJan1st[2][13]=
  {
    {0,31,59,90,120,151,181,212,243,273,304,334,365}, // 365 days, non-leap
    {0,31,60,91,121,152,182,213,244,274,305,335,366}  // 366 days, leap
  };
/*
  400 years:

  1st hundred, starting immediately after a leap year that's a multiple of 400:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n n

  2nd hundred:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n n

  3rd hundred:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n n

  4th hundred:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n L <- 97'th leap year every 400 years
*/

  // Re-bias from 1970 to 1601:
  // 1970 - 1601 = 369 = 3*100 + 17*4 + 1 years (incl. 89 leap days) =
  // (3*100*(365+24/100) + 17*4*(365+1/4) + 1*365)*24*3600 seconds
  sec = SecondsSinceEpoch + 11644473600;

  wday = (uint)((sec / 86400 + 1) % 7); // day of week

  // Remove multiples of 400 years (incl. 97 leap days)
  quadricentennials = (uint)(sec / 12622780800ULL); // 400*365.2425*24*3600
  sec %= 12622780800ULL;

  // Remove multiples of 100 years (incl. 24 leap days), can't be more than 3
  // (because multiples of 4*100=400 years (incl. leap days) have been removed)
  centennials = (uint)(sec / 3155673600ULL); // 100*(365+24/100)*24*3600
  if (centennials > 3)
  {
    centennials = 3;
  }
  sec -= centennials * 3155673600ULL;

  // Remove multiples of 4 years (incl. 1 leap day), can't be more than 24
  // (because multiples of 25*4=100 years (incl. leap days) have been removed)
  quadrennials = (uint)(sec / 126230400); // 4*(365+1/4)*24*3600
  if (quadrennials > 24)
  {
    quadrennials = 24;
  }
  sec -= quadrennials * 126230400ULL;

  // Remove multiples of years (incl. 0 leap days), can't be more than 3
  // (because multiples of 4 years (incl. leap days) have been removed)
  annuals = (uint)(sec / 31536000); // 365*24*3600
  if (annuals > 3)
  {
    annuals = 3;
  }
  sec -= annuals * 31536000ULL;

  // Calculate the year and find out if it's leap
  year = 1601 + quadricentennials * 400 + centennials * 100 + quadrennials * 4 + annuals;
  leap = !(year % 4) && (year % 100 || !(year % 400));

  // Calculate the day of the year and the time
  yday = sec / 86400;
  sec %= 86400;
  hour = sec / 3600;
  sec %= 3600;
  min = sec / 60;
  sec %= 60;

  // Calculate the month
  for (mday = month = 1; month < 13; month++)
  {
    if (yday < daysSinceJan1st[leap][month])
    {
      mday += yday - daysSinceJan1st[leap][month - 1];
      break;
    }
  }

  // Fill in C's "struct tm"
  memset(pTm, 0, sizeof(*pTm));
  pTm->tm_sec = sec;          // [0,59]
  pTm->tm_min = min;          // [0,59]
  pTm->tm_hour = hour;        // [0,23]
  pTm->tm_mday = mday;        // [1,31]  (day of month)
  pTm->tm_mon = month - 1;    // [0,11]  (month)
  pTm->tm_year = year - 1900; // 70+     (year since 1900)
  pTm->tm_wday = wday;        // [0,6]   (day since Sunday AKA day of week)
  pTm->tm_yday = yday;        // [0,365] (day since January 1st AKA day of year)
  pTm->tm_isdst = -1;         // daylight saving time flag

  return pTm;
}

inline struct timeval make_timeval(time_t sec, int usec) {
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	return tv;
}

static const struct timeval kInvalidTimeval_ =  make_timeval(-1, 0);
const time_t DateTime::kInvalidEpoch_ = -1;

DateTime::DateTime()
{
	gettimeofday(&t_, NULL);
}

DateTime::DateTime(long long ticks)
{
   t_.tv_sec  = (ticks / 1000);
   t_.tv_usec = (ticks % 1000) * 1000;
}

DateTime::DateTime(const DateTime& date) : t_(date.t_) 
{
}

DateTime::DateTime(int year, int month, int day, int hour, int minute, int second, int ms)
{
    struct tm utc;
    memset(&utc, 0, sizeof(utc));
    utc.tm_year = year;
    utc.tm_mon  = month;
    utc.tm_mday = day;
    utc.tm_hour = hour;
    utc.tm_min  = minute;
    utc.tm_sec  = second;

    utc.tm_year -= 1900;
    --utc.tm_mon;

    t_ = make_timeval(timegm(&utc), ms * 1000);
}


#if 1
DateTime::DateTime(const string& date)
{
	struct tm utc;
	memset(&utc, 0, sizeof(utc));

	const char* remaining = date.c_str();
    bool hasTimeStr = strchr(remaining, 'T') != NULL;
    bool result = false;

    if (ParseIntComponent("", 4, &remaining, &utc.tm_year) &&
        ParseIntComponent("-", 2, &remaining, &utc.tm_mon) &&
        ParseIntComponent("-", 2, &remaining, &utc.tm_mday)) {
        if (hasTimeStr) {
            if (ParseIntComponent("T", 2, &remaining, &utc.tm_hour) &&
                ParseIntComponent(":", 2, &remaining, &utc.tm_min) &&
                ParseIntComponent(":", 2, &remaining, &utc.tm_sec)) {
                result = true;
            }
        } else {
            result = true;
        }
    }

    if (result) {
        utc.tm_year -= 1900;
        --utc.tm_mon;
    } else {
        remaining = NULL;
	}

	int usec = 0;
	if (remaining && *remaining == '.' && isdigit(remaining[1])) {
		int multiple = 1000000;
		for (++remaining; isdigit(*remaining); ++remaining) {
			multiple /= 10;
			usec = 10 * usec + *remaining - '0';
		}
		usec *= multiple;
		t_ = make_timeval(timegm(&utc), usec);
	} else {
        t_ = make_timeval(timegm(&utc), 0);
    }
#if 0
	if (!remaining) {
		MarkInvalid();
	} else if (*remaining == 'Z') {
		if (remaining[1]) {
			MarkInvalid();
		} else {
			t_ = make_timeval(timegm(&utc), usec);
		}
	} else if (*remaining == '+' || *remaining == '-') {
		int hours = 0, mins = 0;
		if (sscanf(remaining + 1, "%02d:%02d", &hours, &mins) != 2
			|| strlen(remaining + 1) != 5
			|| mins < 0 || mins > 59 || hours < 0 || hours > 23) {
				MarkInvalid();
		} else {
			int factor = *remaining == '-' ? 1 : -1;
			time_t adjustment = ((hours * 60) + mins) * 60;
			time_t epoch = timegm(&utc);
			t_ = make_timeval(epoch + factor * adjustment, usec);
		}
	} else {
		MarkInvalid();
	}
#endif	
	if (!isValid()) {
		std::cerr << "Invalid date [" << date << "]";
	}
}
#else
DateTime::DateTime(const string& d)
{
    boost::int64_t t = milliseconds_from_epoch(d);
    t_.tv_sec = t / 1000;
    t_.tv_usec = t % 1000;
}
#endif

DateTime::~DateTime() 
{
}

void DateTime::markInvalid() {
	t_ = kInvalidTimeval_;
}

string DateTime::toString() const {
    struct tm utc = { 0 };
	string frac;
    static char str[256] = { 0 };
	if (t_.tv_usec) {
		// add fraction as either millis or micros depending on resolution we need.
		int micros = t_.tv_usec;
		int millis = micros / 1000;
		if (millis * 1000 == micros) {
			sprintf(str, ".%03d", millis);
			frac = str;
		} else {
			sprintf(str, ".%06d", micros);
			frac = str;
		}
    } else {
        sprintf(str, ".%03d", 0);
        frac = str;
    }

	const time_t t = t_.tv_sec;
#if _WIN32
	gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif

    memset(str, 0, sizeof(str));
	sprintf(str, "%04d-%02d-%02d "
		"%02d:%02d:%02d%s",
		utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
		utc.tm_hour, utc.tm_min, utc.tm_sec, frac.c_str());

	return str;
}

int DateTime::dateNum() const
{
    struct tm t;
    SecondsSinceEpochToDateTime(&t, t_.tv_sec);

    return (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
}

int DateTime::month() const
{
    struct tm t;
    SecondsSinceEpochToDateTime(&t, t_.tv_sec);

    return t.tm_mon + 1;
}

int DateTime::dayOfMonth() const
{
    struct tm t;
    SecondsSinceEpochToDateTime(&t, t_.tv_sec);

    return t.tm_mday;
}

std::ostream& operator<<(std::ostream& out, const DateTime& d)
{
    return out << d.toString();
}

} // namespace xBacktest