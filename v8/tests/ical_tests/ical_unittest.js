function testFmtInt() {
  assertEquals('002', ICAL_fmtInt(2, 3));
  assertEquals(4, ICAL_firstDayOfWeekInMonth(2004, 7));
}

function testDaysInMonth() {
  assertEquals(31, ICAL_daysInMonth(2004, 7));
  assertEquals(31, ICAL_daysInMonth(2004, 8));
  assertEquals(30, ICAL_daysInMonth(2004, 9));
  assertEquals(29, ICAL_daysInMonth(2004, 2));
  assertEquals(28, ICAL_daysInMonth(2003, 2));
  assertEquals(31, ICAL_daysInMonth(2004, 12));
  assertEquals(31, ICAL_daysInMonth(2005, 3));
  assertEquals(31, ICAL_daysInMonth(2005, 3));
  assertEquals(29, ICAL_daysInMonth(2000, 2));
  assertEquals(28, ICAL_daysInMonth(1999, 2));
}

function testDaysBetween() {
  assertEquals(   0, ICAL_daysBetween(2003, 12, 31, 2003, 12, 31));
  assertEquals( -60, ICAL_daysBetween(2003, 12, 31, 2004, 2, 29));
  assertEquals( -66, ICAL_daysBetween(2003, 12, 31, 2004, 3, 6));
  assertEquals( -69, ICAL_daysBetween(2003, 12, 31, 2004, 3, 9));
  assertEquals(-305, ICAL_daysBetween(2003, 12, 31, 2004, 10, 31));
  assertEquals(-306, ICAL_daysBetween(2003, 12, 31, 2004, 11, 1));

  assertEquals(  60, ICAL_daysBetween(2004, 2, 29, 2003, 12, 31));
  assertEquals(   0, ICAL_daysBetween(2004, 2, 29, 2004, 2, 29));
  assertEquals(  -6, ICAL_daysBetween(2004, 2, 29, 2004, 3, 6));
  assertEquals(  -9, ICAL_daysBetween(2004, 2, 29, 2004, 3, 9));
  assertEquals(-245, ICAL_daysBetween(2004, 2, 29, 2004, 10, 31));
  assertEquals(-246, ICAL_daysBetween(2004, 2, 29, 2004, 11, 1));

  assertEquals(  66, ICAL_daysBetween(2004, 3, 6, 2003, 12, 31));
  assertEquals(   6, ICAL_daysBetween(2004, 3, 6, 2004, 2, 29));
  assertEquals(   0, ICAL_daysBetween(2004, 3, 6, 2004, 3, 6));
  assertEquals(  -3, ICAL_daysBetween(2004, 3, 6, 2004, 3, 9));
  assertEquals(-239, ICAL_daysBetween(2004, 3, 6, 2004, 10, 31));
  assertEquals(-240, ICAL_daysBetween(2004, 3, 6, 2004, 11, 1));

  assertEquals(  69, ICAL_daysBetween(2004, 3, 9, 2003, 12, 31));
  assertEquals(   9, ICAL_daysBetween(2004, 3, 9, 2004, 2, 29));
  assertEquals(   3, ICAL_daysBetween(2004, 3, 9, 2004, 3, 6));
  assertEquals(   0, ICAL_daysBetween(2004, 3, 9, 2004, 3, 9));
  assertEquals(-236, ICAL_daysBetween(2004, 3, 9, 2004, 10, 31));
  assertEquals(-237, ICAL_daysBetween(2004, 3, 9, 2004, 11, 1));

  assertEquals( 305, ICAL_daysBetween(2004, 10, 31, 2003, 12, 31));
  assertEquals( 245, ICAL_daysBetween(2004, 10, 31, 2004, 2, 29));
  assertEquals( 239, ICAL_daysBetween(2004, 10, 31, 2004, 3, 6));
  assertEquals( 236, ICAL_daysBetween(2004, 10, 31, 2004, 3, 9));
  assertEquals(   0, ICAL_daysBetween(2004, 10, 31, 2004, 10, 31));
  assertEquals(  -1, ICAL_daysBetween(2004, 10, 31, 2004, 11, 1));

  assertEquals( 306, ICAL_daysBetween(2004, 11, 1, 2003, 12, 31));
  assertEquals( 246, ICAL_daysBetween(2004, 11, 1, 2004, 2, 29));
  assertEquals( 240, ICAL_daysBetween(2004, 11, 1, 2004, 3, 6));
  assertEquals( 237, ICAL_daysBetween(2004, 11, 1, 2004, 3, 9));
  assertEquals(   1, ICAL_daysBetween(2004, 11, 1, 2004, 10, 31));
  assertEquals(   0, ICAL_daysBetween(2004, 11, 1, 2004, 11, 1));

  assertEquals(-365, ICAL_daysBetween(2003, 1, 1, 2004, 1, 1));
  assertEquals(-366, ICAL_daysBetween(2004, 1, 1, 2005, 1, 1));
  assertEquals(-365, ICAL_daysBetween(2005, 1, 1, 2006, 1, 1));
}

