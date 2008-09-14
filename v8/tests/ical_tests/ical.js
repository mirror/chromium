// Copyright 2004 Google Inc.
// All Rights Reserved.
// Author: msamuel@google.com

/** <h3>Functions for manipulating icalendar data values: dates, times,
  * etc.</h3>
  * --------------------------------
  * <p>This implements several iCal objects.</p><blockquote>
  *   ICAL_Date - specifies a day <br>
  *   ICAL_Time - a time object <br>
  *   ICAL_DateTime - a day and time of day <br>
  *   ICAL_Duration - a delta time <br>
  *   ICAL_DTBuilder - a mutable temporal that can be converted to one of the
  *     other types. <br>
  *   ICAL_PeriodOfTime - a range of DateTimes.
  *
  *   ICAL_PartialDate - a day which may be incomplete<br>
  *   ICAL_PartialDateTime - a day and time of day which may be incomplete<br>
  *   ICAL_PartialPeriodOfTime - a range of ICAL_PartialDateTimes.
  * </blockquote>
  *
  * <h3>TimeZones</h3>
  * ---------
  * <p>All Dates are assumed to have been converted to the display timezone
  * before being passed to the client.</p>x
  *
  * <h3>Class Hierarchy</h3>
  * ---------------
  * <p>ICAL_Temporal - defines a set of calendar fields and field access methods
  * <blockquote>
  *   ICAL_Date extends Temporal <br>
  *   ICAL_Time extends Temporal <br>
  *   ICAL_DateTime extends Temporal <br>
  *   ICAL_Duration extends Temporal <br>
  *   ICAL_DTBuilder extends Temporal</blockquote>
  * <p>ICAL_PeriodOfTime contains 2 DateTimes</p>
  *
  * <h3>Field String Representation</h3>
  * ---------------------------
  * <p>Times are represented as sets of integer fields where fields include
  * year, month, date, hour, minute, second, etc.
  * <br>The string representations are as specified in
  * <a href="http://www.corp.google.com/~msamuel/rfc2445.html">RFC 2045</a></p>
  */

// some common functions
function ICAL_sign(n) { return n < 0 ? -1 : 1; }
function ICAL_trunc(n) { return n | 0; }
function ICAL_exception(msg) {
  DumpError(msg);
  throw msg;
}
function ICAL_fmtInt(n, w) {
  var s = n.toString();
  while (s.length < w) { s = '0' + s; }
  return s;
}
/** parses an int as a decimal number.  @private */
function ICAL_toInt_(s, a, b) {
  return parseInt(s.substring(a, b), 10);
}
/** lookup table used by {@link #ICAL_daysInMonth}. @private */
var ICAL_daysInMonthCache_ = [
    undefined,
    31, undefined, 31, 30,  // feb is computed
    31, 30,        31, 31,
    30, 31,        30, 31];
/** returns the number of days in the given month.  This is consistent with the
  * javascript date object.
  */
function ICAL_daysInMonth(yr, mo) {
  if (2 !== mo) { return ICAL_daysInMonthCache_[mo]; }
  var k = yr << 4;
  var n = ICAL_daysInMonthCache_[k];
  if (!n) {
    n = Math.round((Date.UTC(yr, 2, 1) - Date.UTC(yr, 1, 1)) / (24 * 3600000));
    ICAL_daysInMonthCache_[k] = n;
  }
  return n;
}
function ICAL_daysInYear(year) {
  return ((29 !== ICAL_daysInMonth(year, 2)) ? 365 : 366);
}
var ICAL_firstDayOfWeekInMonthCache_ = new Object();
/** the index of the first day of the week in the given month. */
function ICAL_firstDayOfWeekInMonth(year, month) {
  var k = (year << 4) | month;
  var n = ICAL_firstDayOfWeekInMonthCache_[k];
  if (!n) {
    n = (new Date(year, month - 1, 1, 0, 0, 0, 0)).getDay();
    ICAL_firstDayOfWeekInMonthCache_[k] = n;
  }
  return n;
}
/** the day of the week for the given temporal. */
function ICAL_getDayOfWeek(temporal) {
  return (temporal.date - 1 +
          ICAL_firstDayOfWeekInMonth(temporal.year, temporal.month)) % 7;
}
/** the number of days between two dates specified as (yr, month, dayOfMonth)
  * triplets.
  */
function ICAL_daysBetween(y1, m1, d1, y2, m2, d2) {
  var d;
  if (y1 === y2) {
    if ((d = m1 - m2) === 0) {
      return d1 - d2;
    } else if (d < 0) {
      d = d1 - d2;  // 23
      do {
        d -= ICAL_daysInMonth(y1, m1++);
      } while (m1 < m2);
      return d;
    } else {
      d = d1 - d2;
      do {
        d += ICAL_daysInMonth(y2, m2++);
      } while (m2 < m1);
      return d;
    }
  } else {
    return Math.round((Date.UTC(y1, m1 - 1, d1) - Date.UTC(y2, m2 - 1, d2)) /
                      (24 * 3600 * 1000));
  }
}
/** the number of days between two dates specified as (yr, month, dayOfMonth)
  * triplets.
  */
function ICAL_daysBetweenDates(d1, d2) {
  return ICAL_daysBetween(d1.year, d1.month, d1.date,
                          d2.year, d2.month, d2.date);
}

/** true iff the given date falls on today or less than nDays in the past. */
function ICAL_rangeContainsToday(date, nDays) {
  var k = ICAL_daysBetweenDates(ICAL_todaysDate, date);
  return k >= 0 && k < nDays;
}

function ICAL_dateMin(a, b) {
  return a.getComparable() < b.getComparable() ? a : b;
}
function ICAL_dateMax(a, b) {
  return a.getComparable() > b.getComparable() ? a : b;
}

/**
 * the range of overlap between period1 and period2 or null if no overlap or a
 * zero width overlap.
 */
