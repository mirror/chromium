// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @unrestricted
 */
SDK.ServerTiming = class {
  /**
   * @param {string} metric
   * @param {?number} value
   * @param {?string} description
   */
  constructor(metric, value, description) {
    this.metric = metric;
    this.value = value;
    this.description = description;
  }

  /**
   * @param {!Array<!SDK.NetworkRequest.NameValue>} headers
   * @return {?Array<!SDK.ServerTiming>}
   */
  static parseHeaders(headers) {
    var rawServerTimingHeaders = headers.filter(item => item.name.toLowerCase() === 'server-timing');
    if (!rawServerTimingHeaders.length)
      return null;

    var serverTimings = rawServerTimingHeaders.reduce((memo, header) => {
      var timing = this.createFromHeaderValue(header.value);
      memo.pushAll(timing.map(function(entry) {
        return new SDK.ServerTiming(
            entry.name, entry.hasOwnProperty('duration') ? entry.duration : null,
            entry.hasOwnProperty('description') ? entry.description : '');
      }));
      return memo;
    }, []);
    serverTimings.sort((a, b) => a.metric.toLowerCase().compareTo(b.metric.toLowerCase()));
    return serverTimings;
  }

  /**
   * @param {?string} valueString
   * @return {?Array<!Object>}
   */
  static createFromHeaderValue(valueString) {
    function trimLeadingWhiteSpace() {
      valueString = valueString.replace(/^\s*/, '');
    }
    function consumeDelimiter(char) {
      console.assert(char.length === 1);
      trimLeadingWhiteSpace();
      if (valueString.charAt(0) !== char)
        return false;

      valueString = valueString.substring(1);
      trimLeadingWhiteSpace();
      return true;
    }
    function consumeToken() {
      // https://tools.ietf.org/html/rfc7230#appendix-B
      var result = /^(?:\s*)([\w!#$%&'*+\-.^`|~]+)(?:\s*)(.*)/.exec(valueString);
      if (!result)
        return false;

      valueString = result[2];
      return result;
    }
    function consumeTokenOrQuotedString() {
      if (valueString.charAt(0) === '"')
        return consumeQuotedString();

      var valueMatch = consumeToken();
      if (valueMatch)
        return valueMatch[1];
    }
    function consumeQuotedString() {
      console.assert(valueString.charAt(0) === '"');
      valueString = valueString.substring(1);  // remove leading DQUOTE

      var value = '';
      while (valueString.length) {
        // split into two parts:
        //  -everything before the first " or \
        //  -everything else
        var result = /^([^"\\]*)(.*)/.exec(valueString);
        value += result[1];
        if (result[2].charAt(0) === '"') {
          // we have found our closing "
          valueString = result[2].substring(1);  // strip off everything after the closing "
          return value;                          // we are done here
        }

        console.assert(result[2].charAt(0) === '\\');
        // special rules for \ found in quoted-string (https://tools.ietf.org/html/rfc7230#section-3.2.6)
        value += result[2].charAt(1);          // grab the character AFTER the \ (if there was one)
        valueString = result[2].substring(2);  // strip off \ and next character
      }

      return null;  // not a valid quoted-string
    }

    var result = [];
    var nameMatch;
    while ((nameMatch = consumeToken()) !== false) {
      var entry = {name: nameMatch[1]};

      while (consumeDelimiter(';')) {
        var paramNameMatch;
        if (!(paramNameMatch = consumeToken()))
          continue;

        var paramName = paramNameMatch[1].toLowerCase();
        var parseParameter = this.getParserForParameter(paramName);
        if (!consumeDelimiter('=')) {
          if (parseParameter)
            this.showNoValueWarning(paramName);  // only show this warning if it is a parameter name we recognize
          continue;
        }

        // always parse the value, even if we don't recognize the parameter name
        var paramValue = consumeTokenOrQuotedString();
        if (!parseParameter) {
          this.showWarning(`Unrecognized parameter \"${paramName}\".`);
          continue;
        }
        if (paramValue === null) {
          this.showNoValueWarning(paramName);
          continue;
        }

        parseParameter.call(this, entry, paramValue);
      }

      result.push(entry);
      if (!consumeDelimiter(','))
        break;
    }

    if (valueString.length)
      this.showWarning(`Extraneous trailing characters.`);
    return result;
  }

  static getParserForParameter(paramName) {
    switch (paramName) {
      case 'dur':
        return function(entry, paramValue) {
          if (!entry.hasOwnProperty('duration')) {
            var duration = parseFloat(paramValue);
            if (isNaN(duration))
              this.showWarning(`Unable to parse \"${paramName}\" value \"${paramValue}\".`);
            else
              entry.duration = duration;
          }
        };

      case 'desc':
        return function(entry, paramValue) {
          if (!entry.hasOwnProperty('description'))
            entry.description = paramValue;
        };
    }
  }

  static showNoValueWarning(paramName) {
    this.showWarning(`No value found for parameter \"${paramName}\".`);
  }

  static showWarning(msg) {
    Common.console.warn(`ServerTiming: ${msg}`);
  }
};
