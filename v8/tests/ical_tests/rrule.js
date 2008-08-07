// Copyright 2005 Google Inc.
// All Rights Reserved.
// msamuel@google.com

// Functions for parsing, manipulating, and serializing RFC 2445 RRULEs.
// Caveats:
// - The UNTIL field is not treated properly.  We require the display timezone,
//   whereas RFC 2445 specifies that the end date must be in UTC.  We
//   translate dates in the RRULE on the java side.

// requires: debug.js common.js ical.js textEnglish.js util.js

// This file has been translated into java at
// java/com/google/calendar/common/RRule.java.
// Any changes to this file should be mirrored there.



/////////////////////////////////
// Public API
/////////////////////////////////

/**
 * given a set of heterogenous content lines (such as RRULES, EXDATES, etc.),
 * returns a list of folded content lines whose names match the given pattern.
 *
 * @param icalString a string of folded RFC2445 content lines.
 * @param pattern a regular expression as a string literal used to match
 *   content line names.  Must have matched parentheses and brackets.
 * @param opt_reverse if true, reverse the match to return those not matching
 *   the given pattern.
 */
function RRule_Fetch2445ContentLinesMatching(
    /*!String*/ icalString, /*!String*/ pattern, /*!boolean*/ opt_reverse) {
  // unfold content lines
  icalString = icalString.replace(/(?:\r\n?|\n)[ \t]/g, '');
  if (!icalString) { return []; }  // split -> [''] given empty string

  var lines = icalString.split(/[\r\n]+/);

  var matching = [];
  var regexp = new RegExp("^(?:" + pattern + ")[;:]", 'i');
  for (var i = 0; i < lines.length; ++i) {
    if ((!lines[i].match(regexp)) != !opt_reverse) {
      matching.push(lines[i]);
    }
  }

  return matching;
}

/**
 * encapsulates a recurrence rule.
 * This class provides a representation as an ical content line, the rrule
 * broken down by rule part, and operations to serialize it as an ical content
 * line and render a human readable description.
 */
