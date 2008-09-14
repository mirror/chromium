var success = true;
function fail(message) {
  print("fail: " + message);
  success = false;
}

function done() {
  if (success == true) {
    print("pass");
  }
}

var t = new Date();
// The result of t.getTimezoneOffset MIGHT depend on where you are!!!
// But it must fall in the range of (-12*60 ~ 12*60)
var tm_gmoff = t.getTimezoneOffset();

if (-12*60 > tm_gmoff || tm_gmoff > 12*60)
  fail("invalid timezone offset");


// The result of this test will depend on current timezone settings.
var goodDate = new Date("Jan 01 1970 PST");
var strDate = goodDate.toUTCString();
if (strDate != "Thu, 01 Jan 1970 08:00:00 GMT") {
  fail("toUTCString failed");
}

// Test date constructor & conversion
var dateArray = new Array(
// year, mon, day,  hr, min, sec,  ms, result

// mbelshe: commenting out until we have a fix: Bug #698063
//    1200,   0,   1,   0,   0,   0,   0, "Sat Jan 01 1200 00:00:00",
    1600,  11,  31,   0,   0,   0,   0, "Sun, 31 Dec 1600 00:00:00 GMT",
    1601,  11,  31,   0,   0,   0,   0, "Mon, 31 Dec 1601 00:00:00 GMT",
       0,   0,   0,   0,   0,   0,   0, "Sun, 31 Dec 1899 00:00:00 GMT",
       0,   0,   1,   0,   0,   0,   0, "Mon, 01 Jan 1900 00:00:00 GMT",
       0,   0,   2,   0,   0,   0,   0, "Tue, 02 Jan 1900 00:00:00 GMT",
       0,   1,  29,   0,   0,   0,   0, "Thu, 01 Mar 1900 00:00:00 GMT",
       1,   1,  29,   0,   0,   0,   0, "Fri, 01 Mar 1901 00:00:00 GMT",
      71,   0,   0,   0,   0,   0,   0, "Thu, 31 Dec 1970 00:00:00 GMT",
      71,   0,  11,  22,  30,   0,   0, "Mon, 11 Jan 1971 22:30:00 GMT",
    2005,   0,   1,   0,   0,   0,   0, "Sat, 01 Jan 2005 00:00:00 GMT",
    2037,   9,  31,   0,   0,   0,   0, "Sat, 31 Oct 2037 00:00:00 GMT",
    2038,   9,  31,   0,   0,   0,   0, "Sun, 31 Oct 2038 00:00:00 GMT",
    2039,   9,  31,   0,   0,   0,   0, "Mon, 31 Oct 2039 00:00:00 GMT"
    );
for (idx=0; idx < dateArray.length; idx+=8) {
  var testDate = 
        new Date(Date.UTC(dateArray[idx], dateArray[idx+1], dateArray[idx+2],
                          dateArray[idx+3], dateArray[idx+4], dateArray[idx+5],
                          dateArray[idx+6], dateArray[idx+7]));
  var convertedDate = testDate.toUTCString();

  if (convertedDate != dateArray[idx+7])
    fail("Date " + idx/8 + ":\n\tGot     : " + convertedDate + 
         "\n\tExpected: " + dateArray[idx+7]);
}


Function.prototype.map = function (array) {
  for (var i=0; i < array.length; i++) this.apply(null, array[i]);
};

// Test invalid date parsing.
(function(string) {
  if (!isNaN((new Date(string)).getTime()))
    fail("Expected: isNaN((new Date(\"" + string + "\")).getTime())");
}).map([
  ["JJJ 05 1999"],
  [" 05 1999"], 
  ["Jan 32 1999"],
  ["Jan 50 1999"], 
  ["Aug 05 -1234"]
]);

// Test broken date parsing.
(function(string, month, date, year) {
  var d = new Date(string);
  if (d.getMonth() != month || d.getDate() != date || d.getFullYear() != year)
    fail("Broken date failed:" + string);
}).map([
  ["JanuaryX 05 1999",  0, 5, 1999],
  ["Jan -1 1999",       0, 1, 1999],
  ["Feb 29 1999",       2, 1, 1999]
]);


// Test DST
// Note: JS does not obey historical DST settings.  It uses current
//       DST settings, and applies it to past years.
// Note: Linux and Windows disagree on the boundaries of DST.
//       This makes it hard to test cross-platform
var dateArray = new Array(
// NOTE: THE FOLLOWING TEST IS TIMEZONE DEPENDENT.
//    "Jan 05 2006 00:00:00 PST", "Thu Jan 05 2006 00:00:00 GMT-0800",
// NOTE: THE FOLLOWING TEST IS TIMEZONE DEPENDENT.
//    "Mar 12 2006 00:00:00 PST", "Sun Mar 12 2006 00:00:00 GMT-0800",
// Linux does not agree with this boundary for DST, although it is 
// correct.
//    "Mar 13 2006 00:00:00 PST", "Mon Mar 13 2006 01:00:00 GMT-0700",
// NOTE: THE FOLLOWING TEST IS TIMEZONE DEPENDENT.
//    "May 01 2006 00:00:00 PST", "Mon May 01 2006 01:00:00 GMT-0700",
// Linux does not agree with this boundary for DST, although it is 
// correct.
//    "Nov 05 2006 00:00:00 PST", "Sun Nov 05 2006 01:00:00 GMT-0700",
// NOTE: THE FOLLOWING TEST IS TIMEZONE DEPENDENT.
//    "Nov 06 2006 00:00:00 PST", "Mon Nov 06 2006 00:00:00 GMT-0800"
);
for (idx=0; idx < dateArray.length; idx+=2) {
  var badDate = new Date(dateArray[idx]);
  var convertedDate = badDate.toString();
  convertedDate = convertedDate.substring(0,dateArray[idx+1].length);
  if (convertedDate != dateArray[idx+1])
    fail("DateDST " + idx/2 + ":\n\tGot     : " + convertedDate + 
         "\n\tExpected: " + dateArray[idx+1]);
}

done();
