// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that server-timing headers are parsed correctly.\n');

  function testServerTimingHeader(headerValue, expectedResults) {
    TestRunner.addResult(headerValue)
    SDK.ServerTiming.createFromHeaderValue(headerValue).forEach(function(entry, index) {
      TestRunner.addResult(index + ' ' + JSON.stringify(entry))
    })
  }

  testServerTimingHeader("", []);
  testServerTimingHeader("metric");
  testServerTimingHeader("metric;duration");
  testServerTimingHeader("metric;duration=123.4");
  testServerTimingHeader("metric;duration=\"123.4\"");

  testServerTimingHeader("metric;description");
  testServerTimingHeader("metric;description=description");
  testServerTimingHeader("metric;description=\"description\"");

  testServerTimingHeader("metric;duration;description");
  testServerTimingHeader("metric;duration=123.4;description");
  testServerTimingHeader("metric;duration;description=description");
  testServerTimingHeader("metric;duration=123.4;description=description");
  testServerTimingHeader("metric;description;duration");
  testServerTimingHeader("metric;description;duration=123.4");
  testServerTimingHeader("metric;description=description;duration");
  testServerTimingHeader("metric;description=description;duration=123.4");

  //  special chars in name
  testServerTimingHeader("aB3!#$%&'*+-.^_`|~")

  // whitespace
  testServerTimingHeader("metric ; ");
  testServerTimingHeader("metric , ");
  testServerTimingHeader("metric ; duration = 123.4 ; description = description");
  testServerTimingHeader("metric ; description = description ; duration = 123.4");

  // multiple entries
  testServerTimingHeader(
      "metric1;duration=12.3;description=description1,metric2;duration=45.6;description=description2,metric3;duration=78.9;description=description3");

  testServerTimingHeader("metric1,metric2 ,metric3, metric4 , metric5");

  // quoted-strings
  testServerTimingHeader("metric;description=\\");
  testServerTimingHeader("metric;description=\"");
  testServerTimingHeader("metric;description=\\\\");
  testServerTimingHeader("metric;description=\\\"");
  testServerTimingHeader("metric;description=\"\\");
  testServerTimingHeader("metric;description=\"\"");
  testServerTimingHeader("metric;description=\\\\\\");
  testServerTimingHeader("metric;description=\\\\\"");
  testServerTimingHeader("metric;description=\\\"\\");
  testServerTimingHeader("metric;description=\\\"\"");
  testServerTimingHeader("metric;description=\"\\\\");
  testServerTimingHeader("metric;description=\"\\\"");
  testServerTimingHeader("metric;description=\"\"\\");
  testServerTimingHeader("metric;description=\"\"\"");
  testServerTimingHeader("metric;description=\\\\\\\\");
  testServerTimingHeader("metric;description=\\\\\\\"");
  testServerTimingHeader("metric;description=\\\\\"\\");
  testServerTimingHeader("metric;description=\\\\\"\"");
  testServerTimingHeader("metric;description=\\\"\\\\");
  testServerTimingHeader("metric;description=\\\"\\\"");
  testServerTimingHeader("metric;description=\\\"\"\\");
  testServerTimingHeader("metric;description=\\\"\"\"");
  testServerTimingHeader("metric;description=\"\\\\\\");
  testServerTimingHeader("metric;description=\"\\\\\"");
  testServerTimingHeader("metric;description=\"\\\"\\");
  testServerTimingHeader("metric;description=\"\\\"\"");
  testServerTimingHeader("metric;description=\"\"\\\\");
  testServerTimingHeader("metric;description=\"\"\\\"");
  testServerTimingHeader("metric;description=\"\"\"\\");
  testServerTimingHeader("metric;description=\"\"\"\"");

  // duplicate entry names
  testServerTimingHeader("metric;duration=12.3;description=description1,metric;duration=45.6;description=description2");

  // non-numeric durations
  testServerTimingHeader("metric;duration=foo");
  testServerTimingHeader("metric;duration=\"foo\"");

  // unrecognized param names
  testServerTimingHeader("metric;foo=bar;description=description;foo=bar;duration=123.4;foo=bar");

  // duplicate param names
  testServerTimingHeader("metric;duration=123.4;duration=567.8");
  testServerTimingHeader("metric;description=description1;description=description2");

  // unspecified param values
  testServerTimingHeader("metric;duration;duration=123.4");
  testServerTimingHeader("metric;description;description=description");

  // param name case
  testServerTimingHeader("metric;DuRaTiOn=123.4;DeScRiPtIoN=description");

  // nonsense
  testServerTimingHeader("metric=foo;duration;duration=123.4,metric2");
  testServerTimingHeader("metric\"foo;duration;duration=123.4,metric2");

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




/*

function createFromHeaderValue(valueString) {
  function consumeChar(char) {
    if (valueString[0] === char) {
      valueString = valueString.substring(1);
      return true;
    }
    return false;
  }
  function skipChars(chars) {
    while (valueString.length && chars.indexOf(valueString[0]) > -1)
      valueString = valueString.substring(1);
  }
  function skipWhitespace() {
    skipChars(' \t');
  }
  function consumeToken(obj) {
    while (valueString.length && isToken(valueString[0]))
      shift(obj);
    return obj.hasOwnProperty('value');
  }
  function isToken(char) {
    // https://tools.ietf.org/html/rfc7230#appendix-B
    switch (char) {
      case '!':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '*':
      case '+':
      case '-':
      case '.':
      case '^':
      case '_':
      case '`':
      case '|':
      case '~':
        return true;

      default:
        var charCode = char.charCodeAt(0);
        return (
        (charCode >= 48 && charCode <= 57) || (charCode >= 97 && charCode <= 122) ||
        (charCode >= 65 && charCode <= 90));
    }
  }
  function consumeTokenOrQuotedString(obj) {
    return consumeChar('"') ? consumeQuotedString(obj) : consumeToken(obj);
  }
  function consumeQuotedString(obj) {
    while (valueString.length) {
      if (valueString[0] === '"') {
        valueString = valueString.substring(1);
        return obj.hasOwnProperty('value');
      }
      if (valueString[0] === '\\') {
        valueString = valueString.substring(1);
        if (!valueString.length)
          return false;
      }
      shift(obj);
    }
    return false;
  }
  function shift(obj) {
    obj.value = obj.value || '';
    obj.value += valueString[0];
    valueString = valueString.substring(1);
  }

  var name, result = [];
  while (skipChars(', \t'), name = {}, consumeToken(name)) {
    skipWhitespace();
    var entry = {name: name.value};

    while (skipWhitespace(), consumeChar(';')) {
      skipWhitespace();

      var paramName;
      if (paramName = {}, consumeToken(paramName)) {
        skipWhitespace();

        if (consumeChar('=')) {
          skipWhitespace();

          var paramValue = {};
          if (consumeTokenOrQuotedString(paramValue)) {
            paramName = paramName.value.toLowerCase();
            if (!entry.hasOwnProperty(paramName)) {
              switch (paramName) {
                case 'duration':
                  var duration = parseFloat(paramValue.value);
                  if (!isNaN(duration))
                    entry.duration = duration;
                  break;
                case 'description':
                  entry.description = paramValue.value;
                  break;
              }
            }
          }
        }
      }
    }

    result.push(entry)
  }
  return result;
}

    */