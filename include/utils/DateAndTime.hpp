#pragma once

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>
#include <string_view>

namespace uilo {

/*
    Weekday:
    - Desc: Sunday is 0 to match C's tm_wday and the usual left-hand column of
            a calendar grid. Widgets that start their week on Monday do so by
            passing a different firstDayOfWeek, not by renumbering this.
*/
enum class Weekday : unsigned {
    Sunday = 0, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday
};


/*
    Month:
    - Desc: January is 1, so the value is the month number itself. Every
            function below takes a plain unsigned month for that reason; this
            enum exists for readability at call sites, not as a separate
            currency.
*/
enum class Month : unsigned {
    January = 1, February, March, April, May, June,
    July, August, September, October, November, December
};


/*
    Date:
    - Desc: A calendar day with no time and no zone attached. Default-
            constructs to the Unix epoch rather than to zeros, so a default
            Date is always valid. - Comparison is defaulted, which sorts year,
            then month, then day -- i.e. chronological order for any valid
            date.
*/
struct Date {
    int      year  = 1970;
    unsigned month = 1;
    unsigned day   = 1;

    constexpr Date() = default;
    constexpr Date(int y, unsigned m, unsigned d) : year(y), month(m), day(d) {}
    constexpr Date(int y, Month m, unsigned d)
        : year(y), month(static_cast<unsigned>(m)), day(d) {}

    friend constexpr bool operator==(const Date&, const Date&) = default;
    friend constexpr auto operator<=>(const Date&, const Date&) = default;
};


/*
    Time:
    - Desc: A time of day, millisecond resolution, no zone attached. Hour is
            0-23; the 12-hour split is a formatting concern.
*/
struct Time {
    unsigned hour        = 0;
    unsigned minute      = 0;
    unsigned second      = 0;
    unsigned millisecond = 0;

    constexpr Time() = default;
    constexpr Time(unsigned h, unsigned mi, unsigned s = 0, unsigned ms = 0)
        : hour(h), minute(mi), second(s), millisecond(ms) {}

    friend constexpr bool operator==(const Time&, const Time&) = default;
    friend constexpr auto operator<=>(const Time&, const Time&) = default;
};


/*
    DateTime:
    - Desc: A Date and a Time together. Which zone it is expressed in depends
            on where it came from: nowLocal() and nowUTC() differ by exactly
            the offset timeZoneOffsetMinutes() reports.
*/
struct DateTime {
    Date date;
    Time time;

    constexpr DateTime() = default;
    constexpr DateTime(const Date& d, const Time& t = {}) : date(d), time(t) {}

    friend constexpr bool operator==(const DateTime&, const DateTime&) = default;
    friend constexpr auto operator<=>(const DateTime&, const DateTime&) = default;
};


/*
    DateAndTime
    - Desc: Everything calendar- and clock-related, in one place: what day it
            is, how long a month is, what weekday a date falls on, date
            arithmetic, and text in both directions. Static functions only,
            like OS -- there is nothing to construct.
    - Calendar math goes through the C++20 <chrono> calendar types, so it is
      correct across leap years and the proleptic Gregorian calendar rather
      than approximate. Nothing here consults the time zone database (which
      libc++ still ships incomplete): the only zone-aware entry points are
      nowLocal(), fromTimestampLocal(), toTimestampLocal(), and
      timeZoneOffsetMinutes(), all of which go through the C library.
    - Formatting uses its own token set rather than strftime, so the same
      pattern string works the same way on every platform:

            YYYY 2026   YY 26                    (year)
            MMMM July   MMM Jul   MM 07   M 7    (month)
            DD 05       D 5                      (day)
            dddd Friday ddd Fri                  (weekday)
            HH 09       H 9                      (hour, 24)
            hh 09       h 9      A PM   a pm     (hour, 12)
            mm 04       m 4                      (minute)
            ss 07       s 7      SSS 042         (second, millis)

      Anything else passes through untouched, and text inside single quotes is
      taken literally ('at' -> at), so a quote-free pattern never surprises
      you: format(d, "YYYY-MM-DD") is "2026-07-31".
*/
class DateAndTime {
public:
    DateAndTime() = delete;

    /* Now. */
    /* Today in local time. */
    static Date     today();
    /* Current local time of day. Milliseconds come from the system clock, so
       this is not just second-resolution. */
    static Time     timeOfDay();
    static DateTime nowLocal();
    static DateTime nowUTC();
    /* Seconds since the Unix epoch. */
    static std::time_t timestamp();
    static int64_t     millisecondsSinceEpoch();
    /* Minutes to add to UTC to get local time: -300 for UTC-5, +60 for UTC+1.
       Reflects daylight saving as it applies right now. */
    static int timeZoneOffsetMinutes();

    /* Validity. */
    /* A real day on the calendar: month 1-12 and day within that month's
       length for. */
    static bool isValid(const Date& date);
    static bool isValid(const Time& time);
    static bool isValid(const DateTime& dateTime);
    /* The nearest real date: month clamped to 1-12, then day clamped to that
       month's length. Leaves an already-valid date alone. */
    static Date normalize(const Date& date);

