# DateAndTime.hpp

`include/utils/DateAndTime.hpp`

[← index](../README.md)

## Types

- [Weekday](#weekday)
- [Month](#month)
- [Date](#date)
- [Time](#time)
- [DateTime](#datetime)

---

### Weekday

Sunday is 0 to match C's tm_wday and the usual left-hand column of a calendar grid. Widgets that start their week on Monday do so by passing a different firstDayOfWeek, not by renumbering this.

---

### Month

January is 1, so the value is the month number itself. Every function below takes a plain unsigned month for that reason; this enum exists for readability at call sites, not as a separate currency.

---

### Date

A calendar day with no time and no zone attached. Default- constructs to the Unix epoch rather than to zeros, so a default Date is always valid. - Comparison is defaulted, which sorts year, then month, then day -- i.e. chronological order for any valid date.

---

### Time

A time of day, millisecond resolution, no zone attached. Hour is 0-23; the 12-hour split is a formatting concern.

---

### DateTime

A [Date](#date) and a [Time](#time) together. Which zone it is expressed in depends on where it came from: nowLocal() and nowUTC() differ by exactly the offset timeZoneOffsetMinutes() reports.
