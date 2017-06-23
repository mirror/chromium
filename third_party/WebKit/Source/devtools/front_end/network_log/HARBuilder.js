/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// See http://www.softwareishard.com/blog/har-12-spec/ for HAR specification.
NetworkLog.HARBuilder = class {
  /**
   * @param {!Array<!SDK.NetworkRequest>} requests
   * @param {boolean} includeContent
   * @param {!Common.Progress=} progress
   * @return {!Promise<!NetworkLog.HARRoot>}
   */
  static async build(requests, includeContent, progress) {
    if (progress)
      progress.setTotalWork(requests.length);
    var promises = requests.map(request => NetworkLog.HARBuilder.buildEntry(request, includeContent, progress));
    return new NetworkLog.HARRoot({
      log: {
        version: '1.2',
        creator: {
          name: 'Chrome Devtools',
          version: SDK.MultitargetNetworkManager.patchUserAgentWithChromeVersion('%s') || 'n/a'
        },
        pages: NetworkLog.HARBuilder._buildPages(requests),
        entries: await Promise.all(promises)
      }
    });
  }

  /**
   * @param {!Array<!SDK.NetworkRequest>} requests
   * @return {!Array<!Object>}
   */
  static _buildPages(requests) {
    var pages = new Map();
    for (var request of requests) {
      var page = NetworkLog.networkLog.pageLoadForRequest(request);
      if (!page || pages.has(page))
        continue;
      pages.set(page, NetworkLog.HARBuilder._buildPage(page));
    }
    return Array.from(pages.values());
  }

  /**
   * @param {!NetworkLog.PageLoad} page
   * @return {!Object}
   */
  static _buildPage(page) {
    var contentLoadTime = -1;
    if (page.startTime !== -1 && page.contentLoadTime !== -1)
      contentLoadTime = (page.contentLoadTime - page.startTime) * 1000;
    var loadTime = -1;
    if (page.loadTime !== -1 && page.contentLoadTime !== -1)
      loadTime = (page.loadTime - page.startTime) * 1000;
    return {
      startedDateTime: new Date(page.mainRequest.pseudoWallTime(page.startTime)),
      id: 'page_' + page.id,
      title: page.url,  // We don't have actual page title here. URL is probably better than nothing.
      pageTimings: {onContentLoad: contentLoadTime, onLoad: loadTime}
    };
  }

  /**
   * @param {!SDK.NetworkRequest} request
   * @param {boolean} includeContent
   * @param {!Common.Progress=} progress
   * @return {!Promise<!Object>}
   */
  static async buildEntry(request, includeContent, progress) {
    var contentData = includeContent ? await request.contentData() : null;
    if (progress)
      progress.worked();
    var ipAddress = request.remoteAddress();
    var portPositionInString = ipAddress.lastIndexOf(':');
    if (portPositionInString !== -1)
      ipAddress = ipAddress.substr(0, portPositionInString);

    var pageLoad = NetworkLog.networkLog.pageLoadForRequest(request);
    var headersText = request.requestHeadersText();

    var url = request.url();
    var hashIndex = url.indexOf('#');
    if (hashIndex !== -1)
      url = url.substring(0, hashIndex);
    var requestCookies = request.requestCookies;
    var responseCookies = request.responseCookies;

    var compression = undefined;
    var isCacheOrRedirect = request.cached() || request.statusCode === 304;
    var responseBodySize = isCacheOrRedirect ? 0 : -1;
    if (!isCacheOrRedirect && request.responseHeadersText) {
      responseBodySize = request.transferSize - request.responseHeadersText.length;
      if (request.statusCode !== 206)  // Partial content.
        compression = request.resourceSize - responseBodySize;
    }

    var bodySize = request.requestFormData ? request.requestFormData.length : 0;
    var postData = undefined;
    if (request.requestFormData) {
      postData = {
        mimeType: request.requestContentType(),
        text: request.requestFormData,
        params: request.formParameters ? request.formParameters.slice() : undefined
      };
    }

    return {
      connection: (request.connectionId !== '0') ? request.connectionId : undefined,
      pageref: pageLoad ? ('page_' + pageLoad.id) : undefined,
      startedDateTime: new Date(request.pseudoWallTime(request.startTime)),
      time: request.duration !== -1 ? (request.duration * 1000) : 0,
      request: {
        method: request.requestMethod,
        url: url,
        httpVersion: request.requestHttpVersion(),
        headers: request.requestHeaders(),
        queryString: request.queryParameters ? request.queryParameters.slice() : [],
        cookies: requestCookies ? requestCookies.map(buildCookie) : [],
        headersSize: headersText ? headersText.length : -1,
        bodySize: bodySize,
        postData: postData
      },
      response: {
        status: request.statusCode,
        statusText: request.statusText,
        httpVersion: request.responseHttpVersion(),
        headers: request.responseHeaders,
        cookies: responseCookies ? responseCookies.map(buildCookie) : [],
        content: {
          size: request.resourceSize,
          mimeType: request.mimeType || 'x-unknown',
          compression: compression,
          text: (contentData && contentData.content) ? contentData.content : undefined,
          encoding: (contentData && contentData.encoded) ? 'base64' : undefined
        },
        redirectURL: request.responseHeaderValue('Location') || '',
        headersSize: request.responseHeadersText ? request.responseHeadersText.length : -1,
        bodySize: responseBodySize,
        _transferSize: request.transferSize,
        _error: request.localizedFailDescription
      },
      cache: {},  // Not supported yet.
      timings: NetworkLog.HARBuilder._buildTimings(request.timing, request.duration * 1000),
      serverIPAddress: ipAddress
    };

    /**
     * @param {!SDK.Cookie} cookie
     * @return {!Object}
     */
    function buildCookie(cookie) {
      return {
        name: cookie.name(),
        value: cookie.value(),
        path: cookie.path(),
        domain: cookie.domain(),
        expires: cookie.expiresDate(request.startTime !== -1 ? request.pseudoWallTime(request.startTime) : null),
        httpOnly: cookie.httpOnly(),
        secure: cookie.secure(),
        sameSite: cookie.sameSite()
      };
    }
  }

  /**
   * @param {!Protocol.Network.ResourceTiming|undefined} timing
   * @param {number} duration
   * @return {!Object}
   */
  static _buildTimings(timing, duration) {
    // Order of events: request_start = 0, [proxy], [dns], [connect [ssl]], [send], receive_headers_end
    // HAR 'blocked' time is time before first network activity.
    if (!timing)
      return {blocked: -1, dns: -1, connect: -1, send: 0, wait: 0, receive: 0, ssl: -1};

    var blocked = [timing.dnsStart, timing.connectStart, timing.sendStart].find(time => time !== -1) || -1;

    var dns = (timing.dnsStart >= 0) ? ([timing.connectStart, timing.sendStart].find(time => time !== -1) || -1) : -1;
    if (dns !== -1)
      dns -= timing.dnsStart;

    var connect = (timing.connectStart >= 0) ? (timing.sendStart - timing.connectStart) : -1;
    var send = timing.sendEnd - timing.sendStart;
    var wait = timing.receiveHeadersEnd - timing.sendEnd;
    // TODO(allada) We may want to calculate duration different here, since duration is using endTime we pass it now.
    var receive = duration - timing.receiveHeadersEnd;
    var ssl = (timing.sslStart >= 0 && timing.sslEnd >= 0) ? (timing.sslEnd - timing.sslStart) : -1;

    return {blocked: blocked, dns: dns, connect: connect, send: send, wait: wait, receive: receive, ssl: ssl};
  }
};