function ICAL_overlap(period1, period2) {
  var s1 = period1.start,
      e1 = period1.end,
      s2 = period2.start,
      e2 = period2.end;
  // is there any overlap?
  if (e1.getComparable() <= s2.getComparable()
      || e2.getComparable() <= s1.getComparable()) {
    return null;
  }
  return new ICAL_PeriodOfTime(
      ICAL_dateMax(s1, s2), ICAL_dateMin(e1, e2));
}

/** Base class.  @constructor */
function ICAL_Temporal(opt_yr, opt_mo, opt_da, opt_hr, opt_mi, opt_se) {
  if (!isNaN(opt_yr)) { this.year = opt_yr; }
  if (!isNaN(opt_mo)) { this.month = opt_mo; }
  if (!isNaN(opt_da)) { this.date = opt_da; }
  if (!isNaN(opt_hr)) { this.hour = opt_hr; }
  if (!isNaN(opt_mi)) { this.minute = opt_mi; }
  if (!isNaN(opt_se)) { this.second = opt_se; }
}
ICAL_Temporal.prototype.year = NaN;
ICAL_Temporal.prototype.month = NaN;
ICAL_Temporal.prototype.date = NaN;
ICAL_Temporal.prototype.hour = NaN;
ICAL_Temporal.prototype.minute = NaN;
ICAL_Temporal.prototype.second = NaN;
ICAL_Temporal.prototype.getDayOfWeek = function () {
  return ICAL_getDayOfWeek(this);
};
ICAL_Temporal.prototype.toString = function() {
  if (this.str_ !== undefined) return this.str_;
  this.str_ = this.toStringRepresentation_();
  return this.str_;
}

/**
 * a placeholder interface for ICAL_Temporal subclasses that represent a date
 * or datetime value.
 * <p>Prefer <tt>AssertType(foo, ICAL_DateValue);</tt> over
 * <tt>AssertTrue(foo instanceof ICAL_Date || foo instanceof ICAL_DateTime)</tt>
 */
function ICAL_DateValue() {
  // placeholder
}
ICAL_DateValue.prototype = new ICAL_Temporal();
ICAL_DateValue.prototype.constructor = ICAL_DateValue;

/**
 * ICAL_Date is an icalendar date. This constructor is private --
 * use ICAL_Date.create() instead.
 *
 * @constructor
 * @private
 */
function ICAL_Date(yr, mo, da) {
  AssertTrue(mo && da, /* SAFE */ 'invalid date params: ' + mo + ' ' + da);
  ICAL_Temporal.call(this, yr, mo, da, NaN, NaN, NaN);
}
ICAL_Date.prototype = new ICAL_DateValue();
ICAL_Date.prototype.constructor = ICAL_Date;
ICAL_Date.now = function () {
  var d = new Date();
  return ICAL_Date.create(d.getFullYear(), d.getMonth() + 1, d.getDate());
};
ICAL_Date.prototype.type = /* SAFE */ 'Date';
ICAL_Date.prototype.toDate = function () { return this; };
ICAL_Date.prototype.toDateTime = function () {
  return new ICAL_DateTime(this.year, this.month, this.date, 0, 0, 0);
};
ICAL_Date.prototype.getComparable = function () {
  if (undefined === this.cmp_) {
    // only if someone uses the constructor (which shouldn't happen, but might)
    // will this.cmp_ need to be calculated here
    this.cmp_ =
        ICAL_CalculateDateComparable_(this.year, this.month, this.date);
  }
  return this.cmp_;
};
function ICAL_CalculateDateComparable_(year, month, date) {
  return (((((year - 1970) * 12) + month) << 5) + date) * (24 * 60 * 60);
}
ICAL_Date.prototype.isComplete = function () { return true; };
ICAL_Date.prototype.toStringRepresentation_ = function () {
  return ICAL_fmtInt(this.year, 4) + ICAL_fmtInt(this.month, 2) +
    ICAL_fmtInt(this.date, 2);
};
ICAL_Date.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.date === that.date &&
    this.month === that.month &&
    this.year === that.year;
};

/**
 * cache_ is a cache of ICAL_Date objects. The keys are comparables of
 * ICAL_Date objects and the values are the corresponding ICAL_Date objects.
 */
ICAL_Date.cache_ = {};
ICAL_Date.cacheSize_ = 0;
ICAL_Date.MAX_CACHE_SIZE_ = 200;

/**
 * Factory method for creating ICAL_Date objects.
 */
ICAL_Date.create = function(year, month, date) {
  var cmp = ICAL_CalculateDateComparable_(year, month, date);
  if (cmp in ICAL_Date.cache_) {
    return ICAL_Date.cache_[cmp];
  } else {
    var dt = new ICAL_Date(year, month, date);
    dt.cmp_ = cmp;
    if (ICAL_Date.cacheSize_ < ICAL_Date.MAX_CACHE_SIZE_) {
      ICAL_Date.cache_[cmp] = dt;
    }
    return dt;
  }
}

/** an icalendar date-time.  @constructor */
function ICAL_DateTime(yr, mo, da, hr, mi, se) {
  ICAL_Temporal.call(this, yr, mo, da, hr, mi, se);
}
ICAL_DateTime.prototype = new ICAL_DateValue();
ICAL_DateTime.prototype.constructor = ICAL_DateTime;
ICAL_DateTime.now = function () {
  var d = new Date();
  return new ICAL_DateTime(d.getFullYear(), d.getMonth() + 1, d.getDate(),
                           d.getHours(), d.getMinutes(), d.getSeconds());
};
ICAL_DateTime.prototype.type = /* SAFE */ 'DateTime';
ICAL_DateTime.prototype.toDate = function () {
  return ICAL_Date.create(this.year, this.month, this.date);
};
ICAL_DateTime.prototype.toDateTime = function () { return this; };
ICAL_DateTime.prototype.toTime = function () {
  return new ICAL_Time(this.hour, this.minute, this.second);
};

