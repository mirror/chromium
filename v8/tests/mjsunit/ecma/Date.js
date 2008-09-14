// ------------------------------------------------------
// Some useful functions from ECMA-262 section 15.9.1.
// Based on shell.js in Mozilla test suite.

// ECMA-262 15.9.1.10
var HOURS_PER_DAY = 24;
var MINUTES_PER_HOUR = 60;
var SECONDS_PER_MINUTE = 60;

// ECMA-262 15.9.1.10
var MS_PER_SECOND = 1000;
var MS_PER_MINUTE = MS_PER_SECOND * SECONDS_PER_MINUTE;
var MS_PER_HOUR = MS_PER_MINUTE * MINUTES_PER_HOUR;

// ECMA-262 15.9.1.2
var MS_PER_DAY = 86400000;

var TZ_DIFF = getTimeZoneDiff();

// ECMA-262 15.9.1.8
//
// Local time zone adjustment.
var LOCAL_TZA = TZ_DIFF * MS_PER_HOUR;

// Use a date not subject to DST.
function getTimeZoneDiff() {
  return -((new Date(2000, 1, 1)).getTimezoneOffset()) / 60;
}

// ECMA-262 15.9.1.2
function day(time) {
  return Math.floor(time / MS_PER_DAY);
}

// ECMA-262 15.9.1.2
function timeWithinDay(time) {
  if (time < 0) {
    return (time % MS_PER_DAY) + MS_PER_DAY;
  }

  return time % MS_PER_DAY;
}

// ECMA-262 15.9.1.3
function daysInYear(year) {
  if (year % 4 != 0) {
    return 365;
  }

  if ((year % 4 == 0) && (year % 100 != 0)) {
    return 366;
  }

  if ((year % 100 == 0) && (year % 400 != 0)) {
    return 365;
  }

  if (year % 400 == 0) {
    return 366;
  }
}

// ECMA-262 15.9.1.3
function dayFromYear(year) {
  return 365 * (year - 1970) + Math.floor((year - 1969) / 4) -
         Math.floor((year - 1901) / 100) + Math.floor((year - 1601) / 400);
}

// ECMA-262 15.9.1.3
function timeFromYear(year) {
  return MS_PER_DAY * dayFromYear(year);
}

// ECMA-262 15.9.1.3
function yearFromTime(time) {
  var sign = (time < 0) ? -1 : 1;
  var year = (sign < 0) ? 1969 : 1970;

  var timeLeft = time;
  while (true) {
    timeLeft -= sign * timeInYear(year);

    if (sign > 0) {
      if (sign * timeLeft < 0) {
        return year;
      }
    } else {
      if (sign * timeLeft <= 0) {
        return year;
      }
    }

    year += sign;
  }
}

// ECMA-262 15.9.1.3
function inLeapYear(time) {
  if (daysInYear(yearFromTime(time)) == 365) {
    return 0;
  }

  if (daysInYear(yearFromTime(time)) == 366) {
    return 1;
  }
}

// ECMA-262 15.9.1.4
function dayWithinYear(time) {
  return day(time) - dayFromYear(yearFromTime(time));
}

// ECMA-262 15.9.1.4
//
// 0 - January
// 1 - February
// 2 - March
// 3 - April
// 4 - May
// 5 - June
// 6 - July
// 7 - August
// 8 - September
// 9 - October
// 10 - November
// 11 - December
function monthFromTime(time) {
  if (dayWithinYear(time) >= 0 && dayWithinYear(time) < 31) {
    return 0;
  }

  if (dayWithinYear(time) >= 31 &&
      dayWithinYear(time) < (59 + inLeapYear(time))) {
    return 1;
  }

  if (dayWithinYear(time) >= (59 + inLeapYear(time)) &&
      dayWithinYear(time) < (90 + inLeapYear(time))) {
    return 2;
  }

  if (dayWithinYear(time) >= (90 + inLeapYear(time)) &&
      dayWithinYear(time) < (120 + inLeapYear(time))) {
    return 3;
  }

  if (dayWithinYear(time) >= (120 + inLeapYear(time)) &&
      dayWithinYear(time) < (151 + inLeapYear(time))) {
    return 4;
  }

  if (dayWithinYear(time) >= (151 + inLeapYear(time)) &&
      dayWithinYear(time) < (181 + inLeapYear(time))) {
    return 5;
  }

  if (dayWithinYear(time) >= (181 + inLeapYear(time)) &&
      dayWithinYear(time) < (212 + inLeapYear(time))) {
    return 6;
  }

  if (dayWithinYear(time) >= (212 + inLeapYear(time)) &&
      dayWithinYear(time) < (243 + inLeapYear(time))) {
    return 7;
  }

  if (dayWithinYear(time) >= (243 + inLeapYear(time)) &&
     dayWithinYear(time) < (273 + inLeapYear(time))) {
    return 8;
  }

  if (dayWithinYear(time) >= (273 + inLeapYear(time)) &&
      dayWithinYear(time) < (304 + inLeapYear(time))) {
    return 9;
  }

  if (dayWithinYear(time) >= (304 + inLeapYear(time)) &&
      dayWithinYear(time) < (334 + inLeapYear(time))) {
    return 10;
  }

  if (dayWithinYear(time) >= (334 + inLeapYear(time)) &&
      dayWithinYear(time) < (365 + inLeapYear(time))) {
    return 11;
  }
}

