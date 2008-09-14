var dtstart = ICAL_Parse('19850922');

function typeOf(obj) {
  var t = typeof obj;
  if ('object' != t) { return t; }
  if (null == obj) { return '&lt;null&gt;'; }
  if ('type' in obj) { return HtmlEscape(obj.type.toString()); }
  var ctr = obj.constructor.toString();
  var m = ctr.match(/^function (\w+)[^\s\S]*/);
  if (m) { ctr = m[1]; }
  if ('Array' == ctr && obj.length) { ctr += '<' + RR_Type(obj[0]) + '>'; }
  return HtmlEscape(ctr);
}

function debugRRule(icalString) {
  var rr = new _RRule(icalString);
  var html = [];
  html.push('<pre style="border:1px solid black">', HtmlEscape(icalString),
            '<br><br>');
  html.push('<table>');
  html.push('<tr><td>Content<td colspan=2>', HtmlEscape(rr.content));
  for (var k in rr.params) {
    html.push('<tr><td>Param<td>', HtmlEscape(k), '<td>&nbsp;',
              HtmlEscape(rr.params[k].toString()), ' : ', typeOf(rr.params[k]));
  }
  for (var k in rr.members) {
    html.push('<tr><td>Member<td>', HtmlEscape(k), '<td>&nbsp;',
              HtmlEscape(rr.members[k].toString()), ' : ',
              typeOf(rr.members[k]));
  }
  var icalOut = rr._ToIcal();
  html.push('<tr><td>Rendered<td colspan=2>');
  html.push(icalOut);
  html.push('<tr><td>Presentation<td colspan=2>');
  var humanReadable = rr._ToHumanLanguage(dtstart);
  if (null === humanReadable) {
    html.push('&lt;null&gt;');
  } else {
    html.push(HtmlEscape(humanReadable).
              replace(/undefined/g, '<b style=color:red>undefined</b>'));
  }
  html.push('<tr><td>IntervalInDays<td colspan=2>' +
            rr.ApproximateIntervalInDays());
  html.push('<\/table>');
  html.push('<\/pre>');
  return html.join('');
}

function runTest(icalString, expectedHumanForm) {
  var rr = new _RRule(icalString);
  var icalOut = rr._ToIcal();
  assertEquals(icalOut, icalString.replace(/(?:\r\n?|\n)[ \t]/g, ''));

  var humanReadable = rr._ToHumanLanguage(dtstart);
  assertEquals(expectedHumanForm, humanReadable);
}