function testDate() {
  var d1, d2;
  var delta;

  d1 = ICAL_Parse('20040712');
  assertEquals(2004, d1.year);
  assertEquals(7, d1.month);
  assertEquals(12, d1.date);
  assertTrue(isNaN(d1.hour));
  assertTrue(isNaN(d1.minute));
  assertTrue(isNaN(d1.second));
  assertEquals('20040712', d1.toString());
  assertEquals(1, ICAL_getDayOfWeek(d1));
  assertEquals(1, d1.getDayOfWeek());

  d2 = ICAL_Parse('20040915');
  assertEquals(2004, d2.year);
  assertEquals(9, d2.month);
  assertEquals(15, d2.date);
  delta = ICAL_DurationBetween(d1, d2);
  assertEquals(-65, delta.date);
  assertEquals(0, delta.hour);
  assertEquals(0, delta.minute);
  assertEquals(0, delta.second);
  assertEquals('-P65D', delta.toString());

  assertTrue(d1.equals(d1));
  assertTrue(d2.equals(d2));
  assertTrue(!d1.equals(d2));
  assertTrue(!d2.equals(d1));

  var d3 = ICAL_Parse(d1.toString());
  assertTrue(d1.equals(d3));
  assertTrue(d1.equals(d3));
  assertTrue(!d3.equals(d2));
  assertTrue(!d2.equals(d3));
}

function testTime() {
  var t1, t2;
  t1 = ICAL_Parse('T123059');
  assertEquals(12, t1.hour);
  assertEquals(30, t1.minute);
  assertEquals(59, t1.second);
  assertTrue(isNaN(t1.year));
  assertTrue(isNaN(t1.month));
  assertTrue(isNaN(t1.date));
  assertEquals('T123059', t1.toString());

  t2 = ICAL_Parse('T090000');
  assertEquals(9, t2.hour);
  assertEquals(0, t2.minute);
  assertEquals(0, t2.second);

  assertTrue(t1.equals(t1));
  assertTrue(t2.equals(t2));
  assertTrue(!t1.equals(t2));
  assertTrue(!t2.equals(t1));

  var t3 = ICAL_Parse(t1.toString());
  assertTrue(t1.equals(t3));
  assertTrue(t1.equals(t3));
  assertTrue(!t3.equals(t2));
  assertTrue(!t2.equals(t3));
}

function testDateTime() {
  var dt1, dt2;
  dt1 = ICAL_Parse('20041124T144030');
  dt2 = ICAL_Parse('20041123T123000');
  assertEquals(2004, dt1.year);
  assertEquals(11, dt1.month);
  assertEquals(24, dt1.date);
  assertEquals(14, dt1.hour);
  assertEquals(40, dt1.minute);
  assertEquals(30, dt1.second);
  assertEquals('20041124T144030', dt1.toString());
  assertEquals('20041123T123000', dt2.toString());
  assertEquals('20041124', dt1.toDate().toString());
  assertEquals('T144030', dt1.toTime().toString());
  assertEquals('P1DT2H10M30S', ICAL_DurationBetween(dt1, dt2).toString());
  ICAL_DurationBetween(dt2, dt1);
  assertEquals('-P1DT2H10M30S', ICAL_DurationBetween(dt2, dt1).toString());

  assertEquals(3, dt1.getDayOfWeek());
  assertEquals(2, dt2.getDayOfWeek());

  assertTrue(dt1.equals(dt1));
  assertTrue(dt2.equals(dt2));
  assertTrue(!dt1.equals(dt2));
  assertTrue(!dt2.equals(dt1));

  var dt3 = ICAL_Parse(dt1.toString());
  assertTrue(dt1.equals(dt3));
  assertTrue(dt1.equals(dt3));
  assertTrue(!dt3.equals(dt2));
  assertTrue(!dt2.equals(dt3));

  var dt4 = ICAL_Parse('20041128T125000');
  var b = ical_builderCopy(dt4);
  b.minute += 30;
  var dt5 = b.toDateTime();
  assertEquals('20041128T132000', dt5.toString());
  assertEquals('PT30M', ICAL_DurationBetween(dt5, dt4).toString());
}

