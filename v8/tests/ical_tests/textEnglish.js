// Copyright 2005, Google Inc.
// All Rights Reserved.
//
// davem@google.com

// Support functions for messages.js

var CG_MDAYS_OF_THE_WEEK =
  [ MSG_SUN, MSG_MON, MSG_TUES, MSG_WED, MSG_THUR, MSG_FRI, MSG_SAT ];

var CG_DAYS_OF_THE_WEEK =
  [ MSG_SU, MSG_M, MSG_TU, MSG_W, MSG_TH, MSG_F, MSG_SA ];

var FULL_WEEKDAYS = [MSG_SUNDAY, MSG_MONDAY, MSG_TUESDAY, MSG_WEDNESDAY,
                     MSG_THURSDAY, MSG_FRIDAY, MSG_SATURDAY];
var FULL_WEEKDAYS_PLURAL = [
    MSG_SUNDAYS, MSG_MONDAYS, MSG_TUESDAYS, MSG_WEDNESDAYS, MSG_THURSDAYS,
    MSG_FRIDAYS, MSG_SATURDAYS];

/** 1 indexed array of month names. */
var MONTHS = [ , MSG_JAN, MSG_FEB, MSG_MAR, MSG_APR, MSG_MAY, MSG_JUN, MSG_JUL,
              MSG_AUG, MSG_SEP, MSG_OCT, MSG_NOV, MSG_DEC ];


var FULL_MONTHS = [, MSG_JANUARY, MSG_FEBRUARY, MSG_MARCH, MSG_APRIL,
                   MSG_MAY_LONG, MSG_JUNE, MSG_JULY, MSG_AUGUST, MSG_SEPTEMBER,
                   MSG_OCTOBER, MSG_NOVEMBER, MSG_DECEMBER ];

var MSG_SETTINGS_TAB_NAMES = [
    MSG_GENERAL, MSG_CALENDARS, MSG_NOTIFICATION, MSG_TAB_UPLOAD];

function _MSG_DayOfWeekMask(daysOfWeek) {
  var dowMask = 0;
  for (var i = 0; i < daysOfWeek.length; ++i) {
    dowMask |= (1 << daysOfWeek[i]);
  }
  return dowMask;
}

/**
 * true iff MSG_DaysOfWeek would use a group name ("Weekdays", "All days")
 * to refer to these days.
 */
function MSG_IsDayOfWeekGroup(daysOfWeek) {
  var dowMask = _MSG_DayOfWeekMask(daysOfWeek);
  return dowMask === 127 || dowMask === 62;
}

/** @return text/plain */
function MSG_DaysOfWeek(daysOfWeek, opt_plural) {
  if (1 == daysOfWeek.length) { return MSG_DayOfWeek(daysOfWeek[0]); }

  switch (_MSG_DayOfWeekMask(daysOfWeek)) {
    case 127: // first 7 bits set
      return opt_plural ? MSG_DAYSET_DAYS : MSG_DAYSET_EVERY_DAY;
    case 62: // weekdays
      // TODO(msamuel): i18n weekday bitmask
      return opt_plural ? MSG_DAYSET_WEEKDAYS : MSG_DAYSET_EVERY_WEEKDAY;
    default:
      break;
  }
  // TODO(msamuel): once we have access to preferred first day of week,
  // sort by
  // daysOfWeek.sort(function (a, b) {
  //   return (a + startDay) % 7 - (b + startDay) % 7 });
  // so that days show up in the user's expected ordering.
  var buf = [];
  for (var i = 0; i < daysOfWeek.length; ++i) {
    buf.push(FULL_WEEKDAYS[daysOfWeek[i]]);
  }
  return MSG_Join(buf);
}

function MSG_DayOfWeek(dayOfWeek) {
  return FULL_WEEKDAYS[dayOfWeek];
}

/** @return text/plain */
function MSG_Months(months) {
  var buf = [];
  for (var i = 0; i < months.length; ++i) {
    buf.push(MSG_Month(months[i]));
  }
  return MSG_Join(buf);
}

function MSG_Month(month) {
  // use numeric months for CJK
  if (MESSAGE_LOCALE_LANG === 'ja' || MESSAGE_LOCALE_LANG === 'zh') {
    return month + '\u6708';
  } else if (MESSAGE_LOCALE_LANG === 'ko') {
    return month + '\uC6D4';
  } else if (MESSAGE_LOCALE_LANG === 'ru') {
    // full month names in Russian switch case based on the day number, so use
    // the short name here
    return MONTHS[month];
  } else {
    return FULL_MONTHS[month];
  }
}

function MSG_CommaList(nums) {
  return MSG_Join(nums);
}

function MSG_Join(parts) {
  if ('zh' === MESSAGE_LOCALE_LANG) {
    var len = parts.length;
    if (len > 2) {
      return parts.slice(0, len - 1).join('\u3001') +
        ('zh_TW' == MESSAGE_LOCALE ? '\u8207' : '\u548c') + parts[len - 1];
    } else {
      return parts.join('\u3001');
    }
  } else {
    return parts.join(', ');
  }
}
