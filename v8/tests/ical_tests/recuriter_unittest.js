var emitTestLog;

function testDayOfYear() {
  assertEquals(0, ICAL_dayOfYear(2006, 1, 1));
  assertEquals(0, ICAL_dayOfYear(2005, 1, 1));
  assertEquals(0, ICAL_dayOfYear(2004, 1, 1));
  assertEquals(4, ICAL_dayOfYear(2006, 1, 5));
  assertEquals(4, ICAL_dayOfYear(2005, 1, 5));
  assertEquals(4, ICAL_dayOfYear(2004, 1, 5));
  assertEquals(35, ICAL_dayOfYear(2006, 2, 5));
  assertEquals(35, ICAL_dayOfYear(2005, 2, 5));
  assertEquals(35, ICAL_dayOfYear(2004, 2, 5));
  assertEquals(63, ICAL_dayOfYear(2006, 3, 5));
  assertEquals(63, ICAL_dayOfYear(2005, 3, 5));
  assertEquals(64, ICAL_dayOfYear(2004, 3, 5));
  assertEquals(364, ICAL_dayOfYear(2006, 12, 31));
  assertEquals(364, ICAL_dayOfYear(2005, 12, 31));
  assertEquals(365, ICAL_dayOfYear(2004, 12, 31));
}

function testCountCondition() {
  var cc = countCondition_(3);
  assertTrue(cc());
  assertTrue(cc());
  assertTrue(cc());
  assertTrue(!cc());
}

