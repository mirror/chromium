// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

HarImporter.HarBase = class {
  /**
   * @param {*} data
   */
  constructor(data) {
    if (!data || typeof data !== 'object')
      throw 'First parameter is expected to be an object';
  }

  /**
   * @param {*} data
   * @return {!Date}
   */
  static _safeDate(data) {
    var date = new Date(data);
    if (!Number.isNaN(date.getTime()))
      return date;
    throw 'Invalid date format';
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
    return data !== undefined ? HarImporter.HarBase._safeNumber(data) : undefined;
  }

  /**
   * @param {*} data
   * @return {string|undefined}
   */
  static _optionalString(data) {
    return data !== undefined ? String(data) : undefined;
  }

  /**
   * @param {string} name
   * @return {string|undefined}
   */
  customAsString(name) {
    // Har specification says starting with '_' is a custom property, but closure uses '_' as a private property.
    var value = /** @type {!Object} */ (this)['_' + name];
    return value !== undefined ? String(value) : undefined;
  }

  /**
   * @param {string} name
   * @return {number|undefined}
   */
  customAsNumber(name) {
    // Har specification says starting with '_' is a custom property, but closure uses '_' as a private property.
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
HarImporter.HarRoot = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.log = new HarImporter.HarLog(data['log']);
  }
};

HarImporter.HarLog = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.version = String(data['version']);
    this.creator = new HarImporter.HarCreator(data['creator']);
    this.browser = data['browser'] ? new HarImporter.HarCreator(data['browser']) : undefined;
    this.pages = Array.isArray(data['pages']) ? data['pages'].map(page => new HarImporter.HarPage(page)) : [];
    if (!Array.isArray(data['entries']))
      throw 'log.entries is expected to be an array';
    this.entries = data['entries'].map(entry => new HarImporter.HarEntry(entry));
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarCreator = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data['name']);
    this.version = String(data['version']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarPage = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.startedDateTime = HarImporter.HarBase._safeDate(data['startedDateTime']);
    this.id = String(data['id']);
    this.title = String(data['title']);
    this.pageTimings = new HarImporter.HarPageTimings(data['pageTimings']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarPageTimings = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.onContentLoad = HarImporter.HarBase._safeNumber(data['onContentLoad']) || undefined;
    this.onLoad = HarImporter.HarBase._safeNumber(data['onLoad']) || undefined;
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarEntry = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.pageref = HarImporter.HarBase._optionalString(data['pageref']);
    this.startedDateTime = HarImporter.HarBase._safeDate(data['startedDateTime']);
    this.time = HarImporter.HarBase._safeNumber(data['time']);
    this.request = new HarImporter.HarRequest(data['request']);
    this.response = new HarImporter.HarResponse(data['response']);
    this.cache = {};  // Not yet implemented.
    this.timings = new HarImporter.HarTimings(data['timings']);
    this.serverIPAddress = HarImporter.HarBase._optionalString(data['serverIPAddress']);
    this.connection = HarImporter.HarBase._optionalString(data['connection']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);

    // Chrome specific.
    this._fromCache = HarImporter.HarBase._optionalString(data['_fromCache']);
  }
};

HarImporter.HarRequest = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.method = String(data['method']);
    this.url = String(data['url']);
    this.httpVersion = String(data['httpVersion']);
    this.cookies =
        Array.isArray(data['cookies']) ? data['cookies'].map(cookie => new HarImporter.HarCookie(cookie)) : [];
    this.headers =
        Array.isArray(data['headers']) ? data['headers'].map(header => new HarImporter.HarHeader(header)) : [];
    this.queryString =
        Array.isArray(data['queryString']) ? data['queryString'].map(qs => new HarImporter.HarQueryString(qs)) : [];
    this.postData = data['postData'] ? new HarImporter.HarPostData(data['postData']) : undefined;
    this.headersSize = HarImporter.HarBase._safeNumber(data['headersSize']);
    this.bodySize = HarImporter.HarBase._safeNumber(data['bodySize']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarResponse = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.status = HarImporter.HarBase._safeNumber(data['status']);
    this.statusText = String(data['statusText']);
    this.httpVersion = String(data['httpVersion']);
    this.cookies =
        Array.isArray(data['cookies']) ? data['cookies'].map(cookie => new HarImporter.HarCookie(cookie)) : [];
    this.headers =
        Array.isArray(data['headers']) ? data['headers'].map(header => new HarImporter.HarHeader(header)) : [];
    this.content = new HarImporter.HarContent(data['content']);
    this.redirectURL = String(data['redirectURL']);
    this.headersSize = HarImporter.HarBase._safeNumber(data['headersSize']);
    this.bodySize = HarImporter.HarBase._safeNumber(data['bodySize']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);

    // Chrome specific.
    this._transferSize = HarImporter.HarBase._optionalNumber(data['_transferSize']);
    this._error = HarImporter.HarBase._optionalString(data['_error']);
  }
};

HarImporter.HarCookie = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data['name']);
    this.value = String(data['value']);
    this.path = HarImporter.HarBase._optionalString(data['path']);
    this.domain = HarImporter.HarBase._optionalString(data['domain']);
    this.expires = data['expires'] ? HarImporter.HarBase._safeDate(data['expires']) : undefined;
    this.httpOnly = data['httpOnly'] !== undefined ? !!data['httpOnly'] : undefined;
    this.secure = data['secure'] !== undefined ? !!data['secure'] : undefined;
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarHeader = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data['name']);
    this.value = String(data['value']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarQueryString = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data['name']);
    this.value = String(data['value']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarPostData = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.mimeType = String(data['mimeType']);
    this.params = Array.isArray(data['params']) ? data['params'].map(param => new HarImporter.HarParam(param)) : [];
    this.text = String(data['text']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarParam = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.name = String(data['name']);
    this.value = HarImporter.HarBase._optionalString(data['value']);
    this.fileName = HarImporter.HarBase._optionalString(data['fileName']);
    this.contentType = HarImporter.HarBase._optionalString(data['contentType']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarContent = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.size = HarImporter.HarBase._safeNumber(data['size']);
    this.compression = HarImporter.HarBase._optionalNumber(data['compression']);
    this.mimeType = String(data['mimeType']);
    this.text = HarImporter.HarBase._optionalString(data['text']);
    this.encoding = HarImporter.HarBase._optionalString(data['encoding']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);
  }
};

HarImporter.HarTimings = class extends HarImporter.HarBase {
  /**
   * @param {*} data
   */
  constructor(data) {
    super(data);
    this.blocked = HarImporter.HarBase._optionalNumber(data['blocked']);
    this.dns = HarImporter.HarBase._optionalNumber(data['dns']);
    this.connect = HarImporter.HarBase._optionalNumber(data['connect']);
    this.send = HarImporter.HarBase._safeNumber(data['send']);
    this.wait = HarImporter.HarBase._safeNumber(data['wait']);
    this.receive = HarImporter.HarBase._safeNumber(data['receive']);
    this.ssl = HarImporter.HarBase._optionalNumber(data['ssl']);
    this.comment = HarImporter.HarBase._optionalString(data['comment']);

    // Chrome specific.
    this._blocked_queueing = HarImporter.HarBase._optionalNumber(data['_blocked_queueing']);
    this._blocked_proxy = HarImporter.HarBase._optionalNumber(data['_blocked_proxy']);
  }
};
