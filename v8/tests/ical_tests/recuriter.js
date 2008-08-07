// Copyright 2004-2005 Google Inc.
// All Rights Reserved.
// msamuel@google.com

/*
 * for calculating the occurrences of an individual RFC 2445 RRULE.
 *
 * Public API
 * ==========
 * - RecurrenceIterator createRecurrenceIterator(
 *         rrule : RRule, dtStart: ICAL_Date|ICAL_DateTime)
 * - class RecurrenceIterator
 *     boolean HasNext()
 *     ICAL_Date|ICalDateTime Next()
 *
 * Requires
 * ========
 * ical.js
 * rrule.js
 *
 * To Do
 * =====
 * - we don't allow recurrences more frequently than daily, but there are
 *   legitimate uses for BY{HOUR,MINUTE,SECOND}
 *   implement BYHOUR,BYMINUTE,BYSECOND with 1 value so that people can specify
 *   Tuesday at 4pm and Thursday at 6 with the RRULE pair
 *   RRULE:FREQ=WEEKLY;BYDAY=TU;BYHOUR=16;BYMINUTE=0;BYSECOND=0
 *   RRULE:FREQ=WEEKLY;BYDAY=TH;BYHOUR=18;BYMINUTE=0;BYSECOND=0
 * - implement a layer on top that receives dates from multpile
 *   {R,EX}{RULE,DATES} and merges them appropriately into a single stream
 * - add asserts: that dates strictly increasing, that first date >= DTSTART
 * - properly prefix class and global function names.
 *
 * Glossary
 * ========
 * Period - year|month|day|...
 * Day of the week - an int in [0-6].  See RRULE_WDAY_* in rrule.js
 * Day of the year - zero indexed in [0,365]
 * Day of the month - 1 indexed in [1,31]
 * Month - 1 indexed integer in [1,12]
 *
 * Abstractions
 * ============
 * Generator - a function corresponding to an RRULE part that takes a date and
 *   returns a later (year or month or day depending on its period) within the
 *   next larger period.
 *   A generator ignores all periods in its input smaller than its period.
 * Filter - a function that returns true iff the given date matches the subrule.
 * Condition - returns true if the given date is past the end of the recurrence.
 *
 * All the functions that represent rule parts are stateful.
 */