// Note: this does NOT give you the number of seconds since an epoch.
// It just gives you an integer value, such that if time1 is before
// time2 then time1.getComparable() < time2.getComparable()
ICAL_DateTime.prototype.getComparable = function () {
  if (undefined === this.cmp_) {
    this.cmp_ =
      (((((((((((this.year - 1970) * 12) + this.month) << 5) +
             this.date) * 24) + this.hour) * 60) + this.minute) * 60) +
       this.second);
  }
  return this.cmp_;
};
ICAL_DateTime.prototype.isComplete = function () { return true; };
ICAL_DateTime.prototype.toStringRepresentation_ = function () {
  return ICAL_fmtInt(this.year, 4) + ICAL_fmtInt(this.month, 2) +
    ICAL_fmtInt(this.date, 2) + 'T' + ICAL_fmtInt(this.hour, 2) +
    ICAL_fmtInt(this.minute, 2) + ICAL_fmtInt(this.second, 2);
};
ICAL_DateTime.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.date === that.date &&
    this.month === that.month &&
    this.year === that.year &&
    this.hour === that.hour &&
    this.minute === that.minute &&
    this.second === that.second;
};
ICAL_DateTime.prototype.clone = function() {
  var dt = new ICAL_DateTime(this.year, this.month, this.date,
                             this.hour, this.minute, this.second);
  if (this.str_ !== undefined) dt.str_ = this.str_;
  return dt;
}

/** an icalendar time.  @constructor */
function ICAL_Time(hr, mi, se) {
  ICAL_Temporal.call(this, NaN, NaN, NaN, hr, mi, se);
}
ICAL_Time.prototype = new ICAL_Temporal();
ICAL_Time.prototype.constructor = ICAL_Time;
ICAL_Time.prototype.type = /* SAFE */ 'Time';
ICAL_Time.prototype.toTime = function () { return this; };
ICAL_Time.prototype.toStringRepresentation_ = function () {
  return "T" + ICAL_fmtInt(this.hour, 2) +
    ICAL_fmtInt(this.minute, 2) + ICAL_fmtInt(this.second, 2);
};
ICAL_Time.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.hour === that.hour &&
    this.minute === that.minute &&
    this.second === that.second;
};
ICAL_Time.prototype.getComparable = function () {
  return ((this.hour) * 60 + this.minute) * 60 + this.second;
};

/** an icalendar duration.  @constructor */
function ICAL_Duration(da, hr, mi, se) {
  // normalize to make sure all non zero fields have the same sign and are
  // in the expected ranges

  var totSecs = se + (60 * (mi + 60 * (hr + 24 * da)));
  var normDays = ICAL_trunc(totSecs / (24 * 60 * 60));
  totSecs -= normDays * (24 * 60 * 60);
  var normHours = ICAL_trunc(totSecs / (60 * 60));
  totSecs -= normHours * (60 * 60);
  var normMinutes = ICAL_trunc(totSecs / 60);
  totSecs -= normMinutes * 60;
  var normSecs = ICAL_trunc(totSecs);

  ICAL_Temporal.call(this, NaN, NaN,
                     normDays, normHours, normMinutes, normSecs);
}
ICAL_Duration.prototype = new ICAL_Temporal();
ICAL_Duration.prototype.constructor = ICAL_Duration;
ICAL_Duration.prototype.type = /* SAFE */ 'Duration';
ICAL_Duration.prototype.toDays = function () {
  return this.date;
};
ICAL_Duration.prototype.toHours = function () {
  return this.date * 24 + this.hour;
};
ICAL_Duration.prototype.toMinutes = function () {
  return 24 * 60 * this.date + this.hour * 60 + this.minute;
};
ICAL_Duration.prototype.toSeconds = function () {
  return this.second + this.minute * 60 + this.hour * 3600 +
    3600 * 24 * this.date;
};
ICAL_Duration.prototype.getComparable = function () {
  if (undefined === this.cmp_) {
    this.cmp_ = ((this.date * 24 + this.hour) * 60 + this.minute) * 60 +
                this.second;
  }
  return this.cmp_;
};
/** string form of a duration.  This is a bit different from the RFC2445 format
  * since icalendar durations don't allow month or year diffs.
  * Well formed durations shouldn't have these either, but I want to allow them
  * in intermediate form.
  * <br>The general form of a duration is
  * <tt>[+-]?P(\d+Y)?(\d+N)?(\d+W)?(\d+D)?(T(\d+H)?(\d+M)?(\d+S)?)?</tt>.
  * <br>where Y = year, N = month, W = week, D = day, H = hour, M = minute,
  * S = second.
  */
ICAL_Duration.prototype.toStringRepresentation_ = function () {
  var sign = this.year ? ICAL_sign(this.year) :
             this.month ? ICAL_sign(this.month) :
             this.date ? ICAL_sign(this.date) :
             this.hour ? ICAL_sign(this.hour) :
             this.minute ? ICAL_sign(this.minute) :
             this.second ? ICAL_sign(this.second) : 0;

  var s = sign < 0 ? '-P' : 'P';
  // duration should not have year or month fields.  include these so bugs are
  // obvious
  if (this.year) { s += (sign * this.year) + 'Y'; }
  if (this.month) { s += (sign * this.month) + 'N'; }
  // duration is specified as a number of weeks, hours, minutes, and seconds
  if (this.date) {
    s += (this.date % 7) ?
         (sign * this.date) + 'D' :
         ((sign * this.date) / 7) + 'W';
  }
  if (this.hour || this.minute || this.second) { s += 'T'; }
  if (this.hour) { s += (sign * this.hour) + 'H'; }
  if (this.minute) { s += (sign * this.minute) + 'M'; }
  if (this.second) { s += (sign * this.second) + 'S'; }

  if (!sign) { s += '0D'; }
  return s;
};
ICAL_Duration.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.date === that.date &&
    this.hour === that.hour &&
    this.minute === that.minute &&
    this.second === that.second;
};


