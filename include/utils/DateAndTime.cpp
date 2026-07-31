#include "DateAndTime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>

namespace uilo {

namespace {

const std::array<const char*, 12> kMonthNames = {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December"
};

const std::array<const char*, 7> kWeekdayNames = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

// Bridges to <chrono>'s calendar types, which own all the awkward parts (leap
// years, month lengths, day-of-week) and get them right.
std::chrono::year_month_day toYMD(const Date& d) {
    return std::chrono::year_month_day{
        std::chrono::year{d.year},
        std::chrono::month{d.month},
        std::chrono::day{d.day}
    };
}

Date fromYMD(const std::chrono::year_month_day& ymd) {
    return Date{
        static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()),
        static_cast<unsigned>(ymd.day())
    };
}

std::chrono::sys_days toSysDays(const Date& d) {
    return std::chrono::sys_days{toYMD(d)};
}

// localtime/gmtime are not thread-safe in their plain form and the reentrant
// spelling differs on Windows, so both are funnelled through here.
bool breakDown(std::time_t t, bool local, std::tm& out) {
#if defined(_WIN32)
    return local ? localtime_s(&out, &t) == 0
                 : gmtime_s(&out, &t) == 0;
#else
    return local ? localtime_r(&t, &out) != nullptr
                 : gmtime_r(&t, &out) != nullptr;
#endif
}

std::tm makeTm(const DateTime& dt) {
    std::tm tm{};
    tm.tm_year  = dt.date.year - 1900;
    tm.tm_mon   = static_cast<int>(dt.date.month) - 1;
    tm.tm_mday  = static_cast<int>(dt.date.day);
    tm.tm_hour  = static_cast<int>(dt.time.hour);
    tm.tm_min   = static_cast<int>(dt.time.minute);
    tm.tm_sec   = static_cast<int>(dt.time.second);
    tm.tm_isdst = -1;   // let the C library decide whether DST applies
    return tm;
}

DateTime fromTm(const std::tm& tm, unsigned milliseconds = 0) {
    return DateTime{
        Date{ tm.tm_year + 1900,
              static_cast<unsigned>(tm.tm_mon) + 1u,
              static_cast<unsigned>(tm.tm_mday) },
        Time{ static_cast<unsigned>(tm.tm_hour),
              static_cast<unsigned>(tm.tm_min),
              static_cast<unsigned>(tm.tm_sec),
              milliseconds }
    };
}

std::string pad(long long value, int width) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%0*lld", width, value);
    return buf;
}

// ---------------------------------------------------------------------------
// Pattern scanning, shared by format() and parseDate()
// ---------------------------------------------------------------------------

// How many times `c` repeats starting at `i`. Tokens are runs of one letter,
// so this is what decides MM from MMMM.
std::size_t runLength(std::string_view pattern, std::size_t i) {
    const char c = pattern[i];
    std::size_t n = 0;
    while (i + n < pattern.size() && pattern[i + n] == c) ++n;
    return n;
}

// A quoted run is literal text. Returns the literal and advances past the
// closing quote; a doubled '' is one apostrophe.
std::string readQuoted(std::string_view pattern, std::size_t& i) {
    std::string out;
    ++i;    // opening quote
    while (i < pattern.size()) {
        if (pattern[i] == '\'') {
            if (i + 1 < pattern.size() && pattern[i + 1] == '\'') { out += '\''; i += 2; continue; }
            ++i;
            return out;
        }
        out += pattern[i++];
    }
    return out;
}