/** create a recurrence iterator from an rrule. */
function createRecurrenceIterator(rrule, dtStart) {
  var freq = rrule._members[RRULE_MEMBER_FREQ];
  var wkst = rrule._members[RRULE_MEMBER_WKST];
  var until = rrule._members[RRULE_MEMBER_UNTIL];
  var count = rrule._members[RRULE_MEMBER_COUNT];
  var interval = rrule._members[RRULE_MEMBER_INTERVAL];
  var byDay = rrule._members[RRULE_MEMBER_BYDAY];
  var byMonth = rrule._members[RRULE_MEMBER_BYMONTH];
  var byMonthDay = rrule._members[RRULE_MEMBER_BYMONTHDAY];
  var byWeekNo = rrule._members[RRULE_MEMBER_BYWEEKNO];
  var byYearDay = rrule._members[RRULE_MEMBER_BYYEARDAY];
  var bySetPos = rrule._members[RRULE_MEMBER_BYSETPOS];
  var throttle = { limit_: 100, current_: 100 };

  interval = interval || 1;
  if (undefined === wkst) {
    wkst = RRULE_WDAY_MONDAY;
  }

  // recurrences are implemented as a sequence of periodic generators.
  // First a year is generated, and then months, and within months, days
  var yearGenerator =
    serialYearGenerator_(freq === RRULE_FREQ_YEARLY ? interval : 1, dtStart,
                         throttle);
  var monthGenerator =
    serialMonthGenerator_(freq === RRULE_FREQ_MONTHLY ? interval : 1, dtStart);
  var dayGenerator =
    serialDayGenerator_(freq === RRULE_FREQ_DAILY ? interval : 1, dtStart);

  // When multiple generators are specified for a period, they act as a union
  // operator.  We could have multiple generators (for day say) and then
  // run each and merge the results, but some generators are more efficient
  // than others, so to avoid generating 53 sundays and throwing away all but
  // 1 for RRULE:FREQ=YEARLY;BYDAY=TU;BYWEEKNO=1, we reimplement some of the
  // more prolific generators as filters.
  var filters = [];

  // choose the appropriate generators and filters
  switch (freq) {
    case RRULE_FREQ_DAILY:
      break;
    case RRULE_FREQ_WEEKLY:
      // week is not considered a period because a week may span multiple months
      // &| years.  There are no week generators, but so a filter is used to
      // make sure that FREQ=WEEKLY;INTERVAL=2 only generates dates within the
      // proper week.
      if (byDay) {
        dayGenerator = byDayGenerator_(byDay, false, dtStart);
        if (interval > 1) {
          filters.push(weekIntervalFilter_(interval, wkst, dtStart));
        }
      } else {
        dayGenerator = serialDayGenerator_(interval * 7, dtStart);
      }
      if (byMonthDay) {
        filters.push(byMonthDayFilter_(byMonthDay));
      }
      break;
    case RRULE_FREQ_YEARLY:
      if (byYearDay) {
        // The BYYEARDAY rule part specifies a COMMA separated list of days of
        // the year. Valid values are 1 to 366 or -366 to -1. For example, -1
        // represents the last day of the year (December 31st) and -306
        // represents the 306th to the last day of the year (March 1st).
        dayGenerator = byYearDayGenerator_(byYearDay, dtStart);
        if (byDay) {
          filters.push(byDayFilter_(byDay, true, wkst));
        }
        if (byMonthDay) {
          filters.push(byMonthDayFilter_(byMonthDay));
        }
        // TODO(msamuel): filter byWeekNo and write unit tests
        break;
      }
      // fallthru to monthly cases
    case RRULE_FREQ_MONTHLY:
      if (byMonthDay) {
        // The BYMONTHDAY rule part specifies a COMMA separated list of days of
        // the month. Valid values are 1 to 31 or -31 to -1. For example, -10
        // represents the tenth to the last day of the month.
        dayGenerator = byMonthDayGenerator_(byMonthDay, dtStart);
        if (byDay) {
          filters.push(byDayFilter_(byDay, RRULE_FREQ_YEARLY === freq, wkst));
        }
        // TODO(msamuel): filter byWeekNo and write unit tests
      } else if (byWeekNo && RRULE_FREQ_YEARLY === freq) {
        // The BYWEEKNO rule part specifies a COMMA separated list of ordinals
        // specifying weeks of the year.  This rule part is only valid for
        // YEARLY rules.
        dayGenerator = byWeekNoGenerator_(byWeekNo, wkst, dtStart);
        if (byDay) {
          filters.push(byDayFilter_(byDay, true, wkst));
        }
      } else if (byDay) {
        // Each BYDAY value can also be preceded by a positive (+n) or negative
        // (-n) integer. If present, this indicates the nth occurrence of the
        // specific day within the MONTHLY or YEARLY RRULE. For example, within
        // a MONTHLY rule, +1MO (or simply 1MO) represents the first Monday
        // within the month, whereas -1MO represents the last Monday of the
        // month. If an integer modifier is not present, it means all days of
        // this type within the specified frequency. For example, within a
        // MONTHLY rule, MO represents all Mondays within the month.
        dayGenerator = byDayGenerator_(
            byDay, RRULE_FREQ_YEARLY === freq && !byMonth, dtStart);
      } else {
        if (RRULE_FREQ_YEARLY === freq) {
          monthGenerator = byMonthGenerator_([dtStart.month], dtStart);
        }
        dayGenerator = byMonthDayGenerator_([dtStart.date], dtStart);
      }
      break;
    default:
      throw new Error('Can\'t iterate more frequently than weekly');
  }

  // generator inference common to all periods
  if (byMonth) {
    monthGenerator = byMonthGenerator_(byMonth, dtStart);
  }

  // the condition tells the iterator when to halt.
  // The condition is exclusive, so the date that triggers it will not be
  // included.
  var condition = optimisticCondition_;
  var canShortcutAdvance = true;
  if (count) {
    condition = countCondition_(count);
    // We can't shortcut because the countCondition must see every generated
    // instance.
    // TODO(msamuel): if count is large, we might try predicting the end date so
    // that we can convert the COUNT condition to an UNTIL condition.
    canShortcutAdvance = false;
  } else if (until) {
    condition = untilCondition_(until);
  }

  // combine filters into a single function
  var filter;
  switch (filters.length) {
    case 0:
      filter = permissiveFilter_;
      break;
    case 1:
      filter = filters[0];
      break;
    case 2:
      filter = chainedFilters_(filters);
      break;
  }

  var instanceGenerator = serialInstanceGenerator_;
  if (bySetPos) {
    switch (freq) {
    case RRULE_FREQ_WEEKLY:
    case RRULE_FREQ_MONTHLY:
    case RRULE_FREQ_YEARLY:
      instanceGenerator = bySetPosInstanceGenerator_(bySetPos, freq, wkst);
      break;
    }
  }

  return new RecurrenceIterator(
      dtStart, condition, filter, instanceGenerator,
      yearGenerator, monthGenerator, dayGenerator, canShortcutAdvance,
      throttle);
}