/** a mutable temporal.  @constructor */
function ical_createBuilder() {
  return new ICAL_DTBuilder();
}

function ical_builderCopy(/*!ICAL_Temporal*/ t) {
  AssertType(t, ICAL_Temporal);
  var b = new ICAL_DTBuilder();
  b.year = t.year || 0;
  b.month = t.month || 0;
  b.date = t.date || 0;
  b.hour = t.hour || 0;
  b.minute = t.minute || 0;
  b.second = t.second || 0;
  return b;
}

/**
 * a builder that contains a copy of the given, possibly partial, date value.
 */
function ical_partialBuilderCopy(/*!ICAL_DateValue*/ t) {
  AssertType(t, ICAL_DateValue);
  var b = new ICAL_DTBuilder();
  b.year = t.year;
  b.month = t.month;
  b.date = t.date;
  b.hour = t.hour;
  b.minute = t.minute;
  b.second = t.second;
  return b;
}

function ical_dateBuilder(year, month, date) {
  AssertTrue(!(isNaN(year) | isNaN(month) | isNaN(date)));
  var b = new ICAL_DTBuilder();
  b.year = year || 0;
  b.month = month || 0;
  b.date = date || 0;
  return b;
}

function ical_dateTimeBuilder(year, month, date, hour, minute, second) {
  AssertTrue(!(isNaN(year) | isNaN(month) | isNaN(date) |
	       isNaN(hour) | isNaN(minute) | isNaN(second)));
  var b = new ICAL_DTBuilder();
  b.year = year || 0;
  b.month = month || 0;
  b.date = date || 0;
  b.hour = hour || 0;
  b.minute = minute || 0;
  b.second = second || 0;
  return b;
}

function ICAL_DTBuilder() {
}
ICAL_DTBuilder.prototype = new ICAL_Temporal();
ICAL_DTBuilder.prototype.constructor = ICAL_DTBuilder;
ICAL_DTBuilder.prototype.type = /* SAFE */ 'DTBuilder';
ICAL_DTBuilder.prototype.year =
  ICAL_DTBuilder.prototype.month =
  ICAL_DTBuilder.prototype.date =
  ICAL_DTBuilder.prototype.hour =
  ICAL_DTBuilder.prototype.minute =
  ICAL_DTBuilder.prototype.second = 0;
ICAL_DTBuilder.prototype.getComparable = function () {
  this.normalize_();
  var result;
  if (isNaN(this.hour)) {
    result = ICAL_CalculateDateComparable_(this.year, this.month, this.date);
  } else {
    result = (((((((((((this.year - 1970) * 12) + this.month) << 5) +
                    this.date) * 24) + this.hour) * 60) + this.minute) * 60) +
              this.second);
  }
  return result;
};
ICAL_DTBuilder.prototype.advance = function (duration) {
  if (duration.date) { this.date += duration.date; }
  if (duration.hour) { this.hour += duration.hour; }
  if (duration.minute) { this.minute += duration.minute; }
  if (duration.second) { this.second += duration.second; }
};
ICAL_DTBuilder.prototype.normalize_ = function () {
  this.normalizeHMS_();
  this.normalizeMonth_();
  var n = ICAL_daysInMonth(this.year, this.month);
  while (this.date < 1) {
    this.month -= 1;
    this.normalizeMonth_();
    n = ICAL_daysInMonth(this.year, this.month);
    this.date += n;
  }
  while (this.date > n) {
    this.date -= n;
    this.month += 1;
    this.normalizeMonth_();
    n = ICAL_daysInMonth(this.year, this.month);
  }
};
/** normalize the hour, minute, and second fields.  @private */
ICAL_DTBuilder.prototype.normalizeHMS_ = function () {
  var n;
  if (this.second < 0) {
    n = Math.ceil(this.second / -60);
    this.second += 60 * n;
    this.minute -=n;
  } else if (this.second >= 60) {
    n = Math.floor(this.second / 60);
    this.second -= 60 * n;
    this.minute += n;
  }
  if (this.minute < 0) {
    n = Math.ceil(this.minute / -60);
    this.minute += 60 * n;
    this.hour -=n;
  } else if (this.minute >= 60) {
    n = Math.floor(this.minute / 60);
    this.minute -= 60 * n;
    this.hour += n;
  }
  if (this.hour < 0) {
    n = Math.ceil(this.hour / -24);
    this.hour += 24 * n;
    this.date -=n;
  } else if (this.hour >= 24) {
    n = Math.floor(this.hour / 24);
    this.hour -= 24 * n;
    this.date += n;
  }
};
/** normalize the month field.  @private */
ICAL_DTBuilder.prototype.normalizeMonth_ = function () {
  var n;
  if (this.month < 1) {
    n = Math.ceil((this.month - 1) / -12);
    this.month += 12 * n;
    this.year -= n;
  } else if (this.month > 12) {
    n = Math.floor((this.month - 1) / 12);
    this.month -= 12 * n;
    this.year += n;
  }
};
ICAL_DTBuilder.prototype.toDate = function () {
  this.normalize_();
  return ICAL_Date.create(this.year, this.month, this.date);
};
ICAL_DTBuilder.prototype.toDateTime = function () {
  this.normalize_();
  return new ICAL_DateTime(this.year, this.month, this.date,
                           this.hour, this.minute, this.second);
};
ICAL_DTBuilder.prototype.toPartialDate = function () {
  // comparisons to undefined return false so will not affect partial fields
  this.normalize_();
  return new ICAL_PartialDate(isFinite(this.year) ? this.year : undefined,
                              isFinite(this.month) ? this.month : undefined,
                              isFinite(this.date) ? this.date : undefined);
};
ICAL_DTBuilder.prototype.toPartialDateTime = function () {
  // comparisons to undefined return false so will not affect partial fields
  this.normalize_();
  return new ICAL_PartialDateTime(
      isFinite(this.year) ? this.year : undefined,
      isFinite(this.month) ? this.month : undefined,
      isFinite(this.date) ? this.date : undefined,
      isFinite(this.hour) ? this.hour : undefined,
      isFinite(this.minute) ? this.minute : undefined,
      isFinite(this.second) ? this.second : undefined);
};
ICAL_DTBuilder.prototype.toTime = function () {
  this.normalize_();
  return new ICAL_Time(this.hour, this.minute, this.second);
};
ICAL_DTBuilder.prototype.toDuration = function () {
  if (this.year || this.month) {
    ICAL_exception(
        /* SAFE */ 'Can\'t convert months or years to ICAL_Duration');
    return undefined;
  } else {
    return new ICAL_Duration(this.date, this.hour, this.minute, this.second);
  }
};
/** true iff this date builder can be converted to a proper ICAL_Date
  * (post normalization).
  */