function RRule(icalString) {
  this.name = null;
  /**
   * paramter values.  Does not currently allow multiple values for the same
   * property.
   */
  this.params = {};
  this.content = '';

  var unfolded = icalString.replace(/(?:\r\n?|\n)[ \t]/g, '');
  {
    var m = unfolded.match(
        /^((?:[^:;\"]|\"[^\"]*\")+)(;(?:[^:\"]|\"[^\"]*\")+)?:([\s\S]*)$/);
    if (!m) { RRule_BadContent_(icalString); }
  }
  this.name = m[1].toUpperCase();
  var paramText = m[2];
  this.content = m[3];
  if (paramText) {
    var rest = paramText;
    do {
      var m = rest.replace(/^;([^=]+)=(?:([^\";]*)|\"([^\"]*)\")/, "");
      if (!m) { RRule_BadPart_(rest); }
      rest = rest.substring(m[0].length);
      var k = m[1].toUpperCase();
      var v = m[2] && !m[3] ? m[3] : m[2];
      if (k in this.params) { RRule_DupePart_(m[0]); }
      this.params[k] = v;
    } while (rest);
  }
  /**
   * a map of the parsed members of the recur production.
   * The RRULE_MEMBER_* values are keys into this map.
   */
  this._members = RRule_ApplyObjectSchema_(
        this.name, this.params, this.content, RRule_Schema_);
}
var _RRule = RRule;
/**
 * formats as an *unfolded* RFC 2445 content line.
 * To fold, try <code>rrule._ToIcal().replace(/(.{0,75})/g, '$1\n ');</code>.
 *
 * @param opt_dtstart if supplied, then any until clause will be coerced to
 *   be a DateTime iff <code>(opt_dtstart instanceof ICAL_DateTime)</code> and
 *   will have the same time-of-day if FREQ >= DAILY.
 */
RRule.prototype._ToIcal = function (/*!ICAL_Temporal*/ opt_dtstart) {
  var buf = [];
  buf.push(this.name);
  for (var k in this.params) {
    buf.push(';', k, '=');
    var v = this.params[k];
    var s = '';
    if (v && v.constructor === Array) {
      for (var i = 0; i < v.length; ++i) {
        if (i) { s += ','; }
        var el = v[i];
        s += (el._ToIcal || el.toString).call(el);
      }
    } else {
      switch (k) {
        default:
          s = (v._ToIcal || v.toString).call(v);
          break;
      }
    }
    if (s.match(/[;:]/)) { buf.push('"', s, '"'); } else { buf.push(s); }
  }
  buf.push(':');
  var wroteMember = false;
  for (var k in this._members) {
    if (!wroteMember) { wroteMember = true; } else { buf.push(';'); }
    buf.push(k, '=');
    var v = this._members[k];
    var s = '';
    if (v && v.constructor === Array) {
      for (var i = 0; i < v.length; ++i) {
        if (i) { s += ','; }
        var el = v[i];
        s += (el._ToIcal || el.toString).call(el);
      }
    } else {
      switch (k) {
        case RRULE_MEMBER_FREQ:
          s = RRule_FreqToString(v);
          break;
        case RRULE_MEMBER_WKST:
          s = RRule_WeekDayToTwoLtrString(v);
          break;
        case RRULE_MEMBER_UNTIL:
          var untilDate = v;
          // coerce the until date to the same class as opt_dtstart
          if (opt_dtstart) {
            if (opt_dtstart instanceof ICAL_DateTime) {
              if (this._members[RRULE_MEMBER_FREQ] >= RRULE_FREQ_DAILY) {
                untilDate = ical_builderCopy(untilDate);
                untilDate.hour = opt_dtstart.hour;
                untilDate.minute = opt_dtstart.minute;
                untilDate.second = opt_dtstart.second;
              } else if (!(untilDate instanceof ICAL_DateTime)) {
                untilDate = ical_builderCopy(untilDate);
                untilDate.hour = 23;
                untilDate.minute = 59;
                untilDate.second = 59;
              }
              untilDate = untilDate.toDateTime();
            } else {
              untilDate = untilDate.toDate();
            }
          }
          s = untilDate.toString();
          break;
        default:
          s = (v._ToIcal || v.toString).call(v);
          break;
      }
    }
    buf.push(s);
  }
  return buf.join('');
};

/**
 * returns a human language description as html or null if this rule is
 * inscrutable.
 */
RRule.prototype._ToHumanLanguage = function (dtstart) {
  AssertTrue(dtstart instanceof ICAL_Date || dtstart instanceof ICAL_DateTime);
  if (RRULE_MEMBER_BYSETPOS in this._members) { return null; }
  if ((RRULE_MEMBER_BYMONTHDAY in this._members) &&
      (RRULE_MEMBER_BYDAY in this._members)) {
    // can't format "every Friday the 13th"
    return null;
  }

  // one of the RRULE_FREQ attributes.  Required.
  var freq = this._members[RRULE_MEMBER_FREQ];
  // the number of periods to advance each step.  If occurences only fall on
  // every second Period then interval is 2.
  var interval = this._members[RRULE_MEMBER_INTERVAL] || 1;

  // text specifying the period within which repetitions occur.
  var period;
  switch (freq) {
    case RRULE_FREQ_SECONDLY:
    case RRULE_FREQ_MINUTELY:
    case RRULE_FREQ_HOURLY:
      // we don't handle secondly minutely, or hourly
      return null;
    case RRULE_FREQ_DAILY:
      period = 1 === interval ?
               MSG_RECUR_DAILY :
               MSG_RECUR_N_DAILY(interval);
      break;
    case RRULE_FREQ_WEEKLY:
      period = 1 === interval ?
               MSG_RECUR_WEEKLY :
               MSG_RECUR_N_WEEKLY(interval);
      break;
    case RRULE_FREQ_MONTHLY:
      period = 1 === interval ?
               MSG_RECUR_MONTHLY :
               MSG_RECUR_N_MONTHLY(interval);
      break;
    case RRULE_FREQ_YEARLY:
      period = 1 === interval ?
               MSG_RECUR_ANNUALLY :
               MSG_RECUR_N_ANNUALLY(interval);
      break;
    default:
      Fail('bad freq ' + freq);
      break;
  }

  // The phase specifies the days within each period that events occur on.
  // It may specify multiple events, since, e.g., a weekly event can fall on
  // both tuesday and thursday within that week.
  var phase;
  switch (freq) {
    case RRULE_FREQ_DAILY:
      phase = '';
      break;
    case RRULE_FREQ_WEEKLY:
      // If rrule specifies a dow, use that, otherwise, assume from dtstart
      var daysOfWeek;
      if (RRULE_MEMBER_BYDAY in this._members) {
        var byday = this._members[RRULE_MEMBER_BYDAY];
        daysOfWeek = new Array(byday.length);
        for (var i = 0; i < byday.length; ++i) {
          daysOfWeek[i] = byday[i].wday;
        }
      } else {
        daysOfWeek = [ dtstart.getDayOfWeek() ];
      }
      if (MSG_IsDayOfWeekGroup(daysOfWeek)) {
        phase = MSG_RECUR_WEEKLY_PLURAL_DOW(MSG_DaysOfWeek(daysOfWeek, true));
      } else {
        phase = MSG_RECUR_WEEKLY_DOW(MSG_DaysOfWeek(daysOfWeek, true));
      }
      break;
    case RRULE_FREQ_MONTHLY:
      // If rrule specifies days within weeks, use those
      if (RRULE_MEMBER_BYDAY in this._members) {
        var decomposed = RRule_DecomposeByDayList_(
          this._members[RRULE_MEMBER_BYDAY], 5);
        if (!decomposed) { return null; }

        if (decomposed.weeks[0] < 0) {
          var weekNum = decomposed.weeks[0];
          if (weekNum == -1) {
            phase = MSG_RECUR_MONTHLY_LAST_WEEK(
                MSG_DaysOfWeek(decomposed.daysOfWeek));
          } else {
            phase = MSG_RECUR_MONTHLY_NEG_WEEK(
                MSG_DaysOfWeek(decomposed.daysOfWeek), -weekNum);
          }
        } else {
          if (!decomposed.allWeeks) {
            if (1 !== decomposed.weeks.length) {
              phase = MSG_RECUR_MONTHLY_POS_WEEKS(
                  MSG_DaysOfWeek(decomposed.daysOfWeek),
                  MSG_CommaList(decomposed.weeks));
            } else {
              var phaseMsg;
              switch (decomposed.weeks[0]) {
                case 1:
                  phaseMsg = MSG_RECUR_MONTHLY_FIRST_WEEK;
                  break;
                case 2:
                  phaseMsg = MSG_RECUR_MONTHLY_SECOND_WEEK;
                  break;
                case 3:
                  phaseMsg = MSG_RECUR_MONTHLY_THIRD_WEEK;
                  break;
                case 4:
                  phaseMsg = MSG_RECUR_MONTHLY_FOURTH_WEEK;
                  break;
                case 5:
                  phaseMsg = MSG_RECUR_MONTHLY_FIFTH_WEEK;
                  break;
                default:
                  return null;
              }
              phase = phaseMsg(MSG_DaysOfWeek(decomposed.daysOfWeek));
            }
          } else {
            phase = MSG_RECUR_MONTHLY_EVERY_WEEK(
                MSG_DaysOfWeek(decomposed.daysOfWeek));
          }
        }

      } else {
        var monthDays;
        // If rrule specifies days of the months, use those
        if (RRULE_MEMBER_BYMONTHDAY in this._members) {
          monthDays = this._members[RRULE_MEMBER_BYMONTHDAY];
        } else {
          // Otherwise, assume day of the month from dtstart
          // I can't find an example in 2445 which supports this, but the
          // ICAL parser we're using server side has this behavior.

          /*
          RRULE:FREQ=MONTHLY
          DTSTART:20020402T114500
          #### Start Tue Apr  2 03:45:00 2002
          #### Tue Apr  2 03:45:00 2002
          #### // daylight savings time
          #### Thu May  2 04:45:00 2002
          #### Sun Jun  2 04:45:00 2002
          ...
          #### Mon Nov  2 03:45:00 2037
          #### Wed Dec  2 03:45:00 2037
          Parser state: ICALPARSER_SUCCESS
          */
          monthDays = [dtstart.date];
        }

        // we can deal with all positive days, or a single negative day
        if (1 === monthDays.length && monthDays[0] < 0) {
          if (-1 === monthDays[0]) {
            phase = MSG_RECUR_MONTHLY_LAST_DAY;
          } else {
            phase = MSG_RECUR_MONTHLY_NEG_DAY(-monthDays[0]);
          }
        } else if (!RRule_ContainsNegs_(monthDays)) {
          if (1 !== monthDays.length) {
            phase = MSG_RECUR_MONTHLY_POS_DAYS(MSG_CommaList(monthDays));
          } else {
            phase = MSG_RECUR_MONTHLY_POS_DAY(monthDays[0]);
          }
        } else {  // can't format
          return null;
        }
      }
      break;
    case RRULE_FREQ_YEARLY:
      var months = null;
      var days = null;
      var monthdays = null;
      var yeardays = null;

      if (RRULE_MEMBER_BYMONTH in this._members) {
        months = this._members[RRULE_MEMBER_BYMONTH];
        if (RRULE_MEMBER_BYDAY in this._members) {
          days = this._members[RRULE_MEMBER_BYDAY];
        } else if (RRULE_MEMBER_BYMONTHDAY in this._members) {
          monthdays = this._members[RRULE_MEMBER_BYMONTHDAY];
        } else {
          monthdays = [dtstart.date];
        }
      } else if (RRULE_MEMBER_BYMONTHDAY in this._members) {
        months = [dtstart.month];
        monthdays = this._members[RRULE_MEMBER_BYMONTHDAY];
      } else if (RRULE_MEMBER_BYYEARDAY in this._members) {
        yeardays = this._members[RRULE_MEMBER_BYYEARDAY];
      } else if (RRULE_MEMBER_BYWEEKNO in this._members) {
        var weeknos = this._members[RRULE_MEMBER_BYWEEKNO];
        if (RRULE_MEMBER_BYDAY in this._members) {
          var byday = this._members[RRULE_MEMBER_BYDAY];
          days = [];
          for (var j = 0; j < byday.length; ++j) {
            if (0 === byday[j].num) {
              for (var i = 0; i < weeknos.length; ++i) {
                days.push(new RRule_WeekDayNum(weeknos[i], byday[j].wday));
              }
            } else {
              // since both filters are applied, wd only takes effect if
              // it matches a number in weekno
              for (var i = 0; i < weeknos.length; ++i) {
                if (weeknos[i] === byday[j].num) {
                  days.push(byday[j]);
                  break;
                }
              }
            }
          }
        } else {
          var dow = dtstart.getDayOfWeek();
          days = [];
          for (var i = 0; i < weeknos.length; ++i) {
            days.push(new RRule_WeekDayNum(weeknos[i], dow));
          }
        }
      } else if (RRULE_MEMBER_BYDAY in this._members) {
        // interval had better not be 1
        months = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        days = this._members[RRULE_MEMBER_BYDAY];
      } else {
        months = [dtstart.month];
        monthdays = [dtstart.date];
      }

      if (yeardays) {
        if (RRule_ContainsNegs_(yeardays)) {
          if (1 !== yeardays.length) { return null; }
          if (-1 == yeardays[0]) {
            phase = MSG_RECUR_ANNUALLY_LAST_DAY_OF_YEAR;
          } else {
            phase = MSG_RECUR_ANNUALLY_NEG_DAY_OF_YEAR(-yeardays[0]);
          }
        } else {
          if (1 !== yeardays.length) {
            phase = MSG_RECUR_ANNUALLY_DAYS_OF_YEAR(MSG_CommaList(yeardays));
          } else {
            phase = MSG_RECUR_ANNUALLY_DAY_OF_YEAR(yeardays[0]);
          }
        }
      } else if (monthdays) {
        if (RRule_ContainsNegs_(monthdays)) {
          if (1 !== monthdays.length) { return null; }
          if (-1 == monthdays[0]) {
            phase = MSG_RECUR_ANNUALLY_LAST_DAY_OF_MONTH(MSG_Months(months));
          } else {
            phase = MSG_RECUR_ANNUALLY_NEG_DAY_OF_MONTH(
                -monthdays[0], MSG_Months(months));
          }
        } else {
          if (1 === monthdays.length) {
            if (1 === months.length) {
              // the most common case
              phase = MSG_RECUR_ANNUALLY_DAY_OF_MONTH(
                  monthdays[0], MSG_Month(months[0]));
            } else {
              phase = MSG_RECUR_ANNUALLY_DAY_OF_MONTHS(
                  monthdays[0], MSG_Months(months));
            }
          } else {
            phase = MSG_RECUR_ANNUALLY_DAYS_OF_MONTHS(
                MSG_CommaList(monthdays), MSG_Months(months));
          }
        }
      } else if (days) {
        var decomposed = RRule_DecomposeByDayList_(days, months ? 5 : 53);
        if (!decomposed) { return null; }

        if (months) {
          if (decomposed.weeks[0] < 0) {
            if (-1 !== decomposed.weeks[0]) {
              phase = MSG_RECUR_ANNUALLY_NEG_WEEK_OF_MONTHS(
                MSG_DaysOfWeek(decomposed.daysOfWeek),
                -decomposed.weeks[0], MSG_Months(months));
            } else {
              phase = MSG_RECUR_ANNUALLY_LAST_WEEK_OF_MONTHS(
                MSG_DaysOfWeek(decomposed.daysOfWeek), MSG_Months(months));
            }
          } else {
            if (!decomposed.allWeeks) {
              if (1 !== decomposed.weeks.length) {
                phase = MSG_RECUR_ANNUALLY_WEEKS_OF_MONTHS(
                    MSG_DaysOfWeek(decomposed.daysOfWeek),
                    MSG_CommaList(decomposed.weeks), MSG_Months(months));
              } else {
                var phaseMsg;
                switch (decomposed.weeks[0]) {
                  case 1:
                    phaseMsg = MSG_RECUR_ANNUALLY_FIRST_WEEK_OF_MONTHS;
                    break;
                  case 2:
                    phaseMsg = MSG_RECUR_ANNUALLY_SECOND_WEEK_OF_MONTHS;
                    break;
                  case 3:
                    phaseMsg = MSG_RECUR_ANNUALLY_THIRD_WEEK_OF_MONTHS;
                    break;
                  case 4:
                    phaseMsg = MSG_RECUR_ANNUALLY_FOURTH_WEEK_OF_MONTHS;
                    break;
                  case 5:
                    phaseMsg = MSG_RECUR_ANNUALLY_FIFTH_WEEK_OF_MONTHS;
                    break;
                  default:
                    return null;
                }

                phase = phaseMsg(MSG_DaysOfWeek(decomposed.daysOfWeek),
                                 MSG_Months(months));
              }
            } else {
              phase = MSG_RECUR_ANNUALLY_EVERY_WEEK_OF_MONTHS(
                MSG_DaysOfWeek(decomposed.daysOfWeek), MSG_Months(months));
            }
          }
        } else {
          if (decomposed.weeks[0] < 0) {
            if (-1 !== decomposed.weeks[0]) {
              phase = MSG_RECUR_ANNUALLY_NEG_WEEK_OF_YEAR(
                MSG_DaysOfWeek(decomposed.daysOfWeek), -decomposed.weeks[0]);
            } else {
              phase = MSG_RECUR_ANNUALLY_LAST_WEEK_OF_YEAR(
                MSG_DaysOfWeek(decomposed.daysOfWeek));
            }
          } else {
            if (!decomposed.allWeeks) {
              if (1 !== decomposed.weeks.length) {
                phase = MSG_RECUR_ANNUALLY_WEEKS_OF_YEAR(
                    MSG_DaysOfWeek(decomposed.daysOfWeek),
                    MSG_CommaList(decomposed.weeks));
              } else {
                phase = MSG_RECUR_ANNUALLY_WEEK_OF_YEAR(
                    MSG_DaysOfWeek(decomposed.daysOfWeek),
                    decomposed.weeks[0]);
              }
            } else {
              phase = MSG_RECUR_ANNUALLY_EVERY_WEEK_OF_YEAR(
                MSG_DaysOfWeek(decomposed.daysOfWeek));
            }
          }
        }
      } else {
        return null;
      }
      break;
  }

  var limit;
  if (RRULE_MEMBER_UNTIL in this._members) {
    limit = MSG_RECUR_UNTIL(
        FormatDate(this._members[RRULE_MEMBER_UNTIL], false));
  } else if (RRULE_MEMBER_COUNT in this._members) {
    var count = this._members[RRULE_MEMBER_COUNT];
    if (1 == count) {
      return MSG_RECUR_ONCE;
    }

    limit = MSG_RECUR_COUNT(count);
  } else {
    limit = MSG_RECUR_FOREVER;
  }

  var message;
  if (dtstart instanceof ICAL_DateTime) {
    message = MSG_RECUR_AT_TIME(period, phase, limit, TimeToString(dtstart));
  } else {
    message = MSG_RECUR_ALLDAY(period, phase, limit);
  }
  message = message.replace(/(?:&nbsp;|&\#32;)+$/, '');
  message = message.replace(/[,\s]+$/, '');
  message = message.replace(/\s*,+/g, ',');
  message = message.replace(/^[\s,]+/g, '');

  return message;
};

/** an approximate number of days between occurences. */
RRule.prototype.ApproximateIntervalInDays = function () {
  var freqLengthDays;
  var nPerPeriod = 0;
  switch (this._members[RRULE_MEMBER_FREQ]) {
    case RRULE_FREQ_DAILY:
      freqLengthDays = 1;
      break;
    case RRULE_FREQ_WEEKLY:
      freqLengthDays = 7;
      if (RRULE_MEMBER_BYDAY in this._members) {
        nPerPeriod = this._members[RRULE_MEMBER_BYDAY].length;
      }
      break;
    case RRULE_FREQ_MONTHLY:
      freqLengthDays = 30;
      if (RRULE_MEMBER_BYDAY in this._members) {
        var byday = this._members[RRULE_MEMBER_BYDAY];
        for (var i = 0; i < byday.length; ++i) {
          // if it's every weekday in the month, assume four of that weekday,
          // otherwise there is one of that week-in-month,weekday pair
          nPerPeriod += byday[i].num ? 1 : 4;
        }
      } else if (RRULE_MEMBER_BYMONTHDAY in this._members) {
        nPerPeriod = this._members[RRULE_MEMBER_BYMONTHDAY].length;
      }
      break;
    case RRULE_FREQ_YEARLY:
      freqLengthDays = 365;

      var monthCount = 12;
      if (RRULE_MEMBER_BYMONTH in this._members) {
        monthCount = this._members[RRULE_MEMBER_BYMONTH].length;
      }

      if (RRULE_MEMBER_BYDAY in this._members) {
        var byday = this._members[RRULE_MEMBER_BYDAY];

        for (var i = 0; i < byday.length; ++i) {
          // if it's every weekend in the months in the year,
          // assume 4 of that weekday per month,
          // otherwise there is one of that week-in-month,weekday pair per month
          nPerPeriod += (byday[i].num ? 1 : 4) * monthCount;
        }
      } else if (RRULE_MEMBER_BYMONTHDAY in this._members) {
        nPerPeriod +=
          monthCount * this._members[RRULE_MEMBER_BYMONTHDAY].length;
      } else if (RRULE_MEMBER_BYYEARDAY in this._members) {
        nPerPeriod += this._members[RRULE_MEMBER_BYYEARDAY].length;
      }
      break;
    default: freqLengthDays = 0;
  }
  nPerPeriod = nPerPeriod || 1;

  var interval = this._members[RRULE_MEMBER_INTERVAL] || 1;
  return ((freqLengthDays / nPerPeriod) * interval) | 0;
};

// days of the week enum
// agrees with values returned by builtin Date.getDayOfWeek and the
// corresponding ical.js function/method.
var RRULE_WDAY_SUNDAY = 0;
var RRULE_WDAY_MONDAY = 1;
var RRULE_WDAY_TUESDAY = 2;
var RRULE_WDAY_WEDNESDAY = 3;
var RRULE_WDAY_THURSDAY = 4;
var RRULE_WDAY_FRIDAY = 5;
var RRULE_WDAY_SATURDAY = 6;

// frequency
var RRULE_FREQ_SECONDLY = 0;
var RRULE_FREQ_MINUTELY = 1;
var RRULE_FREQ_HOURLY = 2;
var RRULE_FREQ_DAILY = 3;
var RRULE_FREQ_WEEKLY = 4;
var RRULE_FREQ_MONTHLY = 5;
var RRULE_FREQ_YEARLY = 6;

// member names
var RRULE_MEMBER_FREQ = 'FREQ';
var RRULE_MEMBER_WKST = /* SAFE */ 'WKST';
var RRULE_MEMBER_UNTIL = /* SAFE */ 'UNTIL';
var RRULE_MEMBER_COUNT = 'COUNT';
var RRULE_MEMBER_INTERVAL = 'INTERVAL';
var RRULE_MEMBER_BYDAY = 'BYDAY';
var RRULE_MEMBER_BYMONTH = 'BYMONTH';
var RRULE_MEMBER_BYMONTHDAY = 'BYMONTHDAY';
var RRULE_MEMBER_BYWEEKNO = 'BYWEEKNO';
var RRULE_MEMBER_BYYEARDAY = 'BYYEARDAY';
var RRULE_MEMBER_BYSETPOS = 'BYSETPOS';
var RRULE_MEMBER_BYHOUR = 'BYHOUR';
var RRULE_MEMBER_BYMINUTE = 'BYMINUTE';
var RRULE_MEMBER_BYSECOND = 'BYSECOND';

function RRule_StringToFreq(str) {
  switch (str.toUpperCase()) {
    case 'SECONDLY': return RRULE_FREQ_SECONDLY;
    case 'MINUTELY': return RRULE_FREQ_MINUTELY;
    case 'HOURLY':   return RRULE_FREQ_HOURLY;
    case 'DAILY':    return RRULE_FREQ_DAILY;
    case 'WEEKLY':   return RRULE_FREQ_WEEKLY;
    case 'MONTHLY':  return RRULE_FREQ_MONTHLY;
    case 'YEARLY':   return RRULE_FREQ_YEARLY;
    default: return null;
  }
}

function RRule_TwoLtrStringToWeekDay(str) {
  switch (str.toUpperCase()) {
    case 'SU': return RRULE_WDAY_SUNDAY; break;
    case 'MO': return RRULE_WDAY_MONDAY; break;
    case 'TU': return RRULE_WDAY_TUESDAY; break;
    case 'WE': return RRULE_WDAY_WEDNESDAY; break;
    case 'TH': return RRULE_WDAY_THURSDAY; break;
    case 'FR': return RRULE_WDAY_FRIDAY; break;
    case 'SA': return RRULE_WDAY_SATURDAY; break;
    default: return null;
  }
}

function RRule_WeekDayToTwoLtrString(weekday) {
  switch (weekday) {
    case RRULE_WDAY_SUNDAY:    return 'SU'; break;
    case RRULE_WDAY_MONDAY:    return 'MO'; break;
    case RRULE_WDAY_TUESDAY:   return 'TU'; break;
    case RRULE_WDAY_WEDNESDAY: return 'WE'; break;
    case RRULE_WDAY_THURSDAY:  return 'TH'; break;
    case RRULE_WDAY_FRIDAY:    return 'FR'; break;
    case RRULE_WDAY_SATURDAY:  return 'SA'; break;
    default: return null;
  }
}

function RRule_FreqToString(freq) {
  switch (freq) {
    case RRULE_FREQ_SECONDLY: return 'SECONDLY';
    case RRULE_FREQ_MINUTELY: return 'MINUTELY';
    case RRULE_FREQ_HOURLY:   return 'HOURLY';
    case RRULE_FREQ_DAILY:    return 'DAILY';
    case RRULE_FREQ_WEEKLY:   return 'WEEKLY';
    case RRULE_FREQ_MONTHLY:  return 'MONTHLY';
    case RRULE_FREQ_YEARLY:   return 'YEARLY';
    default: return null;
  }
}

/**
 * represents a day of the week in a month such as the
 * third monday of the month.  A negative num indicates it counts from the
 * end of the month.
 * @constructor
 */
function RRule_WeekDayNum(
    /*int in [-53,53]*/ num, /*RRULE_WDAY*/ wday) {
  AssertTrue(num === (num | 0));  // is integral
  AssertTrue(-53 <= num && 53 >= num);
  AssertTrue(RRule_WeekDayToTwoLtrString(wday));
  /*
   Each BYDAY value can also be preceded by a positive (+n) or negative
   (-n) integer. If present, this indicates the nth occurrence of the
   specific day within the MONTHLY or YEARLY RRULE. For example, within
   a MONTHLY rule, +1MO (or simply 1MO) represents the first Monday
   within the month, whereas -1MO represents the last Monday of the
   month. If an integer modifier is not present, it means all days of
   this type within the specified frequency. For example, within a
   MONTHLY rule, MO represents all Mondays within the month.
  */
  this.num = num;
  this.wday = wday;
}
RRule_WeekDayNum.prototype.toString = function () {
  /* SAFE */
  return '[weekday num=' + this.num + ', wday=' +
    RRule_WeekDayToTwoLtrString(this.wday) + ']';
};
RRule_WeekDayNum.prototype._ToIcal = function () {
  return this.num ? '' + this.num + RRule_WeekDayToTwoLtrString(this.wday) :
    RRule_WeekDayToTwoLtrString(this.wday);
};

/////////////////////////////////
// Parser Helper functions and classes
/////////////////////////////////

function RRule_IntListFunctor_(absmin, absmax) {
  return function (commaSeparatedString) {
    var parts = commaSeparatedString.split(/,/);
    for (var i = 0; i < parts.length; ++i) {
      var n = parseInt(parts[i], 10);
      var absn = n < 0 ? -n : n;
      if (!(absmin <= absn && absmax >= absn)) {
        RRule_BadPart_(commaSeparatedString);
      }
      parts[i] = n;
    }
    return parts;
  };
}

function RRule_UnsignedIntListFunctor_(min, max) {
  return function (commaSeparatedString) {
    var parts = commaSeparatedString.split(/,/);
    for (var i = 0; i < parts.length; ++i) {
      var n = parseInt(parts[i], 10);
      if (!(min <= n && max >= n)) { RRule_BadPart_(commaSeparatedString); }
      parts[i] = n;
    }
    return parts;
  };
}

function RRule_ContainsNegs_(intArray) {
  for (var i = 0; i < intArray.length; ++i) {
    if (intArray[i] < 0) { return true; }
  }
  return false;
}

function RRule_DecomposeByDayList_(byday, numWeeks) {
  // a set of bitmasks per week.  If numWeeks is 5:
  // (5 negative, 5 positive, and 0 stands for all week).
  // Elements 0-4 stand for RRule_WeekDayNum.num in [-5..-1]
  // Element 5 stands for RRule_WeekDayNum.num == 0 which means all weeks
  // Elements 6-10 stands for RRule_WeekDayNum.num in [1..5]
  var dowPerWeek = new Array(numWeeks * 2 + 1);
  for (var i = dowPerWeek.length; --i >= 0;) { dowPerWeek[i] = 0; }

  // we can handle this if all weeks mentioned include the same days of
  // the week, all weeks are positive, or there is one week and it is
  // negative
  for (var i = 0; i < byday.length; ++i) {
    /*!RRule_WeekDayNum*/ var daySpec = byday[i];
    if (daySpec.num < -numWeeks || daySpec.num > numWeeks) { return null; }
    dowPerWeek[daySpec.num + numWeeks] |= (1 << daySpec.wday);
  }
  // set of days of the week mentioned for any week.
  var superMask = 0;
  // number of positive week mentioned.
  var negWeek = -1;
  // offset index of the negative week mentioned if any
  var numPos = 0;
  for (var i = 0; i < dowPerWeek.length; i++) {
    var mask = dowPerWeek[i];
    superMask |= mask;
    // RRule_WeekDayNum.num == 0 implies that all weeks are intended
    if (i > numWeeks) {
      if (0 !== (dowPerWeek[i] |= dowPerWeek[numWeeks])) {
        ++numPos;
      }
    } else if (i < numWeeks && mask) {
      // can't handle multiple negative weeks
      if (negWeek >= 0 && negWeek !== i) { return null; }
      negWeek = i;
    }
  }

  // can't handle a mixture of negative and positive weeks
  if (negWeek >= 0) {
    if (numPos) { return null; }
    for (var i = 0; i < dowPerWeek.length; i++) {
      if (dowPerWeek[i]) {
        // ensure that all weeks mentioned include all the days of the
        // week because we can't format
        //   "Tuesdays and Fridays on the second week, and
        //    Wednesdays on the third week, every month."
        if (superMask != dowPerWeek[i]) {
          return null;
        }
      }
    }
  }

  var daysOfWeek = [];
  for (var i = 0; superMask; ++i, superMask >>= 1) {
    if (superMask & 1) { daysOfWeek.push(i); }
  }

  var weeks;
  var allWeeks = false;
  if (negWeek >= 0) {
    weeks = [negWeek - numWeeks];
  } else {
    weeks = [];
    allWeeks = true;
    for (var i = numWeeks + 1; i < dowPerWeek.length; i++) {
      if (dowPerWeek[i]) {
        weeks.push(i - numWeeks);
      } else {
        allWeeks = false;
      }
    }
  }

  return {
    daysOfWeek: daysOfWeek,
    weeks: weeks,
    allWeeks: allWeeks
  };
}

/////////////////////////////////
// Parser Core
/////////////////////////////////

var rrule_ruleStack_ = [];  // for debugging
function RRule_ApplyParamsSchema_(rule, params, schema) {
  for (var name in params) {
    rrule_ruleStack_.push(rule);
    try {
      params[name] = (schema[rule]).call(schema, name, params[name]);
    } finally {
      rrule_ruleStack_.pop();
    }
  }
}

function RRule_ApplyParamSchema_(rule, name, value, schema) {
  // all elements are allowed extension parameters
  if (name.match(/^X-[A-Z0-9\-]+$/i)) { return value; }
  // if not an extension, apply the rule
  rrule_ruleStack_.push(rule);
  try {
    value = (schema[rule]).call(schema, name, value);
  } finally {
    rrule_ruleStack_.pop();
  }
  return value;
}

function RRule_ApplyContentSchema_(rule, content, schema) {
  rrule_ruleStack_.push(rule);
  //alert('rule=' + rule + ', content=' + content + ', schema=' + schema);
  try {
    content = (schema[rule]).call(schema, content);
  } finally {
    rrule_ruleStack_.pop();
  }
  return content;
}

function RRule_ApplyObjectSchema_(rule, params, content, schema) {
  rrule_ruleStack_.push(rule);
  try {
    return (schema[rule]).call(schema, params, content);
  } finally {
    rrule_ruleStack_.pop();
  }
}

/////////////////////////////////
// Parser Error Handling
/////////////////////////////////

function RRule_BadParam_(name, value) {
  Fail('parameter ' + name + ' has bad value [[' + value + ']] in ' +
       rrule_ruleStack_.join('::'));
}

function RRule_BadPart_(part, opt_msg) {
  if (opt_msg) { opt_msg = ' : ' + opt_msg; } else { opt_msg = ''; }
  Fail('cannot parse [[' + part + ']] in ' + rrule_ruleStack_.join('::') +
       opt_msg);
}

function RRule_DupePart_(part) {
  Fail('duplicate part [[' + part + ']] in ' + rrule_ruleStack_.join('::'));
}

function RRule_MissingPart_(partName, content) {
  Fail('missing part ' + partName + ' from [[' + content + ']] in ' +
       rrule_ruleStack_.join('::'));
}

function RRule_BadContent_(content) {
  Fail('cannot parse content line [[' + content + ']] in ' +
       rrule_ruleStack_.join('::'));
}

/////////////////////////////////
// ICAL Object Schema
/////////////////////////////////

/* SAFE */
var RRule_Schema_ = {
  // rrule      = "RRULE" rrulparam ":" recur CRLF
  'RRULE': function (params, content) {  // a content line rule
    RRule_ApplyParamsSchema_('rrulparam', params, this);
    return RRule_ApplyContentSchema_('recur', content, this);
  },
  // rrulparam  = *(";" xparam)
  'rrulparam': function (name, value) {  // a param rule
    RRule_BadParam_(name, value);
  },
  /*
recur      = "FREQ"=freq *(

             ; either UNTIL or COUNT may appear in a 'recur',
             ; but UNTIL and COUNT MUST NOT occur in the same 'recur'

             ( ";" "UNTIL" "=" enddate ) /
             ( ";" "COUNT" "=" 1*DIGIT ) /

             ; the rest of these keywords are optional,
             ; but MUST NOT occur more than once

             ( ";" "INTERVAL" "=" 1*DIGIT )          /
             ( ";" "BYSECOND" "=" byseclist )        /
             ( ";" "BYMINUTE" "=" byminlist )        /
             ( ";" "BYHOUR" "=" byhrlist )           /
             ( ";" "BYDAY" "=" bywdaylist )          /
             ( ";" "BYMONTHDAY" "=" bymodaylist )    /
             ( ";" "BYYEARDAY" "=" byyrdaylist )     /
             ( ";" "BYWEEKNO" "=" bywknolist )       /
             ( ";" "BYMONTH" "=" bymolist )          /
             ( ";" "BYSETPOS" "=" bysplist )         /
             ( ";" "WKST" "=" weekday )              /

             ( ";" x-name "=" text )
             )
  */
  'recur' : function (content) {  // a content rule
    var parts = content.split(/;/);
    var partMap = {};
    for (var i = 0; i < parts.length; ++i) {
      var p = parts[i];
      var m = parts[i].match(/^(FREQ|UNTIL|COUNT|INTERVAL|BYSECOND|BYMINUTE|BYHOUR|BYDAY|BYMONTHDAY|BYYEARDAY|BYWEEKDAY|BYWEEKNO|BYMONTH|BYSETPOS|WKST|X-[A-Z0-9\-]+)=([\s\S]*)/i);
      if (!m) { RRule_BadPart_(p, undefined); }
      var k = m[1].toUpperCase();
      if (k in partMap) { RRule_DupePart_(p); }
      partMap[k] = m[2];
    }
    if (!('FREQ' in partMap)) { RRule_MissingPart_('FREQ', content); }
    if (('UNTIL' in partMap) && ('COUNT' in partMap)) {
      RRule_BadPart_(content, 'UNTIL & COUNT are exclusive');
    }
    for (var name in partMap) {
      partMap[name] =
        RRule_ApplyContentSchema_(name, partMap[name], RRule_RecurPartSchema_);
    }
    return partMap;
  },

  // exdate     = "EXDATE" exdtparam ":" exdtval *("," exdtval) CRLF
  'EXDATE': function (params, content) {  // a content line rule
    RRule_ApplyParamsSchema_('exdtparam', params, this);
    if (!content) { RRule_BadContent_(content); }
    var parts = content.split(/,/);
    for (var i = 0; i < parts.length; ++i) {
      parts[i] = RRule_ApplyContentSchema_('exdtval', parts[i], undefined);
    }
    return parts;
  },
  /*
exdtparam  = *(

               ; the following are optional,
               ; but MUST NOT occur more than once

               (";" "VALUE" "=" ("DATE-TIME" / "DATE")) /

               (";" tzidparam) /

               ; the following is optional,
               ; and MAY occur more than once

               (";" xparam)

               )
  */
  'exdtparam': function (name, value) {  // a params rule
    if (/* SAFE */ 'VALUE' == name) {
      if (!value.match(/^DATE-TIME|DATE$/i)) { RRule_BadParam_(name, value); }
      return value;
    } else {
      return RRule_ApplyParamSchema_('tzidparam', name, value, this);
    }
  },
  // tzidparam  = "TZID" "=" [tzidprefix] paramtext CRLF
  // tzidprefix = "/"
  'tzidparam': function (name, value) {  // a param rule
    if ('TZID' != name) { RRule_BadParam_(name, value); }
    var m = value.match(/\/?(.*)$/);
    if (!m) { RRule_BadParam_(name, value); }
    return m[1];
  },

  'VALUE': function (name, value) {  // a param rule
    // the VALUE param is used to specify the type of the content
    switch (name) {
      case 'DATE-TIME': return ICAL_DateTime;
      case 'DATE': return ICAL_Date;
      case 'TIME': return ICAL_Time;
      case 'PERIOD': return ICAL_PeriodOfTime;
      case 'BINARY': return String;
      default:
        RRule_BadParam_(name, value);
    }
  },

  // so a trailing comma won't break it in IE everytime I add a field
  'ENDMARKER': null
};

/*

// list of exception dates

exdtval    = date-time / date
;Value MUST match value type

// exception rules
exrule     = "EXRULE" exrparam ":" recur CRLF

exrparam   = *(";" xparam)

// recurrence dates, a comma separated group of periods
rdate      = "RDATE" rdtparam ":" rdtval *("," rdtval) CRLF
rdtparam   = *(

               ; the following are optional,
                ; but MUST NOT occur more than once

                (";" "VALUE" "=" ("DATE-TIME" / "DATE" / "PERIOD")) /
                (";" tzidparam) /

                ; the following is optional,
                ; and MAY occur more than once

                (";" xparam)

                )

rdtval     = date-time / date / period
;Value MUST match value type

*/

/* SAFE */
var RRule_RecurPartSchema_ = {

  // "FREQ"=freq *(
  'FREQ': function (value) {
    return RRule_ApplyContentSchema_('freq', value, this);
  },

  //  ( ";" "UNTIL" "=" enddate ) /
  'UNTIL': function (value) {
    return RRule_ApplyContentSchema_('enddate', value, this);
  },

  //  ( ";" "COUNT" "=" 1*DIGIT ) /
  'COUNT': function (value) {
    if (!value.match(/^[0-9]+$/)) { RRule_BadContent_(value); }
    return parseInt(value, 10);
  },

  //  ( ";" "INTERVAL" "=" 1*DIGIT )          /
  'INTERVAL': function (value) {
    if (!value.match(/^[0-9]+$/)) { RRule_BadContent_(value); }
    return parseInt(value, 10);
  },

  //  ( ";" "BYSECOND" "=" byseclist )        /
  'BYSECOND': function (value) {
    return RRule_ApplyContentSchema_('byseclist', value, this);
  },

  //  ( ";" "BYMINUTE" "=" byminlist )        /
  'BYMINUTE': function (value) {
    return RRule_ApplyContentSchema_('byminlist', value, this);
  },

  //  ( ";" "BYHOUR" "=" byhrlist )           /
  'BYHOUR': function (value) {
    return RRule_ApplyContentSchema_('byhrlist', value, this);
  },

  //  ( ";" "BYDAY" "=" bywdaylist )          /
  'BYDAY': function (value) {
    return RRule_ApplyContentSchema_('bywdaylist', value, this);
  },

  //  ( ";" "BYMONTHDAY" "=" bymodaylist )    /
  'BYMONTHDAY': function (value) {
    return RRule_ApplyContentSchema_('bymodaylist', value, this);
  },

  //  ( ";" "BYYEARDAY" "=" byyrdaylist )     /
  'BYYEARDAY': function (value) {
    return RRule_ApplyContentSchema_('byyrdaylist', value, this);
  },

  //  ( ";" "BYWEEKNO" "=" bywknolist )       /
  'BYWEEKNO': function (value) {
    return RRule_ApplyContentSchema_('bywknolist', value, this);
  },

  //  ( ";" "BYMONTH" "=" bymolist )          /
  'BYMONTH': function (value) {
    return RRule_ApplyContentSchema_('bymolist', value, this);
  },

  //  ( ";" "BYSETPOS" "=" bysplist )         /
  'BYSETPOS': function (value) {
    return RRule_ApplyContentSchema_('bysplist', value, this);
  },

  //  ( ";" "WKST" "=" weekday )              /
  'WKST': function (value) {
    return RRule_ApplyContentSchema_('weekday', value, this);
  },

  // freq       = "SECONDLY" / "MINUTELY" / "HOURLY" / "DAILY"
  //            / "WEEKLY" / "MONTHLY" / "YEARLY"
  'freq': function (value) {
    var result = RRule_StringToFreq(value);
    if (!result) { RRule_BadContent_(value); }
    return result;
  },

  // enddate    = date
  // enddate    =/ date-time            ;An UTC value
  'enddate': function (value) {
    // We are not dealing with zulu time.  The UNTIL date MUST be in the display
    // timezone.
    // This is in violation of the spec which requires UTC time.
    var date = ICAL_Parse(value);
    if (!(date instanceof ICAL_Date || date instanceof ICAL_DateTime)) {
      RRule_BadContent_(value);
    }
    return date;
  },

  // byseclist  = seconds / ( seconds *("," seconds) )
  // seconds    = 1DIGIT / 2DIGIT       ;0 to 59
  'byseclist': RRule_UnsignedIntListFunctor_(0, 59),

  // byminlist  = minutes / ( minutes *("," minutes) )
  // minutes    = 1DIGIT / 2DIGIT       ;0 to 59
  'byminlist': RRule_UnsignedIntListFunctor_(0, 59),

  // byhrlist   = hour / ( hour *("," hour) )
  // hour       = 1DIGIT / 2DIGIT       ;0 to 23
  'byhrlist': RRule_UnsignedIntListFunctor_(0, 23),

  // plus       = "+"
  // minus      = "-"

  // bywdaylist = weekdaynum / ( weekdaynum *("," weekdaynum) )
  // weekdaynum = [([plus] ordwk / minus ordwk)] weekday
  // ordwk      = 1DIGIT / 2DIGIT       ;1 to 53
  // weekday    = "SU" / "MO" / "TU" / "WE" / "TH" / "FR" / "SA"
  // ;Corresponding to SUNDAY, MONDAY, TUESDAY, WEDNESDAY, THURSDAY,
  // ;FRIDAY, SATURDAY and SUNDAY days of the week.
  'bywdaylist': function (value) {
    var parts = value.split(/,/);
    for (var i = 0; i < parts.length; ++i) {
      var p = parts[i];
      var m = p.match(/^([+\-]?\d\d?)?(SU|MO|TU|WE|TH|FR|SA)$/i);
      if (!m) { RRule_BadPart_(p); }
      var wday = RRule_TwoLtrStringToWeekDay(m[2]);
      var n;
      if (!m[1]) {
        n = 0;
      } else {
        n = parseInt(m[1], 10);
        var absn = n < 0 ? -n : n;
        if (!(1 <= absn && 53 >= absn)) { RRule_BadPart_(p); }
      }
      parts[i] = new RRule_WeekDayNum(n, wday);
    }
    return parts;
  },

  'weekday': function (value) {
    var wday = RRule_TwoLtrStringToWeekDay(value);
    if (null === wday) { RRule_BadContent_(value); }
    return wday;
  },

  // bymodaylist = monthdaynum / ( monthdaynum *("," monthdaynum) )
  // monthdaynum = ([plus] ordmoday) / (minus ordmoday)
  // ordmoday   = 1DIGIT / 2DIGIT       ;1 to 31
  'bymodaylist': RRule_IntListFunctor_(1, 31),

  // byyrdaylist = yeardaynum / ( yeardaynum *("," yeardaynum) )
  // yeardaynum = ([plus] ordyrday) / (minus ordyrday)
  // ordyrday   = 1DIGIT / 2DIGIT / 3DIGIT      ;1 to 366
  'byyrdaylist': RRule_IntListFunctor_(1, 366),

  // bywknolist = weeknum / ( weeknum *("," weeknum) )
  // weeknum    = ([plus] ordwk) / (minus ordwk)
  'bywknolist': RRule_IntListFunctor_(1, 53),

  // bymolist   = monthnum / ( monthnum *("," monthnum) )
  // monthnum   = 1DIGIT / 2DIGIT       ;1 to 12
  'bymolist': RRule_IntListFunctor_(1, 12),

  // bysplist   = setposday / ( setposday *("," setposday) )
  // setposday  = yeardaynum
  'bysplist': RRule_IntListFunctor_(1, 366),

  // so a trailing comma won't break it in IE everytime I add a field
  'ENDMARKER': null
};