function testDateRollover() {
  var b;

  var d1 = ICAL_Parse('20041201');
  b = ical_builderCopy(d1);
  b.date -= 1;
  assertEquals('20041130', b.toDate().toString());

  var d2 = ICAL_Parse('20041130');
  b = ical_builderCopy(d2);
  b.date += 1;
  assertEquals('20041201', b.toDate().toString());

  var d3 = ICAL_Parse('20040101');
  b = ical_builderCopy(d3);
  b.date -= 1;
  assertEquals('20031231', b.toDate().toString());

  var d4 = ICAL_Parse('20041231');
  b = ical_builderCopy(d4);
  b.date += 1;
  assertEquals('20050101', b.toDate().toString());
}

function testDuration() {
  var delta;
  delta = new ICAL_Duration(0, 48, 12, 180);
  assertEquals('P2DT15M', delta.toString());
  delta = new ICAL_Duration(0, -48, -12, -180);
  assertEquals('-P2DT15M', delta.toString());
  delta = new ICAL_Duration(0, -48, -12, 180);
  assertEquals('-P2DT9M', delta.toString());
}

function testPeriodOfTime() {
  var pot = ICAL_Parse('20041124T120000/P1W4DT3H30M0S');
  assertEquals('20041124T120000', pot.start.toString());
  assertEquals('20041205T153000', pot.end.toString());
  assertEquals('20041124T120000/20041205T153000', pot.toString());
  assertEquals('P11DT3H30M', pot.duration.toString());
}

function testPeriodOfTime2() {
  var pot = ICAL_Parse('20060118/20060123');
  assertEquals(5, pot.duration.date);
}

function testDTBuilder() {
  var pot = ICAL_Parse('20041124T120000/P1W4DT3H30M0S');
  var b;
  b = ical_builderCopy(pot.start);
  b.date += 11;
  b.hour += 3;
  b.minute += 30;
  assertEquals('20041205T153000', b.toDateTime().toString());
  assertTrue(b.toDateTime().equals(pot.end));
}

function testDTBuilderMidnight() {
  var dt = ICAL_Parse('20050311T200000');
  var b = ical_builderCopy(dt);
  b.hour += 4;

  var dt2 = b.toDateTime();
  var dt3 = ICAL_Parse('20050312T000000');
  assertEquals(dt2.toString(), dt3.toString());
  assertTrue(dt2.equals(dt3));
}