    /* Calendar facts */
    static bool     isLeapYear(int year);
    static unsigned daysInMonth(int year, unsigned month);
    static unsigned daysInYear(int year);
    static Weekday  weekdayOf(const Date& date);
    static Weekday  firstWeekdayOfMonth(int year, unsigned month);
    /* 1-366. */
    static unsigned dayOfYear(const Date& date);
    /* ISO 8601 week number, 1-53. */
    static unsigned weekOfYear(const Date& date);
    static int      isoWeekYear(const Date& date);

    /* Relationships */
    static bool isToday(const Date& date);
    static bool isSameMonth(const Date& a, const Date& b);
    static bool isSameWeek(const Date& a, const Date& b, Weekday firstDayOfWeek = Weekday::Sunday);
    static bool isWeekend(const Date& date);
    /* Inclusive of both ends. */
    static bool isBetween(const Date& date, const Date& first, const Date& last);
    /* Ordered so that first <= last regardless of which way round they arrive. */
    static void order(Date& first, Date& last);

    /* Arithmetic */
    static Date addDays(const Date& date, int days);
    static Date addWeeks(const Date& date, int weeks);
    /* Keeps the day of month where it can: 31 January plus one month is 28 or
       29 February, not 3 March. */
    static Date addMonths(const Date& date, int months);
    static Date addYears(const Date& date, int years);
    static DateTime addSeconds(const DateTime& dateTime, int64_t seconds);
    static DateTime addMinutes(const DateTime& dateTime, int64_t minutes);
    static DateTime addHours(const DateTime& dateTime, int64_t hours);
    /* Positive when `to` is later than `from`. */
    static int     daysBetween(const Date& from, const Date& to);
    static int64_t secondsBetween(const DateTime& from, const DateTime& to);

    static Date startOfMonth(const Date& date);
    static Date endOfMonth(const Date& date);
    static Date startOfYear(const Date& date);
    static Date endOfYear(const Date& date);
    static Date startOfWeek(const Date& date, Weekday firstDayOfWeek = Weekday::Sunday);
    static Date endOfWeek(const Date& date, Weekday firstDayOfWeek = Weekday::Sunday);
    /* Clamped into [min, max]. Either bound may be left out. */
    static Date clamp(const Date& date,
                      const std::optional<Date>& min,
                      const std::optional<Date>& max);

    /* Month grids (what a calendar widget lays out). */
    /* The date shown in the top-left cell of a month grid: the first day of
       the. */
    static Date gridStart(int year, unsigned month, Weekday firstDayOfWeek = Weekday::Sunday);
    /* Week rows a month needs to fit whole weeks -- 4 (February starting on
       the first day of the week, non-leap), 5, or 6. */
    static unsigned weeksInMonthGrid(int year, unsigned month, Weekday firstDayOfWeek = Weekday::Sunday);
    /* Which column a weekday sits in when the week starts on firstDayOfWeek. */
    static unsigned columnOf(Weekday day, Weekday firstDayOfWeek = Weekday::Sunday);
    /* The weekday shown in column `index`, counting from firstDayOfWeek. */
    static Weekday  weekdayInColumn(unsigned index, Weekday firstDayOfWeek = Weekday::Sunday);

    /* Names. */
    /* English names. */
    static std::string monthName(unsigned month, bool abbreviated = false);
    static std::string weekdayName(Weekday day, bool abbreviated = false);
    /* Just enough to head a grid column: "S", "M", "T"... Ambiguous by design,
       which is what a narrow calendar wants. */
    static std::string weekdayInitial(Weekday day);

    /* Conversion */
    static int64_t toDaysSinceEpoch(const Date& date);
    static Date    fromDaysSinceEpoch(int64_t days);
    static Date    dateFromTimestampLocal(std::time_t seconds);
    static Date    dateFromTimestampUTC(std::time_t seconds);
    static DateTime fromTimestampLocal(std::time_t seconds);
    static DateTime fromTimestampUTC(std::time_t seconds);
    /* Local interpretation, so it round-trips with fromTimestampLocal.
       Milliseconds are dropped. */
    static std::time_t toTimestampLocal(const DateTime& dateTime);
    static std::time_t toTimestampUTC(const DateTime& dateTime);

    /* Text out. */
    /* Token set documented on the class. */
    static std::string format(const Date& date, std::string_view pattern);
    static std::string format(const DateTime& dateTime, std::string_view pattern);
    static std::string format(const Time& time, std::string_view pattern);
    /* "2026-07-31". */
    static std::string toISO(const Date& date);
    /* "2026-07-31T14:05:09". With millis: "2026-07-31T14:05:09.042". */
    static std::string toISO(const DateTime& dateTime, bool withMilliseconds = false);
    /* "14:05:09", or "14:05" when withSeconds is false. */
    static std::string toISO(const Time& time, bool withSeconds = true);
    /* A span of seconds as "1:05" / "2:03:04". Hours appear once the span
       reaches an hour, or always when forceHours is set. */
    static std::string formatDuration(int64_t seconds, bool forceHours = false);

    /* Text in. */
    /* Strict: the whole string must match the pattern, and the result must be. */
    static std::optional<Date>     parseDate(std::string_view text, std::string_view pattern);
    static std::optional<DateTime> parseDateTime(std::string_view text, std::string_view pattern);
    /* "2026-07-31", and the same with a time appended after 'T' or a space. */
    static std::optional<Date>     parseISODate(std::string_view text);
    static std::optional<DateTime> parseISODateTime(std::string_view text);
};

} // namespace uilo