/** An iterator that generates dates from an RFC2445 Recurrence Rule */
function RecurrenceIterator(
    dtStart, condition, filter, instanceGenerator,
    yearGenerator, monthGenerator, dayGenerator, canShortcutAdvance,
    throttle) {
  /**
   * a function that determines when the recurrence ends.
   * Takes a date builder and yields shouldContinue:boolean.
   */
  this.condition_ = condition;
  /**
   * a function that applies secondary rules to eliminate some dates.
   * Takes a builder and yields isPartOfRecurrence:boolean.
   */
  this.filter_ = filter;
  /**
   * a function that applies the various period generators to generate an entire
   * date.
   * This may involve generating a set of dates and discarding all but those
   * that match the BYSETPOS rule.
   */
  this.instanceGenerator_ = instanceGenerator;
  /**
   * a function that takes a builder and populates the year field.
   * Returns false if no more years available.
   */
  this.yearGenerator_ = yearGenerator;
  /**
   * a function that takes a builder and populates the month field.
   * Returns false if no more months available in the builder's year.
   */
  this.monthGenerator_ = monthGenerator;
  /**
   * a function that takes a builder and populates the day of month field.
   * Returns false if no more days available in the builder's month.
   */
  this.dayGenerator_ = dayGenerator;
  /**
   * a date that has been computed but not yet yielded to the user.
   */
  this.pending_ = null;
  /**
   * used to build successive dates.
   * At the start of the building process, contains the last date generated.
   * Different periods are successively inserted into it.
   */
  this.builder_ = null;
  /** true iff the recurrence has been exhausted. */
  this.done_ = false;
  /** true iff the generated dates should be date-times. */
  this.hasTime_ = dtStart instanceof ICAL_DateTime;
  /** the start date */
  this.dtStart_ = dtStart;
  /**
   * false iff shorcutting advance would break the semantics of the iteration.
   * This may happen when, for example, the end condition requires that it see
   * every item.
   */
  this.canShortcutAdvance_ = canShortcutAdvance;

  /**
   * used to prevent runaway recurrence rules from doing infinite computation.
   * Has properties "limit_" and "current_" that specify how many distinct years
   * may be sampled between generated occurrences, and how can be sampled before
   * the next occurence is generated before we conclude there are none to
   * generate.
   */
  this.throttle_ = throttle;

  // initialize the builder and skip over any extraneous start instances
  this.builder_ = ical_builderCopy(dtStart);
  // apply the year and month generators so that we can start with the day
  // generator on the first call to FetchNext_.
  this.yearGenerator_(this.builder_);
  this.monthGenerator_(this.builder_);

  var startCmp = dtStart.getComparable();
  while (!this.done_) {
    this.pending_ = this.GenerateInstance_();
    if (!this.pending_) {
      this.done = true;
      break;
    } else if (this.pending_.getComparable() >= startCmp) {
      // we only apply the condition to the ones past dtStart to avoid counting
      // useless instances
      if (!this.condition_(this.pending_)) {
        this.done_ = true;
        this.pending_ = null;
      }
      break;
    }
  }

}

/** are there more dates in this recurrence? */
RecurrenceIterator.prototype.HasNext = function () {
  return null !== this.pending_ || (this.FetchNext_(), null !== this.pending_);
};

/** fetch and return the next date in this recurrence. */
RecurrenceIterator.prototype.Next = function () {
  var next = this.pending_ || (this.FetchNext_(), this.pending_);
  this.pending_ = null;
  return next;
};

/**
 * skip over all instances of the recurrence before the given date, so that the
 * next call to {@link #Next} will return a date on or after the given date
 * (assuming the recurrence includes such a date.
 */
RecurrenceIterator.prototype.AdvanceTo = function (date) {
  var dateCmp = date.getComparable();
  if (this.builder_ && dateCmp < this.builder_.toDate().getComparable()) {
    return;
  }
  this.pending_ = null;

  if (this.canShortcutAdvance_) {
    // skip years before date.year
    if (this.builder_.year < date.year) {
      do {
        if (!this.yearGenerator_(this.builder_)) {
          this.done_ = true;
          return;
        }
      } while (this.builder_.year < date.year);
      while (!this.monthGenerator_(this.builder_)) {
        if (!this.yearGenerator_(this.builder_)) {
          this.done_ = true;
          return;
        }
      }
    }
    // skip months before date.year/date.month
    while (this.builder_.year === date.year &&
           this.builder_.month < date.month) {
      while (!this.monthGenerator_(this.builder_)) {
        // if there are more years available fetch one
        if (!this.yearGenerator_(this.builder_)) {
          // otherwise the recurrence is exhausted
          this.done_ = true;
          return;
        }
      }
    }
  }

  // consume any remaining instances
  while (!this.done_) {
    var d = this.GenerateInstance_();
    if (!d) {
      this.done_ = true;
    } else {
      if (!this.condition_(d)) {
        this.done_ = true;
      } else if (d.getComparable() >= dateCmp) {
        this.pending_ = d;
        break;
      }
    }
  }
};

/** calculates and stored the next date in this recurrence. */
RecurrenceIterator.prototype.FetchNext_ = function () {
  if (this.pending_ || this.done_) { return; }

  var d = this.GenerateInstance_();

  // check the exit condition
  if (d && this.condition_(d)) {
    this.pending_ = d;
    // we did some useful work, so reset the throttle..
    this.throttle_.current_ = this.throttle_.limit_;
  } else {
    this.done_ = true;
  }
};

RecurrenceIterator.prototype.GenerateInstance_ = function () {
  if (!this.instanceGenerator_(this.builder_)) { return null; }
  // TODO(msamuel): apply byhour, byminute, bysecond rules here
  return this.hasTime_ ? this.builder_.toDateTime() : this.builder_.toDate();
};

