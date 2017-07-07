// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

HarImporter.HARBase = class {
  /**
   * @param {*} data
   */
  constructor(data) {
    if (typeof data !== 'object')
      throw 'First parameter is expected to be an object';
  }

  /**
   * @param {!Date} date
   * @return {!Date}
   */
  static _safeDate(date) {
    if (!Number.isNaN(date.getTime()))
      return date;
    throw 'Invalid date format.';
  }

  /**
   * @param {*} data
   * @return {number}
   */
  static _safeNumber(data) {
    var result = Number(data);
    if (!Number.isNaN(result))
      return result;
    throw 'Casting to number results in NaN';
  }

  /**
   * @param {*} data
   * @return {number|undefined}
   */
  static _optionalNumber(data) {
    return (data !== undefined) ? HarImporter.HARBase._safeNumber(data) : undefined;
  }

  /**
   * @param {*} data
   * @return {string|undefined}
   */
  static _optionalString(data) {
    return (data !== undefined) ? String(data) : undefined;
  }

  /**
   * @param {string} name
   * @return {string|undefined}
   */
  customAsString(name) {
    // HAR specification says starting with '_' is a custom property, but closure uses '_' as a private property.
    var value = /** @type {!Object} */ (this)['_' + name];
    return value !== undefined ? String(value) : undefined;
  }

  /**
   * @param {string} name
   * @return {number|undefined}
   */
  customAsNumber(name) {
    // HAR specification says starting with '_' is a custom property, but closure uses '_' as a private property.
    var value = /** @type {!Object} */ (this)['_' + name];
    if (value === undefined)
      return;
    value = Number(value);
    if (Number.isNaN(value))
      return;
    return value;
  }
};

// Using any of these classes may throw.
HarImporter.HARRoot = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.log = new HarImporter.HARLog(data.log);
  }
};

HarImporter.HARLog = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.version = String(data.version);
    this.creator = new HarImporter.HARCreator(data.creator);
    this.browser = data.browser ? new HarImporter.HARCreator(data.browser) : undefined;
    this.pages = Array.isArray(data.pages) ? data.pages.map(page => new HarImporter.HARPage(page)) : [];
    if (!Array.isArray(data.entries))
      throw 'log.entries is expected to be an array';
    this.entries = data.entries.map(entry => new HarImporter.HAREntry(entry));
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARCreator = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.version = String(data.version);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARPage = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.startedDateTime = HarImporter.HARBase._safeDate(new Date(data.startedDateTime));
    this.id = String(data.id);
    this.title = String(data.title);
    this.pageTimings = new HarImporter.HARPageTimings(data.pageTimings);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARPageTimings = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.onContentLoad = HarImporter.HARBase._safeNumber(data.onContentLoad) || undefined;
    this.onLoad = HarImporter.HARBase._safeNumber(data.onLoad) || undefined;
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HAREntry = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.pageref = HarImporter.HARBase._optionalString(data.pageref);
    this.startedDateTime = HarImporter.HARBase._safeDate(new Date(data.startedDateTime));
    if (Number.isNaN(this.startedDateTime.getTime()))
      throw 'entry.startedDateTime must be a valid date';
    this.time = HarImporter.HARBase._safeNumber(data.time);
    this.request = new HarImporter.HARRequest(data.request);
    this.response = new HarImporter.HARResponse(data.response);
    this.cache = {};  // Not yet implemented.
    this.timings = new HarImporter.HARTimings(data.timings);
    this.serverIPAddress = HarImporter.HARBase._optionalString(data.serverIPAddress);
    this.connection = HarImporter.HARBase._optionalString(data.connection);
    this.comment = HarImporter.HARBase._optionalString(data.comment);

    // Chrome specific.
    this._fromCache = HarImporter.HARBase._optionalString(data._fromCache);
  }
};

