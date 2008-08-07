// Copyright 2005 Google Inc.
// All Rights Reserved.
// Author: davem@google.com

// Functions for manipulating date and time

/** Given a string, and base datetime, try to resolve to a time */
function ParseTime(s, basedate) {
  var time = LDP_ParseInputTime(s, null);
  if (!time) { return null; }
  var date = ical_builderCopy(basedate);
  date.hour = time.hour;
  date.minute = time.minute;
  date.second = time.second;
  return date.validateDateTime() ? date.toDateTime() : null;
}

/**
 * Go from a time to a string, and follow the preference rules
 * ie - "10:40pm" or "20:40"
 *
 * <p>If the abbreviated format option is true, then the time
 * will have the minutes omitted if they are 0 and the suffix
 * ommitted if is "am."  This can also be known as "Carl format."
 *
 * @param t {ICAL_DateTime} - the time
 * @param opt_abbreviated {boolean} if the time should appear in
 *           an abbreviated format. DEFAULT: false
 * @return {string} plain text.
 */
function TimeToString(/*ICAL_DateTime*/ t, opt_abbreviated) {
  if (t instanceof ICAL_PartialDateTime) {
    return PartialTimeToString(t, opt_abbreviated);
  }
  AssertType(t, ICAL_DateTime);
  var use24 = GetPrefs(uid).getBool(
      USER_PREFERENCE_KEY_FORMAT_24_HOUR_TIME,
      up_bool(USER_PREFERENCE_DEFAULT_FORMAT_24_HOUR_TIME));
  return (use24
          ? MSG_TIME24HM
          : (opt_abbreviated
             ? (!t.minute ? MSG_TIME12H_ABBREV : MSG_TIME12HM_ABBREV)
             : MSG_TIME12HM)
          )(t.hour, t.minute);
}

function PartialTimeToString(/*ICAL_{Partial,}DateTime*/ t, opt_abbreviated) {
  AssertTrue(t instanceof ICAL_PartialDateTime || t instanceof ICAL_DateTime);
  var use24 = GetPrefs(uid).getBool(
      USER_PREFERENCE_KEY_FORMAT_24_HOUR_TIME,
      up_bool(USER_PREFERENCE_DEFAULT_FORMAT_24_HOUR_TIME));
  return (use24 || undefined == t.hour
          ? MSG_TIME24HM
          : (opt_abbreviated
             ? (0 === t.minute
                ? MSG_TIME12H_ABBREV
                : MSG_TIME12HM_ABBREV)
             : MSG_TIME12HM)
          )(t.hour, t.minute);
}

function DurationToString(/*ICAL_Duration*/ dur, /*boolean*/ opt_abbreviated) {
  // TODO(msamuel): see if we can use this for the DateFormatter stuff too.
  var d = dur.date;
  var h = dur.hour;
  var m = dur.minute;

  if (opt_abbreviated) {
    // for abbreviated form, convert 1 hour, 30 minutes to 1.5 hours.
    if (h && m && !(m % 15)) {
      h += (m / 60);
      m = 0;
    }
    if (d && h && !m && !(h % 6)) {
      d += (h / 24);
      h = 0;
    }
  }

  if (0 !== d) {
    d = (1 !== d
         ? MSG_INTERVAL_DAY_PLURAL(undefined != d ? d : '?')
         : MSG_INTERVAL_DAY_SINGULAR);
  }
  if (0 !== h) {
    h = (1 !== h
         ? MSG_INTERVAL_HOUR_PLURAL(undefined != h ? h : '?')
         : MSG_INTERVAL_HOUR_SINGULAR);
  }
  if (0 !== m) {
    m = (1 !== m
         ? MSG_INTERVAL_MINUTE_PLURAL(undefined != m ? m : '?')
         : MSG_INTERVAL_MINUTE_SINGULAR);
  }
  if (d && m && !h) {  // display hours if date and minute displayed
    h = MSG_INTERVAL_HOUR_PLURAL(h);
  }

  var parts = [];
  if (d) { parts.push(d); }
  if (h) { parts.push(h); }
  if (m) { parts.push(m); }
  if (parts.length) {
    return parts.join(', ');
  } else {
    return MSG_INTERVAL_MINUTE_PLURAL(0);
  }
}

/**
 * Format a date to a string, using the user's preferences.
 * @return {string} e.g. "12/31/2005" or "31/12/2005"
 */
function DateToString(date) {
  AssertTrue(date instanceof ICAL_Date || date instanceof ICAL_DateTime);
  var y = date.year,
      m = date.month,
      d = date.date;
  switch (GetPrefs(uid).get(USER_PREFERENCE_KEY_DATE_FIELD_ORDER)) {
    case DATE_FIELD_ORDER_DMY:
      return d + '/' + m + '/' + y;
    case DATE_FIELD_ORDER_YMD:
      return y + '-' + (m < 10 ? '0' + m : m) + '-' + (d < 10 ? '0' + d : d);
    default:
      return m + '/' + d + '/' + y;
  }
}

/**
 * Format a partial date to a string, using the user's preferences.
 * @return {string} e.g. "12/31/2005" or "31/12/2005"
 */