/**
 * constructs a function that generates years successively counting from the
 * first year passed in.
 * @param interval {int} number of years to advance each step.
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 * @param throttle an object with limit_ and current_ properties as described in
 *   the documentation for RecurrenceIterator.throttle_
 * @return the year in dtStart the first time called and interval + last return
 *   value on subsequent calls.
 */
function serialYearGenerator_(interval, dtStart, throttle) {
  // these parameter declarations are accessible by the generated function and
  // so serve as C++-style static local variables

  // the last year seen
  var year = dtStart.year - interval;
  return function (builder) {
    // make sure things halt even if the rrule is bad.
    // Rules like
    //   FREQ=YEARLY;BYMONTHDAY=30;BYMONTH=2
    // should halt.
    // There is corresponding code in RecurrenceIterator::FetchNext that resets
    // current_ when useful work is done.
    if (--throttle.current_ < 0) { return false; }
    builder.year = year += interval;
    return true;
  };
}

/**
 * constructs a function that generates months in the given builder's year
 * successively counting from the first month passed in.
 * @param interval {int} number of months to advance each step.
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 * @return the year in dtStart the first time called and interval + last return
 *   value on subsequent calls.
 */
function serialMonthGenerator_(interval, dtStart) {
  var year = dtStart.year;
  var month = dtStart.month - interval;
  while (month < 1) { month += 12; --year; }
  return function (builder) {
    var nmonth;
    if (year !== builder.year) {
      var monthsBetween = (builder.year - year) * 12 - (month - 1);
      nmonth = ((interval - (monthsBetween % interval)) % interval) + 1;
      if (nmonth > 12) {
        // don't update year so that the difference calculation above is
        // correct when this function is reentered with a different year
        return false;
      }
      year = builder.year;
    } else {
      nmonth = month + interval;
      if (nmonth > 12) {
        return false;
      }
    }
    month = builder.month = nmonth;
    return true;
  };
}

/**
 * constructs a function that generates every day in the current month that is
 * an integer multiple of interval days from dtStart.
 */
function serialDayGenerator_(interval, dtStart) {
  var year, month, date;
  var nDays;  // ndays in the last month encountered

  // step back one interval
  var dtStartMinus1 = ical_builderCopy(dtStart);
  dtStartMinus1.date -= interval;
  dtStartMinus1 = dtStartMinus1.toDate();
  year = dtStartMinus1.year;
  month = dtStartMinus1.month;
  date = dtStartMinus1.date;
  dtStartMinus1 = null;
  nDays = ICAL_daysInMonth(year, month);

  return function (builder) {
    var ndate;
    if (year === builder.year && month === builder.month) {
      ndate = date + interval;
      if (ndate > nDays) {
        return false;
      }
    } else {
      nDays = ICAL_daysInMonth(builder.year, builder.month);
      if (interval !== 1) {
        // calculate the number of days between the first of the new month and
        // the old date and extend it to make it an integer multiple of
        // interval
        var daysBetween = ICAL_daysBetween(builder.year, builder.month, 1,
                                           year, month, date);
        ndate = ((interval - (daysBetween % interval)) % interval) + 1;
        if (ndate > nDays) {
          // need to early out without updating year or month so that the
          // next time we enter, with a different month, the daysBetween call
          // above compares against the proper last date
          return false;
        }
      } else {
        ndate = 1;
      }
      year = builder.year;
      month = builder.month;
    }
    date = builder.date = ndate;
    return true;
  };
}

/**
 * constructs a function that yields the specified years in increasing order.
 */
function byYearGenerator_(years, dtStart) {
  years = years.slice(0, years.length);  // defensive copy
  uniquify(years);

  // index into years
  var i = 0;
  while (i < years.length && dtStart.year > years[i]) { ++i; }

  return function (builder) {
    if (i >= years.length) { return false; }
    builder.year = years[i++];
    return true;
  };
}

/**
 * constructs a function that yields the specified months in increasing order
 * for each year.
 * @param {Array<int in [1-12]>} month
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 */
function byMonthGenerator_(months, dtStart) {
  months = months.slice(0, months.length);
  uniquify(months);
  var i = 0;
  var year = dtStart.year;

  return function (builder) {
    if (year !== builder.year) {
      i = 0;
      year = builder.year;
    }
    if (i >= months.length) { return false; }
    builder.month = months[i++];
    return true;
  };
}

/**
 * constructs a function that yields the specified dates
 * (possibly relative to end of month) in increasing order
 * for each month seen.
 * @param dates {Array<int in [-53,53] != 0>}
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 */
function byMonthDayGenerator_(dates, dtStart) {
  dates = dates.slice(0, dates.length);
  uniquify(dates);

  var year = dtStart.year;
  var month = dtStart.month;
  // list of generated dates for the current month
  var posDates = [];
  // index of next date to return
  var i = 0;

  convertDatesToAbsolute_();

  function convertDatesToAbsolute_() {
    posDates = [];
    var nDays = ICAL_daysInMonth(year, month);
    for (var j = 0; j < dates.length; ++j) {
      var date = dates[j];
      if (date < 0) {
        date += nDays + 1;
      }
      if (date >= 1 && date <= nDays) {
        posDates.push(date);
      }
    }
    uniquify(posDates);
  };

  return function (builder) {
    if (year !== builder.year || month !== builder.month) {
      year = builder.year;
      month = builder.month;

      convertDatesToAbsolute_();

      i = 0;
    }
    if (i >= posDates.length) { return false; }
    builder.date = posDates[i++];
    return true;
  };
}

