// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

HarImporter.Importer = class {
  /**
   * @param {!HarImporter.HARLog} log
   * @return {!Array<!SDK.NetworkRequest>}
   */
  static requestsFromHARLog(log) {
    /** @type {!Map<string, !HarImporter.HARPage>} */
    var pages = new Map();
    for (var page of log.pages)
      pages.set(page.id, page);

    log.entries.sort((a, b) => a.startedDateTime - b.startedDateTime);

    /** @type {!Map<string, !NetworkLog.PageLoad>} */
    var pageLoads = new Map();
    /** @type {!Array<!SDK.NetworkRequest>} */
    var requests = [];
    for (var i = 0; i < log.entries.length; i++) {
      var entry = log.entries[i];
      var pageLoad = pageLoads.get(entry.pageref) || null;
      var documentURL = pageLoad ? pageLoad.mainRequest.url() : entry.request.url;
      var request = new SDK.NetworkRequest('har-' + i, entry.request.url, documentURL, '', '', null);
      var page = pages.get(entry.pageref);
      if (!pageLoad && page) {
        pageLoad = HarImporter.Importer._buildPageLoad(page, request);
        pageLoads.set(entry.pageref, pageLoad);
      }
      HarImporter.Importer._fillRequestFromHAREntry(request, entry, pageLoad);
      if (pageLoad)
        pageLoad.registerRequest(request);
      requests.push(request);
    }
    return requests;
  }

  /**
   * @param {!HarImporter.HARPage} page
   * @param {!SDK.NetworkRequest} mainRequest
   * @return {!NetworkLog.PageLoad}
   */
  static _buildPageLoad(page, mainRequest) {
    var pageLoad = new NetworkLog.PageLoad(mainRequest);
    pageLoad.startTime = page.startedDateTime;
    pageLoad.contentLoadTime = page.pageTimings.onContentLoad * 1000;
    pageLoad.loadTime = page.pageTimings.onLoad * 1000;
    return pageLoad;
  }

  /**
   * @param {!SDK.NetworkRequest} request
   * @param {!HarImporter.HAREntry} entry
   * @param {?NetworkLog.PageLoad} pageLoad
   */
  static _fillRequestFromHAREntry(request, entry, pageLoad) {
    // Request data.
    if (entry.request.postData)
      request.requestFormData = entry.request.postData.text;
    request.connectionId = entry.connection || '';
    request.requestMethod = entry.request.method;
    request.setRequestHeaders(entry.request.headers);
    request.setRequestHeadersText(entry.request.headers.map(header => header.name + ': ' + header.value).join('\n'));

    // Response data.
    if (entry.response.content.mimeType && entry.response.content.mimeType !== 'x-unknown')
      request.mimeType = entry.response.content.mimeType;
    request.responseHeaders = entry.response.headers;
    request.responseHeadersText = entry.response.headers.map(header => header.name + ': ' + header.value).join('\n');
    request.statusCode = entry.response.status;
    request.statusText = entry.response.statusText;
    var protocol = entry.response.httpVersion.toLowerCase();
    if (protocol === 'http/2.0')
      protocol = 'h2';
    request.protocol = protocol.replace(/^http\/2\.0?\+quic/, 'http/2+quic');

    // Timing data.
    var baseTime = entry.startedDateTime.getTime() / 1000;
    request.setIssueTime(baseTime, baseTime);

    // Content data.
    var contentSize = entry.response.content.size > 0 ? entry.response.content.size : 0;
    var headersSize = entry.response.headersSize > 0 ? entry.response.headersSize : 0;
    var bodySize = entry.response.bodySize > 0 ? entry.response.bodySize : 0;
    request.resourceSize = contentSize || (headersSize + bodySize);
    var transferSize = entry.response.customAsNumber('transferSize');
    if (transferSize === undefined)
      transferSize = entry.response.headersSize + entry.response.bodySize;
    request.setTransferSize(transferSize >= 0 ? transferSize : 0);

    var fromCache = entry.customAsString('fromCache');
    if (fromCache === 'memory')
      request.setFromMemoryCache();
    if (fromCache === 'disk')
      request.setFromDiskCache();

    var contentData = {error: null, content: null, encoded: entry.response.content.encoding === 'base64'};
    if (entry.response.content.text !== undefined)
      contentData.content = entry.response.content.text;
    request.setContentData(contentData);

    // Timing data.
    HarImporter.Importer._setupTiming(request, baseTime, entry.time, entry.timings);

    // Meta data.
    request.setRemoteAddress(entry.serverIPAddress || '', 80);  // HAR does not support port numbers.
    var resourceType = (pageLoad && pageLoad.mainRequest === request) ? Common.resourceTypes.Document : null;
    if (!resourceType)
      resourceType = Common.ResourceType.fromMimeType(entry.response.content.mimeType);
    if (!resourceType)
      resourceType = Common.ResourceType.fromURL(entry.request.url) || Common.resourceTypes.Other;
    request.setResourceType(resourceType);

    request.finished = true;
  }

  /**
   * @param {!SDK.NetworkRequest} request
   * @param {number} baseTime
   * @param {number} entryTotalDuration
   * @param {!HarImporter.HARTimings} timings
   */
  static _setupTiming(request, baseTime, entryTotalDuration, timings) {
    /**
     * @param {number|undefined} timing
     * @return {number}
     */
    function accumulateTime(timing) {
      if (timing === undefined || timing < 0)
        return -1;
      lastEntry += timing;
      return lastEntry;
    }
    var lastEntry = timings.blocked >= 0 ? timings.blocked : 0;

    var proxy = timings.customAsNumber('blocked_proxy') || -1;
    var queueing = timings.customAsNumber('blocked_queueing') || -1;

    if (timings.connect > 0)
      timings.connect -= (timings.ssl >= 0 ? timings.ssl : 0);
    var timing = {
      proxyStart: proxy > 0 ? lastEntry - proxy : -1,
      proxyEnd: proxy > 0 ? lastEntry : -1,
      requestTime: baseTime + (queueing > 0 ? queueing : 0) / 1000,
      dnsStart: timings.dns >= 0 ? lastEntry : -1,
      dnsEnd: accumulateTime(timings.dns),

      connectStart: timings.connect >= 0 ? lastEntry : -1,
      connectEnd: accumulateTime(timings.connect) + (timings.ssl >= 0 ? timings.ssl : 0),

      sslStart: timings.ssl >= 0 ? lastEntry : -1,
      sslEnd: accumulateTime(timings.ssl),

      workerStart: -1,
      workerReady: -1,
      sendStart: timings.send >= 0 ? lastEntry : -1,
      sendEnd: accumulateTime(timings.send),
      pushStart: 0,
      pushEnd: 0,
      receiveHeadersEnd: accumulateTime(timings.wait)
    };
    accumulateTime(timings.receive);

    request.timing = timing;
    request.endTime = baseTime + Math.max(entryTotalDuration, lastEntry) / 1000;
  }
};