function testDayNumToDateInMonth() {
  //        March 2006
  // Su Mo Tu We Th Fr Sa
  //           1  2  3  4
  //  5  6  7  8  9 10 11
  // 12 13 14 15 16 17 18
  // 19 20 21 22 23 24 25
  // 26 27 28 29 30 31
  var dow0 = RRULE_WDAY_WEDNESDAY;
  var nDays = 31;
  var d0 = 0;

  assertEquals(
    1, dayNumToDate_(dow0, nDays, 1, RRULE_WDAY_WEDNESDAY, d0, nDays));
  assertEquals(
    8, dayNumToDate_(dow0, nDays, 2, RRULE_WDAY_WEDNESDAY, d0, nDays));
  assertEquals(
    29, dayNumToDate_(dow0, nDays, -1, RRULE_WDAY_WEDNESDAY, d0, nDays));
  assertEquals(
    22, dayNumToDate_(dow0, nDays, -2, RRULE_WDAY_WEDNESDAY, d0, nDays));

  assertEquals(
    3, dayNumToDate_(dow0, nDays, 1, RRULE_WDAY_FRIDAY, d0, nDays));
  assertEquals(
    10, dayNumToDate_(dow0, nDays, 2, RRULE_WDAY_FRIDAY, d0, nDays));
  assertEquals(
    31, dayNumToDate_(dow0, nDays, -1, RRULE_WDAY_FRIDAY, d0, nDays));
  assertEquals(
    24, dayNumToDate_(dow0, nDays, -2, RRULE_WDAY_FRIDAY, d0, nDays));

  assertEquals(
    7, dayNumToDate_(dow0, nDays, 1, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    14, dayNumToDate_(dow0, nDays, 2, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    28, dayNumToDate_(dow0, nDays, 4, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    0, dayNumToDate_(dow0, nDays, 5, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    28, dayNumToDate_(dow0, nDays, -1, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    21, dayNumToDate_(dow0, nDays, -2, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    7, dayNumToDate_(dow0, nDays, -4, RRULE_WDAY_TUESDAY, d0, nDays));
  assertEquals(
    0, dayNumToDate_(dow0, nDays, -5, RRULE_WDAY_TUESDAY, d0, nDays));
}

function testDayNumToDateInYear() {
  //        January 2006
  //    Su Mo Tu We Th Fr Sa
  //  1  1  2  3  4  5  6  7
  //  2  8  9 10 11 12 13 14
  //  3 15 16 17 18 19 20 21
  //  4 22 23 24 25 26 27 28
  //  5 29 30 31

  //        February 2006
  //    Su Mo Tu We Th Fr Sa
  //  5           1  2  3  4
  //  6  5  6  7  8  9 10 11
  //  7 12 13 14 15 16 17 18
  //  8 19 20 21 22 23 24 25
  //  9 26 27 28

  //           March 2006
  //    Su Mo Tu We Th Fr Sa
  //  9           1  2  3  4
  // 10  5  6  7  8  9 10 11
  // 11 12 13 14 15 16 17 18
  // 12 19 20 21 22 23 24 25
  // 13 26 27 28 29 30 31

  var dow0 = RRULE_WDAY_SUNDAY;
  var nDaysInMonth = 31;
  var nDays = 365;
  var d0 = 59;

  // TODO(msamuel): check that these answers are right
  assertEquals(
    1, dayNumToDate_(dow0, nDays, 9, RRULE_WDAY_WEDNESDAY, d0, nDaysInMonth));
  assertEquals(
    8, dayNumToDate_(dow0, nDays, 10, RRULE_WDAY_WEDNESDAY, d0, nDaysInMonth));
  assertEquals(
    29, dayNumToDate_(
      dow0, nDays, -40, RRULE_WDAY_WEDNESDAY, d0, nDaysInMonth));
  assertEquals(
    22, dayNumToDate_(
      dow0, nDays, -41, RRULE_WDAY_WEDNESDAY, d0, nDaysInMonth));

  assertEquals(
    3, dayNumToDate_(dow0, nDays, 9, RRULE_WDAY_FRIDAY, d0, nDaysInMonth));
  assertEquals(
    10, dayNumToDate_(dow0, nDays, 10, RRULE_WDAY_FRIDAY, d0, nDaysInMonth));
  assertEquals(
    31, dayNumToDate_(dow0, nDays, -40, RRULE_WDAY_FRIDAY, d0, nDaysInMonth));
  assertEquals(
    24, dayNumToDate_(dow0, nDays, -41, RRULE_WDAY_FRIDAY, d0, nDaysInMonth));

  assertEquals(
    7, dayNumToDate_(dow0, nDays, 10, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    14, dayNumToDate_(dow0, nDays, 11, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    28, dayNumToDate_(dow0, nDays, 13, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    0, dayNumToDate_(dow0, nDays, 14, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    28, dayNumToDate_(dow0, nDays, -40, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    21, dayNumToDate_(dow0, nDays, -41, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    7, dayNumToDate_(dow0, nDays, -43, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
  assertEquals(
    0, dayNumToDate_(
      dow0, nDays, -44, RRULE_WDAY_TUESDAY, d0, nDaysInMonth));
}

function testUniquify() {
  var ints = [1,4,4,2,7,3,8,0,0,3];
  uniquify(ints);
  assertEquals('0,1,2,3,4,7,8', ints.toString());
}

function testRollToNextWeekStart() {
  var builder;

  builder = ical_dateBuilder(2006, 1, 23);
  rollToNextWeekStart_(builder, RRULE_WDAY_TUESDAY);
  assertEquals('20060124', builder.toDate().toString());

  builder = ical_dateBuilder(2006, 1, 24);
  rollToNextWeekStart_(builder, RRULE_WDAY_TUESDAY);
  assertEquals('20060124', builder.toDate().toString());

  builder = ical_dateBuilder(2006, 1, 25);
  rollToNextWeekStart_(builder, RRULE_WDAY_TUESDAY);
  assertEquals('20060131', builder.toDate().toString());

  builder = ical_dateBuilder(2006, 1, 23);
  rollToNextWeekStart_(builder, RRULE_WDAY_MONDAY);
  assertEquals('20060123', builder.toDate().toString());

  builder = ical_dateBuilder(2006, 1, 24);
  rollToNextWeekStart_(builder, RRULE_WDAY_MONDAY);
  assertEquals('20060130', builder.toDate().toString());

  builder = ical_dateBuilder(2006, 1, 25);
  rollToNextWeekStart_(builder, RRULE_WDAY_MONDAY);
  assertEquals('20060130', builder.toDate().toString());

  builder = ical_dateBuilder(2006, 1, 31);
  rollToNextWeekStart_(builder, RRULE_WDAY_MONDAY);
  assertEquals('20060206', builder.toDate().toString());
}

function testNextWeekStart() {
  assertEquals('20060124',
               '' + nextWeekStart_(ICAL_Parse('20060123'), RRULE_WDAY_TUESDAY));
  assertEquals('20060124',
               '' + nextWeekStart_(ICAL_Parse('20060124'), RRULE_WDAY_TUESDAY));
  assertEquals('20060131',
               '' + nextWeekStart_(ICAL_Parse('20060125'), RRULE_WDAY_TUESDAY));
  assertEquals('20060123',
               '' + nextWeekStart_(ICAL_Parse('20060123'), RRULE_WDAY_MONDAY));
  assertEquals('20060130',
               '' + nextWeekStart_(ICAL_Parse('20060124'), RRULE_WDAY_MONDAY));
  assertEquals('20060130',
               '' + nextWeekStart_(ICAL_Parse('20060125'), RRULE_WDAY_MONDAY));
  assertEquals('20060206',
               '' + nextWeekStart_(ICAL_Parse('20060131'), RRULE_WDAY_MONDAY));
}

function testCountInPeriod() {
  //        January 2006
  //  Su Mo Tu We Th Fr Sa
  //   1  2  3  4  5  6  7
  //   8  9 10 11 12 13 14
  //  15 16 17 18 19 20 21
  //  22 23 24 25 26 27 28
  //  29 30 31
  assertEquals(5, countInPeriod_(RRULE_WDAY_SUNDAY, RRULE_WDAY_SUNDAY, 31));
  assertEquals(5, countInPeriod_(RRULE_WDAY_MONDAY, RRULE_WDAY_SUNDAY, 31));
  assertEquals(5, countInPeriod_(RRULE_WDAY_TUESDAY, RRULE_WDAY_SUNDAY, 31));
  assertEquals(4, countInPeriod_(RRULE_WDAY_WEDNESDAY, RRULE_WDAY_SUNDAY, 31));
  assertEquals(4, countInPeriod_(RRULE_WDAY_THURSDAY, RRULE_WDAY_SUNDAY, 31));
  assertEquals(4, countInPeriod_(RRULE_WDAY_FRIDAY, RRULE_WDAY_SUNDAY, 31));
  assertEquals(4, countInPeriod_(RRULE_WDAY_SATURDAY, RRULE_WDAY_SUNDAY, 31));

  //      February 2006
  //  Su Mo Tu We Th Fr Sa
  //            1  2  3  4
  //   5  6  7  8  9 10 11
  //  12 13 14 15 16 17 18
  //  19 20 21 22 23 24 25
  //  26 27 28
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_SUNDAY, RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_MONDAY, RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_TUESDAY, RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_WEDNESDAY, RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_THURSDAY, RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_FRIDAY, RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(4,
               countInPeriod_(RRULE_WDAY_SATURDAY, RRULE_WDAY_WEDNESDAY, 28));
}

function testInvertWeekdayNum() {

  //        January 2006
  //  # Su Mo Tu We Th Fr Sa
  //  1  1  2  3  4  5  6  7
  //  2  8  9 10 11 12 13 14
  //  3 15 16 17 18 19 20 21
  //  4 22 23 24 25 26 27 28
  //  5 29 30 31

  // the 1st falls on a sunday, so dow0 == SU
  assertEquals(
      5,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_SUNDAY),
                        RRULE_WDAY_SUNDAY, 31));
  assertEquals(
      5,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_MONDAY),
                        RRULE_WDAY_SUNDAY, 31));
  assertEquals(
      5,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_TUESDAY),
                        RRULE_WDAY_SUNDAY, 31));
  assertEquals(
      4,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_WEDNESDAY),
                        RRULE_WDAY_SUNDAY, 31));
  assertEquals(
      3,
      invertWeekdayNum_(new RRule_WeekDayNum(-2, RRULE_WDAY_WEDNESDAY),
                        RRULE_WDAY_SUNDAY, 31));


  //      February 2006
  //  # Su Mo Tu We Th Fr Sa
  //  1           1  2  3  4
  //  2  5  6  7  8  9 10 11
  //  3 12 13 14 15 16 17 18
  //  4 19 20 21 22 23 24 25
  //  5 26 27 28

  assertEquals(
      4,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_SUNDAY),
                        RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(
      4,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_MONDAY),
                        RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(
      4,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_TUESDAY),
                        RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(
      4,
      invertWeekdayNum_(new RRule_WeekDayNum(-1, RRULE_WDAY_WEDNESDAY),
                        RRULE_WDAY_WEDNESDAY, 28));
  assertEquals(
      3,
      invertWeekdayNum_(new RRule_WeekDayNum(-2, RRULE_WDAY_WEDNESDAY),
                        RRULE_WDAY_WEDNESDAY, 28));
}


function runGeneratorTests(generator, builder, field, golden, opt_max) {
  var output = [];
  while ((!opt_max || output.length < opt_max) && generator(builder) &&
         output.length < 50) {
    output.push(builder[field]);
  }
  assertEquals(golden.toString(), output.toString());
}

function testByYearDayGenerator() {
  runGeneratorTests(
      byYearDayGenerator_([1, 5, -1, 100], ICAL_Parse('20060101')),
      ical_dateBuilder(2006, 1, 1), 'date', [ 1, 5 ]);
  runGeneratorTests(
      byYearDayGenerator_([1, 5, -1, 100], ICAL_Parse('20060102')),
      ical_dateBuilder(2006, 1, 2), 'date', [ 1, 5 ]);
  runGeneratorTests(
      byYearDayGenerator_([100], ICAL_Parse('20060106')),
      ical_dateBuilder(2006, 1, 6), 'date', []);
  runGeneratorTests(
      byYearDayGenerator_([], ICAL_Parse('20060106')),
      ical_dateBuilder(2006, 1, 6), 'date', []);
  runGeneratorTests(
      byYearDayGenerator_([1, 5, -1, 100], ICAL_Parse('20060201')),
      ical_dateBuilder(2006, 2, 1), 'date', []);
  runGeneratorTests(
      byYearDayGenerator_([1, 5, -1, 100], ICAL_Parse('20061201')),
      ical_dateBuilder(2006, 12, 1), 'date', [31]);
  runGeneratorTests(
      byYearDayGenerator_([1, 5, -1, 100], ICAL_Parse('20060401')),
      ical_dateBuilder(2006, 4, 1), 'date', [10]);
}

function testByWeekNoGenerator() {
  var g = byWeekNoGenerator_([22], RRULE_WDAY_SUNDAY, ICAL_Parse('20060101'));
  runGeneratorTests(g, ical_dateBuilder(2006, 1, 1), 'date', []);
  runGeneratorTests(g, ical_dateBuilder(2006, 2, 1), 'date', []);
  runGeneratorTests(g, ical_dateBuilder(2006, 3, 1), 'date', []);
  runGeneratorTests(g, ical_dateBuilder(2006, 4, 1), 'date', []);
  runGeneratorTests(
    g, ical_dateBuilder(2006, 5, 1), 'date', [28, 29, 30, 31]);
  runGeneratorTests(g, ical_dateBuilder(2006, 6, 1), 'date', [1, 2, 3]);
  runGeneratorTests(g, ical_dateBuilder(2006, 7, 1), 'date', []);

  // weekstart of monday shifts each week forward by one
  var g2 = byWeekNoGenerator_([22], RRULE_WDAY_MONDAY, ICAL_Parse('20060101'));
  runGeneratorTests(g2, ical_dateBuilder(2006, 1, 1), 'date', []);
  runGeneratorTests(g2, ical_dateBuilder(2006, 2, 1), 'date', []);
  runGeneratorTests(g2, ical_dateBuilder(2006, 3, 1), 'date', []);
  runGeneratorTests(g2, ical_dateBuilder(2006, 4, 1), 'date', []);
  runGeneratorTests(g2, ical_dateBuilder(2006, 5, 1), 'date', [29, 30, 31]);
  runGeneratorTests(g2, ical_dateBuilder(2006, 6, 1), 'date', [1, 2, 3, 4]);
  runGeneratorTests(g2, ical_dateBuilder(2006, 7, 1), 'date', []);

  // 2004 with a week start of monday has no orphaned days.
  // 2004-01-01 falls on Thursday
  var g3 = byWeekNoGenerator_([14], RRULE_WDAY_MONDAY, ICAL_Parse('20040101'));
  runGeneratorTests(g3, ical_dateBuilder(2004, 1, 1), 'date', []);
  runGeneratorTests(g3, ical_dateBuilder(2004, 2, 1), 'date', []);
  runGeneratorTests(g3, ical_dateBuilder(2004, 3, 1), 'date', [29, 30, 31]);
  runGeneratorTests(g3, ical_dateBuilder(2004, 4, 1), 'date', [1, 2, 3, 4]);
  runGeneratorTests(g3, ical_dateBuilder(2004, 5, 1), 'date', []);
}

function testByDayGenerator(dtStart) {
  var days = [
      new RRule_WeekDayNum(0, RRULE_WDAY_SUNDAY), // every sunday
      new RRule_WeekDayNum(1, RRULE_WDAY_MONDAY), // first monday
      new RRule_WeekDayNum(5, RRULE_WDAY_MONDAY), // fifth monday
      new RRule_WeekDayNum(-2, RRULE_WDAY_TUESDAY) // second to last tuesday
      ];
  var g = byDayGenerator_(days, false, ICAL_Parse('20060101'));
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 1, 1), 'date',
                    [ 1, 2, 8, 15, 22, 24, 29, 30 ]);
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 2, 1), 'date',
                    [ 5, 6, 12, 19, 21, 26 ]);
}

function testByMonthDayGenerator() {
  var monthDays = [1, 15, 29];
  runGeneratorTests(byMonthDayGenerator_(monthDays, ICAL_Parse('20060101')),
                    ical_dateBuilder(2006, 1, 1), 'date',
                    [ 1, 15, 29 ]);
  runGeneratorTests(byMonthDayGenerator_(monthDays, ICAL_Parse('20060115')),
                    ical_dateBuilder(2006, 1, 15), 'date',
                    [ 1, 15, 29 ]);
  runGeneratorTests(byMonthDayGenerator_(monthDays, ICAL_Parse('20060201')),
                    ical_dateBuilder(2006, 2, 1), 'date',
                    [ 1, 15 ]);
  runGeneratorTests(byMonthDayGenerator_(monthDays, ICAL_Parse('20060216')),
                    ical_dateBuilder(2006, 2, 16), 'date',
                    [ 1, 15 ]);

  var monthDays = [1, -30, 30];
  var g2 = byMonthDayGenerator_(monthDays, ICAL_Parse('20060101'));
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 1, 1), 'date',
                    [ 1, 2, 30 ]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 2, 1), 'date',
                    [ 1 ]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 3, 1), 'date',
                    [ 1, 2, 30 ]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 4, 1), 'date',
                    [ 1, 30 ]);
}