HarImporter.HARRequest = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.method = String(data.method);
    this.url = String(data.url);
    this.httpVersion = String(data.httpVersion);
    this.cookies = Array.isArray(data.cookies) ? data.cookies.map(cookie => new HarImporter.HARCookie(cookie)) : [];
    this.headers = Array.isArray(data.headers) ? data.headers.map(header => new HarImporter.HARHeader(header)) : [];
    this.queryString =
        Array.isArray(data.queryString) ? data.queryString.map(qs => new HarImporter.HARQueryString(qs)) : [];
    this.postData = data.postData ? new HarImporter.HARPostData(data.postData) : undefined;
    this.headersSize = HarImporter.HARBase._safeNumber(data.headersSize);
    this.bodySize = HarImporter.HARBase._safeNumber(data.bodySize);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARResponse = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.status = HarImporter.HARBase._safeNumber(data.status);
    this.statusText = String(data.statusText);
    this.httpVersion = String(data.httpVersion);
    this.cookies = Array.isArray(data.cookies) ? data.cookies.map(cookie => new HarImporter.HARCookie(cookie)) : [];
    this.headers = Array.isArray(data.headers) ? data.headers.map(header => new HarImporter.HARHeader(header)) : [];
    this.content = new HarImporter.HARContent(data.content);
    this.redirectURL = String(data.redirectURL);
    this.headersSize = HarImporter.HARBase._safeNumber(data.headersSize);
    this.bodySize = HarImporter.HARBase._safeNumber(data.bodySize);
    this.comment = HarImporter.HARBase._optionalString(data.comment);

    // Chrome specific.
    this._transferSize = HarImporter.HARBase._optionalNumber(data._transferSize);
    this._error = HarImporter.HARBase._optionalString(data._error);
  }
};

HarImporter.HARCookie = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = String(data.value);
    this.path = HarImporter.HARBase._optionalString(data.path);
    this.domain = HarImporter.HARBase._optionalString(data.domain);
    this.expires = data.expires ? HarImporter.HARBase._safeDate(new Date(data.expires)) : undefined;
    if (this.expires && Number.isNaN(this.expires.getTime()))
      this.expires = undefined;
    this.httpOnly = data.httpOnly !== undefined ? !!data.httpOnly : undefined;
    this.secure = data.secure !== undefined ? !!data.secure : undefined;
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARHeader = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = String(data.value);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARQueryString = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = String(data.value);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARPostData = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.mimeType = String(data.mimeType);
    this.params = Array.isArray(data.params) ? data.params.map(param => new HarImporter.HARParam(param)) : [];
    this.text = String(data.text);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARParam = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = HarImporter.HARBase._optionalString(data.value);
    this.fileName = HarImporter.HARBase._optionalString(data.fileName);
    this.contentType = HarImporter.HARBase._optionalString(data.contentType);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARContent = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.size = HarImporter.HARBase._safeNumber(data.size);
    this.compression = HarImporter.HARBase._optionalNumber(data.compression);
    this.mimeType = String(data.mimeType);
    this.text = HarImporter.HARBase._optionalString(data.text);
    this.encoding = HarImporter.HARBase._optionalString(data.encoding);
    this.comment = HarImporter.HARBase._optionalString(data.comment);
  }
};

HarImporter.HARTimings = class extends HarImporter.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.blocked = HarImporter.HARBase._optionalNumber(data.blocked);
    this.dns = HarImporter.HARBase._optionalNumber(data.dns);
    this.connect = HarImporter.HARBase._optionalNumber(data.connect);
    this.send = HarImporter.HARBase._safeNumber(data.send);
    this.wait = HarImporter.HARBase._safeNumber(data.wait);
    this.receive = HarImporter.HARBase._safeNumber(data.receive);
    this.ssl = HarImporter.HARBase._optionalNumber(data.ssl);
    this.comment = HarImporter.HARBase._optionalString(data.comment);

    // Chrome specific.
    this._blocked_queueing = HarImporter.HARBase._optionalNumber(data._blocked_queueing);
    this._blocked_proxy = HarImporter.HARBase._optionalNumber(data._blocked_proxy);
  }
};
