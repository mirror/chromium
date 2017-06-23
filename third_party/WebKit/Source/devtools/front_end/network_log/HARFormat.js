// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

NetworkLog.HARBase = class {
  /**
   * @param {*} data
   */
  constructor(data) {
    if (typeof data !== 'object')
      throw 'first parameter is expected to be an object';
  }

  /**
   * @param {*} data
   * @return {number}
   */
  static safeNumber(data) {
    var result = Number(data);
    if (!Number.isNaN(result))
      return result;
    throw 'Casting to number results in NaN';
  }

  /**
   * @param {*} data
   * @return {number|undefined}
   */
  static optionalNumber(data) {
    return (data !== undefined) ? NetworkLog.HARBase.safeNumber(data) : undefined;
  }

  /**
   * @param {*} data
   * @return {string|undefined}
   */
  static optionalString(data) {
    return (data !== undefined) ? String(data) : undefined;
  }
};

// Using any of these classes may throw.
NetworkLog.HARRoot = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.log = new NetworkLog.HARLog(data.log);
  }
};

NetworkLog.HARLog = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.version = String(data.version);
    this.creator = new NetworkLog.HARCreator(data.creator);
    this.browser = data.browser ? new NetworkLog.HARCreator(data.browser) : undefined;
    this.pages = Array.isArray(data.pages) ? data.pages.map(page => new NetworkLog.HARPage(page)) : [];
    if (!Array.isArray(data.entries))
      throw 'log.entries is expected to be an array';
    this.entries = data.entries.map(entry => new NetworkLog.HAREntry(entry));
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARCreator = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.version = String(data.version);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARPage = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.startedDateTime = new Date(data.startedDateTime);
    this.id = String(data.id);
    this.title = String(data.title);
    this.pageTimings = new NetworkLog.HARPageTimings(data.pageTimings);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARPageTimings = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.onContentLoad = NetworkLog.HARBase.safeNumber(data.onContentLoad) || undefined;
    this.onLoad = NetworkLog.HARBase.safeNumber(data.onLoad) || undefined;
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HAREntry = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.pageref = NetworkLog.HARBase.optionalString(data.pageref);
    this.startedDateTime = new Date(data.startedDateTime);
    if (Number.isNaN(this.startedDateTime.getTime()))
      throw 'entry.startedDateTime must be a valid date';
    this.time = NetworkLog.HARBase.safeNumber(data.time);
    this.request = new NetworkLog.HARRequest(data.request);
    this.response = new NetworkLog.HARResponse(data.response);
    this.cache = {};  // Not yet implemented.
    this.timings = new NetworkLog.HARTimings(data.timings);
    this.serverIPAddress = NetworkLog.HARBase.optionalString(data.serverIPAddress);
    this.connection = NetworkLog.HARBase.optionalString(data.connection);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARRequest = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.method = String(data.method);
    this.url = String(data.url);
    this.httpVersion = String(data.httpVersion);
    this.cookies = Array.isArray(data.cookies) ? data.cookies.map(cookie => new NetworkLog.HARCookie(cookie)) : [];
    this.headers = Array.isArray(data.headers) ? data.headers.map(header => new NetworkLog.HARHeader(header)) : [];
    this.queryString =
        Array.isArray(data.queryString) ? data.queryString.map(qs => new NetworkLog.HARQueryString(qs)) : [];
    this.postData = data.postData ? new NetworkLog.HARPostData(data.postData) : undefined;
    this.headersSize = NetworkLog.HARBase.safeNumber(data.headersSize);
    this.bodySize = NetworkLog.HARBase.safeNumber(data.bodySize);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);

    // Chrome specific.
    this._transferSize = NetworkLog.HARBase.optionalNumber(data._transferSize);
    this._error = NetworkLog.HARBase.optionalString(data._error);
  }
};

NetworkLog.HARResponse = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.status = NetworkLog.HARBase.safeNumber(data.status);
    this.statusText = String(data.statusText);
    this.httpVersion = String(data.httpVersion);
    this.cookies = Array.isArray(data.cookies) ? data.cookies.map(cookie => new NetworkLog.HARCookie(cookie)) : [];
    this.headers = Array.isArray(data.headers) ? data.headers.map(header => new NetworkLog.HARHeader(header)) : [];
    this.content = new NetworkLog.HARContent(data.content);
    this.redirectURL = String(data.redirectURL);
    this.headersSize = NetworkLog.HARBase.safeNumber(data.headersSize);
    this.bodySize = NetworkLog.HARBase.safeNumber(data.bodySize);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARCookie = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = String(data.value);
    this.path = NetworkLog.HARBase.optionalString(data.path);
    this.domain = NetworkLog.HARBase.optionalString(data.domain);
    this.expires = data.expires ? new Date(data.expires) : undefined;
    if (this.expires && Number.isNaN(this.expires.getTime()))
      this.expires = undefined;
    this.httpOnly = data.httpOnly !== undefined ? !!data.httpOnly : undefined;
    this.secure = data.secure !== undefined ? !!data.secure : undefined;
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARHeader = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = String(data.value);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARQueryString = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = String(data.value);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARPostData = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.mimeType = String(data.mimeType);
    this.params = Array.isArray(data.params) ? data.params.map(param => new NetworkLog.HARParam(param)) : [];
    this.text = String(data.text);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARParam = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data.name);
    this.value = NetworkLog.HARBase.optionalString(data.value);
    this.fileName = NetworkLog.HARBase.optionalString(data.fileName);
    this.contentType = NetworkLog.HARBase.optionalString(data.contentType);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARContent = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.size = NetworkLog.HARBase.safeNumber(data.size);
    this.compression = NetworkLog.HARBase.optionalNumber(data.compression);
    this.mimeType = String(data.mimeType);
    this.text = NetworkLog.HARBase.optionalString(data.text);
    this.encoding = NetworkLog.HARBase.optionalString(data.encoding);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};

NetworkLog.HARTimings = class extends NetworkLog.HARBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.blocked = NetworkLog.HARBase.optionalNumber(data.blocked);
    this.dns = NetworkLog.HARBase.optionalNumber(data.dns);
    this.connect = NetworkLog.HARBase.optionalNumber(data.connect);
    this.send = NetworkLog.HARBase.safeNumber(data.send);
    this.wait = NetworkLog.HARBase.safeNumber(data.wait);
    this.receive = NetworkLog.HARBase.safeNumber(data.receive);
    this.ssl = NetworkLog.HARBase.optionalNumber(data.ssl);
    this.comment = NetworkLog.HARBase.optionalString(data.comment);
  }
};