function testByMonthGenerator() {
  runGeneratorTests(byMonthGenerator_([2, 8, 6, 10], ICAL_Parse('20060101')),
                    ical_dateBuilder(2006, 1, 1), 'month',
                    [ 2, 6, 8, 10 ]);
  var g = byMonthGenerator_([2, 8, 6, 10], ICAL_Parse('20060401'));
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 4, 1), 'month',
                    [ 2, 6, 8, 10 ]);
  runGeneratorTests(g,
                    ical_dateBuilder(2007, 11, 1), 'month',
                    [2, 6, 8, 10]);
}

function testByYearGenerator() {
  runGeneratorTests(
      byYearGenerator_([1066, 1492, 1876, 1975, 2006], ICAL_Parse('20060101')),
      ical_dateBuilder(2006, 1, 1), 'year',
      [ 2006 ]);
  runGeneratorTests(
      byYearGenerator_([1066, 1492, 1876, 1975, 2006], ICAL_Parse('20070101')),
      ical_dateBuilder(2007, 1, 1), 'year',
      []);
  runGeneratorTests(
      byYearGenerator_([1066, 1492, 1876, 1975, 2006], ICAL_Parse('10660701')),
      ical_dateBuilder(1066, 7, 1), 'year',
      [ 1066, 1492, 1876, 1975, 2006 ]);
  runGeneratorTests(
      byYearGenerator_([1066, 1492, 1876, 1975, 2006], ICAL_Parse('19000701')),
      ical_dateBuilder(1900, 7, 1), 'year',
      [ 1975, 2006 ]);
}