function testHumanLanguageForm() {
  runTest('RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-1', null);
  runTest('RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR', 'Monthly every weekday');
  runTest('RRULE:FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR', 'Weekly on weekdays');
  runTest('RRULE:FREQ=YEARLY;INTERVAL=2;BYMONTH=1;BYDAY=SU;BYHOUR=8,9;\n' +
          ' BYMINUTE=30', 'Every 2 years on every Sunday in January');
  runTest('RRULE:FREQ=YEARLY', 'Annually on September 22');
  runTest('RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=10',
          'Annually on the last Sunday in October');
  runTest('RRULE:FREQ=YEARLY;BYDAY=1SU;BYMONTH=4',
          'Annually on the first Sunday in April');
  runTest('RRULE:FREQ=YEARLY;' +
          '\n BYDAY=2SU;BYMONTH=4;UNTIL=19980404T110000',
          'Annually on the second Sunday in April, until Apr 4, 1998');
  runTest('RRULE:FREQ=DAILY;COUNT=10', 'Daily, 10 times');
  runTest('RRULE:FREQ=DAILY;INTERVAL=2', 'Every 2 days');
  runTest('RRULE:FREQ=DAILY;INTERVAL=10;COUNT=5', 'Every 10 days, 5 times');
  runTest('RRULE:FREQ=YEARLY;UNTIL=20000131T090000;\n' +
          ' BYMONTH=1;BYDAY=SU,MO,TU,WE,TH,FR,SA',
          'Annually on every day in January, until Jan 31, 2000');
  runTest('RRULE:FREQ=DAILY;UNTIL=20000131T090000;BYMONTH=1',
          'Daily, until Jan 31, 2000');
  runTest('RRULE:FREQ=WEEKLY;COUNT=10', 'Weekly on Sunday, 10 times');
  runTest('RRULE:FREQ=WEEKLY;UNTIL=19971224T000000',
          'Weekly on Sunday, until Dec 24, 1997');
  runTest('RRULE:FREQ=WEEKLY;INTERVAL=2;WKST=SU',
          'Every 2 weeks on Sunday');
  runTest('RRULE:FREQ=WEEKLY;UNTIL=19971007T000000;WKST=SU;BYDAY=TU,TH',
          'Weekly on Tuesday, Thursday, until Oct 7, 1997');
  runTest('RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=19971224T000000;WKST=SU;\n' +
          ' BYDAY=MO,WE,FR',
          'Every 2 weeks on Monday, Wednesday, Friday, until Dec 24, 1997');
  runTest('RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=8;WKST=SU;BYDAY=TU,TH',
          'Every 2 weeks on Tuesday, Thursday, 8 times');
  runTest('RRULE:FREQ=MONTHLY;COUNT=10;BYDAY=1FR',
          'Monthly on the first Friday, 10 times');
  // first and last sunday of every second month for 20 months
  runTest('RRULE:FREQ=MONTHLY;INTERVAL=2;COUNT=10;BYDAY=1SU,-1SU', null);
  runTest('RRULE:FREQ=MONTHLY;COUNT=6;BYDAY=-2MO',
          'Monthly on Monday, 2 weeks before the end of the month, 6 times');
  runTest('RRULE:FREQ=MONTHLY;BYMONTHDAY=-3',
          'Monthly 3 days before the end of the month');
  runTest('RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=2,15',
          'Monthly on days 2, 15 of the month, 10 times');
  // first and last day of the month for 10 months
  runTest('RRULE:FREQ=MONTHLY;COUNT=10;BYMONTHDAY=1,-1', null);
  runTest(
      'RRULE:FREQ=MONTHLY;INTERVAL=18;COUNT=10;BYMONTHDAY=10,11,12,13,14,\n' +
      ' 15',
      'Every 18 months on days 10, 11, 12, 13, 14, 15 of the month, 10 times');
  runTest('RRULE:FREQ=MONTHLY;INTERVAL=2;BYDAY=TU',
          'Every 2 months every Tuesday');
  runTest('RRULE:FREQ=YEARLY;COUNT=10;BYMONTH=6,7',
          'Annually on day 22 of June, July, 10 times');
  runTest('RRULE:FREQ=YEARLY;INTERVAL=2;COUNT=10;BYMONTH=1,2,3',
          'Every 2 years on day 22 of January, February, March, 10 times');
  runTest('RRULE:FREQ=YEARLY;INTERVAL=3;COUNT=10;BYYEARDAY=1,100,200',
          'Every 3 years on days 1, 100, 200 of the year, 10 times');
  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=20;BYDAY=MO',
          'Annually on Monday of week 20 of the year');
  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=-1;BYDAY=MO,TH',
          'Annually on the last Monday, Thursday of the year');
  runTest('RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=TH',
          'Annually on every Thursday in March');
  runTest('RRULE:FREQ=YEARLY;BYDAY=TH;BYMONTH=6,7,8',
          'Annually on every Thursday in June, July, August');
  runTest('RRULE:FREQ=MONTHLY;BYDAY=FR;BYMONTHDAY=13', null);
  runTest('RRULE:FREQ=MONTHLY;BYDAY=SA;BYMONTHDAY=7,8,9,10,11,12,13', null);
  runTest('RRULE:FREQ=YEARLY;INTERVAL=4;BYMONTH=11;BYDAY=TU;BYMONTHDAY=2,3,\n' +
          ' 4,5,6,7,8', null);
  runTest('RRULE:FREQ=MONTHLY;COUNT=3;BYDAY=TU,WE,TH;BYSETPOS=3', null);
  runTest('RRULE:FREQ=MONTHLY;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-2', null);
  runTest('RRULE:FREQ=HOURLY;INTERVAL=3;UNTIL=19970902T170000', null);
  runTest('RRULE:FREQ=MINUTELY;INTERVAL=15;COUNT=6', null);
  runTest('RRULE:FREQ=MINUTELY;INTERVAL=90;COUNT=4', null);
  runTest('RRULE:FREQ=DAILY;BYHOUR=9,10,11,12,13,14,15,16;BYMINUTE=0,20,40',
          'Daily');
  runTest('RRULE:FREQ=MINUTELY;INTERVAL=20;BYHOUR=9,10,11,12,13,14,15,16',
          null);
  runTest('RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=MO',
          'Every 2 weeks on Tuesday, Sunday, 4 times');
  runTest('RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=4;BYDAY=TU,SU;WKST=SU',
          'Every 2 weeks on Tuesday, Sunday, 4 times');

  runTest('RRULE:FREQ=MONTHLY;BYDAY=-1SU',
          'Monthly on the last Sunday');

  runTest('RRULE:FREQ=MONTHLY;BYDAY=1SU,3SU',
          'Monthly on Sunday of weeks 1, 3 of the month');

  runTest('RRULE:FREQ=MONTHLY;BYDAY=2SU',
          'Monthly on the second Sunday');

  runTest('RRULE:FREQ=MONTHLY;BYDAY=3SU',
          'Monthly on the third Sunday');

  runTest('RRULE:FREQ=MONTHLY;BYDAY=4SU',
          'Monthly on the fourth Sunday');

  runTest('RRULE:FREQ=MONTHLY;BYDAY=5SU',
          'Monthly on the fifth Sunday');

  runTest('RRULE:FREQ=MONTHLY',
          'Monthly on day 22');

  runTest('RRULE:FREQ=MONTHLY;BYMONTHDAY=-1',
          'Monthly on the last day');

  runTest('RRULE:FREQ=YEARLY;BYMONTHDAY=-1',
          'Annually on the last day of September');

  runTest('RRULE:FREQ=YEARLY;BYMONTHDAY=22',
          'Annually on September 22');

  runTest('RRULE:FREQ=YEARLY;BYMONTH=1,2,3;BYMONTHDAY=1,15',
          'Annually on days 1, 15 of January, February, March');

  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=14;BYDAY=TU,TH',
          'Annually on Tuesday, Thursday of week 14 of the year');

  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=14;BYDAY=TU,14TH',
          'Annually on Tuesday, Thursday of week 14 of the year');

  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=14;BYDAY=TU,7TH',
          'Annually on Tuesday of week 14 of the year');

  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=14',
          'Annually on Sunday of week 14 of the year');

  runTest('RRULE:FREQ=YEARLY;BYYEARDAY=-1',
          'Annually on the last day of the year');

  runTest('RRULE:FREQ=YEARLY;BYYEARDAY=-2',
          'Annually 2 days before the end of the year');

  runTest('RRULE:FREQ=YEARLY;BYYEARDAY=-1,-2',
          null);

  runTest('RRULE:FREQ=YEARLY;BYYEARDAY=100,150',
          'Annually on days 100, 150 of the year');

  runTest('RRULE:FREQ=YEARLY;BYYEARDAY=100',
          'Annually on day 100 of the year');

  runTest('RRULE:FREQ=YEARLY;INTERVAL=2;BYDAY=TU',
          'Every 2 years on every Tuesday in January, February, March, '
          + 'April, May, June, July, August, September, October, November, '
          + 'December');

  runTest('RRULE:FREQ=YEARLY;BYMONTHDAY=-1;BYMONTH=10',
          'Annually on the last day of October');

  runTest('RRULE:FREQ=YEARLY;BYMONTHDAY=-10;BYMONTH=10',
          'Annually 10 days before the end of October');

  runTest('RRULE:FREQ=YEARLY;BYMONTHDAY=-1,-11;BYMONTH=10',
          null);

  runTest('RRULE:FREQ=YEARLY;BYDAY=-2SU;BYMONTH=10',
          'Annually on Sunday, 2 weeks before the end of October');

  runTest('RRULE:FREQ=YEARLY;BYDAY=-2SU,1SU;BYMONTH=10',
          null);

  runTest('RRULE:FREQ=YEARLY;BYDAY=1SU,3SU;BYMONTH=10,11,12',
          'Annually on Sunday of weeks 1, 3 in October, November, December');

  runTest('RRULE:FREQ=YEARLY;BYDAY=3SU;BYMONTH=10',
          'Annually on the third Sunday in October');

  runTest('RRULE:FREQ=YEARLY;BYDAY=4SU;BYMONTH=10',
          'Annually on the fourth Sunday in October');

  runTest('RRULE:FREQ=YEARLY;BYDAY=5SU;BYMONTH=10',
          'Annually on the fifth Sunday in October');

  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=-2',
          'Annually on Sunday, 2 weeks before the end of the year');

  runTest('RRULE:FREQ=YEARLY;BYWEEKNO=1,2,4',
          'Annually on Sunday of weeks 1, 2, 4 of the year');

  runTest('RRULE:FREQ=YEARLY;INTERVAL=2;BYWEEKNO='
          + '1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19'
          + ',20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40'
          + ',41,42,43,44,45,46,47,48,49,50,51,52,53',
          'Every 2 years on every Sunday of the year');

  runTest('RRULE:FREQ=YEARLY;COUNT=1',
          'Once');
}