/**
 * constructs a day generator based on a BYDAY rule.
 *
 * @param days {Array<RRule_WeekDayNum>} day of week, number pairs, e.g. SU,3MO
 *   means every sunday and the 3rd monday.
 * @param weeksInYear {boolean} are the week numbers meant to be weeks in the
 *   current year, or weeks in the current month.
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 */
function byDayGenerator_(days, weeksInYear, dtStart) {
  days = days.slice(0, days.length);

  var year = dtStart.year;
  var month = dtStart.month;
  // list of generated dates for the current month
  var dates = [];
  // index of next date to return
  var i = 0;

  generateDates_();

  function generateDates_() {
    var nDays;
    var dow0;
    var nDaysInMonth = ICAL_daysInMonth(year, month);
    // index of the first day of the month in the month or year
    var d0;

    if (weeksInYear) {
      nDays = ICAL_daysInYear(year);
      dow0 = ICAL_firstDayOfWeekInMonth(year, 1);
      d0 = ICAL_dayOfYear(year, month, 1);
    } else {
      nDays = nDaysInMonth;
      dow0 = ICAL_firstDayOfWeekInMonth(year, month);
      d0 = 0;
    }

    // an index not greater than the first week of the month in the month or
    // year
    var w0 = (d0 / 7) | 0;

    // iterate through days and resolve each [week, day of week] pair to a
    // day of the month
    dates = [];
    for (var j = 0; j < days.length; ++j) {
      var day = days[j];
      if (day.num) {
        var date = dayNumToDate_(
            dow0, nDays, day.num, day.wday, d0, nDaysInMonth);
        if (date) { dates.push(date); }
      } else {
        var wn = w0 + 6;
        for (var w = w0; w <= wn; ++w) {
          var date = dayNumToDate_(
              dow0, nDays, w, day.wday, d0, nDaysInMonth);
          if (date) { dates.push(date); }
        }
      }
    }
    uniquify(dates);
  };

  return function (builder) {
    if (year !== builder.year || month !== builder.month) {
      year = builder.year;
      month = builder.month;

      generateDates_();
      // start at the beginning of the month
      i = 0;
    }
    if (i >= dates.length) { return false; }
    var date = dates[i++];
    builder.date = date;
    return true;
  };
}

/**
 * constructs a day filter based on a BYDAY rule.
 * @param days {Array<RRule_WeekDayNum>} days
 * @param weeksInYear {boolean} are the week numbers meant to be weeks in the
 *   current year, or weeks in the current month.
 */
function byDayFilter_(days, weeksInYear, wkst) {
  return function (builder) {
    var dow = ICAL_getDayOfWeek(builder);

    var nDays;
    // first day of the week in the given year or month
    var dow0;
    // where does builder appear in the year or month?
    // in [0, lengthOfMonthOrYear - 1]
    var instance;
    if (weeksInYear) {
      nDays = ICAL_daysInYear(builder.year);
      dow0 = ICAL_firstDayOfWeekInMonth(builder.year, 1);
      instance = ICAL_dayOfYear(builder.year, builder.month, builder.date);
    } else {
      nDays = ICAL_daysInMonth(builder.year, builder.month);
      dow0 = ICAL_firstDayOfWeekInMonth(builder.year, builder.month);
      instance = builder.date - 1;
    }

    var dateWeekNo;
    if (wkst <= dow) {
      dateWeekNo = (1 + instance / 7) | 0;
    } else {
      dateWeekNo = (instance / 7) | 0;
    }

    // TODO(msamuel): according to section 4.3.10
    //     Week number one of the calendar year is the first week which
    //     contains at least four (4) days in that calendar year. This
    //     rule part is only valid for YEARLY rules.
    // That's mentioned under the BYWEEKNO rule, and there's no mention
    // of it in the earlier discussion of the BYDAY rule.
    // Does it apply to yearly week numbers calculated for BYDAY rules in
    // a FREQ=YEARLY rule?

    for (var i = days.length; --i >= 0;) {
      var day = days[i];
      if (day.wday === dow) {
        var weekNo = day.num;
        if (!weekNo) { return true; }

        if (weekNo < 0) { weekNo = invertWeekdayNum_(day, dow0, nDays); }

        if (dateWeekNo === weekNo) { return true; }
      }
    }
    return false;
  };
}

/**
 * constructs a day filter based on a BYDAY rule.
 * @param days {Array<RRule_WeekDayNum in [-31, 31] != 0>} days of the month
 */
function byMonthDayFilter_(monthDays) {
  return function (builder) {
    var nDays = ICAL_daysInMonth(builder.year, builder.month);
    for (var i = monthDays.length; --i >= 0;) {
      var day = monthDays[i];
      if (day < 0) { day += nDays + 1; }
      if (day === builder.date) { return true; }
    }
    return false;
  };
}