function testSerialDayGenerator() {
  runGeneratorTests(serialDayGenerator_(1, ICAL_Parse('20060115')),
                    ical_dateBuilder(2006, 1, 15), 'date',
                    [ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                      25, 26, 27, 28, 29, 30, 31 ]);
  runGeneratorTests(serialDayGenerator_(1, ICAL_Parse('20060101')),
                    ical_dateBuilder(2006, 1, 1), 'date',
                    [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                      11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                      21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                      31 ]);
  var g = serialDayGenerator_(2, ICAL_Parse('20060101'));
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 1, 1), 'date',
                    [ 1, 3, 5, 7, 9,
                      11, 13, 15, 17, 19,
                      21, 23, 25, 27, 29,
                      31 ]);
  // now g should start on the second of February
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 2, 1), 'date',
                    [ 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28 ]);
  // and if we skip way ahead to June, it should start on the 1st
  // This test is limited to one output value
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 4, 1), 'date',
                    [ 1 ], 1);
  // test with intervals longer than 30 days
  var g2 = serialDayGenerator_(45, ICAL_Parse('20060101'));
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 1, 1), 'date',
                    [1]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 2, 1), 'date',
                    [15]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 3, 1), 'date',
                    []);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 4, 1), 'date',
                    [1]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 5, 1), 'date',
                    [16]);

}

function testSerialMonthGenerator() {
  runGeneratorTests(serialMonthGenerator_(1, ICAL_Parse('20060101')),
                    ical_dateBuilder(2006, 1, 1), 'month',
                    [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 ]);
  runGeneratorTests(serialMonthGenerator_(1, ICAL_Parse('20060401')),
                    ical_dateBuilder(2006, 4, 1), 'month',
                    [ 4, 5, 6, 7, 8, 9, 10, 11, 12 ]);
  runGeneratorTests(serialMonthGenerator_(2, ICAL_Parse('20060401')),
                    ical_dateBuilder(2006, 4, 1), 'month',
                    [ 4, 6, 8, 10, 12 ]);
  var g = serialMonthGenerator_(7, ICAL_Parse('20060401'));
  runGeneratorTests(g,
                    ical_dateBuilder(2006, 4, 1), 'month',
                    [ 4, 11 ]);
  runGeneratorTests(g,
                    ical_dateBuilder(2007, 11, 1), 'month',
                    [ 6 ]);
  runGeneratorTests(g,
                    ical_dateBuilder(2008, 6, 1), 'month',
                    [ 1, 8 ]);
  var g2 = serialMonthGenerator_(18, ICAL_Parse('20060401'));
  runGeneratorTests(g2,
                    ical_dateBuilder(2006, 4, 1), 'month',
                    [ 4 ]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2007, 11, 1), 'month',
                    [ 10 ]);
  runGeneratorTests(g2,
                    ical_dateBuilder(2008, 6, 1), 'month',
                    []);
  runGeneratorTests(g2,
                    ical_dateBuilder(2009, 6, 1), 'month',
                    [ 4 ]);
}

function testSerialYearGenerator() {
  runGeneratorTests(serialYearGenerator_(1, ICAL_Parse('20060101'),
                                         { limit_: 100, current_: 100 }),
                    ical_dateBuilder(2006, 1, 1), 'year',
                    [ 2006, 2007, 2008, 2009, 2010 ], 5);
  runGeneratorTests(serialYearGenerator_(2, ICAL_Parse('20060101'),
                                         { limit_: 100, current_: 100 }),
                    ical_dateBuilder(2006, 1, 1), 'year',
                    [ 2006, 2008, 2010, 2012, 2014 ], 5);
}

function testWeekIntervalFilter() {
  // *s match those that are in the weeks that should pass the filter

  var f1 = weekIntervalFilter_(2, RRULE_WDAY_MONDAY, ICAL_Parse('20050911'));
  // FOR f1
  //    September 2005
  //  Su  Mo  Tu  We  Th  Fr  Sa
  //                   1   2   3
  //   4  *5  *6  *7  *8  *9 *10
  // *11  12  13  14  15  16  17
  //  18 *19 *20 *21 *22 *23 *24
  // *25  26  27  28  29  30
  assertTrue( f1(ICAL_Parse('20050909')));
  assertTrue( f1(ICAL_Parse('20050910')));
  assertTrue( f1(ICAL_Parse('20050911')));
  assertTrue(!f1(ICAL_Parse('20050912')));
  assertTrue(!f1(ICAL_Parse('20050913')));
  assertTrue(!f1(ICAL_Parse('20050914')));
  assertTrue(!f1(ICAL_Parse('20050915')));
  assertTrue(!f1(ICAL_Parse('20050916')));
  assertTrue(!f1(ICAL_Parse('20050917')));
  assertTrue(!f1(ICAL_Parse('20050918')));
  assertTrue( f1(ICAL_Parse('20050919')));
  assertTrue( f1(ICAL_Parse('20050920')));
  assertTrue( f1(ICAL_Parse('20050921')));
  assertTrue( f1(ICAL_Parse('20050922')));
  assertTrue( f1(ICAL_Parse('20050923')));
  assertTrue( f1(ICAL_Parse('20050924')));
  assertTrue( f1(ICAL_Parse('20050925')));
  assertTrue(!f1(ICAL_Parse('20050926')));

  var f2 = weekIntervalFilter_(2, RRULE_WDAY_SUNDAY, ICAL_Parse('20050911'));
  // FOR f2
  //    September 2005
  //  Su  Mo  Tu  We  Th  Fr  Sa
  //                   1   2   3
  //   4   5   6   7   8   9  10
  // *11 *12 *13 *14 *15 *16 *17
  //  18  19  20  21  22  23  24
  // *25 *26 *27 *28 *29 *30
  assertTrue(!f2(ICAL_Parse('20050909')));
  assertTrue(!f2(ICAL_Parse('20050910')));
  assertTrue( f2(ICAL_Parse('20050911')));
  assertTrue( f2(ICAL_Parse('20050912')));
  assertTrue( f2(ICAL_Parse('20050913')));
  assertTrue( f2(ICAL_Parse('20050914')));
  assertTrue( f2(ICAL_Parse('20050915')));
  assertTrue( f2(ICAL_Parse('20050916')));
  assertTrue( f2(ICAL_Parse('20050917')));
  assertTrue(!f2(ICAL_Parse('20050918')));
  assertTrue(!f2(ICAL_Parse('20050919')));
  assertTrue(!f2(ICAL_Parse('20050920')));
  assertTrue(!f2(ICAL_Parse('20050921')));
  assertTrue(!f2(ICAL_Parse('20050922')));
  assertTrue(!f2(ICAL_Parse('20050923')));
  assertTrue(!f2(ICAL_Parse('20050924')));
  assertTrue( f2(ICAL_Parse('20050925')));
  assertTrue( f2(ICAL_Parse('20050926')));
}

function runRecurrenceIteratorTest(
    rruleText, dtStart, limit, golden, opt_advanceTo) {
  if (emitTestLog) { emitTestLog('<h4>' + rruleText + '</h4>'); }
  var ri = createRecurrenceIterator(new RRule(rruleText), dtStart);
  if (opt_advanceTo) {
    ri.AdvanceTo(opt_advanceTo);
  }
  var dates = [];
  while (ri.HasNext() && --limit >= 0) {
    dates.push(ri.Next());
  }
  if (limit < 0) { dates.push('...'); }
  assertEquals(golden, dates.toString());
}