function testDTBuilderValidate() {
  var d1 = ICAL_Parse('20041201');
  var dtb = ical_builderCopy(d1);

  assertTrue( dtb.validateDate());
  assertTrue( dtb.validateTime());
  assertTrue( dtb.validateDateTime());

  dtb.hour = undefined;
  dtb.minute = undefined;
  dtb.second = undefined;

  assertTrue( dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue(!dtb.validateTime());

  dtb.hour = '0';
  dtb.minute = 32;
  dtb.second = 4;

  assertTrue( dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue(!dtb.validateTime());

  dtb.hour = 9;

  assertTrue( dtb.validateDate());
  assertTrue( dtb.validateTime());
  assertTrue( dtb.validateDateTime());

  dtb.date = null;

  assertTrue(!dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue( dtb.validateTime());

  dtb.minute = NaN;

  assertTrue(!dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue(!dtb.validateTime());

  dtb.minute = 0;

  dtb.date = [];

  assertTrue(!dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue( dtb.validateTime());

  dtb.date = Infinity;

  assertTrue(!dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue( dtb.validateTime());

  dtb.date = NaN;

  assertTrue(!dtb.validateDate());
  assertTrue(!dtb.validateDateTime());
  assertTrue( dtb.validateTime());

  dtb.date = 1;

  assertTrue( dtb.validateDate());
  assertTrue( dtb.validateDateTime());
  assertTrue( dtb.validateTime());
}

function testNormalization() {
  assertEquals('20060301', ical_dateBuilder(2006,  2, 29).toDate().toString());
  assertEquals('20071001', ical_dateBuilder(2006, 22, 1).toDate().toString());
  assertEquals('20050801', ical_dateBuilder(2006, -4, 1).toDate().toString());
  assertEquals('20060520', ical_dateBuilder(2006,  4, 50).toDate().toString());
  assertEquals('20060331', ical_dateBuilder(2006,  4, 0).toDate().toString());
  assertEquals('20060321', ical_dateBuilder(2006,  4, -10).toDate().toString());

  assertEquals('20041115', ical_dateBuilder(2006, -13, 15).toDate().toString());
  assertEquals('20041215', ical_dateBuilder(2006, -12, 15).toDate().toString());
  assertEquals('20050115', ical_dateBuilder(2006, -11, 15).toDate().toString());
  assertEquals('20051215', ical_dateBuilder(2006,   0, 15).toDate().toString());
  assertEquals('20061115', ical_dateBuilder(2006,  11, 15).toDate().toString());
  assertEquals('20061215', ical_dateBuilder(2006,  12, 15).toDate().toString());
  assertEquals('20070115', ical_dateBuilder(2006,  13, 15).toDate().toString());
  assertEquals('20081115', ical_dateBuilder(2006,  35, 15).toDate().toString());
  assertEquals('20081215', ical_dateBuilder(2006,  36, 15).toDate().toString());
  assertEquals('20090115', ical_dateBuilder(2006,  37, 15).toDate().toString());

  assertEquals('20330831',
               ical_dateBuilder(2006, 4, 10015).toDate().toString());
  assertEquals('19781128',
               ical_dateBuilder(2006, 4, -9985).toDate().toString());
}

function testParsePartial() {
  var partialDate = ICAL_ParsePartial('20060623');
  assertEquals(2006, partialDate.year);
  assertEquals(6, partialDate.month);
  assertEquals(23, partialDate.date);
  partialDate = ICAL_ParsePartial('2006??23');
  assertEquals(2006, partialDate.year);
  assertEquals(undefined, partialDate.month);
  assertEquals(23, partialDate.date);
}

function testDateMin() {
  var jan1 = ICAL_Parse('20060101'), jan2 = ICAL_Parse('20060102');
  assertEquals(jan1, ICAL_dateMin(jan1, jan2));
  assertEquals(jan1, ICAL_dateMin(jan2, jan1));
  assertEquals(jan1, ICAL_dateMin(jan1, jan1));
}

function testDateMax() {
  var jan1 = ICAL_Parse('20060101'), jan2 = ICAL_Parse('20060102');
  assertEquals(jan2, ICAL_dateMax(jan1, jan2));
  assertEquals(jan2, ICAL_dateMax(jan2, jan1));
  assertEquals(jan1, ICAL_dateMax(jan1, jan1));
}

function testOverlap() {
  var jan1 = ICAL_Parse('20060101'),
      jan2 = ICAL_Parse('20060102'),
      jan3 = ICAL_Parse('20060103')
      jan4 = ICAL_Parse('20060104')
      jan5 = ICAL_Parse('20060105')
      jan6 = ICAL_Parse('20060106')
      jan7 = ICAL_Parse('20060107');
  var jan1_3 = new ICAL_PeriodOfTime(jan1, jan3),
      jan1_5 = new ICAL_PeriodOfTime(jan1, jan5),
      jan1_6 = new ICAL_PeriodOfTime(jan1, jan6),
      jan2_3 = new ICAL_PeriodOfTime(jan2, jan3),
      jan2_4 = new ICAL_PeriodOfTime(jan2, jan4),
      jan2_7 = new ICAL_PeriodOfTime(jan2, jan7),
      jan5_6 = new ICAL_PeriodOfTime(jan5, jan6);
      jan5_7 = new ICAL_PeriodOfTime(jan5, jan7);

  // 7 cases
  // (1) no overlap - left
  //        AAAA
  //              BBBB
  assertEquals(null, ICAL_overlap(jan1_3, jan5_7));
  // (2) no overlap - right
  //              AAAA
  //        BBBB
  assertEquals(null, ICAL_overlap(jan5_7, jan1_3));
  // (3) partial overlap - left
  //        AAAAAAAA
  //              BBBB
  assertEquals(jan5_6.toString(), ICAL_overlap(jan1_6, jan5_7).toString());
  // (4) partial overlap - right
  //          AAAAAAAA
  //        BBBB
  assertEquals(jan2_3.toString(), ICAL_overlap(jan2_7, jan1_3).toString());
  // (5) subset
  //        AAAAAAAA
  //          BBBB
  assertEquals(jan2_4.toString(), ICAL_overlap(jan1_5, jan2_4).toString());
  // (6) superset
  //        AAAAAAAA
  //          BBBB
  assertEquals(jan2_4.toString(), ICAL_overlap(jan2_4, jan1_5).toString());
  // (7) equal
  //          AAAA
  //          BBBB
  assertEquals(jan2_4.toString(), ICAL_overlap(jan2_4, jan2_4).toString());
}
