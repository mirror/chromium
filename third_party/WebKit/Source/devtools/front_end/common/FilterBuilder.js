// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Common.FilterBuilder = class {
  /**
   * @param {string} query
   * @param {!Array<string>} keys
   * @return {{text: !Array<string>, filters: !Array<!Common.FilterBuilder.Filter>}}
   */
  static parseQuery(query, keys) {
    var filters = [];
    var text = [];
    var parts = query.split(/\s+/);
    for (var i = 0; i < parts.length; ++i) {
      var part = parts[i];
      if (!part)
        continue;
      var colonIndex = part.indexOf(':');
      if (colonIndex === -1) {
        text.push(part);
        continue;
      }
      var key = part.substring(0, colonIndex);
      var negative = key.startsWith('-');
      if (negative)
        key = key.substring(1);
      if (keys.indexOf(key) === -1) {
        text.push(part);
        continue;
      }
      var value = part.substring(colonIndex + 1);
      filters.push({type: key, data: value, negative: negative});
    }
    return {text: text, filters: filters};
  }
};

/** @typedef {{type: string, data: string, negative: boolean}} */
Common.FilterBuilder.Filter;