function testSimpleDaily() {
  runRecurrenceIteratorTest('RRULE:FREQ=DAILY', ICAL_Parse('20060120'), 5,
                            '20060120,20060121,20060122,20060123,20060124,...');
}

function testSimpleWeekly() {
  runRecurrenceIteratorTest('RRULE:FREQ=WEEKLY', ICAL_Parse('20060120'), 5,
                            '20060120,20060127,20060203,20060210,20060217,...');
}

function testSimpleMonthly() {
  runRecurrenceIteratorTest('RRULE:FREQ=MONTHLY', ICAL_Parse('20060120'), 5,
                            '20060120,20060220,20060320,20060420,20060520,...');
}

function testSimpleYearly() {
  runRecurrenceIteratorTest('RRULE:FREQ=YEARLY', ICAL_Parse('20060120'), 5,
                            '20060120,20070120,20080120,20090120,20100120,...');
}

// from section 4.3.10
function testMultipleByParts() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=2;BYMONTH=1;BYDAY=SU',
      ICAL_Parse('19970105'), 8,
      '19970105,19970112,19970119,19970126,' +
      '19990103,19990110,19990117,19990124,...');
}

function testCountWithInterval() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=DAILY;COUNT=10;INTERVAL=2',
      ICAL_Parse('19970105'), 11,
      '19970105,19970107,19970109,19970111,19970113,' +
      '19970115,19970117,19970119,19970121,19970123');
}

// from section 4.6.5
function testNegativeOffsets() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=10',
      ICAL_Parse('19970105'), 5,
      '19971026,19981025,19991031,20001029,20011028,...');
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYDAY=1SU;BYMONTH=4',
      ICAL_Parse('19970105'), 5,
      '19970406,19980405,19990404,20000402,20010401,...');
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYDAY=1SU;BYMONTH=4;UNTIL=19980404T070000',
      ICAL_Parse('19970105'), 5, '19970406');
}

// feom section 4.8.5.4
function testDailyFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=DAILY;COUNT=10',
      ICAL_Parse('19970902T090000'), 11,
      '19970902T090000,19970903T090000,19970904T090000,19970905T090000,' +
      '19970906T090000,19970907T090000,19970908T090000,19970909T090000,' +
      '19970910T090000,19970911T090000');

}

function testDailyUntilDec4() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=DAILY;UNTIL=19971204',
      ICAL_Parse('19971128'), 11,
      '19971128,19971129,19971130,19971201,19971202,19971203,19971204');
}

function testEveryOtherDayForever() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=DAILY;INTERVAL=2',
      ICAL_Parse('19971128'), 5,
      '19971128,19971130,19971202,19971204,19971206,...');
}

function testEvery10Days5Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=DAILY;INTERVAL=10;COUNT=5',
      ICAL_Parse('19970902'), 5,
      '19970902,19970912,19970922,19971002,19971012');
}

function goldenDateRange(dateStr, opt_interval) {
  var interval = opt_interval || 1;
  var period = ICAL_Parse(dateStr);
  var b = ical_builderCopy(period.start);
  var out = [];
  while (true) {
    var d = b.toDate();
    if (d.getComparable() > period.end.getComparable()) { break; }
    out.push(d);
    b.date += interval;
  }
  return out.toString();
}

function testEveryDayInJanuaryFor3Years() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;UNTIL=20000131T090000;\n' +
      ' BYMONTH=1;BYDAY=SU,MO,TU,WE,TH,FR,SA',
      ICAL_Parse('19980101'), 100,
      [goldenDateRange('19980101/19980131'),
       goldenDateRange('19990101/19990131'),
       goldenDateRange('20000101/20000131')].join(','));
}

function testWeeklyFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;COUNT=10',
      ICAL_Parse('19970902'), 10,
      '19970902,19970909,19970916,19970923,19970930,' +
      '19971007,19971014,19971021,19971028,19971104');
}

function testWeeklyUntilDec24() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;UNTIL=19971224',
      ICAL_Parse('19970902'), 25,
      '19970902,19970909,19970916,19970923,19970930,' +
      '19971007,19971014,19971021,19971028,19971104,' +
      '19971111,19971118,19971125,19971202,19971209,' +
      '19971216,19971223');
}

function testEveryOtherWeekForever() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;INTERVAL=2;WKST=SU',
      ICAL_Parse('19970902'), 11,
      '19970902,19970916,19970930,19971014,19971028,' +
      '19971111,19971125,19971209,19971223,19980106,' +
      '19980120,...');
}

function testWeeklyOnTuesdayAndThursdayFor5Weeks() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;UNTIL=19971006T160000;WKST=SU;BYDAY=TU,TH',
      ICAL_Parse('19970902'), 11,
      '19970902,19970904,19970909,19970911,19970916,' +
      '19970918,19970923,19970925,19970930,19971002');
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;COUNT=10;WKST=SU;BYDAY=TU,TH',
      ICAL_Parse('19970902'), 11,
      '19970902,19970904,19970909,19970911,19970916,' +
      '19970918,19970923,19970925,19970930,19971002');
}

function testEveryOtherWeekOnMWFUntilDec24() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=19971223T160000;WKST=SU;\n' +
      ' BYDAY=MO,WE,FR',
      ICAL_Parse('19970903'), 25,
      '19970903,19970905,19970915,19970917,' +
      '19970919,19970929,19971001,19971003,19971013,' +
      '19971015,19971017,19971027,19971029,19971031,' +
      '19971110,19971112,19971114,19971124,19971126,' +
      '19971128,19971208,19971210,19971212,19971222');

  // if the UNTIL date is timed, when the start is not, the time should be
  // ignored, so we get one more instance
  // TODO: copy test to js
  runRecurrenceIteratorTest(
      "RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=19971224T000000;WKST=SU;\n" +
      " BYDAY=MO,WE,FR",
      ICAL_Parse("19970903"), 25,
      "19970903,19970905,19970915,19970917," +
      "19970919,19970929,19971001,19971003," +
      "19971013,19971015,19971017,19971027," +
      "19971029,19971031,19971110,19971112," +
      "19971114,19971124,19971126,19971128," +
      "19971208,19971210,19971212,19971222," +
      "19971224");

  // test with an alternate timezone
  if (false) {  // TODO(msamuel): timezone support
    runRecurrenceIteratorTest(
        "RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=19971224T090000Z;WKST=SU;\n" +
        " BYDAY=MO,WE,FR",
        ICAL_Parse("19970903T090000"), 25,
        "19970903T160000,19970905T160000,19970915T160000,19970917T160000," +
        "19970919T160000,19970929T160000,19971001T160000,19971003T160000," +
        "19971013T160000,19971015T160000,19971017T160000,19971027T170000," +
        "19971029T170000,19971031T170000,19971110T170000,19971112T170000," +
        "19971114T170000,19971124T170000,19971126T170000,19971128T170000," +
        "19971208T170000,19971210T170000,19971212T170000,19971222T170000",
        PST);
  }
}

