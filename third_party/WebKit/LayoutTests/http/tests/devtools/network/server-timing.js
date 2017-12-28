// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that server-timing headers are parsed correctly.\n');

  function testServerTimingHeader(headerValue, expectedResults) {
    var actualResults = SDK.ServerTiming.createFromHeaderValue(headerValue)
    if (JSON.stringify(actualResults) !== JSON.stringify(expectedResults)) {
      TestRunner.addResult('Test failure header=' + headerValue)
      TestRunner.addResult('  expected=' + JSON.stringify(expectedResults))
      TestRunner.addResult('    actual=' + JSON.stringify(actualResults))
    }
  }

  testServerTimingHeader("", []);
  testServerTimingHeader("metric", [{"name":"metric"}]);
  testServerTimingHeader("metric;dur", [{"name":"metric","dur":0}]);
  testServerTimingHeader("metric;dur=123.4", [{"name":"metric","dur":123.4}]);
  testServerTimingHeader("metric;dur=\"123.4\"", [{"name":"metric","dur":123.4}]);

  testServerTimingHeader("metric;desc", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=description", [{"name":"metric","desc":"description"}]);
  testServerTimingHeader("metric;desc=\"description\"", [{"name":"metric","desc":"description"}]);

  testServerTimingHeader("metric;dur;desc", [{"name":"metric","dur":0,"desc":""}]);
  testServerTimingHeader("metric;dur=123.4;desc", [{"name":"metric","dur":123.4,"desc":""}]);
  testServerTimingHeader("metric;dur;desc=description", [{"name":"metric","dur":0,"desc":"description"}]);
  testServerTimingHeader("metric;dur=123.4;desc=description", [{"name":"metric","dur":123.4,"desc":"description"}]);
  testServerTimingHeader("metric;desc;dur", [{"name":"metric","desc":"","dur":0}]);
  testServerTimingHeader("metric;desc;dur=123.4", [{"name":"metric","desc":"","dur":123.4}]);
  testServerTimingHeader("metric;desc=description;dur", [{"name":"metric","desc":"description","dur":0}]);
  testServerTimingHeader("metric;desc=description;dur=123.4", [{"name":"metric","desc":"description","dur":123.4}]);

  //  special chars in name
  testServerTimingHeader("aB3!#$%&'*+-.^_`|~", [{"name":"aB3!#$%&'*+-.^_`|~"}])

  // spaces
  testServerTimingHeader("metric ; ", [{"name":"metric"}]);
  testServerTimingHeader("metric , ", [{"name":"metric"}]);
  testServerTimingHeader("metric ; dur = 123.4 ; desc = description", [{"name":"metric","dur":123.4,"desc":"description"}]);
  testServerTimingHeader("metric ; desc = description ; dur = 123.4", [{"name":"metric","desc":"description","dur":123.4}]);
  testServerTimingHeader("metric;desc = \"description\"", [{"name":"metric","desc":"description"}]);

  // tabs
  testServerTimingHeader("metric\t;\t", [{"name":"metric"}]);
  testServerTimingHeader("metric\t,\t", [{"name":"metric"}]);
  testServerTimingHeader("metric\t;\tdur\t=\t123.4\t;\tdesc\t=\tdescription", [{"name":"metric","dur":123.4,"desc":"description"}]);
  testServerTimingHeader("metric\t;\tdesc\t=\tdescription\t;\tdur\t=\t123.4", [{"name":"metric","desc":"description","dur":123.4}]);
  testServerTimingHeader("metric;desc\t=\t\"description\"", [{"name":"metric","desc":"description"}]);

  // multiple entries
  testServerTimingHeader("metric1;dur=12.3;desc=description1,metric2;dur=45.6;desc=description2,metric3;dur=78.9;desc=description3", [{"name":"metric1","dur":12.3,"desc":"description1"}, {"name":"metric2","dur":45.6,"desc":"description2"}, {"name":"metric3","dur":78.9,"desc":"description3"}]);

  testServerTimingHeader("metric1,metric2 ,metric3, metric4 , metric5", [{"name":"metric1"}, {"name":"metric2"}, {"name":"metric3"}, {"name":"metric4"}, {"name":"metric5"}]);

   // quoted-strings - happy path
  testServerTimingHeader("metric;desc=\"description\"", [{"name":"metric","desc":"description"}]);
  testServerTimingHeader("metric;desc=\"\t description \t\"", [{"name":"metric","desc":"\t description \t"}]);
  testServerTimingHeader("metric;desc=\"descr\\\"iption\"", [{"name":"metric","desc":"descr\"iption"}]);

  // quoted-strings - others
  testServerTimingHeader("metric;desc=\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\\\"", [{"name":"metric","desc":"\\"}]);
  testServerTimingHeader("metric;desc=\"\\\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\"\"", [{"name":"metric","desc":"\""}]);
  testServerTimingHeader("metric;desc=\"\"\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\"\"", [{"name":"metric","desc":""}]);

   // duplicate entry names
  testServerTimingHeader("metric;dur=12.3;desc=description1,metric;dur=45.6;desc=description2", [{"name":"metric","dur":12.3,"desc":"description1"}, {"name":"metric","dur":45.6,"desc":"description2"}]);

  // non-numeric durations
  testServerTimingHeader("metric;dur=foo", [{"name":"metric","dur":0}]);
  testServerTimingHeader("metric;dur=\"foo\"", [{"name":"metric","dur":0}]);

  // unrecognized param names
  testServerTimingHeader("metric;foo=bar;desc=description;foo=bar;dur=123.4;foo=bar", [{"name":"metric","desc":"description","dur":123.4}]);

  // duplicate param names
  top.cav = true;
  testServerTimingHeader("metric;dur=123.4;dur=567.8", [{"name":"metric","dur":123.4}]);
  top.cav = false;
  testServerTimingHeader("metric;dur=foo;dur=567.8", [{"name":"metric","dur":0}]);
  testServerTimingHeader("metric;desc=description1;desc=description2", [{"name":"metric","desc":"description1"}]);

  // incomplete params
  testServerTimingHeader("metric;dur=;dur=567.8;desc=description", [{"name":"metric","dur":0,"desc":"description"}]);
  testServerTimingHeader("metric;dur;dur=123.4;desc=description", [{"name":"metric","dur":0,"desc":"description"}]);
  testServerTimingHeader("metric;desc=;desc=description;dur=123.4", [{"name":"metric","desc":"","dur":123.4}]);
  testServerTimingHeader("metric;desc;desc=description;dur=123.4", [{"name":"metric","desc":"","dur":123.4}]);

  // invalid params
  testServerTimingHeader("metric;dur=123.4 567.8", [{"name":"metric","dur":123.4}]);
  testServerTimingHeader("metric;desc=description1 description2", [{"name":"metric","desc":"description1"}]);

  // param name case sensitivity
  testServerTimingHeader("metric;DuR=123.4;DeSc=description", [{"name":"metric","dur":123.4,"desc":"description"}]);

  // nonsense
  testServerTimingHeader("metric=foo;dur;dur=123.4,metric2", [{"name":"metric"}]);
  testServerTimingHeader("metric\"foo;dur;dur=123.4,metric2", [{"name":"metric"}]);

  // nonsense - return zero entries
  testServerTimingHeader(" ", []);
  testServerTimingHeader("=", []);
  testServerTimingHeader(";", []);
  testServerTimingHeader(",", []);
  testServerTimingHeader("=;", []);
  testServerTimingHeader(";=", []);
  testServerTimingHeader("=,", []);
  testServerTimingHeader(",=", []);
  testServerTimingHeader(";,", []);
  testServerTimingHeader(",;", []);

  TestRunner.addResult('Tests ran to completion.\n');
  TestRunner.completeTest();
})();