ICAL_DTBuilder.prototype.validateDate = function () {
  // 1 + (n % 1) will evaluate to 1 iff n is an integer.  It will be a fraction
  // greater than 1 for a non-integral n, and NaN for n in (NaN, Infinity)
  /* SAFE */
  return ('number' == typeof this.year && (1 + (this.year % 1)) === 1 &&
          'number' == typeof this.month && (1 + (this.month % 1)) === 1 &&
          'number' == typeof this.date && (1 + (this.date % 1)) === 1);
};
/** true iff this date builder can be converted to a proper ICAL_DateTime
  * (post normalization).
  */
ICAL_DTBuilder.prototype.validateDateTime = function () {
  /* SAFE */
  return this.validateDate() && this.validateTime();
};
/** true iff this date builder can be converted to a proper ICAL_Time
  * (post normalization).
  */
ICAL_DTBuilder.prototype.validateTime = function () {
  /* SAFE */
  return ('number' == typeof this.hour && (1 + (this.hour % 1)) === 1 &&
          'number' == typeof this.minute && (1 + (this.minute % 1)) === 1 &&
          'number' == typeof this.second && (1 + (this.second % 1)) === 1);
};
ICAL_DTBuilder.prototype.toString = function () {
  return (
      '[' + (undefined !== this.year ? ICAL_fmtInt(this.year, 4) : '????') +
      '/' + (undefined !== this.month ? ICAL_fmtInt(this.month, 2) : '??') +
      '/' + (undefined !== this.date ? ICAL_fmtInt(this.date, 2) : '??') +
      ' ' + (undefined !== this.hour ? ICAL_fmtInt(this.hour, 2) : '??') +
      ' ' + (undefined !== this.minute ? ICAL_fmtInt(this.minute, 2) : '??') +
      ' ' + (undefined !== this.second ? ICAL_fmtInt(this.second, 2) : '??') +
      ']');
};
ICAL_DTBuilder.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.date === that.date &&
    this.month === that.month &&
    this.year === that.year &&
    this.hour === that.hour &&
    this.minute === that.minute &&
    this.second === that.second;
};

/** an icalendar period-of-time.  @constructor */
function ICAL_PeriodOfTime(t1, t2) {
  AssertTrue(t1 instanceof ICAL_DateTime || t1 instanceof ICAL_Date);
  this.start = t1;
  if (t2.constructor == ICAL_Duration) {
    var b = ical_builderCopy(t1);
    b.advance(t2);
    this.end =
      this.start instanceof ICAL_DateTime ? b.toDateTime() : b.toDate();
  } else {
    AssertEquals(t2.constructor, t1.constructor);
    this.end = t2;
  }
  this.duration = ICAL_DurationBetween(this.end, this.start);
}
ICAL_PeriodOfTime.prototype.type = /* SAFE */ 'PeriodOfTime';
ICAL_PeriodOfTime.prototype.toString = function () {
  if (this.str_ !== undefined) return this.str_;
  this.str_ = this.start + '/' + this.end;
  return this.str_;
};
ICAL_PeriodOfTime.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.start.equals(that.start) && this.end.equals(that.end);
};
ICAL_PeriodOfTime.prototype.overlaps = function (that) {
  // Two regions A and B don't overlap if A ends before B starts or
  // A starts after B ends.  Since ends are exclusive we use >= and <=:
  //   overlap = !(A.end <= B.start || A.start >= B.end)
  // By DeMorgan's:
  //   overlap = (A.end > B.start) && (A.start < B.end)
  return that.end.getComparable() > this.start.getComparable() &&
         that.start.getComparable() < this.end.getComparable();
};
ICAL_PeriodOfTime.prototype.overlapsDateTimes = function (start, end) {
  return end.getComparable() > this.start.getComparable() &&
         start.getComparable() < this.end.getComparable();
};
/**
 * Checks if this region overlaps the region [start, end], allowing for zero
 * length overlap. This deviates from RFC2445 which does not consider intervals 
 * that intersect in a point to be overlapping.
 * @param start {ICAL_Date} the start of the region to compare to.
 * @param end {ICAL_Date} the end of the region to cmopare to.
 * @return true iff this overlaps [start,end] (zero length overlap is allowed).
 */
ICAL_PeriodOfTime.prototype.overlapsZeroLength = function (start, end) {
  return end.getComparable() >= this.start.getComparable() &&
         start.getComparable() <= this.end.getComparable();
};
ICAL_PeriodOfTime.prototype.contains = function (that) {
  return this.start.getComparable() <= that.start.getComparable() &&
         this.end.getComparable() >= that.end.getComparable();
};