function testEveryOtherWeekOnTuThFor8Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=8;WKST=SU;BYDAY=TU,TH',
      ICAL_Parse('19970902'), 8,
      '19970902,19970904,19970916,19970918,19970930,' +
      '19971002,19971014,19971016');
}

function testMonthlyOnThe1stFridayFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;COUNT=10;BYDAY=1FR',
      ICAL_Parse('19970905'), 10,
      '19970905,19971003,19971107,19971205,19980102,' +
      '19980206,19980306,19980403,19980501,19980605');
}

function testMonthlyOnThe1stFridayUntilDec24() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;UNTIL=19971223T160000;BYDAY=1FR',
      ICAL_Parse('19970905'), 4,
      '19970905,19971003,19971107,19971205');
}

function testEveryOtherMonthOnThe1stAndLastSundayFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;INTERVAL=2;COUNT=10;BYDAY=1SU,-1SU',
      ICAL_Parse('19970907'), 10,
      '19970907,19970928,19971102,19971130,19980104,' +
      '19980125,19980301,19980329,19980503,19980531');
}

function testMonthlyOnTheSecondToLastMondayOfTheMonthFor6Months() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;COUNT=6;BYDAY=-2MO',
      ICAL_Parse('19970922'), 6,
      '19970922,19971020,19971117,19971222,19980119,' +
      '19980216');
}

function testMonthlyOnTheThirdToLastDay() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;BYMONTHDAY=-3',
      ICAL_Parse('19970928'), 6,
      '19970928,19971029,19971128,19971229,19980129,19980226,...');
}

function testMonthlyOnThe2ndAnd15thFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=2,15',
      ICAL_Parse('19970902'), 10,
      '19970902,19970915,19971002,19971015,19971102,' +
      '19971115,19971202,19971215,19980102,19980115');
}

function testMonthlyOnTheFirstAndLastFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=1,-1',
      ICAL_Parse('19970930'), 10,
      '19970930,19971001,19971031,19971101,19971130,' +
      '19971201,19971231,19980101,19980131,19980201');
}

function testEvery18MonthsOnThe10thThru15thFor10Occ() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;INTERVAL=18;COUNT=10;BYMONTHDAY=10,11,12,13,14,\n' +
      ' 15',
      ICAL_Parse('19970910'), 10,
      '19970910,19970911,19970912,19970913,19970914,' +
      '19970915,19990310,19990311,19990312,19990313');
}

function testEveryTuesdayEveryOtherMonth() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU',
      ICAL_Parse('19970902'), 18,
      '19970902,19970909,19970916,19970923,19970930,' +
      '19971104,19971111,19971118,19971125,19980106,' +
      '19980113,19980120,19980127,19980303,19980310,' +
      '19980317,19980324,19980331,...');
}

function testYearlyInJuneAndJulyFor10Occurrences() {
  // Note: Since none of the BYDAY, BYMONTHDAY or BYYEARDAY components
  // are specified, the day is gotten from DTSTART
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;COUNT=10;BYMONTH=6,7',
      ICAL_Parse('19970610'), 10,
      '19970610,19970710,19980610,19980710,19990610,' +
      '19990710,20000610,20000710,20010610,20010710');
}

function testEveryOtherYearOnJanuaryFebruaryAndMarchFor10Occurrences() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=2;COUNT=10;BYMONTH=1,2,3',
      ICAL_Parse('19970310'), 10,
      '19970310,19990110,19990210,19990310,20010110,' +
      '20010210,20010310,20030110,20030210,20030310');
}

function testEvery3rdYearOnThe1st100thAnd200thDayFor10Occurrences() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;BYYEARDAY=1,100,200',
      ICAL_Parse('19970101'), 10,
      '19970101,19970410,19970719,20000101,20000409,' +
      '20000718,20030101,20030410,20030719,20060101');
}

function testEvery20thMondayOfTheYearForever() {
  runRecurrenceIteratorTest(
    'RRULE:FREQ=YEARLY;BYDAY=20MO',
    ICAL_Parse('19970519'), 3,
    '19970519,19980518,19990517,...');
}

function testMondayOfWeekNumber20WhereTheDefaultStartOfTheWeekIsMonday() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYWEEKNO=20;BYDAY=MO',
      ICAL_Parse('19970512'), 3,
      '19970512,19980511,19990517,...');
}

function testEveryThursdayInMarchForever() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=TH',
      ICAL_Parse('19970313'), 11,
      '19970313,19970320,19970327,19980305,19980312,' +
      '19980319,19980326,19990304,19990311,19990318,' +
      '19990325,...');
}

function testEveryThursdayButOnlyDuringJuneJulyAndAugustForever() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYDAY=TH;BYMONTH=6,7,8',
      ICAL_Parse('19970605'), 39,
      '19970605,19970612,19970619,19970626,19970703,' +
      '19970710,19970717,19970724,19970731,19970807,' +
      '19970814,19970821,19970828,19980604,19980611,' +
      '19980618,19980625,19980702,19980709,19980716,' +
      '19980723,19980730,19980806,19980813,19980820,' +
      '19980827,19990603,19990610,19990617,19990624,' +
      '19990701,19990708,19990715,19990722,19990729,' +
      '19990805,19990812,19990819,19990826,...');
}

function testEveryFridayThe13thForever() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;BYDAY=FR;BYMONTHDAY=13',
      ICAL_Parse('19970902'), 5,
      '19980213,19980313,19981113,19990813,20001013,' +
      '...');
}

function testTheFirstSaturdayThatFollowsTheFirstSundayOfTheMonthForever() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;BYDAY=SA;BYMONTHDAY=7,8,9,10,11,12,13',
      ICAL_Parse('19970913'), 10,
      '19970913,19971011,19971108,19971213,19980110,' +
      '19980207,19980307,19980411,19980509,19980613,' +
      '...');
}

function testEvery4YearsThe1stTuesAfterAMonInNovForever() {
  // US Presidential Election Day
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=4;BYMONTH=11;BYDAY=TU;BYMONTHDAY=2,3,4,\n' +
      ' 5,6,7,8',
      ICAL_Parse('19961105'), 3,
      '19961105,20001107,20041102,...');
}

function testThe3rdInstanceIntoTheMonthOfOneOfTuesWedThursForTheNext3Months() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;COUNT=3;BYDAY=TU,WE,TH;BYSETPOS=3',
      ICAL_Parse('19970904'), 3,
      '19970904,19971007,19971106');
}

function testThe2ndToLastWeekdayOfTheMonth() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-2',
      ICAL_Parse('19970929'), 7,
      '19970929,19971030,19971127,19971230,19980129,' +
      '19980226,19980330,...');
}

function testEvery3HoursFrom900AmTo500PmOnASpecificDay() {
  if (false) { // TODO(msamuel): implement hourly iteration
    runRecurrenceIteratorTest(
        'RRULE:FREQ=HOURLY;INTERVAL=3;UNTIL=19970902T170000',
        ICAL_Parse('19970902'), 7,
        '00000902,19970909,19970900,19970912,19970900,' +
        '19970915,19970900');
  }
}

