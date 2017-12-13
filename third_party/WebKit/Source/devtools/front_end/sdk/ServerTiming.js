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
        return {
          name: entry.name,
          duration: entry.hasOwnProperty('duration') ? entry.duration : null,
          description: entry.hasOwnProperty('description') ? entry.description : '',
        };
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
    function consumeChar(char) {
      console.assert(['"', ';', '=', ','].indexOf(char) > -1);
      var result = new RegExp(`^(?:\s*)(?:${char})(?:\s*)(.*)`, 'g').exec(valueString);
      if (!result)
        return false;

      valueString = result[1];
      return true;
    }
    function consumeToken() {
      // https://tools.ietf.org/html/rfc7230#appendix-B
      var result = /^(?:\s*)([\w!#$%&'*+\-.^`|~]+)(?:\s*)(.*)/g.exec(valueString);
      if (!result)
        return false;

      valueString = result[2];
      return result;
    }
    function consumeTokenOrQuotedString() {
      if (consumeChar('"'))
        return consumeQuotedString();


      var valueMatch = consumeToken();
      if (valueMatch)
        return valueMatch[1];
    }
    function consumeQuotedString() {
      var result = '';
      while (valueString.length) {
        if (valueString[0] === '"') {
          valueString = valueString.substring(1);
          return result;
        }
        if (valueString[0] === '\\') {
          valueString = valueString.substring(1);
          if (!valueString.length)
            return false;
        }
        result += valueString[0];
        valueString = valueString.substring(1);
      }
      return false;
    }

    var result = [];
    var nameMatch;
    while ((nameMatch = consumeToken()) !== false) {
      var entry = {name: nameMatch[1]};

      while (consumeChar(';')) {
        var paramNameMatch;
        if (!(paramNameMatch = consumeToken()) || !consumeChar('='))
          continue;

        var paramValue;
        if (!(paramValue = consumeTokenOrQuotedString()))
          continue;

        var paramName = paramNameMatch[1].toLowerCase();
        switch (paramName) {
          case 'dur':
            if (!entry.hasOwnProperty('duration')) {
              var duration = parseFloat(paramValue);
              if (!isNaN(duration))
                entry.duration = duration;
            }
            break;
          case 'desc':
            if (!entry.hasOwnProperty('description'))
              entry.description = paramValue;
            break;
        }
      }

      result.push(entry);
      if (!consumeChar(','))
        break;
    }
    return result;
  }
};