function PartialDateToString(date) {
  AssertType(date, ICAL_DateValue);
  var d = date.date ? String(date.date) : '??';
  var m = date.month ? String(date.month) : '??';
  var y = undefined !== date.year ? date.year : '????';
  switch (GetPrefs(uid).get(USER_PREFERENCE_KEY_DATE_FIELD_ORDER)) {
    case DATE_FIELD_ORDER_DMY:
      return d + '/' + m + '/' + y;
    case DATE_FIELD_ORDER_YMD:
      return MSG_YEAR_MONTH_DAY(y, (1 === m.length ? '0' + m : m),
                                (1 === d.length ? '0' + d : d));
    default:
      return m + '/' + d + '/' + y;
  }
}

/**
 * Format a date including the weekday, month, and day of month.
 * @return {String}
 */
function FormatDateLong(d, opt_shortName) {
  var dayArray = (opt_shortName) ? CG_MDAYS_OF_THE_WEEK : FULL_WEEKDAYS;
  return MSG_WEEKDAY_DATE(dayArray[d.getDayOfWeek()], FormatDateShort(d));
}

/**
 * Format of date column header which is used in 2-7 day mode
 * @return {String}
 */
function HeaderDate(d) {
  return MSG_WEEKDAY_DATE(
      DayOfWeekHeader(CG_MDAYS_OF_THE_WEEK[d.getDayOfWeek()]),
      FormatDateShort(d));
}

/**
 * Formats a date as "7/29" or "29/7"
 * @param date to format
 */
function FormatDateShort(date) {
  if (LDP_PrefersMonthBeforeDate()) {
    return date.month + '/' + date.date;
  } else {
    return date.date + '/' + date.month;
  }
}

function FormatDateTime(date) {
  return FormatDate(date, ShouldOmitYear(date)) + ' ' + TimeToString(date);
}

/**
 * In 12-hour mode, will format "7:00pm" as "7pm"
 * In 24-hour mode, times will appear as they normally do
 */
function FormatTimeShort(date) {
  var use24 = GetPrefs(uid).getBool(
      USER_PREFERENCE_KEY_FORMAT_24_HOUR_TIME,
      up_bool(USER_PREFERENCE_DEFAULT_FORMAT_24_HOUR_TIME));
  return (use24 ? MSG_TIME24HM : date.minute ? MSG_TIME12HM : MSG_TIME12H
          )(date.hour, date.minute);
}

function ShouldOmitYear(date) {
  return date.year === ICAL_todaysDate.year &&
         Math.abs(date.month - ICAL_todaysDate.month) < 4;
}

/**
 * Formats a date as "Sep 29th 2005"
 *
 * @param {ICAL_Date} date to format
 * @param {boolean} omitYear should we omit the year?
 */
function FormatDate(date, omitYear) {
  var month = USE_NUMERIC_MONTHS ? date.month : MONTHS[date.month];
  if (omitYear) {
    return MSG_MONTH_DAY(month, date.date);
  } else {
    return MSG_MONTH_DAY_YEAR(month, date.date, date.year);
  }
}

/**
 * @param date {ICAL_Date}
 * @return mm dd[, yyyy]
 */
function FormatDateLongMonth(date) {
  var month = USE_NUMERIC_MONTHS
              ? date.month
              : ('ru' != MESSAGE_LOCALE_LANG
                 ? FULL_MONTHS
                 // Russian uses a different form of the noun depending on the
                 // day of the month, so always use the abbreviated form.
                 // See http://b/issue?id=544293
                 : MONTHS)[date.month];
  if (ShouldOmitYear(date)) {
    return MSG_MONTH_DAY_YEAR(month, date.date, date.year);
  } else {
    return MSG_MONTH_DAY(month, date.date);
  }
}

/**
 * Formats a range of days for an underlay.  If the range only spans one day
 * then no end day is shown.
 * @param {ICAL_Temporal} date_start
 * @param {ICAL_Temporal} date_end inclusive.
 */
function FormatDateForUnderlay(date_start, date_end) {
  var yr1 = date_start.year,
      mo1 = date_start.month,
      da1 = date_start.date;

  var yr2 = date_end.year,
      mo2 = date_end.month,
      da2 = date_end.date;

  // If equal then stop here
  if (yr1 === yr2 && mo1 === mo2) {
    if (da1 === da2) {
      return MSG_MONTH_DAY_YEAR(
          USE_NUMERIC_MONTHS ? mo1 : MONTHS[mo1], da1, yr1);
    } else {
      return MSG_SAME_MONTH_DATERANGE(
          USE_NUMERIC_MONTHS ? mo1 : MONTHS[mo1], da1, da2, yr1);
    }
  }

  return (MSG_DATE_SEPARATOR(
        MSG_MONTH_DAY_YEAR(USE_NUMERIC_MONTHS ? mo1 : MONTHS[mo1], da1, yr1),
        MSG_MONTH_DAY_YEAR(USE_NUMERIC_MONTHS ? mo2 : MONTHS[mo2], da2, yr2)));
}

function FormatMonth(year, month) {
  return MSG_MONTH_YEAR(USE_NUMERIC_MONTHS ? month : MONTHS[month], year);
}

function FormatFullMonth(year, month) {
  return MSG_MONTH_YEAR(USE_NUMERIC_MONTHS ? month : FULL_MONTHS[month], year);
}

/**
 * @param {String} dayOfWeekStr of week name
 * @return {String}
 */
function DayOfWeekHeader(dayOfWeekStr) {
  if ('ru' !== MESSAGE_LOCALE_LANG) {
    return dayOfWeekStr;
  } else {
    // uppercase days of the week for Russian only when used as headers
    return dayOfWeekStr.substring(0, 1).toUpperCase() +
      dayOfWeekStr.substring(1);
  }
}