function testEvery15MinutesFor6Occurrences() {
  if (false) { // TODO(msamuel): implement minutely iteration
    runRecurrenceIteratorTest(
        'RRULE:FREQ=MINUTELY;INTERVAL=15;COUNT=6',
        ICAL_Parse('19970902'), 13,
        '00000902,19970909,19970900,19970909,19970915,' +
        '19970909,19970930,19970909,19970945,19970910,' +
        '19970900,19970910,19970915');
  }
}

function testEveryHourAndAHalfFor4Occurrences() {
  if (false) { // TODO(msamuel): implement minutely iteration
    runRecurrenceIteratorTest(
        'RRULE:FREQ=MINUTELY;INTERVAL=90;COUNT=4',
        ICAL_Parse('19970902'), 9,
        '00000902,19970909,19970900,19970910,19970930,' +
        '19970912,19970900,19970913,19970930');
  }
}

function testAnExampleWhereTheDaysGeneratedMakesADifferenceBecauseOfWkst() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=MO',
      ICAL_Parse('19970805'), 4,
      '19970805,19970810,19970819,19970824');
}

function testAnExampleWhereTheDaysGeneratedMakesADifferenceBecauseOfWkst2() {
  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=SU',
      ICAL_Parse('19970805'), 8,
      '19970805,19970817,19970819,19970831');
}

function testWithByDayAndByMonthDayFilter() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=WEEKLY;COUNT=4;BYDAY=TU,SU;" +
      "BYMONTHDAY=13,14,15,16,17,18,19,20",
      ICAL_Parse("19970805"), 8,
      "19970817,19970819,19970914,19970916");
}

function testAnnuallyInAugustOnTuesAndSunBetween13thAnd20th() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;COUNT=4;BYDAY=TU,SU;" +
      "BYMONTHDAY=13,14,15,16,17,18,19,20;BYMONTH=8",
      ICAL_Parse("19970605"), 8,
      "19970817,19970819,19980816,19980818");
}

function testLastDayOfTheYearIsASundayOrTuesday() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;COUNT=4;BYDAY=TU,SU;BYYEARDAY=-1",
      ICAL_Parse("19940605"), 8,
      "19951231,19961231,20001231,20021231");
}

function testLastWeekdayOfMonth() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=MONTHLY;BYSETPOS=-1;BYDAY=-1MO,-1TU,-1WE,-1TH,-1FR",
      ICAL_Parse("19940605"), 8,
      "19940630,19940729,19940831,19940930,"
      + "19941031,19941130,19941230,19950131,...");
}

function testMonthsThatStartOrEndOnFriday() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=MONTHLY;BYMONTHDAY=1,-1;BYDAY=FR;COUNT=6",
      ICAL_Parse("19940605"), 8,
      "19940701,19940930,19950331,19950630,19950901,19951201");
}

function testMonthsThatStartOrEndOnFridayOnEvenWeeks() {
  // figure out which of the answers from the above fall on even weeks
  var dtStart = ICAL_Parse("19940603");
  var golden = [];
  var candidates = [ ICAL_Parse("19940701"), ICAL_Parse("19940930"),
                     ICAL_Parse("19950331"), ICAL_Parse("19950630"),
                     ICAL_Parse("19950901"), ICAL_Parse("19951201") ];
  for (var i = 0; i < candidates.length; ++i) {
    var candidate = candidates[i];
    if (0 === ICAL_daysBetweenDates(candidate, dtStart) % 14) {
      golden.push(candidate.toString());
    }
  }

  runRecurrenceIteratorTest(
      "RRULE:FREQ=WEEKLY;INTERVAL=2;BYMONTHDAY=1,-1;BYDAY=FR;COUNT=3",
      dtStart, 8, golden.join(','));
}

function testCenturiesThatAreNotLeapYears() {
  // I can't think of a good reason anyone would want to specify both a
  // month day and a year day, so here's a really contrived example
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;INTERVAL=100;BYYEARDAY=60;BYMONTHDAY=1",
      ICAL_Parse("19000101"), 4,
      "19000301,21000301,22000301,23000301,...");
}

function testNextCalledWithoutHasNext() {
  var riter = createRecurrenceIterator(
      new RRule("RRULE:FREQ=DAILY"),
      ICAL_Parse("20000101"));
  assertEquals(ICAL_Parse("20000101"), riter.Next());
  assertEquals(ICAL_Parse("20000102"), riter.Next());
  assertEquals(ICAL_Parse("20000103"), riter.Next());
}

function testNoInstancesGenerated() {
  var riter = createRecurrenceIterator(
      new RRule("RRULE:FREQ=DAILY;UNTIL=19990101"),
      ICAL_Parse("20000101"));
  assertTrue(!riter.HasNext());

  assertNull(riter.Next());
  assertNull(riter.Next());
  assertNull(riter.Next());
}

function testNoInstancesGenerated2() {
  var riter = createRecurrenceIterator(
      new RRule("RRULE:FREQ=YEARLY;BYMONTH=2;BYMONTHDAY=30"),
      ICAL_Parse("20000101"));
  assertTrue(!riter.HasNext());
}

function testNoInstancesGenerated3() {
  var riter = createRecurrenceIterator(
      new RRule("RRULE:FREQ=YEARLY;INTERVAL=4;BYYEARDAY=366"),
      ICAL_Parse("20010101"));
  assertTrue(!riter.HasNext());
}

function testLastWeekdayOfMarch() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=MONTHLY;BYMONTH=3;BYDAY=SA,SU;BYSETPOS=-1",
      ICAL_Parse("20000101"), 4,
      "20000326,20010331,20020331,20030330,...");
}

function testFirstWeekdayOfMarch() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=MONTHLY;BYMONTH=3;BYDAY=SA,SU;BYSETPOS=1",
      ICAL_Parse("20000101"), 4,
      "20000304,20010303,20020302,20030301,...");
}


//     January 1999
// Mo Tu We Th Fr Sa Su
//              1  2  3    // < 4 days, so not a week
//  4  5  6  7  8  9 10

//     January 2000
// Mo Tu We Th Fr Sa Su
//                 1  2    // < 4 days, so not a week
//  3  4  5  6  7  8  9

//     January 2001
// Mo Tu We Th Fr Sa Su
//  1  2  3  4  5  6  7
//  8  9 10 11 12 13 14

//     January 2002
// Mo Tu We Th Fr Sa Su
//     1  2  3  4  5  6
//  7  8  9 10 11 12 13

/**
 * Find the first weekday of the first week of the year.
 * The first week of the year may be partial, and the first week is considered
 * to be the first one with at least four days.
 */
function testFirstWeekdayOfFirstWeekOfYear() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYWEEKNO=1;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=1",
      ICAL_Parse("19990101"), 4,
      "19990104,20000103,20010101,20020101,...");
}