/**
 * Compute an absolute week number given a relative one.
 * The day number -1SU refers to the last Sunday, so if there are 5 Sundays
 * in a period that starts on dow0 with nDays, then -1SU is 5SU.
 * Depending on where its used it may refer to the last Sunday of the year
 * or of the month.
 *
 * @param weekDayNum {RRule_WeekDayNum} -1SU in the example above.
 * @param dow0 {int} the day of the week of the first day of the week or month.
 *   One of the RRULE_WDAY_* constants.
 * @param nDays {int} the number of days in the month or year.
 * @return {int} an abolute week number, e.g. 5 in the example above.
 *   Valid if in [1,53].
 */
function invertWeekdayNum_(weekdayNum, dow0, nDays) {
  // how many are there of that week?
  return countInPeriod_(weekdayNum.wday, dow0, nDays) + weekdayNum.num + 1;
}

/**
 * the number of occurences of dow in a period nDays long where the first day
 * of the period has day of week dow0.
 */
function countInPeriod_(dow, dow0, nDays) {
  if (dow >= dow0) {
    return 1 + (((nDays - (dow - dow0) - 1) / 7) | 0);
  } else {
    return 1 + (((nDays - (7 - (dow0 - dow)) - 1) / 7) | 0);
  }
}

/**
 * constructs a generator that yields each day in the current month that falls
 * in one of the given weeks of the year.
 * @param weekNos {Array<int in [-53,53] != 0>} week numbers
 * @param wkst {int in RRULE_WDAY_*} day of the week that the week starts on.
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 */
function byWeekNoGenerator_(weekNos, wkst, dtStart) {
  weekNos = weekNos.slice(0, weekNos.length);
  uniquify(weekNos);
  var year = dtStart.year;
  var month = dtStart.month;
  // number of weeks in the last year seen
  var weeksInYear = null;
  // dates generated anew for each month seen
  var dates = [];
  // index into dates
  var i = 0;

  // day of the year of the start of week 1 of the current year.
  // Since week 1 may start on the previous year, this may be negative.
  var doyOfStartOfWeek1;

  function checkYear_() {
    // if the first day of jan is wkst, then there are 7.
    // if the first day of jan is wkst + 1, then there are 6
    // if the first day of jan is wkst + 6, then there is 1
    var dowJan1 = ICAL_firstDayOfWeekInMonth(year, 1);
    var nDaysInFirstWeek = 7 - ((7 + dowJan1 - wkst) % 7);
    // number of days not in any week
    var nOrphanedDays = 0;
    // according to RFC 2445
    //     Week number one of the calendar year is the first week which
    //     contains at least four (4) days in that calendar year.
    if (nDaysInFirstWeek < 4) {
      nOrphanedDays = nDaysInFirstWeek;
      nDaysInFirstWeek = 7;
    }

    // calculate the day of year (possibly negative) of the start of the
    // first week in the year.  This day must be of wkst.
    doyOfStartOfWeek1 = nDaysInFirstWeek - 7 + nOrphanedDays;

    weeksInYear =
      Math.ceil((ICAL_daysInYear(year) - nOrphanedDays) / 7);
  };

  function checkMonth_() {
    // the day of the year of the 1st day in the month
    var doyOfMonth1 = ICAL_dayOfYear(year, month, 1);
    // the week of the year of the 1st day of the month.  approximate.
    var weekOfMonth = (((doyOfMonth1 - doyOfStartOfWeek1) / 7) | 0) + 1;
    // number of days in the month
    var nDays = ICAL_daysInMonth(year, month);

    // generate the dates in the month
    dates = [];
    for (var j = 0; j < weekNos.length; j++) {
      var weekNo = weekNos[j];
      if (weekNo < 0) {
        weekNo += weeksInYear + 1;
      }
      if (weekNo >= weekOfMonth - 1 && weekNo <= weekOfMonth + 6) {
        for (var d = 0; d < 7; ++d) {
          var date =
            ((weekNo - 1) * 7 + d + doyOfStartOfWeek1 - doyOfMonth1) + 1;
          if (date >= 1 && date <= nDays) {
            dates.push(date);
          }
        }
      }
    }
    uniquify(dates);
  };

  checkYear_();
  checkMonth_();

  return function (builder) {

    // this is a bit odd, since we're generating days within the given weeks of
    // the year within the month/year from builder
    if (year !== builder.year || month !== builder.month) {
      if (year !== builder.year) {
        year = builder.year;
        checkYear_();
      }
      month = builder.month;
      checkMonth_();

      i = 0;
    }

    if (i >= dates.length) { return false; }
    builder.date = dates[i++];
    return true;
  };
}

/**
 * constructs a filter that accepts only every interval-th week from the week
 * containing dtStart.
 * @param interval {int > 0} number of weeks
 * @param wkst {int in RRULE_WDAY_*} day of the week that the week starts on.
 * @param dtStart {ICAL_Date|ICAL_DateTime}
 */
