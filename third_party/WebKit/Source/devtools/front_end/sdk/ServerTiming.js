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
      Array.prototype.push.apply(memo, timing.map(function(entry) {
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
      console.assert(char.length === 1);
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
      console.assert(char.length === 1);
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
          }
        }
      }

      result.push(entry);
    }
    return result;
  }
};