// ECMA-262 15.9.1.5
function dateFromTime(time) {
  switch (monthFromTime(time)) {
    case 0:
      return dayWithinYear(time) + 1;
    case 1:
      return dayWithinYear(time) - 30;
    case 2:
      return dayWithinYear(time) - 58 - inLeapYear(time);
    case 3:
      return dayWithinYear(time) - 89 - inLeapYear(time);
    case 4:
      return dayWithinYear(time) - 119 - inLeapYear(time);
    case 5:
      return dayWithinYear(time) - 150 - inLeapYear(time);
    case 6:
      return dayWithinYear(time) - 180 - inLeapYear(time);
    case 7:
      return dayWithinYear(time) - 211 - inLeapYear(time);
    case 8:
      return dayWithinYear(time) - 242 - inLeapYear(time);
    case 9:
      return dayWithinYear(time) - 272 - inLeapYear(time);
    case 10:
      return dayWithinYear(time) - 303 - inLeapYear(time);
    case 11:
      return dayWithinYear(time) - 333 - inLeapYear(time);
  }
}

// ECMA-262 15.9.1.8
//
// 0 - Sunday
// 1 - Monday
// 2 - Tuesday
// 3 - Wednesday
// 4 - Thursday
// 5 - Friday
// 6 - Saturday
//
// Note weekDay(0) = 4, corresponding to Thursday, January 1, 1970.
function weekDay(time) {
  var weekDay = (day(time) + 4) % 7;
  return weekDay < 0 ? weekDay + 7 : weekDay;
}

// ECMA-262 15.9.1.9
//
// Daylight saving time adjustment.
function daylightSavingTA(time) {
  time = time - LOCAL_TZA;

  // Assume DST starts on first Sunday in April and ends on last Sunday in
  // October.  This is not always accurate, so tests that are not consistently
  // either in or out of DST should be avoided.  Also assume that DST starts at
  // 2am, which also may not be accurate.
  var dstStart = getFirstSundayInApril(time) + (2 * MS_PER_HOUR);
  var dstEnd = getLastSundayInOctober(time) + (2 * MS_PER_HOUR);

  if (time >= dstStart && time <= dstEnd) {
    return MS_PER_HOUR;
  }

  return 0;
}

function getFirstSundayInApril(time) {
  var year = yearFromTime(time);
  var leap = inLeapYear(year);

  var aprilStartTime = timeFromYear(year);
  for (var i = 0; i < 3; i++) {
    aprilStartTime += timeInMonth(i, leap);
  }

  var firstSundayTime = aprilStartTime;
  while (weekDay(firstSundayTime) > 0) {
    firstSundayTime += MS_PER_DAY;
  }

  return firstSundayTime;
}

function getLastSundayInOctober(time) {
  var year = yearFromTime(time);
  var leap = inLeapYear(year);

  var octoberStartTime = timeFromYear(year);
  for (var i = 0; i < 10; i++) {
    octoberStartTime += timeInMonth(i, leap);
  }

  var lastSundayTime = octoberStartTime + (daysInMonth(9, leap) * MS_PER_DAY);
  while (weekDay(lastSundayTime) > 0) {
    lastSundayTime -= MS_PER_DAY;
  }

  return lastSundayTime;
}

// ECMA-262 15.9.1.9
function localTime(time) {
  return time + LOCAL_TZA + daylightSavingTA(time);
}

// ECMA-262 15.9.1.9
function utc(time) {
  return time - LOCAL_TZA - daylightSavingTA(time - LOCAL_TZA);
}

// ECMA-262 15.9.1.10
function hourFromTime(time) {
  var hour = Math.floor(time / MS_PER_HOUR) % HOURS_PER_DAY;
  return hour < 0 ? hour + HOURS_PER_DAY : hour;
}

// ECMA-262 15.9.1.10
function minuteFromTime(time) {
  var minute = Math.floor(time / MS_PER_MINUTE) % MINUTES_PER_HOUR;
  return minute < 0 ? minute + MINUTES_PER_HOUR : minute;
}