std::string formatFields(const Date& date, const Time& time, std::string_view pattern) {
    std::string out;
    out.reserve(pattern.size() + 8);

    const Weekday wd = DateAndTime::weekdayOf(date);
    const unsigned hour12 = (time.hour % 12 == 0) ? 12 : time.hour % 12;

    for (std::size_t i = 0; i < pattern.size();) {
        const char c = pattern[i];

        if (c == '\'') { out += readQuoted(pattern, i); continue; }

        const std::size_t n = runLength(pattern, i);
        switch (c) {
            case 'Y':
                if      (n >= 4) out += pad(date.year, 4);
                else if (n == 2) out += pad(((date.year % 100) + 100) % 100, 2);
                else             out += std::to_string(date.year);
                break;
            case 'M':
                if      (n >= 4) out += DateAndTime::monthName(date.month, false);
                else if (n == 3) out += DateAndTime::monthName(date.month, true);
                else if (n == 2) out += pad(date.month, 2);
                else             out += std::to_string(date.month);
                break;
            case 'D':
                if (n >= 2) out += pad(date.day, 2);
                else        out += std::to_string(date.day);
                break;
            case 'd':
                if      (n >= 4) out += DateAndTime::weekdayName(wd, false);
                else if (n == 3) out += DateAndTime::weekdayName(wd, true);
                else             out += DateAndTime::weekdayInitial(wd);
                break;
            case 'H':
                if (n >= 2) out += pad(time.hour, 2);
                else        out += std::to_string(time.hour);
                break;
            case 'h':
                if (n >= 2) out += pad(hour12, 2);
                else        out += std::to_string(hour12);
                break;
            case 'm':
                if (n >= 2) out += pad(time.minute, 2);
                else        out += std::to_string(time.minute);
                break;
            case 's':
                if (n >= 2) out += pad(time.second, 2);
                else        out += std::to_string(time.second);
                break;
            case 'S':
                out += pad(time.millisecond, static_cast<int>(std::min<std::size_t>(n, 3)));
                break;
            case 'A': out += (time.hour < 12 ? "AM" : "PM"); break;
            case 'a': out += (time.hour < 12 ? "am" : "pm"); break;
            default:
                out.append(n, c);
                break;
        }
        i += n;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

struct Parser {
    std::string_view text;
    std::size_t      pos = 0;

    bool done() const { return pos >= text.size(); }

    // Up to maxDigits digits, at least one. Fixed width when exact is set,
    // which is what keeps "202607" from being read as one long year.
    bool number(unsigned maxDigits, bool exact, long long& out) {
        std::size_t start = pos;
        long long value = 0;
        std::size_t count = 0;
        while (pos < text.size() && count < maxDigits
               && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            value = value * 10 + (text[pos] - '0');
            ++pos; ++count;
        }
        if (count == 0) { pos = start; return false; }
        if (exact && count != maxDigits) { pos = start; return false; }
        out = value;
        return true;
    }

    bool signedNumber(unsigned maxDigits, bool exact, long long& out) {
        const bool negative = pos < text.size() && text[pos] == '-';
        if (negative) ++pos;
        if (!number(maxDigits, exact, out)) { if (negative) --pos; return false; }
        if (negative) out = -out;
        return true;
    }

    bool literal(char c) {
        if (pos < text.size() && text[pos] == c) { ++pos; return true; }
        return false;
    }

    // Longest case-insensitive match from the table; yields the 1-based index.
    bool nameFrom(const char* const* names, std::size_t count, unsigned& outIndex) {
        std::size_t bestLen = 0;
        std::size_t bestIdx = 0;
        for (std::size_t i = 0; i < count; ++i) {
            std::string_view name{names[i]};
            if (text.size() - pos < name.size()) continue;
            bool match = true;
            for (std::size_t k = 0; k < name.size(); ++k) {
                const auto a = std::tolower(static_cast<unsigned char>(text[pos + k]));
                const auto b = std::tolower(static_cast<unsigned char>(name[k]));
                if (a != b) { match = false; break; }
            }
            if (match && name.size() > bestLen) { bestLen = name.size(); bestIdx = i + 1; }
        }
        if (bestLen == 0) return false;
        pos += bestLen;
        outIndex = static_cast<unsigned>(bestIdx);
        return true;
    }

    // Abbreviations are the first three letters of the full names, so they get
    // their own pass against a fixed width.
    bool abbreviatedName(const char* const* names, std::size_t count, unsigned& outIndex) {
        if (text.size() - pos < 3) return false;
        for (std::size_t i = 0; i < count; ++i) {
            bool match = true;
            for (std::size_t k = 0; k < 3; ++k) {
                const auto a = std::tolower(static_cast<unsigned char>(text[pos + k]));
                const auto b = std::tolower(static_cast<unsigned char>(names[i][k]));
                if (a != b) { match = false; break; }
            }
            if (match) { pos += 3; outIndex = static_cast<unsigned>(i + 1); return true; }
        }
        return false;
    }
};

// Fills whichever fields the pattern mentions; anything absent keeps the
// fallback's value, so a date-only pattern leaves the time at midnight.
bool parseFields(std::string_view text, std::string_view pattern, DateTime& out) {
    Parser p{text};
    bool pmMarker = false, hasPmMarker = false;
    long long hour12 = -1;

    for (std::size_t i = 0; i < pattern.size();) {
        const char c = pattern[i];

        if (c == '\'') {
            const std::string lit = readQuoted(pattern, i);
            for (char lc : lit) if (!p.literal(lc)) return false;
            continue;
        }

        const std::size_t n = runLength(pattern, i);
        long long value = 0;
        unsigned index  = 0;

        switch (c) {
            case 'Y':
                if (n >= 4) {
                    if (!p.signedNumber(4, true, value)) return false;
                    out.date.year = static_cast<int>(value);
                } else if (n == 2) {
                    if (!p.number(2, true, value)) return false;
                    // Two digits are ambiguous by nature; the usual 1969/2068
                    // split is the least surprising resolution.
                    out.date.year = static_cast<int>(value >= 69 ? 1900 + value : 2000 + value);
                } else {
                    if (!p.signedNumber(6, false, value)) return false;
                    out.date.year = static_cast<int>(value);
                }
                break;
            case 'M':
                if (n >= 4) {
                    if (!p.nameFrom(kMonthNames.data(), kMonthNames.size(), index)) return false;
                    out.date.month = index;
                } else if (n == 3) {
                    if (!p.abbreviatedName(kMonthNames.data(), kMonthNames.size(), index)) return false;
                    out.date.month = index;
                } else {
                    if (!p.number(2, n == 2, value)) return false;
                    out.date.month = static_cast<unsigned>(value);
                }
                break;
            case 'D':
                if (!p.number(2, n >= 2, value)) return false;
                out.date.day = static_cast<unsigned>(value);
                break;
            case 'd':
                // Weekday names carry no information a date needs; consume and
                // discard so "dddd, MMMM D" round-trips.
                if (n >= 4) { if (!p.nameFrom(kWeekdayNames.data(), kWeekdayNames.size(), index)) return false; }
                else if (n == 3) { if (!p.abbreviatedName(kWeekdayNames.data(), kWeekdayNames.size(), index)) return false; }
                else { if (p.done()) return false; ++p.pos; }
                break;
            case 'H':
                if (!p.number(2, n >= 2, value)) return false;
                out.time.hour = static_cast<unsigned>(value);
                break;
            case 'h':
                if (!p.number(2, n >= 2, value)) return false;
                hour12 = value;
                break;
            case 'm':
                if (!p.number(2, n >= 2, value)) return false;
                out.time.minute = static_cast<unsigned>(value);
                break;
            case 's':
                if (!p.number(2, n >= 2, value)) return false;
                out.time.second = static_cast<unsigned>(value);
                break;
            case 'S':
                if (!p.number(3, false, value)) return false;
                out.time.millisecond = static_cast<unsigned>(value);
                break;
            case 'A':
            case 'a': {
                if (p.text.size() - p.pos < 2) return false;
                const auto c0 = std::tolower(static_cast<unsigned char>(p.text[p.pos]));
                const auto c1 = std::tolower(static_cast<unsigned char>(p.text[p.pos + 1]));
                if (c1 != 'm' || (c0 != 'a' && c0 != 'p')) return false;
                pmMarker    = (c0 == 'p');
                hasPmMarker = true;
                p.pos += 2;
                break;
            }
            default:
                for (std::size_t k = 0; k < n; ++k) if (!p.literal(c)) return false;
                break;
        }
        i += n;
    }

    if (!p.done()) return false;   // trailing junk is a mismatch, not a match

    if (hour12 >= 0) {
        if (hour12 < 1 || hour12 > 12) return false;
        unsigned h = static_cast<unsigned>(hour12) % 12;
        if (hasPmMarker && pmMarker) h += 12;
        out.time.hour = h;
    }
    return true;
}

} // namespace


// ---------------------------------------------------------------------------
// Now
// ---------------------------------------------------------------------------

/*
    today():
    - Params:   none
    - Returns:  Date
    - Desc:     The current local calendar day.
*/
Date DateAndTime::today() {
    return nowLocal().date;
}


/*
    timeOfDay():
    - Params:   none
    - Returns:  Time
    - Desc:     The current local time of day, including milliseconds.
*/
Time DateAndTime::timeOfDay() {
    return nowLocal().time;
}


/*
    nowLocal():
    - Params:   none
    - Returns:  DateTime
    - Desc:     Current date and time in the machine's local zone. The seconds
                come from the C library (which owns the zone rules) and the
                sub-second part from the system clock.
*/
DateTime DateAndTime::nowLocal() {
    const int64_t ms = millisecondsSinceEpoch();
    const std::time_t seconds = static_cast<std::time_t>(ms / 1000);

    std::tm tm{};
    if (!breakDown(seconds, true, tm)) return {};
    return fromTm(tm, static_cast<unsigned>(((ms % 1000) + 1000) % 1000));
}


/*
    nowUTC():
    - Params:   none
    - Returns:  DateTime
    - Desc:     Current date and time in UTC.
*/
DateTime DateAndTime::nowUTC() {
    const int64_t ms = millisecondsSinceEpoch();
    const std::time_t seconds = static_cast<std::time_t>(ms / 1000);

    std::tm tm{};
    if (!breakDown(seconds, false, tm)) return {};
    return fromTm(tm, static_cast<unsigned>(((ms % 1000) + 1000) % 1000));
}


/*
    timestamp():
    - Params:   none
    - Returns:  std::time_t
    - Desc:     Seconds since the Unix epoch.
*/
std::time_t DateAndTime::timestamp() {
    return static_cast<std::time_t>(millisecondsSinceEpoch() / 1000);
}


/*
    millisecondsSinceEpoch():
    - Params:   none
    - Returns:  int64_t
    - Desc:     Milliseconds since the Unix epoch, from the system clock.
*/
int64_t DateAndTime::millisecondsSinceEpoch() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


/*
    timeZoneOffsetMinutes():
    - Params:   none
    - Returns:  int
    - Desc:     Minutes to add to UTC for local time, daylight saving included.
                Derived by breaking one instant down both ways and differencing
                the results, which avoids both tm_gmtoff (absent on Windows)
                and the incomplete <chrono> time zone database.
*/
int DateAndTime::timeZoneOffsetMinutes() {
    const std::time_t t = timestamp();

    std::tm localTm{}, utcTm{};
    if (!breakDown(t, true, localTm))  return 0;
    if (!breakDown(t, false, utcTm))   return 0;

    const int64_t localDays = toDaysSinceEpoch(fromTm(localTm).date);
    const int64_t utcDays   = toDaysSinceEpoch(fromTm(utcTm).date);

    const int64_t localMinutes = localDays * 1440 + localTm.tm_hour * 60 + localTm.tm_min;
    const int64_t utcMinutes   = utcDays   * 1440 + utcTm.tm_hour   * 60 + utcTm.tm_min;
    return static_cast<int>(localMinutes - utcMinutes);
}


// ---------------------------------------------------------------------------
// Validity
// ---------------------------------------------------------------------------

/*
    isValid(const Date& date):
    - Params:   const Date& date
    - Returns:  bool
    - Desc:     True when the date names a real day on the calendar.
*/
bool DateAndTime::isValid(const Date& date) {
    return toYMD(date).ok();
}


/*
    isValid(const Time& time):
    - Params:   const Time& time
    - Returns:  bool
    - Desc:     True for a 24-hour time of day. Leap seconds are not allowed.
*/
bool DateAndTime::isValid(const Time& time) {
    return time.hour < 24 && time.minute < 60 && time.second < 60 && time.millisecond < 1000;
}


/*
    isValid(const DateTime& dateTime):
    - Params:   const DateTime& dateTime
    - Returns:  bool
    - Desc:     True when both halves are valid.
*/
bool DateAndTime::isValid(const DateTime& dateTime) {
    return isValid(dateTime.date) && isValid(dateTime.time);
}


/*
    normalize(const Date& date):
    - Params:   const Date& date
    - Returns:  Date
    - Desc:     The nearest real date: month into 1-12, then day into that
                month. Chosen over rolling over into the next month so that
                stepping a picker between months never skips one.
*/
Date DateAndTime::normalize(const Date& date) {
    Date out = date;
    out.month = std::clamp(out.month, 1u, 12u);
    if (out.day < 1) out.day = 1;
    const unsigned last = daysInMonth(out.year, out.month);
    if (out.day > last) out.day = last;
    return out;
}


// ---------------------------------------------------------------------------
// Calendar facts
// ---------------------------------------------------------------------------

/*
    isLeapYear(int year):
    - Params:   int year
    - Returns:  bool
    - Desc:     Proleptic Gregorian leap year test.
*/
bool DateAndTime::isLeapYear(int year) {
    return std::chrono::year{year}.is_leap();
}


/*
    daysInMonth(int year, unsigned month):
    - Params:   int year, unsigned month
    - Returns:  unsigned
    - Desc:     Length of the month, February varying with the year. 0 for a
                month outside 1-12.
*/
unsigned DateAndTime::daysInMonth(int year, unsigned month) {
    if (month < 1 || month > 12) return 0;
    const std::chrono::year_month_day_last last{
        std::chrono::year{year},
        std::chrono::month_day_last{std::chrono::month{month}}
    };
    return static_cast<unsigned>(last.day());
}


/*
    daysInYear(int year):
    - Params:   int year
    - Returns:  unsigned
    - Desc:     365, or 366 in a leap year.
*/
unsigned DateAndTime::daysInYear(int year) {
    return isLeapYear(year) ? 366u : 365u;
}


/*
    weekdayOf(const Date& date):
    - Params:   const Date& date
    - Returns:  Weekday
    - Desc:     Which day of the week the date falls on.
*/
Weekday DateAndTime::weekdayOf(const Date& date) {
    const std::chrono::weekday wd{toSysDays(date)};
    return static_cast<Weekday>(wd.c_encoding());
}


/*
    firstWeekdayOfMonth(int year, unsigned month):
    - Params:   int year, unsigned month
    - Returns:  Weekday
    - Desc:     Weekday of the 1st of the month -- the offset a month grid
                starts at.
*/
Weekday DateAndTime::firstWeekdayOfMonth(int year, unsigned month) {
    return weekdayOf(Date{year, month, 1});
}


/*
    dayOfYear(const Date& date):
    - Params:   const Date& date
    - Returns:  unsigned
    - Desc:     Ordinal day, 1 for 1 January.
*/
unsigned DateAndTime::dayOfYear(const Date& date) {
    const int64_t first = toDaysSinceEpoch(Date{date.year, 1, 1});
    return static_cast<unsigned>(toDaysSinceEpoch(date) - first + 1);
}


/*
    weekOfYear(const Date& date):
    - Params:   const Date& date
    - Returns:  unsigned
    - Desc:     ISO 8601 week number. Weeks run Monday to Sunday and belong to
                the year holding their Thursday, so the first days of January
                can fall in week 52 or 53 of the year before.
*/
unsigned DateAndTime::weekOfYear(const Date& date) {
    // Thursday of this date's week identifies the owning year outright.
    const int64_t days     = toDaysSinceEpoch(date);
    const unsigned mondayIdx = columnOf(weekdayOf(date), Weekday::Monday);
    const int64_t thursday = days - static_cast<int64_t>(mondayIdx) + 3;
    const Date    thuDate  = fromDaysSinceEpoch(thursday);

    const int64_t jan1     = toDaysSinceEpoch(Date{thuDate.year, 1, 1});
    return static_cast<unsigned>((thursday - jan1) / 7 + 1);
}


/*
    isoWeekYear(const Date& date):
    - Params:   const Date& date
    - Returns:  int
    - Desc:     The year the ISO week number is counted against, which differs
                from date.year for a few days either side of New Year.
*/
int DateAndTime::isoWeekYear(const Date& date) {
    const int64_t days       = toDaysSinceEpoch(date);
    const unsigned mondayIdx = columnOf(weekdayOf(date), Weekday::Monday);
    return fromDaysSinceEpoch(days - static_cast<int64_t>(mondayIdx) + 3).year;
}


// ---------------------------------------------------------------------------
// Relationships
// ---------------------------------------------------------------------------

/*
    isToday(const Date& date):
    - Params:   const Date& date
    - Returns:  bool
    - Desc:     True when the date is the current local day.
*/
bool DateAndTime::isToday(const Date& date) {
    return date == today();
}


/*
    isSameMonth(const Date& a, const Date& b):
    - Params:   const Date& a, const Date& b
    - Returns:  bool
    - Desc:     True when both fall in the same month of the same year.
*/
bool DateAndTime::isSameMonth(const Date& a, const Date& b) {
    return a.year == b.year && a.month == b.month;
}


/*
    isSameWeek(const Date& a, const Date& b, Weekday firstDayOfWeek):
    - Params:   const Date& a, const Date& b, Weekday firstDayOfWeek
    - Returns:  bool
    - Desc:     True when both share a week, as delimited by firstDayOfWeek.
*/
bool DateAndTime::isSameWeek(const Date& a, const Date& b, Weekday firstDayOfWeek) {
    return startOfWeek(a, firstDayOfWeek) == startOfWeek(b, firstDayOfWeek);
}


/*
    isWeekend(const Date& date):
    - Params:   const Date& date
    - Returns:  bool
    - Desc:     Saturday or Sunday. Which days count as the weekend is
                regional; widgets that need another rule should test the
                weekday themselves.
*/
bool DateAndTime::isWeekend(const Date& date) {
    const Weekday wd = weekdayOf(date);
    return wd == Weekday::Saturday || wd == Weekday::Sunday;
}


/*
    isBetween(const Date& date, const Date& first, const Date& last):
    - Params:   const Date& date, const Date& first, const Date& last
    - Returns:  bool
    - Desc:     Inclusive range test. The bounds may arrive in either order.
*/
bool DateAndTime::isBetween(const Date& date, const Date& first, const Date& last) {
    Date lo = first, hi = last;
    order(lo, hi);
    return date >= lo && date <= hi;
}


/*
    order(Date& first, Date& last):
    - Params:   Date& first, Date& last
    - Returns:  void
    - Desc:     Swaps the pair if needed so first is the earlier of the two.
                Range selection collects the two ends in click order, which is
                not necessarily chronological.
*/
void DateAndTime::order(Date& first, Date& last) {
    if (last < first) std::swap(first, last);
}


// ---------------------------------------------------------------------------
// Arithmetic
// ---------------------------------------------------------------------------

/*
    addDays(const Date& date, int days):
    - Params:   const Date& date, int days
    - Returns:  Date
    - Desc:     The date that many days later; negative counts go back.
*/
Date DateAndTime::addDays(const Date& date, int days) {
    return fromDaysSinceEpoch(toDaysSinceEpoch(date) + days);
}


/*
    addWeeks(const Date& date, int weeks):
    - Params:   const Date& date, int weeks
    - Returns:  Date
    - Desc:     The same weekday, that many weeks away.
*/
Date DateAndTime::addWeeks(const Date& date, int weeks) {
    return addDays(date, weeks * 7);
}


/*
    addMonths(const Date& date, int months):
    - Params:   const Date& date, int months
    - Returns:  Date
    - Desc:     Steps whole months, holding the day of month where the target
                month is long enough and clamping to its last day otherwise:
                31 March back one month is 28 or 29 February, never 2 or 3
                March. That keeps a month-by-month walk from skipping a month.
*/
Date DateAndTime::addMonths(const Date& date, int months) {
    // Month as a running count from year 0 so the arithmetic is plain integer
    // work with no wrap-around cases.
    const int64_t total = static_cast<int64_t>(date.year) * 12
                        + static_cast<int64_t>(date.month) - 1
                        + months;

    int      year  = static_cast<int>(total >= 0 ? total / 12 : -((-total + 11) / 12));
    unsigned month = static_cast<unsigned>(total - static_cast<int64_t>(year) * 12) + 1u;

    const unsigned last = daysInMonth(year, month);
    return Date{ year, month, std::min(date.day, last) };
}


/*
    addYears(const Date& date, int years):
    - Params:   const Date& date, int years
    - Returns:  Date
    - Desc:     The same month and day in another year, 29 February clamping
                to the 28th when the target year is not a leap year.
*/
Date DateAndTime::addYears(const Date& date, int years) {
    return addMonths(date, years * 12);
}


/*
    addSeconds(const DateTime& dateTime, int64_t seconds):
    - Params:   const DateTime& dateTime, int64_t seconds
    - Returns:  DateTime
    - Desc:     Shifts by a span of seconds, carrying into the date across
                midnight. Zone-independent: no DST adjustment is applied.
*/
DateTime DateAndTime::addSeconds(const DateTime& dateTime, int64_t seconds) {
    constexpr int64_t kDay = 86400;

    int64_t secondOfDay = static_cast<int64_t>(dateTime.time.hour) * 3600
                        + static_cast<int64_t>(dateTime.time.minute) * 60
                        + static_cast<int64_t>(dateTime.time.second)
                        + seconds;

    // Floored division so a negative shift borrows a day rather than
    // truncating toward zero.
    int64_t dayShift = secondOfDay / kDay;
    secondOfDay     %= kDay;
    if (secondOfDay < 0) { secondOfDay += kDay; --dayShift; }

    DateTime out;
    out.date = fromDaysSinceEpoch(toDaysSinceEpoch(dateTime.date) + dayShift);
    out.time = Time{
        static_cast<unsigned>(secondOfDay / 3600),
        static_cast<unsigned>((secondOfDay % 3600) / 60),
        static_cast<unsigned>(secondOfDay % 60),
        dateTime.time.millisecond
    };
    return out;
}


/*
    addMinutes(const DateTime& dateTime, int64_t minutes):
    - Params:   const DateTime& dateTime, int64_t minutes
    - Returns:  DateTime
    - Desc:     Shifts by whole minutes.
*/
DateTime DateAndTime::addMinutes(const DateTime& dateTime, int64_t minutes) {
    return addSeconds(dateTime, minutes * 60);
}


/*
    addHours(const DateTime& dateTime, int64_t hours):
    - Params:   const DateTime& dateTime, int64_t hours
    - Returns:  DateTime
    - Desc:     Shifts by whole hours.
*/
DateTime DateAndTime::addHours(const DateTime& dateTime, int64_t hours) {
    return addSeconds(dateTime, hours * 3600);
}


/*
    daysBetween(const Date& from, const Date& to):
    - Params:   const Date& from, const Date& to
    - Returns:  int
    - Desc:     Whole days from one date to the other, positive when `to` is
                the later of the two.
*/
int DateAndTime::daysBetween(const Date& from, const Date& to) {
    return static_cast<int>(toDaysSinceEpoch(to) - toDaysSinceEpoch(from));
}


/*
    secondsBetween(const DateTime& from, const DateTime& to):
    - Params:   const DateTime& from, const DateTime& to
    - Returns:  int64_t
    - Desc:     Whole seconds between the two, positive when `to` is later.
                Both are taken to be in the same zone.
*/
int64_t DateAndTime::secondsBetween(const DateTime& from, const DateTime& to) {
    auto absolute = [](const DateTime& dt) -> int64_t {
        return toDaysSinceEpoch(dt.date) * 86400
             + static_cast<int64_t>(dt.time.hour) * 3600
             + static_cast<int64_t>(dt.time.minute) * 60
             + static_cast<int64_t>(dt.time.second);
    };
    return absolute(to) - absolute(from);
}


/*
    startOfMonth(const Date& date):
    - Params:   const Date& date
    - Returns:  Date
    - Desc:     The 1st of the date's month.
*/
Date DateAndTime::startOfMonth(const Date& date) {
    return Date{date.year, date.month, 1};
}


/*
    endOfMonth(const Date& date):
    - Params:   const Date& date
    - Returns:  Date
    - Desc:     The last day of the date's month.
*/
Date DateAndTime::endOfMonth(const Date& date) {
    return Date{date.year, date.month, daysInMonth(date.year, date.month)};
}


/*
    startOfYear(const Date& date):
    - Params:   const Date& date
    - Returns:  Date
    - Desc:     1 January of the date's year.
*/
Date DateAndTime::startOfYear(const Date& date) {
    return Date{date.year, 1, 1};
}


/*
    endOfYear(const Date& date):
    - Params:   const Date& date
    - Returns:  Date
    - Desc:     31 December of the date's year.
*/
Date DateAndTime::endOfYear(const Date& date) {
    return Date{date.year, 12, 31};
}


/*
    startOfWeek(const Date& date, Weekday firstDayOfWeek):
    - Params:   const Date& date, Weekday firstDayOfWeek
    - Returns:  Date
    - Desc:     Back to the first day of the containing week.
*/
Date DateAndTime::startOfWeek(const Date& date, Weekday firstDayOfWeek) {
    return addDays(date, -static_cast<int>(columnOf(weekdayOf(date), firstDayOfWeek)));
}


/*
    endOfWeek(const Date& date, Weekday firstDayOfWeek):
    - Params:   const Date& date, Weekday firstDayOfWeek
    - Returns:  Date
    - Desc:     Forward to the last day of the containing week.
*/
Date DateAndTime::endOfWeek(const Date& date, Weekday firstDayOfWeek) {
    return addDays(startOfWeek(date, firstDayOfWeek), 6);
}


/*
    clamp(const Date& date, const std::optional<Date>& min, const std::optional<Date>& max):
    - Params:   const Date& date, const std::optional<Date>& min,
                const std::optional<Date>& max
    - Returns:  Date
    - Desc:     Pulls the date inside whichever bounds are present.
*/
Date DateAndTime::clamp(const Date& date,
                        const std::optional<Date>& min,
                        const std::optional<Date>& max) {
    Date out = date;
    if (min && out < *min) out = *min;
    if (max && out > *max) out = *max;
    return out;
}


// ---------------------------------------------------------------------------
// Month grids
// ---------------------------------------------------------------------------

/*
    gridStart(int year, unsigned month, Weekday firstDayOfWeek):
    - Params:   int year, unsigned month, Weekday firstDayOfWeek
    - Returns:  Date
    - Desc:     The date in the first cell of a month grid: the 1st walked
                back to the start of its week, so the grid holds whole weeks.
                Falls in the previous month unless the 1st happens to land on
                firstDayOfWeek.
*/
Date DateAndTime::gridStart(int year, unsigned month, Weekday firstDayOfWeek) {
    return startOfWeek(Date{year, month, 1}, firstDayOfWeek);
}


/*
    weeksInMonthGrid(int year, unsigned month, Weekday firstDayOfWeek):
    - Params:   int year, unsigned month, Weekday firstDayOfWeek
    - Returns:  unsigned
    - Desc:     How many week rows the month needs. Usually 5, 6 when the
                month spills over, and 4 only for a non-leap February
                beginning exactly on firstDayOfWeek. Widgets wanting a
                fixed-height grid should just use 6.
*/
unsigned DateAndTime::weeksInMonthGrid(int year, unsigned month, Weekday firstDayOfWeek) {
    const unsigned offset = columnOf(firstWeekdayOfMonth(year, month), firstDayOfWeek);
    const unsigned cells  = offset + daysInMonth(year, month);
    return (cells + 6u) / 7u;
}


/*
    columnOf(Weekday day, Weekday firstDayOfWeek):
    - Params:   Weekday day, Weekday firstDayOfWeek
    - Returns:  unsigned
    - Desc:     Grid column 0-6 for a weekday, given where the week starts.
*/
unsigned DateAndTime::columnOf(Weekday day, Weekday firstDayOfWeek) {
    return (static_cast<unsigned>(day) + 7u - static_cast<unsigned>(firstDayOfWeek)) % 7u;
}


/*
    weekdayInColumn(unsigned index, Weekday firstDayOfWeek):
    - Params:   unsigned index, Weekday firstDayOfWeek
    - Returns:  Weekday
    - Desc:     The inverse of columnOf(): which weekday heads a given column.
*/
Weekday DateAndTime::weekdayInColumn(unsigned index, Weekday firstDayOfWeek) {
    return static_cast<Weekday>((static_cast<unsigned>(firstDayOfWeek) + index) % 7u);
}


// ---------------------------------------------------------------------------
// Names
// ---------------------------------------------------------------------------

/*
    monthName(unsigned month, bool abbreviated):
    - Params:   unsigned month, bool abbreviated
    - Returns:  std::string
    - Desc:     English month name, or its first three letters. Empty for a
                month outside 1-12.
*/
std::string DateAndTime::monthName(unsigned month, bool abbreviated) {
    if (month < 1 || month > 12) return {};
    std::string name = kMonthNames[month - 1];
    if (abbreviated) name.resize(3);
    return name;
}


/*
    weekdayName(Weekday day, bool abbreviated):
    - Params:   Weekday day, bool abbreviated
    - Returns:  std::string
    - Desc:     English weekday name, or its first three letters.
*/
std::string DateAndTime::weekdayName(Weekday day, bool abbreviated) {
    const unsigned index = static_cast<unsigned>(day);
    if (index > 6) return {};
    std::string name = kWeekdayNames[index];
    if (abbreviated) name.resize(3);
    return name;
}


/*
    weekdayInitial(Weekday day):
    - Params:   Weekday day
    - Returns:  std::string
    - Desc:     First letter only, for heading a narrow grid column.
*/
std::string DateAndTime::weekdayInitial(Weekday day) {
    const unsigned index = static_cast<unsigned>(day);
    if (index > 6) return {};
    return std::string(1, kWeekdayNames[index][0]);
}


// ---------------------------------------------------------------------------
// Conversion
// ---------------------------------------------------------------------------

/*
    toDaysSinceEpoch(const Date& date):
    - Params:   const Date& date
    - Returns:  int64_t
    - Desc:     Days from 1970-01-01, negative before it. The common currency
                for date arithmetic here.
*/
int64_t DateAndTime::toDaysSinceEpoch(const Date& date) {
    return toSysDays(date).time_since_epoch().count();
}


/*
    fromDaysSinceEpoch(int64_t days):
    - Params:   int64_t days
    - Returns:  Date
    - Desc:     Inverse of toDaysSinceEpoch().
*/
Date DateAndTime::fromDaysSinceEpoch(int64_t days) {
    using namespace std::chrono;
    return fromYMD(year_month_day{sys_days{std::chrono::days{days}}});
}


/*
    dateFromTimestampLocal(std::time_t seconds):
    - Params:   std::time_t seconds
    - Returns:  Date
    - Desc:     The local calendar day containing that instant.
*/
Date DateAndTime::dateFromTimestampLocal(std::time_t seconds) {
    return fromTimestampLocal(seconds).date;
}


/*
    dateFromTimestampUTC(std::time_t seconds):
    - Params:   std::time_t seconds
    - Returns:  Date
    - Desc:     The UTC calendar day containing that instant.
*/
Date DateAndTime::dateFromTimestampUTC(std::time_t seconds) {
    return fromTimestampUTC(seconds).date;
}


/*
    fromTimestampLocal(std::time_t seconds):
    - Params:   std::time_t seconds
    - Returns:  DateTime
    - Desc:     Breaks an epoch timestamp down in the local zone.
*/
DateTime DateAndTime::fromTimestampLocal(std::time_t seconds) {
    std::tm tm{};
    if (!breakDown(seconds, true, tm)) return {};
    return fromTm(tm);
}


/*
    fromTimestampUTC(std::time_t seconds):
    - Params:   std::time_t seconds
    - Returns:  DateTime
    - Desc:     Breaks an epoch timestamp down in UTC.
*/
DateTime DateAndTime::fromTimestampUTC(std::time_t seconds) {
    std::tm tm{};
    if (!breakDown(seconds, false, tm)) return {};
    return fromTm(tm);
}


/*
    toTimestampLocal(const DateTime& dateTime):
    - Params:   const DateTime& dateTime
    - Returns:  std::time_t
    - Desc:     Reads the DateTime as local wall-clock time and returns the
                instant it names. Milliseconds are dropped. Round-trips with
                fromTimestampLocal() except across a DST discontinuity, where
                the C library resolves the ambiguity.
*/
std::time_t DateAndTime::toTimestampLocal(const DateTime& dateTime) {
    std::tm tm = makeTm(dateTime);
    return std::mktime(&tm);
}


/*
    toTimestampUTC(const DateTime& dateTime):
    - Params:   const DateTime& dateTime
    - Returns:  std::time_t
    - Desc:     Reads the DateTime as UTC and returns the instant it names.
                Computed from the day count rather than through timegm, which
                is not portable.
*/
std::time_t DateAndTime::toTimestampUTC(const DateTime& dateTime) {
    const int64_t seconds = toDaysSinceEpoch(dateTime.date) * 86400
                          + static_cast<int64_t>(dateTime.time.hour) * 3600
                          + static_cast<int64_t>(dateTime.time.minute) * 60
                          + static_cast<int64_t>(dateTime.time.second);
    return static_cast<std::time_t>(seconds);
}


// ---------------------------------------------------------------------------
// Text out
// ---------------------------------------------------------------------------

/*
    format(const Date& date, std::string_view pattern):
    - Params:   const Date& date, std::string_view pattern
    - Returns:  std::string
    - Desc:     Renders the date through the token set documented on the
                class. Time tokens resolve against midnight.
*/
std::string DateAndTime::format(const Date& date, std::string_view pattern) {
    return formatFields(date, Time{}, pattern);
}


/*
    format(const DateTime& dateTime, std::string_view pattern):
    - Params:   const DateTime& dateTime, std::string_view pattern
    - Returns:  std::string
    - Desc:     Renders date and time together.
*/
std::string DateAndTime::format(const DateTime& dateTime, std::string_view pattern) {
    return formatFields(dateTime.date, dateTime.time, pattern);
}


/*
    format(const Time& time, std::string_view pattern):
    - Params:   const Time& time, std::string_view pattern
    - Returns:  std::string
    - Desc:     Renders a time of day. Date tokens resolve against the epoch,
                so a pattern mixing the two is a mistake worth noticing.
*/
std::string DateAndTime::format(const Time& time, std::string_view pattern) {
    return formatFields(Date{}, time, pattern);
}


/*
    toISO(const Date& date):
    - Params:   const Date& date
    - Returns:  std::string
    - Desc:     ISO 8601 calendar date, "2026-07-31".
*/
std::string DateAndTime::toISO(const Date& date) {
    return pad(date.year, 4) + "-" + pad(date.month, 2) + "-" + pad(date.day, 2);
}


/*
    toISO(const DateTime& dateTime, bool withMilliseconds):
    - Params:   const DateTime& dateTime, bool withMilliseconds
    - Returns:  std::string
    - Desc:     ISO 8601 date and time joined by 'T'. No zone designator: the
                DateTime does not carry one.
*/
std::string DateAndTime::toISO(const DateTime& dateTime, bool withMilliseconds) {
    std::string out = toISO(dateTime.date) + "T" + toISO(dateTime.time, true);
    if (withMilliseconds) out += "." + pad(dateTime.time.millisecond, 3);
    return out;
}


/*
    toISO(const Time& time, bool withSeconds):
    - Params:   const Time& time, bool withSeconds
    - Returns:  std::string
    - Desc:     "14:05:09", or "14:05" without seconds.
*/
std::string DateAndTime::toISO(const Time& time, bool withSeconds) {
    std::string out = pad(time.hour, 2) + ":" + pad(time.minute, 2);
    if (withSeconds) out += ":" + pad(time.second, 2);
    return out;
}


/*
    formatDuration(int64_t seconds, bool forceHours):
    - Params:   int64_t seconds, bool forceHours
    - Returns:  std::string
    - Desc:     A span as "m:ss", or "h:mm:ss" once it reaches an hour (or
                always, with forceHours). A negative span keeps its sign.
*/
std::string DateAndTime::formatDuration(int64_t seconds, bool forceHours) {
    const bool negative = seconds < 0;
    int64_t total = negative ? -seconds : seconds;

    const int64_t hours   = total / 3600;
    const int64_t minutes = (total % 3600) / 60;
    const int64_t secs    = total % 60;

    std::string out = negative ? "-" : "";
    if (hours > 0 || forceHours) {
        out += std::to_string(hours) + ":" + pad(minutes, 2);
    } else {
        out += std::to_string(minutes);
    }
    out += ":" + pad(secs, 2);
    return out;
}


// ---------------------------------------------------------------------------
// Text in
// ---------------------------------------------------------------------------

/*
    parseDate(std::string_view text, std::string_view pattern):
    - Params:   std::string_view text, std::string_view pattern
    - Returns:  std::optional<Date>
    - Desc:     Reads a date written in the given pattern. The whole string
                must be consumed and the result must be a real date, so a
                partial or nonsense match reports nothing rather than a
                plausible-looking wrong answer.
*/
std::optional<Date> DateAndTime::parseDate(std::string_view text, std::string_view pattern) {
    DateTime out{};
    if (!parseFields(text, pattern, out)) return std::nullopt;
    if (!isValid(out.date)) return std::nullopt;
    return out.date;
}


/*
    parseDateTime(std::string_view text, std::string_view pattern):
    - Params:   std::string_view text, std::string_view pattern
    - Returns:  std::optional<DateTime>
    - Desc:     As parseDate(), for a pattern carrying time tokens as well.
                Fields the pattern omits stay at midnight.
*/
std::optional<DateTime> DateAndTime::parseDateTime(std::string_view text, std::string_view pattern) {
    DateTime out{};
    if (!parseFields(text, pattern, out)) return std::nullopt;
    if (!isValid(out)) return std::nullopt;
    return out;
}


/*
    parseISODate(std::string_view text):
    - Params:   std::string_view text
    - Returns:  std::optional<Date>
    - Desc:     Reads "YYYY-MM-DD", ignoring any time part that follows.
*/
std::optional<Date> DateAndTime::parseISODate(std::string_view text) {
    if (text.size() > 10) text = text.substr(0, 10);
    return parseDate(text, "YYYY-MM-DD");
}


/*
    parseISODateTime(std::string_view text):
    - Params:   std::string_view text
    - Returns:  std::optional<DateTime>
    - Desc:     Reads "YYYY-MM-DDTHH:MM:SS", accepting a space in place of the
                'T', an absent seconds field, and a trailing ".mmm".
*/
std::optional<DateTime> DateAndTime::parseISODateTime(std::string_view text) {
    static const char* kPatterns[] = {
        "YYYY-MM-DD'T'HH:mm:ss.SSS", "YYYY-MM-DD'T'HH:mm:ss", "YYYY-MM-DD'T'HH:mm",
        "YYYY-MM-DD HH:mm:ss.SSS",   "YYYY-MM-DD HH:mm:ss",   "YYYY-MM-DD HH:mm",
        "YYYY-MM-DD"
    };
    for (const char* pattern : kPatterns)
        if (auto parsed = parseDateTime(text, pattern)) return parsed;
    return std::nullopt;
}

} // namespace uilo