function weekIntervalFilter_(interval, wkst, dtStart) {
  // the latest day with day of week wkst on or before dtStart
  var wkStart = ical_builderCopy(dtStart);
  wkStart.date -= (7 + ICAL_getDayOfWeek(dtStart) - wkst) % 7;
  wkStart = wkStart.toDate();

  return function (builder) {
    var daysBetween = ICAL_daysBetween(
        builder.year, builder.month, builder.date,
        wkStart.year, wkStart.month, wkStart.date);
    return !(Math.floor(daysBetween / 7) % interval);
  };
}

/**
 * constructs a day generator that generates dates in the current month that
 * fall on one of the given days of the year.
 * @param {Array<int in [-366,366] != 0>}
 */
function byYearDayGenerator_(yearDays, dtStart) {
  yearDays = yearDays.slice(0, yearDays.length);
  uniquify(yearDays);

  var year = dtStart.year;
  var month = dtStart.month;
  var dates = [];
  var i = 0;

  checkMonth_();

  function checkMonth_() {
    // now, calculate the first week of the month
    var doyOfMonth1 = ICAL_dayOfYear(year, month, 1);
    var nDays = ICAL_daysInMonth(year, month);
    var nYearDays = ICAL_daysInYear(year);
    dates = [];
    for (var j = 0; j < yearDays.length; j++) {
      var yearDay = yearDays[j];
      if (yearDay < 0) { yearDay += nYearDays + 1; }
      var date = yearDay - doyOfMonth1;
      if (date >= 1 && date <= nDays) { dates.push(date); }
    }
    uniquify(dates);
  };

  return function (builder) {
    if (year !== builder.year || month !== builder.month) {
      year = builder.year;
      month = builder.month;

      checkMonth_();

      i = 0;
    }
    if (i >= dates.length) { return false; }
    builder.date = dates[i++];
    return true;
  };
}

/** given an array of integers, sorts it and removes duplicates. */
function uniquify(ints) {
  ints.sort(function (a, b) { return a - b; });
  var last = null;
  for (var i = ints.length; --i >= 0;) {
    if (last === ints[i]) {
      ints.splice(i, 1);
    } else {
      last = ints[i];
    }
  }
}

/**
 * advances builder to the earliest day on or after builder that falls on wkst.
 * @param builder {ICAL_DTBuilder} normalized.
 * @param wkst {int in RRULE_WDAY_*} the day of the week that the week starts on
 */
function rollToNextWeekStart_(builder, wkst) {
  builder.normalize_();
  builder.date += (7 - ((7 + (builder.getDayOfWeek() - wkst)) % 7)) % 7;
  builder.normalize_();
}

/**
 * the earliest day on or after d that falls on wkst.
 * @param {Temporal} d
 * @param {int in RRULE_WDAY_*} wkst the day of the week that the week starts on
 */
function nextWeekStart_(d, wkst) {
  var delta = (7 - ((7 + (d.getDayOfWeek() - wkst)) % 7)) % 7;
  if (delta === 0) { return d.toDate(); }
  var db = ical_builderCopy(d);
  db.date += delta;
  return db.toDate();
}

/**
 * given a weekday number, such as -1SU, returns the day of the month that it
 * falls on.
 * The weekday number may be refer to a week in the current month in some
 * contexts or a week in the current year in other contexts.
 * @param dow0 {int} the day of week of the first day in the current year/month.
 * @param nDays {int} the number of days in the current year/month.
 *   In [28,29,30,31,365,366].
 * @param weekNum {RRule_WeekdayNum} -1SU in the example above.
 * @param d0 {int} the number of days between the 1st day of the current
 *   year/month and the current month.
 * @param nDaysInMonth {int} the number of days in the current month.
 */
function dayNumToDate_(dow0, nDays, weekNum, dow, d0, nDaysInMonth) {
  var date;
  // if dow is wednesday, then this is the date of the first wednesday
  var firstDateOfGivenDow = 1 + ((7 + dow - dow0) % 7);
  if (weekNum > 0) {
    date = ((weekNum - 1) * 7) + firstDateOfGivenDow - d0;
  } else {  // count weeks from end of month
    // calculate last day of the given dow
    var lastDateOfGivenDow = firstDateOfGivenDow + (7 * 54);
    lastDateOfGivenDow -= 7 * Math.ceil(((lastDateOfGivenDow - nDays) / 7));
    date = lastDateOfGivenDow + 7 * (weekNum + 1) - d0;
  }
  if (date <= 0 || date > nDaysInMonth) { return 0; }
  return date;
}

/** a condition that never halts. */
function optimisticCondition_() { return true; }

/** constructs a condition that fails after passing count dates. */
function countCondition_(count) {
  return function () {
    return --count >= 0;
  };
}

/**
 * constructs a condition that passes for every date on or before until.
 * @param until {ICAL_Date|ICAL_DateTime}
 */
function untilCondition_(until) {
  var cmp = until.getComparable();
  return function (builder) {
    return builder.getComparable() <= cmp;
  };
}

/**
 * a filter that allows anything to pass.
 */
function permissiveFilter_() {
  return true;
}

/**
 * constructs a filter that only passes a given input if all the given filters
 * pass it.
 */