/** an icalendar patial period-of-time.  @constructor */
function ICAL_PartialPeriodOfTime(t1, t2) {
  AssertTrue(t1 instanceof ICAL_PartialDateTime ||
             t1 instanceof ICAL_PartialDate);
  this.start = t1;

  AssertEquals(t2.constructor, t1.constructor);
  this.end = t2;

  // TODO(davem) - make a special partial duration function. In a partial
  // period. Extracting the duration might legitimately fail
  try {
    this.duration = ICAL_DurationBetween(this.end, this.start);
  } catch (e) {
    // Cleanup here
    this.duration = null;
  }
}
ICAL_PartialPeriodOfTime.prototype.type = /* SAFE */ 'PartialPeriodOfTime';
ICAL_PartialPeriodOfTime.prototype.toStringRepresentation_ = function () {
  return this.start + '/' + this.end;
};
ICAL_PartialPeriodOfTime.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    this.start.equals(that.start) && this.end.equals(that.end);
};


// Date operations
function ICAL_DurationBetween(t1, t2) {
  if (isNaN(t1.year) != isNaN(t2.year) ||
      isNaN(t1.hour) != isNaN(t2.hour)) {
    ICAL_exception(/* SAFE */ 'diff(' + t1 + ', ' + t2 + ')');
    return undefined;
  }

  var b = ical_builderCopy(t1);
  if (isNaN(t1.year)) {
    b.hour -= t2.hour;
    b.minute -= t2.minute;
    b.second -= t2.second;
  } else {
    b.year = NaN;
    b.month = NaN;
    b.date = ICAL_daysBetween(t1.year, t1.month, t1.date,
                              t2.year, t2.month, t2.date);
    if (!isNaN(t1.hour)) {
      b.hour -= t2.hour;
      b.minute -= t2.minute;
      b.second -= t2.second;
    }
  }
  return b.toDuration();
}

function ICAL_PartialDate(yr, mo, da) {
  this.year = yr;
  this.month = mo;
  this.date = da;
}
ICAL_PartialDate.prototype = new ICAL_DateValue();
ICAL_PartialDate.prototype.constructor = ICAL_PartialDate;
ICAL_PartialDate.prototype.type = 'PartialDate';
ICAL_PartialDate.prototype.toDate = function () {
  return ICAL_Date.create(this.year || 0, this.month || 1, this.date || 1);
};
ICAL_PartialDate.prototype.toDateTime = function () {
  return new ICAL_DateTime(
      this.year || 0, this.month || 1, this.date || 1, 0, 0, 0);
};
ICAL_PartialDate.prototype.toPartialDate = function () { return this; };
ICAL_PartialDate.prototype.toPartialDateTime = function () {
  return new ICAL_PartialDateTime(this.year, this.month, this.date, 0, 0, 0);
};
ICAL_PartialDate.prototype.isComplete = function () {
  return !isNaN(this.getComparable());
};
ICAL_PartialDate.prototype.getComparable = function () {
  if (undefined === this.cmp_) {
    this.cmp_ = (((((this.year - 1970) * 12) + this.month) << 5) + this.date) *
                (24 * 60 * 60);
  }
  return this.cmp_;
};
ICAL_PartialDate.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    (this.date === that.date || (isNaN(this.date) && isNaN(that.date))) &&
    (this.month === that.month || (isNaN(this.month) && isNaN(that.month))) &&
    (this.year === that.year || (isNaN(this.year) && isNaN(that.year)));
};
ICAL_PartialDate.prototype.toStringRepresentation_ = function () {
  return (undefined !== this.year ? ICAL_fmtInt(this.year, 4) : '????') +
         (undefined !== this.month ? ICAL_fmtInt(this.month, 2) : '??') +
         (undefined !== this.date ? ICAL_fmtInt(this.date, 2) : '??');
};

function ICAL_PartialDateTime(yr, mo, da, hr, mi, se) {
  this.year = yr;
  this.month = mo;
  this.date = da;
  this.hour = hr;
  this.minute = mi;
  this.second = se;
}
ICAL_PartialDateTime.prototype = new ICAL_DateValue();
ICAL_PartialDateTime.prototype.constructor = ICAL_PartialDateTime;
ICAL_PartialDateTime.prototype.type = 'PartialDateTime';
ICAL_PartialDateTime.prototype.toDate = function () {
  return ICAL_Date.create(this.year || 0, this.month || 1, this.date || 1);
};
ICAL_PartialDateTime.prototype.toDateTime = function () {
  return new ICAL_DateTime(
      this.year || 0, this.month || 1, this.date || 1,
      this.hour || 0, this.minute || 0, this.second || 0);
};
ICAL_PartialDateTime.prototype.toPartialDate = function () {
  return new ICAL_PartialDate(this.year, this.month, this.date);
};
ICAL_PartialDateTime.prototype.toPartialDateTime = function () { return this; };
ICAL_PartialDateTime.prototype.isComplete = function () {
  return !isNaN(this.getComparable());
};
ICAL_PartialDateTime.prototype.getComparable = function () {
  if (undefined === this.cmp_) {
    this.cmp_ =
      (((((((((((this.year - 1970) * 12) + this.month) << 5) +
             this.date) * 24) + this.hour) * 60) + this.minute) * 60) +
       this.second);
  }
  return this.cmp_;
};
ICAL_PartialDateTime.prototype.equals = function (that) {
  return this.constructor === that.constructor && // same class
    (this.date === that.date || (isNaN(this.date) && isNaN(that.date))) &&
    (this.month === that.month || (isNaN(this.month) && isNaN(that.month))) &&
    (this.year === that.year || (isNaN(this.year) && isNaN(that.year))) &&
    (this.hour === that.hour || (isNaN(this.hour) && isNaN(that.hour))) &&
    (this.minute === that.minute ||
     (isNaN(this.minute) && isNaN(that.minute))) &&
    (this.second === that.second || (isNaN(this.second) && isNaN(that.second)));
};
ICAL_PartialDateTime.prototype.toStringRepresentation_ = function () {
  return (undefined !== this.year ? ICAL_fmtInt(this.year, 4) : '????') +
         (undefined !== this.month ? ICAL_fmtInt(this.month, 2) : '??') +
         (undefined !== this.date ? ICAL_fmtInt(this.date, 2) : '??') +
         'T' +
         (undefined !== this.hour ? ICAL_fmtInt(this.hour, 2) : '??') +
         (undefined !== this.minute ? ICAL_fmtInt(this.minute, 2) : '??') +
         (undefined !== this.second ? ICAL_fmtInt(this.second, 2) : '??');
};


