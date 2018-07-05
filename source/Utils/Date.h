#ifndef XBACKTEST_DATE_H
#define XBACKTEST_DATE_H

#include <time.h>
#include <string>
#include <ostream>
#ifdef _WIN32
#include <Windows.h>
#endif
#include "Export.h"

using namespace std;

namespace xBacktest
{

// Day number
typedef int Day;

/* Day's serial number MOD 7;
    WEEKDAY Excel function is the same except for Sunday = 7.
*/
enum Weekday { Sunday    = 1,
                Monday    = 2,
                Tuesday   = 3,
                Wednesday = 4,
                Thursday  = 5,
                Friday    = 6,
                Saturday  = 7,
                Sun = 1,
                Mon = 2,
                Tue = 3,
                Wed = 4,
                Thu = 5,
                Fri = 6,
                Sat = 7
};

// Month names
enum Month {
    January = 1,
    February = 2,
    March = 3,
    April = 4,
    May = 5,
    June = 6,
    July = 7,
    August = 8,
    September = 9,
    October = 10,
    November = 11,
    December = 12,
    Jan = 1,
    Feb = 2,
    Mar = 3,
    Apr = 4,
    Jun = 6,
    Jul = 7,
    Aug = 8,
    Sep = 9,
    Oct = 10,
    Nov = 11,
    Dec = 12
};

// Year number
typedef int Year;

class DllExport Date
{
public:
    // Default constructor returning a null date.
    Date();

    // Constructor taking a serial number as given by Applix or Excel.
    explicit Date(long serialNumber);

    // More traditional constructor.
    Date(Day d, Month m, Year y);

    Weekday weekday() const;
    Day dayOfMonth() const;

    // One-based (Jan 1st = 1)
    Day dayOfYear() const;
    Month month() const;
    Year year() const;
    long serialNumber() const;

    // increments date by the given number of days
    Date& operator+=(long days);
    // decrement date by the given number of days
    Date& operator-=(long days);
    // 1-day pre-increment
    Date& operator++();
    // 1-day post-increment
    Date operator++(int);
    // 1-day pre-decrement
    Date& operator--();
    // 1-day post-decrement
    Date operator--(int);
    // returns a new date incremented by the given number of days
    Date operator+(long days) const;
    // returns a new date decremented by the given number of days
    Date operator-(long days) const;

    // today's date.
    static Date todaysDate();
    // earliest allowed date
    static Date minDate();
    // latest allowed date
    static Date maxDate();
    // whether the given year is a leap one
    static bool isLeap(Year y);
    // last day of the month to which the given date belongs
    static Date endOfMonth(const Date& d);
    // whether a date is the last day of its month
    static bool isEndOfMonth(const Date& d);
    // next given weekday following or equal to the given date
    /* E.g., the Friday following Tuesday, January 15th, 2002
        was January 18th, 2002.

        see http://www.cpearson.com/excel/DateTimeWS.htm
    */
    static Date nextWeekday(const Date& d,
                            Weekday w);

private:
    long serialNumber_;    // Elapsed days from 1900
    static int monthLength(Month m, bool leapYear);
    static int monthOffset(Month m, bool leapYear);
    static long yearOffset(Year y);
    static long minimumSerialNumber();
    static long maximumSerialNumber();
    static void checkSerialNumber(long serialNumber);
};

DllExport long operator-(const Date& d1, const Date& d2);

DllExport bool operator==(const Date& d1, const Date& d2);

DllExport bool operator!=(const Date& d1, const Date& d2);

DllExport bool operator<(const Date& d1, const Date& d2);

DllExport bool operator<=(const Date& d1, const Date& d2);

DllExport bool operator>(const Date& d1, const Date& d2);

DllExport bool operator>=(const Date& d1, const Date& d2);

} // namespace xBacktest

#endif // XBACKTEST_DATE_H