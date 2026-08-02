# DateAndTime.cpp

`include/utils/DateAndTime.cpp`

[← index](../README.md)

## Functions

- [`toYMD(const Date& d)`](#toymd)
- [`fromYMD(const std::chrono::year_month_day& ymd)`](#fromymd)
- [`toSysDays(const Date& d)`](#tosysdays)
- [`breakDown(std::time_t t, bool local, std::tm& out)`](#breakdown)
- [`makeTm(const DateTime& dt)`](#maketm)
- [`fromTm(const std::tm& tm, unsigned milliseconds)`](#fromtm)
- [`pad(long long value, int width)`](#pad)
- [`runLength(std::string_view pattern, std::size_t i)`](#runlength)
- [`readQuoted(std::string_view pattern, std::size_t& i)`](#readquoted)
- [`formatFields(const Date& date, const Time& time, std::string_view pattern)`](#formatfields)
- [`parseFields(std::string_view text, std::string_view pattern, DateTime& out)`](#parsefields)
- [`today()`](#today)
- [`timeOfDay()`](#timeofday)
- [`nowLocal()`](#nowlocal)
- [`nowUTC()`](#nowutc)
- [`timestamp()`](#timestamp)
- [`millisecondsSinceEpoch()`](#millisecondssinceepoch)
- [`timeZoneOffsetMinutes()`](#timezoneoffsetminutes)
- [`isValid(const Date& date)`](#isvalid)
- [`isValid(const Time& time)`](#isvalid)
- [`isValid(const DateTime& dateTime)`](#isvalid)
- [`normalize(const Date& date)`](#normalize)
- [`isLeapYear(int year)`](#isleapyear)
- [`daysInMonth(int year, unsigned month)`](#daysinmonth)
- [`daysInYear(int year)`](#daysinyear)
- [`weekdayOf(const Date& date)`](#weekdayof)
- [`firstWeekdayOfMonth(int year, unsigned month)`](#firstweekdayofmonth)
- [`dayOfYear(const Date& date)`](#dayofyear)
- [`weekOfYear(const Date& date)`](#weekofyear)
- [`isoWeekYear(const Date& date)`](#isoweekyear)
- [`isToday(const Date& date)`](#istoday)
- [`isSameMonth(const Date& a, const Date& b)`](#issamemonth)
- [`isSameWeek(const Date& a, const Date& b, Weekday firstDayOfWeek)`](#issameweek)
- [`isWeekend(const Date& date)`](#isweekend)
- [`isBetween(const Date& date, const Date& first, const Date& last)`](#isbetween)
- [`order(Date& first, Date& last)`](#order)
- [`addDays(const Date& date, int days)`](#adddays)
- [`addWeeks(const Date& date, int weeks)`](#addweeks)
- [`addMonths(const Date& date, int months)`](#addmonths)
- [`addYears(const Date& date, int years)`](#addyears)
- [`addSeconds(const DateTime& dateTime, int64_t seconds)`](#addseconds)
- [`addMinutes(const DateTime& dateTime, int64_t minutes)`](#addminutes)
- [`addHours(const DateTime& dateTime, int64_t hours)`](#addhours)
- [`daysBetween(const Date& from, const Date& to)`](#daysbetween)
- [`secondsBetween(const DateTime& from, const DateTime& to)`](#secondsbetween)
- [`startOfMonth(const Date& date)`](#startofmonth)
- [`endOfMonth(const Date& date)`](#endofmonth)
- [`startOfYear(const Date& date)`](#startofyear)
- [`endOfYear(const Date& date)`](#endofyear)
- [`startOfWeek(const Date& date, Weekday firstDayOfWeek)`](#startofweek)
- [`endOfWeek(const Date& date, Weekday firstDayOfWeek)`](#endofweek)
- [`clamp(...)`](#clamp)
- [`gridStart(int year, unsigned month, Weekday firstDayOfWeek)`](#gridstart)
- [`weeksInMonthGrid(int year, unsigned month, Weekday firstDayOfWeek)`](#weeksinmonthgrid)
- [`columnOf(Weekday day, Weekday firstDayOfWeek)`](#columnof)
- [`weekdayInColumn(unsigned index, Weekday firstDayOfWeek)`](#weekdayincolumn)
- [`monthName(unsigned month, bool abbreviated)`](#monthname)
- [`weekdayName(Weekday day, bool abbreviated)`](#weekdayname)
- [`weekdayInitial(Weekday day)`](#weekdayinitial)
- [`toDaysSinceEpoch(const Date& date)`](#todayssinceepoch)
- [`fromDaysSinceEpoch(int64_t days)`](#fromdayssinceepoch)
- [`dateFromTimestampLocal(std::time_t seconds)`](#datefromtimestamplocal)
- [`dateFromTimestampUTC(std::time_t seconds)`](#datefromtimestamputc)
- [`fromTimestampLocal(std::time_t seconds)`](#fromtimestamplocal)
- [`fromTimestampUTC(std::time_t seconds)`](#fromtimestamputc)
- [`toTimestampLocal(const DateTime& dateTime)`](#totimestamplocal)
- [`toTimestampUTC(const DateTime& dateTime)`](#totimestamputc)
- [`format(const Date& date, std::string_view pattern)`](#format)
- [`format(const DateTime& dateTime, std::string_view pattern)`](#format)
- [`format(const Time& time, std::string_view pattern)`](#format)
- [`toISO(const Date& date)`](#toiso)
- [`toISO(const DateTime& dateTime, bool withMilliseconds)`](#toiso)
- [`toISO(const Time& time, bool withSeconds)`](#toiso)
- [`formatDuration(int64_t seconds, bool forceHours)`](#formatduration)
- [`parseDate(std::string_view text, std::string_view pattern)`](#parsedate)
- [`parseDateTime(std::string_view text, std::string_view pattern)`](#parsedatetime)
- [`parseISODate(std::string_view text)`](#parseisodate)
- [`parseISODateTime(std::string_view text)`](#parseisodatetime)

---

### toYMD

```cpp
toYMD(const Date& d)
```

**Parameters**

- `const Date& d`

**Returns** — std::chrono::year_month_day

Bridges to <chrono>'s calendar types, which own all the awkward parts (leap years, month lengths, day-of-week) and get them right.

---

### fromYMD

```cpp
fromYMD(const std::chrono::year_month_day& ymd)
```

**Parameters**

- `const std::chrono::year_month_day& ymd`

**Returns** — [Date](DateAndTime.hpp.md#date)

Builds a [Date](DateAndTime.hpp.md#date) from year, month and day without validating it, for the internal paths that have already checked.

---

### toSysDays

```cpp
toSysDays(const Date& d)
```

**Parameters**

- `const Date& d`

**Returns** — std::chrono::sys_days

Converts a [Date](DateAndTime.hpp.md#date) to a chrono sys_days, which is the form the calendar arithmetic and weekday lookups work in.

---

### breakDown

```cpp
breakDown(std::time_t t, bool local, std::tm& out)
```

**Parameters**

- `std::time_t t`
- `bool local`
- `std::tm& out`

**Returns** — bool

localtime/gmtime are not thread-safe in their plain form and the reentrant spelling differs on Windows, so both are funnelled through here.

---

### makeTm

```cpp
makeTm(const DateTime& dt)
```

**Parameters**

- `const DateTime& dt`

**Returns** — std::tm

Fills a std::tm from a [DateTime](DateAndTime.hpp.md#datetime), for handing to the C formatting and parsing functions.

---

### fromTm

```cpp
fromTm(const std::tm& tm, unsigned milliseconds)
```

**Parameters**

- `const std::tm& tm`
- `unsigned milliseconds`

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Reads a std::tm back into a [DateTime](DateAndTime.hpp.md#datetime).

---

### pad

```cpp
pad(long long value, int width)
```

**Parameters**

- `long long value`
- `int width`

**Returns** — std::string

Left-pads a number with zeros to a fixed width, which is what most of the format tokens need.

---

### runLength

```cpp
runLength(std::string_view pattern, std::size_t i)
```

**Parameters**

- `std::string_view pattern`
- `std::size_t i`

**Returns** — std::size_t

How many times `c` repeats starting at `i`. Tokens are runs of one letter, so this is what decides MM from MMMM.

---

### readQuoted

```cpp
readQuoted(std::string_view pattern, std::size_t& i)
```

**Parameters**

- `std::string_view pattern`
- `std::size_t& i`

**Returns** — std::string

A quoted run is literal text. Returns the literal and advances past the closing quote; a doubled '' is one apostrophe.

---

### formatFields

```cpp
formatFields(const Date& date, const Time& time, std::string_view pattern)
```

**Parameters**

- `const Date& date`
- `const Time& time`
- `std::string_view pattern`

**Returns** — std::string

Expands a format pattern against a date and time. Walks the pattern once, replacing each recognised token and copying everything else through, so literal text needs no escaping unless it happens to spell a token.

---

### parseFields

```cpp
parseFields(std::string_view text, std::string_view pattern, DateTime& out)
```

**Parameters**

- `std::string_view text`
- `std::string_view pattern`
- `DateTime& out`

**Returns** — bool

Fills whichever fields the pattern mentions; anything absent keeps the fallback's value, so a date-only pattern leaves the time at midnight.

---

### today

```cpp
today()
```

**Returns** — [Date](DateAndTime.hpp.md#date)

The current local calendar day.

---

### timeOfDay

```cpp
timeOfDay()
```

**Returns** — [Time](DateAndTime.hpp.md#time)

The current local time of day, including milliseconds.

---

### nowLocal

```cpp
nowLocal()
```

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Current date and time in the machine's local zone. The seconds come from the C library (which owns the zone rules) and the sub- second part from the system clock.

---

### nowUTC

```cpp
nowUTC()
```

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Current date and time in UTC.

---

### timestamp

```cpp
timestamp()
```

**Returns** — std::time_t

Seconds since the Unix epoch.

---

### millisecondsSinceEpoch

```cpp
millisecondsSinceEpoch()
```

**Returns** — int64_t

Milliseconds since the Unix epoch, from the system clock.

---

### timeZoneOffsetMinutes

```cpp
timeZoneOffsetMinutes()
```

**Returns** — int

Minutes to add to UTC for local time, daylight saving included. Derived by breaking one instant down both ways and differencing the results, which avoids both tm_gmtoff (absent on Windows) and the incomplete <chrono> time zone database.

---

### isValid

```cpp
isValid(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — bool

True when the date names a real day on the calendar.

---

### isValid

```cpp
isValid(const Time& time)
```

**Parameters**

- `const Time& time`

**Returns** — bool

True for a 24-hour time of day. Leap seconds are not allowed.

---

### isValid

```cpp
isValid(const DateTime& dateTime)
```

**Parameters**

- `const DateTime& dateTime`

**Returns** — bool

True when both halves are valid.

---

### normalize

```cpp
normalize(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — [Date](DateAndTime.hpp.md#date)

The nearest real date: month into 1-12, then day into that month. Chosen over rolling over into the next month so that stepping a picker between months never skips one.

---

### isLeapYear

```cpp
isLeapYear(int year)
```

**Parameters**

- `int year`

**Returns** — bool

Proleptic Gregorian leap year test.

---

### daysInMonth

```cpp
daysInMonth(int year, unsigned month)
```

**Parameters**

- `int year`
- `unsigned month`

**Returns** — unsigned

Length of the month, February varying with the year. 0 for a month outside 1-12.

---

### daysInYear

```cpp
daysInYear(int year)
```

**Parameters**

- `int year`

**Returns** — unsigned

365, or 366 in a leap year.

---

### weekdayOf

```cpp
weekdayOf(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — [Weekday](DateAndTime.hpp.md#weekday)

Which day of the week the date falls on.

---

### firstWeekdayOfMonth

```cpp
firstWeekdayOfMonth(int year, unsigned month)
```

**Parameters**

- `int year`
- `unsigned month`

**Returns** — [Weekday](DateAndTime.hpp.md#weekday)

[Weekday](DateAndTime.hpp.md#weekday) of the 1st of the month -- the offset a month grid starts at.

---

### dayOfYear

```cpp
dayOfYear(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — unsigned

Ordinal day, 1 for 1 January.

---

### weekOfYear

```cpp
weekOfYear(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — unsigned

ISO 8601 week number. Weeks run Monday to Sunday and belong to the year holding their Thursday, so the first days of January can fall in week 52 or 53 of the year before.

---

### isoWeekYear

```cpp
isoWeekYear(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — int

The year the ISO week number is counted against, which differs from date.year for a few days either side of New Year.

---

### isToday

```cpp
isToday(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — bool

True when the date is the current local day.

---

### isSameMonth

```cpp
isSameMonth(const Date& a, const Date& b)
```

**Parameters**

- `const Date& a`
- `const Date& b`

**Returns** — bool

True when both fall in the same month of the same year.

---

### isSameWeek

```cpp
isSameWeek(const Date& a, const Date& b, Weekday firstDayOfWeek)
```

**Parameters**

- `const Date& a`
- `const Date& b`
- `Weekday firstDayOfWeek`

**Returns** — bool

True when both share a week, as delimited by firstDayOfWeek.

---

### isWeekend

```cpp
isWeekend(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — bool

Saturday or Sunday. Which days count as the weekend is regional; widgets that need another rule should test the weekday themselves.

---

### isBetween

```cpp
isBetween(const Date& date, const Date& first, const Date& last)
```

**Parameters**

- `const Date& date`
- `const Date& first`
- `const Date& last`

**Returns** — bool

Inclusive range test. The bounds may arrive in either order.

---

### order

```cpp
order(Date& first, Date& last)
```

**Parameters**

- `Date& first`
- `Date& last`

**Returns** — void

Swaps the pair if needed so first is the earlier of the two. Range selection collects the two ends in click order, which is not necessarily chronological.

---

### addDays

```cpp
addDays(const Date& date, int days)
```

**Parameters**

- `const Date& date`
- `int days`

**Returns** — [Date](DateAndTime.hpp.md#date)

The date that many days later; negative counts go back.

---

### addWeeks

```cpp
addWeeks(const Date& date, int weeks)
```

**Parameters**

- `const Date& date`
- `int weeks`

**Returns** — [Date](DateAndTime.hpp.md#date)

The same weekday, that many weeks away.

---

### addMonths

```cpp
addMonths(const Date& date, int months)
```

**Parameters**

- `const Date& date`
- `int months`

**Returns** — [Date](DateAndTime.hpp.md#date)

Steps whole months, holding the day of month where the target month is long enough and clamping to its last day otherwise: 31 March back one month is 28 or 29 February, never 2 or 3 March. That keeps a month-by-month walk from skipping a month.

---

### addYears

```cpp
addYears(const Date& date, int years)
```

**Parameters**

- `const Date& date`
- `int years`

**Returns** — [Date](DateAndTime.hpp.md#date)

The same month and day in another year, 29 February clamping to the 28th when the target year is not a leap year.

---

### addSeconds

```cpp
addSeconds(const DateTime& dateTime, int64_t seconds)
```

**Parameters**

- `const DateTime& dateTime`
- `int64_t seconds`

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Shifts by a span of seconds, carrying into the date across midnight. Zone-independent: no DST adjustment is applied.

---

### addMinutes

```cpp
addMinutes(const DateTime& dateTime, int64_t minutes)
```

**Parameters**

- `const DateTime& dateTime`
- `int64_t minutes`

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Shifts by whole minutes.

---

### addHours

```cpp
addHours(const DateTime& dateTime, int64_t hours)
```

**Parameters**

- `const DateTime& dateTime`
- `int64_t hours`

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Shifts by whole hours.

---

### daysBetween

```cpp
daysBetween(const Date& from, const Date& to)
```

**Parameters**

- `const Date& from`
- `const Date& to`

**Returns** — int

Whole days from one date to the other, positive when `to` is the later of the two.

---

### secondsBetween

```cpp
secondsBetween(const DateTime& from, const DateTime& to)
```

**Parameters**

- `const DateTime& from`
- `const DateTime& to`

**Returns** — int64_t

Whole seconds between the two, positive when `to` is later. Both are taken to be in the same zone.

---

### startOfMonth

```cpp
startOfMonth(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — [Date](DateAndTime.hpp.md#date)

The 1st of the date's month.

---

### endOfMonth

```cpp
endOfMonth(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — [Date](DateAndTime.hpp.md#date)

The last day of the date's month.

---

### startOfYear

```cpp
startOfYear(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — [Date](DateAndTime.hpp.md#date)

1 January of the date's year.

---

### endOfYear

```cpp
endOfYear(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — [Date](DateAndTime.hpp.md#date)

31 December of the date's year.

---

### startOfWeek

```cpp
startOfWeek(const Date& date, Weekday firstDayOfWeek)
```

**Parameters**

- `const Date& date`
- `Weekday firstDayOfWeek`

**Returns** — [Date](DateAndTime.hpp.md#date)

Back to the first day of the containing week.

---

### endOfWeek

```cpp
endOfWeek(const Date& date, Weekday firstDayOfWeek)
```

**Parameters**

- `const Date& date`
- `Weekday firstDayOfWeek`

**Returns** — [Date](DateAndTime.hpp.md#date)

Forward to the last day of the containing week.

---

### clamp

```cpp
clamp(...)
```

**Parameters**

- `const Date& date`
- `const std::optional<Date>& min`
- `const std::optional<Date>& max`

**Returns** — [Date](DateAndTime.hpp.md#date)

Pulls the date inside whichever bounds are present.

---

### gridStart

```cpp
gridStart(int year, unsigned month, Weekday firstDayOfWeek)
```

**Parameters**

- `int year`
- `unsigned month`
- `Weekday firstDayOfWeek`

**Returns** — [Date](DateAndTime.hpp.md#date)

The date in the first cell of a month grid: the 1st walked back to the start of its week, so the grid holds whole weeks. Falls in the previous month unless the 1st happens to land on firstDayOfWeek.

---

### weeksInMonthGrid

```cpp
weeksInMonthGrid(int year, unsigned month, Weekday firstDayOfWeek)
```

**Parameters**

- `int year`
- `unsigned month`
- `Weekday firstDayOfWeek`

**Returns** — unsigned

How many week rows the month needs. Usually 5, 6 when the month spills over, and 4 only for a non-leap February beginning exactly on firstDayOfWeek. Widgets wanting a fixed-height grid should just use 6.

---

### columnOf

```cpp
columnOf(Weekday day, Weekday firstDayOfWeek)
```

**Parameters**

- `Weekday day`
- `Weekday firstDayOfWeek`

**Returns** — unsigned

Grid column 0-6 for a weekday, given where the week starts.

---

### weekdayInColumn

```cpp
weekdayInColumn(unsigned index, Weekday firstDayOfWeek)
```

**Parameters**

- `unsigned index`
- `Weekday firstDayOfWeek`

**Returns** — [Weekday](DateAndTime.hpp.md#weekday)

The inverse of columnOf(): which weekday heads a given column.

---

### monthName

```cpp
monthName(unsigned month, bool abbreviated)
```

**Parameters**

- `unsigned month`
- `bool abbreviated`

**Returns** — std::string

English month name, or its first three letters. Empty for a month outside 1-12.

---

### weekdayName

```cpp
weekdayName(Weekday day, bool abbreviated)
```

**Parameters**

- `Weekday day`
- `bool abbreviated`

**Returns** — std::string

English weekday name, or its first three letters.

---

### weekdayInitial

```cpp
weekdayInitial(Weekday day)
```

**Parameters**

- `Weekday day`

**Returns** — std::string

First letter only, for heading a narrow grid column.

---

### toDaysSinceEpoch

```cpp
toDaysSinceEpoch(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — int64_t

Days from 1970-01-01, negative before it. The common currency for date arithmetic here.

---

### fromDaysSinceEpoch

```cpp
fromDaysSinceEpoch(int64_t days)
```

**Parameters**

- `int64_t days`

**Returns** — [Date](DateAndTime.hpp.md#date)

Inverse of toDaysSinceEpoch().

---

### dateFromTimestampLocal

```cpp
dateFromTimestampLocal(std::time_t seconds)
```

**Parameters**

- `std::time_t seconds`

**Returns** — [Date](DateAndTime.hpp.md#date)

The local calendar day containing that instant.

---

### dateFromTimestampUTC

```cpp
dateFromTimestampUTC(std::time_t seconds)
```

**Parameters**

- `std::time_t seconds`

**Returns** — [Date](DateAndTime.hpp.md#date)

The UTC calendar day containing that instant.

---

### fromTimestampLocal

```cpp
fromTimestampLocal(std::time_t seconds)
```

**Parameters**

- `std::time_t seconds`

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Breaks an epoch timestamp down in the local zone.

---

### fromTimestampUTC

```cpp
fromTimestampUTC(std::time_t seconds)
```

**Parameters**

- `std::time_t seconds`

**Returns** — [DateTime](DateAndTime.hpp.md#datetime)

Breaks an epoch timestamp down in UTC.

---

### toTimestampLocal

```cpp
toTimestampLocal(const DateTime& dateTime)
```

**Parameters**

- `const DateTime& dateTime`

**Returns** — std::time_t

Reads the [DateTime](DateAndTime.hpp.md#datetime) as local wall-clock time and returns the instant it names. Milliseconds are dropped. Round-trips with fromTimestampLocal() except across a DST discontinuity, where the C library resolves the ambiguity.

---

### toTimestampUTC

```cpp
toTimestampUTC(const DateTime& dateTime)
```

**Parameters**

- `const DateTime& dateTime`

**Returns** — std::time_t

Reads the [DateTime](DateAndTime.hpp.md#datetime) as UTC and returns the instant it names. Computed from the day count rather than through timegm, which is not portable.

---

### format

```cpp
format(const Date& date, std::string_view pattern)
```

**Parameters**

- `const Date& date`
- `std::string_view pattern`

**Returns** — std::string

Renders the date through the token set documented on the class. [Time](DateAndTime.hpp.md#time) tokens resolve against midnight.

---

### format

```cpp
format(const DateTime& dateTime, std::string_view pattern)
```

**Parameters**

- `const DateTime& dateTime`
- `std::string_view pattern`

**Returns** — std::string

Renders date and time together.

---

### format

```cpp
format(const Time& time, std::string_view pattern)
```

**Parameters**

- `const Time& time`
- `std::string_view pattern`

**Returns** — std::string

Renders a time of day. [Date](DateAndTime.hpp.md#date) tokens resolve against the epoch, so a pattern mixing the two is a mistake worth noticing.

---

### toISO

```cpp
toISO(const Date& date)
```

**Parameters**

- `const Date& date`

**Returns** — std::string

ISO 8601 calendar date, "2026-07-31".

---

### toISO

```cpp
toISO(const DateTime& dateTime, bool withMilliseconds)
```

**Parameters**

- `const DateTime& dateTime`
- `bool withMilliseconds`

**Returns** — std::string

ISO 8601 date and time joined by 'T'. No zone designator: the [DateTime](DateAndTime.hpp.md#datetime) does not carry one.

---

### toISO

```cpp
toISO(const Time& time, bool withSeconds)
```

**Parameters**

- `const Time& time`
- `bool withSeconds`

**Returns** — std::string

"14:05:09", or "14:05" without seconds.

---

### formatDuration

```cpp
formatDuration(int64_t seconds, bool forceHours)
```

**Parameters**

- `int64_t seconds`
- `bool forceHours`

**Returns** — std::string

A span as "m:ss", or "h:mm:ss" once it reaches an hour (or always, with forceHours). A negative span keeps its sign.

---

### parseDate

```cpp
parseDate(std::string_view text, std::string_view pattern)
```

**Parameters**

- `std::string_view text`
- `std::string_view pattern`

**Returns** — std::optional&lt;[Date](DateAndTime.hpp.md#date)&gt;

Reads a date written in the given pattern. The whole string must be consumed and the result must be a real date, so a partial or nonsense match reports nothing rather than a plausible-looking wrong answer.

---

### parseDateTime

```cpp
parseDateTime(std::string_view text, std::string_view pattern)
```

**Parameters**

- `std::string_view text`
- `std::string_view pattern`

**Returns** — std::optional&lt;[DateTime](DateAndTime.hpp.md#datetime)&gt;

As parseDate(), for a pattern carrying time tokens as well. Fields the pattern omits stay at midnight.

---

### parseISODate

```cpp
parseISODate(std::string_view text)
```

**Parameters**

- `std::string_view text`

**Returns** — std::optional&lt;[Date](DateAndTime.hpp.md#date)&gt;

Reads "YYYY-MM-DD", ignoring any time part that follows.

---

### parseISODateTime

```cpp
parseISODateTime(std::string_view text)
```

**Parameters**

- `std::string_view text`

**Returns** — std::optional&lt;[DateTime](DateAndTime.hpp.md#datetime)&gt;

Reads "YYYY-MM-DDTHH:MM:SS", accepting a space in place of the 'T', an absent seconds field, and a trailing ".mmm".