// Factory
/** parses ICalendar data values.  The types parsed include
  * <ul>
  * <li><a href="http://www.corp.google.com/~msamuel/rfc2445.html#4.3.4"
  *  >Date</a></li>
  * <li><a href="http://www.corp.google.com/~msamuel/rfc2445.html#4.3.12"
  *  >Time</a></li>
  * <li><a href="http://www.corp.google.com/~msamuel/rfc2445.html#4.3.5"
  *  >DateTime</a></li>
  * <li><a href="http://www.corp.google.com/~msamuel/rfc2445.html#4.3.6"
  *  >Duration</a></li>
  * <li><a href="http://www.corp.google.com/~msamuel/rfc2445.html#4.3.9"
  *  >Period of Time</a></li>
  * </ul>
  */
function ICAL_Parse(icalString) {
  var slash = icalString.indexOf('/');
  var n = icalString.length;
  if (slash >= 0) {
    return new ICAL_PeriodOfTime(
      ICAL_Parse(icalString.substring(0, slash)),
      ICAL_Parse(icalString.substring(slash + 1, n)));
  } else {
    var sign = 1;
    var pos = 0;
    switch (icalString.charAt(0)) {
      case 'T': // Time 'T'HHMMSS
        AssertEquals(n, 7);
        return ical_dateTimeBuilder(0, 0, 0,
                                    ICAL_toInt_(icalString, 1, 3),
                                    ICAL_toInt_(icalString, 3, 5),
                                    ICAL_toInt_(icalString, 5, 7)).toTime();
      case 'P': // Duration 'P'(\d+[WD])+('T'(\d+[HMS])+)?
        return ICAL_ParseDuration_(icalString.substring(1, n), 1);
      case '-':
        sign = -1;
        // fallthru
      case '+':
        pos = 1;
        if ('P' == icalString.charAt(1)) {
          return ICAL_ParseDuration_(icalString.substring(2, n), sign);
        }
        // fallthru
      default:
        var t = icalString.indexOf('T');
        if (t === -1) { // Date YYYYMMDD
          AssertTrue(pos < n-4, /* SAFE */ "Invalid date string");
          return ical_dateBuilder(ICAL_toInt_(icalString, pos, n - 4),
                                  ICAL_toInt_(icalString, n - 4, n - 2),
                                  ICAL_toInt_(icalString, n - 2, n)).toDate();
        }
        // DateTime YYYYMMDD'T'HHMMSS
        AssertEquals(n, t + 7);
        return ical_dateTimeBuilder(
            ICAL_toInt_(icalString, pos, t - 4),
            ICAL_toInt_(icalString, t - 4, t - 2),
            ICAL_toInt_(icalString, t - 2, t),
            ICAL_toInt_(icalString, t + 1, t + 3),
            ICAL_toInt_(icalString, t + 3, t + 5),
            ICAL_toInt_(icalString, t + 5, t + 7)).toDateTime();
    }
  }
}

/* A faster version of ICAL_Parse. It assumes the string is in the form
 * YYYYMMDD or YYYYMMDDTHHMMSS. With values that do not need further
 * normalizing. Runs ~3x faster than the regular version.
 * Note further that it ignores the seconds field [since we never use it]
 */
function ICAL_FastParse(icalString) {
  var y = parseInt(icalString.substring(0, 4), 10);
  var m = parseInt(icalString.substring(4, 6), 10);
  var d = parseInt(icalString.substring(6, 8), 10);
  if (icalString.length === 8) { // Date YYYYMMDD
    return ICAL_Date.create(y,m,d);
  } else if (icalString.length === 15) {
    var h = parseInt(icalString.substring(9, 11), 10);
    var mi = parseInt(icalString.substring(11, 13), 10);
    var s = parseInt(icalString.substring(13, 15), 10);
    return new ICAL_DateTime(y,m,d,h,mi,s);
  } else {
    return ICAL_Parse(icalString);
  }
}

/** Given a Date or DateTime, return the Date representing the next day 
 *
 * This is a special case of the advance function, but it is used in tight
 * loops often enough that I have coded up a faster version which does not
 * create any intermediate ICAL_DTBuilder objects.
*/
function ICAL_dateAfter(/*!ICAL_Temporal*/ t) {
  var d = t.date + 1;
  var m = t.month;
  var y = t.year;
  var daysInMonth = ICAL_daysInMonth(y, m);
  if (d > daysInMonth) {
    d = 1;
    m++;
    if (m === 13) {
      m = 1;
      y++;
    }
  }
  return ICAL_Date.create(y, m, d);
}

/** @private */
function ICAL_ParseDuration_(s, sign) {
  var n = s.length;
  var b = new ICAL_DTBuilder();
  for (var i = 0; i < n; i += 1) {
    var ni = 0;
    do {
      var ch = s.charAt(i);
      if (ch < '0' || ch > '9') { break; }
      ni += 1;
    } while ((i += 1) < n);
    if (ni === 0) {
      AssertEquals('T', s.charAt(i));
      continue;
    }
    var num = ICAL_toInt_(s, i - ni, i);
    switch(s.charAt(i)) {
      case 'W':
        b.date += sign * 7 * num;
        break;
      case 'D':
        b.date += sign * num;
        break;
      case 'H':
        b.hour += sign * num;
        break;
      case 'M':
        b.minute += sign * num;
        break;
      case 'S':
        b.second += sign * num;
        break;
      default:
        ICAL_exception(/* SAFE */ 'Bad Duration ' + s);
        return undefined;
    }
  }
  return b.toDuration();
}

