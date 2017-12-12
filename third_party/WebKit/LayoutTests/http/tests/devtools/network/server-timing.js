// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that server-timing headers are parsed correctly.\n');

  function testServerTimingHeader(headerValue) {
    TestRunner.addResult(headerValue)
    SDK.ServerTiming.createFromHeaderValue(headerValue).forEach(function(entry, index) {
      TestRunner.addResult(index + ' ' + JSON.stringify(entry))
    })
  }

  testServerTimingHeader("");
  testServerTimingHeader("metric");
  testServerTimingHeader("metric;dur");
  testServerTimingHeader("metric;dur=123.4");
  testServerTimingHeader("metric;dur=\"123.4\"");

  testServerTimingHeader("metric;desc");
  testServerTimingHeader("metric;desc=description");
  testServerTimingHeader("metric;desc=\"description\"");

  testServerTimingHeader("metric;dur;desc");
  testServerTimingHeader("metric;dur=123.4;desc");
  testServerTimingHeader("metric;dur;desc=description");
  testServerTimingHeader("metric;dur=123.4;desc=description");
  testServerTimingHeader("metric;desc;dur");
  testServerTimingHeader("metric;desc;dur=123.4");
  testServerTimingHeader("metric;desc=description;dur");
  testServerTimingHeader("metric;desc=description;dur=123.4");

  //  special chars in name
  testServerTimingHeader("aB3!#$%&'*+-.^_`|~")

  // spaces
  testServerTimingHeader("metric ; ");
  testServerTimingHeader("metric , ");
  testServerTimingHeader("metric ; dur = 123.4 ; desc = description");
  testServerTimingHeader("metric ; desc = description ; dur = 123.4");

  // tabs
  testServerTimingHeader("metric\t;\t");
  testServerTimingHeader("metric\t,\t");
  testServerTimingHeader("metric\t;\tdur\t=\t123.4\t;\tdesc\t=\tdescription");
  testServerTimingHeader("metric\t;\tdesc\t=\tdescription\t;\tdur\t=\t123.4");

  // multiple entries
  testServerTimingHeader(
      "metric1;dur=12.3;desc=description1,metric2;dur=45.6;desc=description2,metric3;dur=78.9;desc=description3");

  testServerTimingHeader("metric1,metric2 ,metric3, metric4 , metric5");

  // quoted-strings
  testServerTimingHeader("metric;desc=\\");
  testServerTimingHeader("metric;desc=\"");
  testServerTimingHeader("metric;desc=\\\\");
  testServerTimingHeader("metric;desc=\\\"");
  testServerTimingHeader("metric;desc=\"\\");
  testServerTimingHeader("metric;desc=\"\"");
  testServerTimingHeader("metric;desc=\\\\\\");
  testServerTimingHeader("metric;desc=\\\\\"");
  testServerTimingHeader("metric;desc=\\\"\\");
  testServerTimingHeader("metric;desc=\\\"\"");
  testServerTimingHeader("metric;desc=\"\\\\");
  testServerTimingHeader("metric;desc=\"\\\"");
  testServerTimingHeader("metric;desc=\"\"\\");
  testServerTimingHeader("metric;desc=\"\"\"");
  testServerTimingHeader("metric;desc=\\\\\\\\");
  testServerTimingHeader("metric;desc=\\\\\\\"");
  testServerTimingHeader("metric;desc=\\\\\"\\");
  testServerTimingHeader("metric;desc=\\\\\"\"");
  testServerTimingHeader("metric;desc=\\\"\\\\");
  testServerTimingHeader("metric;desc=\\\"\\\"");
  testServerTimingHeader("metric;desc=\\\"\"\\");
  testServerTimingHeader("metric;desc=\\\"\"\"");
  testServerTimingHeader("metric;desc=\"\\\\\\");
  testServerTimingHeader("metric;desc=\"\\\\\"");
  testServerTimingHeader("metric;desc=\"\\\"\\");
  testServerTimingHeader("metric;desc=\"\\\"\"");
  testServerTimingHeader("metric;desc=\"\"\\\\");
  testServerTimingHeader("metric;desc=\"\"\\\"");
  testServerTimingHeader("metric;desc=\"\"\"\\");
  testServerTimingHeader("metric;desc=\"\"\"\"");

  // duplicate entry names
  testServerTimingHeader("metric;dur=12.3;desc=description1,metric;dur=45.6;desc=description2");

  // non-numeric durations
  testServerTimingHeader("metric;dur=foo");
  testServerTimingHeader("metric;dur=\"foo\"");

  // unrecognized param names
  testServerTimingHeader("metric;foo=bar;desc=description;foo=bar;dur=123.4;foo=bar");

  // duplicate param names
  testServerTimingHeader("metric;dur=123.4;dur=567.8");
  testServerTimingHeader("metric;desc=description1;desc=description2");

  // unspecified param values
  testServerTimingHeader("metric;dur;dur=123.4");
  testServerTimingHeader("metric;desc;desc=description");

  // param name case
  testServerTimingHeader("metric;DuR=123.4;DeSc=description");

  // nonsense
  testServerTimingHeader("metric=foo;dur;dur=123.4,metric2");
  testServerTimingHeader("metric\"foo;dur;dur=123.4,metric2");

  // nonsense - return zero entries
  testServerTimingHeader(" ");
  testServerTimingHeader("=");
  testServerTimingHeader(";");
  testServerTimingHeader(",");
  testServerTimingHeader("=;");
  testServerTimingHeader(";=");
  testServerTimingHeader("=,");
  testServerTimingHeader(",=");
  testServerTimingHeader(";,");
  testServerTimingHeader(",;");

  TestRunner.addResult('Tests ran to completion.\n');
  TestRunner.completeTest();
})();
