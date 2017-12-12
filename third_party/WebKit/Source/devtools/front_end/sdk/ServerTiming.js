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
      skipWhitespace();
      var found = false;
      if (valueString[0] === char) {
        valueString = valueString.substring(1);
        found = true;
      }
      skipWhitespace();
      return found;
    }
    function removeChars(re) {
      valueString = valueString.replace(re, '');
    }
    function skipWhitespace() {
      removeChars(/^( |\t)*/g);
    }
    function consumeToken(obj) {
      skipWhitespace();
      while (valueString.length && isToken(valueString[0]))
        shift(obj);
      skipWhitespace();
      return obj.hasOwnProperty('value');
    }
    function isToken(char) {
      // https://tools.ietf.org/html/rfc7230#appendix-B
      console.assert(char.length === 1);
      return char.match(/[A-Za-z0-9!#$%&'*+\-.^_`|~]/);
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
    function leadingToken() {
      removeChars(/^( |,|\t)*/g);

      name = {};
      return consumeToken(name);
    }

    var name, result = [];

    while (leadingToken()) {
      var entry = {name: name.value};

      while (consumeChar(';')) {
        var paramName = {};
        if (!consumeToken(paramName) || !consumeChar('='))
          continue;

        var paramValue = {};
        if (!consumeTokenOrQuotedString(paramValue))
          continue;

        paramName = paramName.value.toLowerCase();
        switch (paramName) {
          case 'dur':
            if (!entry.hasOwnProperty('duration')) {
              var duration = parseFloat(paramValue.value);
              if (!isNaN(duration))
                entry.duration = duration;
            }
            break;
          case 'desc':
            if (!entry.hasOwnProperty('description'))
              entry.description = paramValue.value;
            break;
        }
      }

      result.push(entry);
    }
    return result;
  }
};