function chainedFilters_(filters) {
  return function (toFilter) {
    for (var i = 0; i < filters.length; ++i) {
      if (!(filters[i](toFilter))) { return false; }
    }
    return true;
  };
}

/**
 * a collector that yields each date in the period without doing any set
 * collecting.
 */
function serialInstanceGenerator_() {
  // cascade through periods to compute the next date
  do {
    // until we run out of days in the current month
    while (!this.dayGenerator_(this.builder_)) {
      // until we run out of months in the current year
      while (!this.monthGenerator_(this.builder_)) {
        // if there are more years available fetch one
        if (!this.yearGenerator_(this.builder_)) {
          // otherwise the recurrence is exhausted
          return false;
        }
      }
    }
    // apply filters to generated dates
  } while (!this.filter_(this.builder_));

  return true;
}

function bySetPosInstanceGenerator_(setPos, freq, wkst) {
  setPos = setPos.slice(0, setPos.length);
  uniquify(setPos);

  var allPositive = setPos[0] > 0;
  var maxPos = setPos[setPos.length - 1];

  var pushback = null;
  var first = true;

  var candidates = [];
  var i = 0;
  return function () {
    while (i >= candidates.length) {
      // (1) Make sure that builder is appropriately initialized so that we
      // only generate instances in the next set

      var d0 = null;
      if (pushback) {
        d0 = pushback;
        this.builder_.year = d0.year;
        this.builder_.month = d0.month;
        this.builder_.date = d0.date;
        pushback = null;
      } else if (!first) {
        // we need to skip ahead to the next item since we didn't exhaust the
        // last period
        switch (freq) {
          case RRULE_FREQ_YEARLY:
            if (!this.yearGenerator_(this.builder_)) { return false; }
            // fallthru
          case RRULE_FREQ_MONTHLY:
            while (!this.monthGenerator_(this.builder_)) {
              if (!this.yearGenerator_(this.builder_)) { return false; }
            }
            break;
          case RRULE_FREQ_WEEKLY:
            // consume because incrementing date won't actually do anything
            var nextWeek = nextWeekStart_(this.builder_, wkst);
            var nextWeekCmp = nextWeek.getComparable();
            do {
              if (!serialInstanceGenerator_.call(this)) {
                return false;
              }
            } while (this.builder_.getComparable() < nextWeekCmp);
            d0 = this.builder_.toDate();
            break;
        }
      } else {
        first = false;
      }

      // (2) Build a set of the dates in the year/month/week that match the
      // other rule.
      var dates = [];
      if (d0) { dates.push(d0); }

      // Optimization: if min(bySetPos) > 0 then we already have absolute
      // positions, so we don't need to generate all of the instances for the
      // period.
      // This speeds up things like the first weekday of the year:
      //     RRULE:FREQ=YEARLY;BYDAY=MO,TU,WE,TH,FR,BYSETPOS=1
      // that would otherwise generate 260+ instances per one emitted
      // TODO(msamuel): this may be premature.  If needed, We could improve more
      // generally by inferring a BYMONTH generator based on distribution of
      // set positions within the year.
      var limit = allPositive ? maxPos : Infinity;

      while (limit > dates.length && serialInstanceGenerator_.call(this)) {
        var d = this.builder_.toDate();
        var contained = false;
        if (null === d0) {
          d0 = d;
          contained = true;
        } else {
          switch (freq) {
            case RRULE_FREQ_WEEKLY:
              var nb = ICAL_daysBetweenDates(d, d0);
              // Two dates (d, d0) are in the same week
              // if there isn't a whole week in between them and the
              // later day is later in the week than the earlier day.
              contained = nb < 7 &&
                          ((7 + d.getDayOfWeek() - wkst) % 7) >
                          ((7 + d0.getDayOfWeek() - wkst) % 7);
              break;
            case RRULE_FREQ_MONTHLY:
              contained = d0.month === d.month && d0.year === d.year;
              break;
            case RRULE_FREQ_YEARLY:
              contained = d0.year === d.year;
              break;
          }
        }
        if (contained) {
          dates.push(d);
        } else {
          // reached end of the set
          pushback = d;  // save d so we can use it later
          break;
        }
      }

      // (3) Resolve the positions to absolute positions and order them
      var absSetPos;
      if (allPositive) {
        absSetPos = setPos;
      } else {
        absSetPos = [];
        for (var j = 0; j < setPos.length; ++j) {
          var p = setPos[j];
          if (p < 0) { p = dates.length + p + 1; }
          absSetPos.push(p);
        }
        uniquify(absSetPos);
      }

      candidates = [];
      for (var j = 0; j < absSetPos.length; ++j) {
        var p = absSetPos[j] - 1;
        if (p >= 0 && p < dates.length) {
          candidates.push(dates[p]);
        }
      }
      i = 0;
      if (0 === candidates.length) {
        // none in this region, so keep looking
        candidates = [];
        continue;
      }
    }
    // (5) Emit a date.  It will be checked against the end condition and
    // dtStart elsewhere
    var d = candidates[i++];
    this.builder_.year = d.year;
    this.builder_.month = d.month;
    this.builder_.date = d.date;
    return true;
  };
}