// ECMA-262 15.9.1.10
function secondFromTime(time) {
  var second = Math.floor(time / MS_PER_SECOND) % SECONDS_PER_MINUTE;
  return second < 0 ? second + SECONDS_PER_MINUTE : second;
}

// ECMA-262 15.9.1.10
function msFromTime(time) {
  var ms = time % MS_PER_SECOND;
  return ms < 0 ? ms + MS_PER_SECOND : ms;
}

// ECMA-262 15.9.1.11
function makeTime(hour, minute, second, ms) {
  if (!isFinite(hour) || !isFinite(minute) ||
      !isFinite(second) || !isFinite(ms)) {
    return Number.NaN;
  }

  hour = toInteger(hour);
  minute = toInteger(minute);
  second = toInteger(second);
  ms = toInteger(ms);

  return (hour * MS_PER_HOUR) + (minute * MS_PER_MINUTE) +
         (second * MS_PER_SECOND) + ms;
}

// ECMA-262 15.9.1.12
function makeDay(year, month, date) {
  if (!isFinite(year) || !isFinite(month) || !isFinite(date)) {
    return Number.NaN;
  }

  year = toInteger(year);
  month = toInteger(month);
  date = toInteger(date);

  var result5 = year + Math.floor(month / 12);
  var result6 = month % 12;

  var t = (year < 1970) ? 1 : 0;

  if (year < 1970) {
    for (var i = 1969; i >= year; i--) {
      t -= timeInYear(i);
    }
  } else {
    for (var i = 1970; i < year; i++) {
      t += timeInYear(i);
    }
  }

  var leap = inLeapYear(t);

  for (var i = 0; i < month; i++) {
    t += timeInMonth(i, leap);
  }

  if (yearFromTime(t) != result5) {
    return Number.NaN;
  }

  if (monthFromTime(t) != result6) {
    return Number.NaN;
  }

  if (dateFromTime(t) != 1) {
    return Number.NaN;
  }

  return day(t) + date - 1;
}

// ECMA-262 15.9.1.13
function makeDate(day, time) {
  if (!isFinite(day) || !isFinite(time)) {
    return Number.NaN;
  }

  return (day * MS_PER_DAY) + time;
}

// ECMA-262 15.9.1.14
function timeClip(time) {
  if (!isFinite(time)) {
    return Number.NaN;
  }

  if (Math.abs(time) > 8.64e15) {
    return Number.NaN;
  }

  return toInteger(time);
}

// ECMA-262 9.4
function toInteger(value) {
  value = Number(value);

  if (isNaN(value) || (value == 0) || (value == -0) ||
      (value == Number.POSITIVE_INFINITY) || (value == Number.NEGATIVE_INFINITY)) {
    return 0;
  }

  var sign = (value < 0) ? -1 : 1;
  return sign * Math.floor(Math.abs(value));
}

// Helper functions not covered by ECMA-262.

function daysInMonth(month, leap) {
  // Months with 30 days: April, June, September, and November.
  if ((month == 3) || (month == 5) || (month == 8) || (month == 10)) {
    return 30;
  }

  // February (check leap year).
  if (month == 1) {
    return leap ? 29 : 28;
  }

  // All other months have 31 days.
  return 31;
}

function timeInMonth(month, leap) {
  return daysInMonth(month, leap) * MS_PER_DAY;
}

function timeInYear(year) {
  return daysInYear(year) * MS_PER_DAY;
}

function isFinite(value) {
  if (isNaN(value)) {
    return false;
  }

  if (value == Number.POSITIVE_INFINITY) {
    return false;
  }

  if (value == Number.NEGATIVE_INFINITY) {
    return false;
  }

  return true;
}

// ------------------------------------------------------
// [ecma/Date/15.9.5.14.js]

function assertHoursEquals(time) {
  assertEquals(hourFromTime(localTime(time)), (new Date(time)).getHours());
}

// Year 0.
assertHoursEquals(-62167215600000);
assertHoursEquals(-62167212000000);
assertHoursEquals(-62167208400000);
assertHoursEquals(-62167204800000);
assertHoursEquals(-62167201200000);
assertHoursEquals(-62167197600000);

// Jan 1, 1970.
assertHoursEquals(3600000);
assertHoursEquals(7200000);
assertHoursEquals(10800000);
assertHoursEquals(14400000);
assertHoursEquals(18000000);
assertHoursEquals(21600000);


// Jan 1, 1900.
assertHoursEquals(-2208985200000);
assertHoursEquals(-2208981600000);
assertHoursEquals(-2208978000000);
assertHoursEquals(-2208974400000);
assertHoursEquals(-2208970800000);
assertHoursEquals(-2208967200000);