var FOUR_OR_MORE_OPT_DIGITS_ = '(?:([0-9]{4,})|\\?{4})';
var TWO_OPT_DIGITS_ = '(?:([0-9]{2})|\\?{2})';
/**
 * the below regexps match date time formats like
 * '2005??01T1230??' where the ??? are unknown parts of an ical date/date-time.
 * The digit groups either match a number of digits, or ??.  If the ?? is
 * matched then the corresponding parenthetical group will have value undefined
 * or null.
 */
var PARTIAL_DATE_RE_ = new RegExp(
    '^' + /*year*/ FOUR_OR_MORE_OPT_DIGITS_ + /*month*/TWO_OPT_DIGITS_ +
    /*date*/TWO_OPT_DIGITS_ + '$');
var PARTIAL_DATE_TIME_RE_ = new RegExp(
    '^' + /*year*/ FOUR_OR_MORE_OPT_DIGITS_ + /*month*/TWO_OPT_DIGITS_ +
    /*date*/TWO_OPT_DIGITS_ + 'T' + /*hour*/TWO_OPT_DIGITS_ +
    /*minute*/TWO_OPT_DIGITS_ + /*second*/TWO_OPT_DIGITS_ + '$');

function ICAL_ParsePartial(icalStr) {

  // If there is a slash present, then split it and recurse on both halves
  var slash = icalStr.indexOf('/');
  var n = icalStr.length;
  if (slash >= 0) {
    return new ICAL_PartialPeriodOfTime(
      ICAL_ParsePartial(icalStr.substring(0, slash)),
      ICAL_ParsePartial(icalStr.substring(slash + 1, n)));
  }

  // No slash, parse it in the normal way
  var m = icalStr.match(PARTIAL_DATE_TIME_RE_);
  if (!m) {
    m = icalStr.match(PARTIAL_DATE_RE_);
    if (!m) {
      /* SAFE */
      ICAL_exception(/* SAFE */ "Failed to parse partial date " + icalStr);
    }
  }
  // the blank parenthetical groups will have undefined on firefox and null on
  // IE, so make sure that they're undefined.
  for (var i = m.length; --i >= 1;) {
    if (!m[i]) {  // matched ??
      m[i] = undefined;
    } else {  // parse a run of digits
      m[i] = parseInt(m[i], 10);
    }
  }
  if (7 == m.length) {  // the date-time pattern was matched
    return new ICAL_PartialDateTime(m[1], m[2], m[3], m[4], m[5], m[6]);
  } else {  // the date pattern was matched
    return new ICAL_PartialDate(m[1], m[2], m[3]);
  }
}


// some useful values

/** the ICAL_Date instance corresponding to "today" local time.
  * Updated around midnight.
  */
var ICAL_todaysDate = undefined;

/** Functions to invoke when the date changes */
var ICAL_todayUpdateListeners = [];

/** the day of the year in [0-365] of the given date. */
function ICAL_dayOfYear(year, month, date) {
  var leapAdjust = month > 2 && 29 === ICAL_daysInMonth(year, 2);
  return ICAL_dayOfYear.MONTH_START_TO_DOY_[month] + leapAdjust + date - 1;
}
ICAL_dayOfYear.MONTH_START_TO_DOY_ = [
    undefined, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 ];

/**
 * Called at setup and every midnight to update {@link #ICAL_todaysDate}.
 * To listen for changes to the date, see AddTodayUpdateListener()
 *
 * @private
 */
function updateTodaysDate_() {
  var now = new Date();
  var oldDate = ICAL_todaysDate;
  ICAL_todaysDate =
    ICAL_Date.create(now.getFullYear(), now.getMonth() + 1, now.getDate());
  if (oldDate && !(oldDate.equals(ICAL_todaysDate))) {
    for (var i = 0; i < ICAL_todayUpdateListeners.length; ++i) {
      var func = ICAL_todayUpdateListeners[i];
      try {
        func(ICAL_todaysDate);
      } catch (e) {
        // ignore failures so arbitrary function provided by client
        // does not prevent the changed-date event from being fired
        // for the other listeners
      }
    }
  }
  var tomorrowMidnight =
    new Date(now.getFullYear(), now.getMonth(), now.getDate(), 0, 0, 0, 0);
  tomorrowMidnight.setDate(tomorrowMidnight.getDate() + 1);
  var diffMilli = tomorrowMidnight.getTime() - now.getTime();

  // Try again in 30 minutes [there's an odd bug in IE where occasionally it
  // spins if you call setTimeout with a value that too big]

  // code below commented out to make it work stand-alone (gri 12/19/06)

  /*
  if (diffMilli < 0 || diffMilli >= 30 * 60 * 1000) {
    diffMilli = 30 * 60 * 1000;
  }
  setTimeout(updateTodaysDate_, diffMilli);
  */
}
updateTodaysDate_();

/**
 * The provided function will be invoked if the current date is updated.
 * (For applications such as Caribou and Doozer that users leave open for
 * days, it is likely that the date will change over the lifetime of the
 * application.)
 *
 * When invoked, the function will take 1 argument, the new (current) date,
 * and any return value will be ignored.
 *
 * @param func {Function}
 */
function AddTodayUpdateListener(/*Function*/ func) {
  AssertType(func, Function,
      'func passed to AddTodayUpdateListener is not a Function');
  ICAL_todayUpdateListeners.push(func);
}