function testFirstSundayOfTheYear1() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYWEEKNO=1;BYDAY=SU",
      ICAL_Parse("19990101"), 4,
      "19990110,20000109,20010107,20020106,...");
}

function testFirstSundayOfTheYear2() {
  // TODO(msamuel): is this right?
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYDAY=1SU",
      ICAL_Parse("19990101"), 4,
      "19990103,20000102,20010107,20020106,...");
}

function testFirstSundayOfTheYear3() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYDAY=SU;BYYEARDAY=1,2,3,4,5,6,7,8,9,10,11,12,13"
      + ";BYSETPOS=1",
      ICAL_Parse("19990101"), 4,
      "19990103,20000102,20010107,20020106,...");
}

function testFirstWeekdayOfYear() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=1",
      ICAL_Parse("19990101"), 4,
      "19990101,20000103,20010101,20020101,...");
}

function testLastWeekdayOfFirstWeekOfYear() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYWEEKNO=1;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-1",
      ICAL_Parse("19990101"), 4,
      "19990108,20000107,20010105,20020104,...");
}

//     January 1999
// Mo Tu We Th Fr Sa Su
//              1  2  3
//  4  5  6  7  8  9 10
// 11 12 13 14 15 16 17
// 18 19 20 21 22 23 24
// 25 26 27 28 29 30 31

function testSecondWeekday1() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=2",
      ICAL_Parse("19990101"), 4,
      "19990105,19990112,19990119,19990126,...");
}

//     January 1997
// Mo Tu We Th Fr Sa Su
//        1  2  3  4  5
//  6  7  8  9 10 11 12
// 13 14 15 16 17 18 19
// 20 21 22 23 24 25 26
// 27 28 29 30 31

function testSecondWeekday2() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=2",
      ICAL_Parse("19970101"), 4,
      "19970102,19970107,19970114,19970121,...");
}

function testByYearDayAndByDayFilterInteraction() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYYEARDAY=15;BYDAY=3MO",
      ICAL_Parse("19990101"), 4,
      "20010115,20070115,20180115,20240115,...");
}

function testByDayWithNegWeekNoAsFilter() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=MONTHLY;BYMONTHDAY=26;BYDAY=-1FR",
      ICAL_Parse("19990101"), 4,
      "19990226,19990326,19991126,20000526,...");
}

function testLastWeekOfTheYear() {
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYWEEKNO=-1",
      ICAL_Parse("19990101"), 6,
      "19991227,19991228,19991229,19991230,19991231,20001225,...");
}


function testAdvanceTo() {
  // a bunch of tests grabbed from above with an advance-to date tacked on

  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=TH',
      ICAL_Parse('19970313'), 11,
      /*'19970313,19970320,19970327,'*/'19980305,19980312,' +
      '19980319,19980326,19990304,19990311,19990318,' +
      '19990325,20000302,20000309,20000316,...',
      ICAL_Parse('19970601'));

  runRecurrenceIteratorTest(
    'RRULE:FREQ=YEARLY;BYDAY=20MO',
    ICAL_Parse('19970519'), 3,
    /*'19970519,'*/'19980518,19990517,20000515,...',
    ICAL_Parse('19980515'));

  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=3;UNTIL=20090101;BYYEARDAY=1,100,200',
      ICAL_Parse('19970101'), 10,
      /*'19970101,19970410,19970719,20000101,'*/'20000409,' +
      '20000718,20030101,20030410,20030719,20060101,20060410,20060719,20090101',
      ICAL_Parse('20000228'));

  // make sure that count preserved
  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;BYYEARDAY=1,100,200',
      ICAL_Parse('19970101'), 10,
      /*'19970101,19970410,19970719,20000101,'*/'20000409,' +
      '20000718,20030101,20030410,20030719,20060101',
      ICAL_Parse('20000228'));

  runRecurrenceIteratorTest(
      'RRULE:FREQ=YEARLY;INTERVAL=2;COUNT=10;BYMONTH=1,2,3',
     ICAL_Parse('19970310'), 10,
      /*'19970310,'*/'19990110,19990210,19990310,20010110,' +
      '20010210,20010310,20030110,20030210,20030310',
      ICAL_Parse('19980401'));

  runRecurrenceIteratorTest(
      'RRULE:FREQ=WEEKLY;UNTIL=19971224',
      ICAL_Parse('19970902'), 25,
      /*'19970902,19970909,19970916,19970923,'*/'19970930,' +
      '19971007,19971014,19971021,19971028,19971104,' +
      '19971111,19971118,19971125,19971202,19971209,' +
      '19971216,19971223',
      ICAL_Parse('19970930'));

  runRecurrenceIteratorTest(
      'RRULE:FREQ=MONTHLY;INTERVAL=18;BYMONTHDAY=10,11,12,13,14,\n' +
      ' 15',
      ICAL_Parse('19970910'), 5,
      /*'19970910,19970911,19970912,19970913,19970914,' +
        '19970915,'*/'19990310,19990311,19990312,19990313,19990314,...',
      ICAL_Parse('19990101'));

  // advancing into the past
  runRecurrenceIteratorTest(
      "RRULE:FREQ=MONTHLY;INTERVAL=18;BYMONTHDAY=10,11,12,13,14,\n" +
      " 15",
      ICAL_Parse("19970910"), 11,
      "19970910,19970911,19970912,19970913,19970914," +
      "19970915,19990310,19990311,19990312,19990313,19990314,...",
      ICAL_Parse("19970901"));

  // skips first instance
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;INTERVAL=100;BYMONTH=2;BYMONTHDAY=29",
      ICAL_Parse("19000101"), 5,
      // would return 2000
      "24000229,28000229,32000229,36000229,40000229,...",
      ICAL_Parse("20040101"));

  // filter hits until date before first instnace
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;INTERVAL=100;BYMONTH=2;BYMONTHDAY=29;UNTIL=21000101",
      ICAL_Parse("19000101"), 5,
      "",
      ICAL_Parse("20040101"));

  // advancing something that returns no instances
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYMONTH=2;BYMONTHDAY=30",
      ICAL_Parse("20000101"), 10,
      "",
      ICAL_Parse("19970901"));

  // advancing something that returns no instances and has a BYSETPOS rule
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYMONTH=2;BYMONTHDAY=30,31;BYSETPOS=1",
      ICAL_Parse("20000101"), 10,
      "",
      ICAL_Parse("19970901"));

  // advancing way past year generator timeout
  runRecurrenceIteratorTest(
      "RRULE:FREQ=YEARLY;BYMONTH=2;BYMONTHDAY=28",
      ICAL_Parse("20000101"), 10,
      "",
      ICAL_Parse("25000101"));

  // TODO(msamuel): check advancement of more examples
}

// TODO(msamuel): test BYSETPOS with FREQ in (WEEKLY,MONTHLY,YEARLY) x
// (setPos absolute, setPos relative, setPos mixed)

// TODO(msamuel): import libical test suites

// TODO(msamuel): test RecurrenceIterator.AdvanceTo operation