// Jan 1, 2000.
assertHoursEquals(946688400000);
assertHoursEquals(946692000000);
assertHoursEquals(946695600000);
assertHoursEquals(946699200000);
assertHoursEquals(946702800000);
assertHoursEquals(946706400000);


// Feb 29, 2000.
assertHoursEquals(949467600000);
assertHoursEquals(949471200000);
assertHoursEquals(949474800000);
assertHoursEquals(949478400000);
assertHoursEquals(949482000000);
assertHoursEquals(949485600000);


// Jan 1, 2005.
assertHoursEquals(1104541200000);
assertHoursEquals(1104544800000);
assertHoursEquals(1104548400000);
assertHoursEquals(1104552000000);
assertHoursEquals(1104555600000);
assertHoursEquals(1104559200000);


assertNaN((new Date(NaN)).getHours());
assertEquals(0, Date.prototype.getHours.length);

// ------------------------------------------------------
// [ecma/Date/15.9.5.34-1.js]

var TIME_1900 = -2208988800000;

assertEquals(2, Date.prototype.setMonth.length);
assertEquals("function", typeof Date.prototype.setMonth);


function assertDate(testDate, utcDate, localDate) {
  assertEquals(utcDate.value,     testDate.getTime());
  assertEquals(utcDate.value,     testDate.valueOf());

  assertEquals(utcDate.year,      testDate.getUTCFullYear());
  assertEquals(utcDate.month,     testDate.getUTCMonth());
  assertEquals(utcDate.date,      testDate.getUTCDate());
  assertEquals(utcDate.day,       testDate.getUTCDay());
  assertEquals(utcDate.hours,     testDate.getUTCHours());
  assertEquals(utcDate.minutes,   testDate.getUTCMinutes());
  assertEquals(utcDate.seconds,   testDate.getUTCSeconds());
  assertEquals(utcDate.ms,        testDate.getUTCMilliseconds());

  assertEquals(localDate.year,    testDate.getFullYear());
  assertEquals(localDate.month,   testDate.getMonth());
  assertEquals(localDate.date,    testDate.getDate());
  assertEquals(localDate.day,     testDate.getDay());
  assertEquals(localDate.hours,   testDate.getHours());
  assertEquals(localDate.minutes, testDate.getMinutes());
  assertEquals(localDate.seconds, testDate.getSeconds());
  assertEquals(localDate.ms,      testDate.getMilliseconds());

  testDate.toString = Object.prototype.toString;
  assertEquals("[object Date]",   testDate.toString());
}

testDate = new Date(0);
testDate.setMonth(1,1,1,1,1,1);
utcDate = utcDateFromTime(setMonth(0,1,1));
localDate = localDateFromTime(setMonth(0,1,1));
assertDate(testDate, utcDate, localDate);

testDate = new Date(0);
testDate.setMonth(0,1);
utcDate = utcDateFromTime(setMonth(0,0,1));
localDate = localDateFromTime(setMonth(0,0,1));
assertDate(testDate, utcDate, localDate);

testDate = new Date(TIME_1900);
testDate.setMonth(11,31);
utcDate = utcDateFromTime(setMonth(TIME_1900,11,31));
localDate = localDateFromTime(setMonth(TIME_1900,11,31));
assertDate(testDate, utcDate, localDate);

function MyDate() {
  this.year = 0;
  this.month = 0;
  this.date = 0;
  this.day = 0;
  this.hours = 0;
  this.minutes = 0;
  this.seconds = 0;
  this.ms = 0;
  this.value = 0;
}

function localDateFromTime(time) {
  time = localTime(time);
  return myDateFromTime(time);
}

function utcDateFromTime(time) {
  return myDateFromTime(time);
}

function myDateFromTime(time) {
  var date = new MyDate();

  date.year = yearFromTime(time);
  date.month = monthFromTime(time);
  date.date = dateFromTime(time);
  date.hours = hourFromTime(time);
  date.minutes = minuteFromTime(time);
  date.seconds = secondFromTime(time);
  date.ms = msFromTime(time);

  date.time = makeTime(date.hours, date.minutes, date.seconds, date.ms);
  date.value = timeClip(makeDate(makeDay(date.year, date.month, date.date), date.time));
  date.day = weekDay(date.value);

  return date;
}

function setMonth(time, month, date) {
  time = localTime(time);
  month = Number(month);
  date = (date == void 0) ? dateFromTime(time) : Number(date);

  var day = makeDay(yearFromTime(time), month, date);
  return timeClip(utc(makeDate(day, timeWithinDay(time))));
}

// TODO(pjohnson): add ecma/Date/15.9.5.10-1.js and ecma/Date/15.9.5.12-1.js.
