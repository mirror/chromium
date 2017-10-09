// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   serviceState: string,
 *   modelStatus: string,
 *   driverStatus: string,
 *   fileMonitorStatus: string
 * }}
 */
var ServiceStatus;

/**
 * @typedef {{
 *   client: string,
 *   guid: string,
 *   state: string,
 *   url: string,
 *   bytes_downloaded: number,
 *   result: string,
 *   driver: {{
 *     state: string,
 *     paused: boolean,
 *     done: boolean
 *   }}
 * }}
 */
var ServiceEntry;

/**
 * @typedef {{
 *   client: string,
 *   guid: string,
 *   result: string
 * }}
 */
var ServiceRequest;

cr.define('downloadInternals', function() {
  /** @interface */
  class DownloadInternalsBrowserProxy {
    /**
     * Gets the current status of the Download Service.
     * @return {!Promise<ServiceStatus>} A promise firing when the service
     *     status is fetched.
     */
    getServiceStatus() {}

    /**
     * Gets the current list of downloads the Download Service is aware of.
     * @return {!Promise<ServiceEntry>} A promise firing when the list of
     *     downloads is fetched.
     */
    getServiceDownloads() {}
  }

  /**
   * @implements {downloadInternals.DownloadInternalsBrowserProxy}
   */
  class DownloadInternalsBrowserProxyImpl {
    /** @override */
    getServiceStatus() {
      return cr.sendWithPromise('getServiceStatus');
    }

    /** @override */
    getServiceDownloads() {
      return cr.sendWithPromise('getServiceDownloads');
    }
  }

  cr.addSingletonGetter(DownloadInternalsBrowserProxyImpl);

  return {
    DownloadInternalsBrowserProxy: DownloadInternalsBrowserProxy,
    DownloadInternalsBrowserProxyImpl: DownloadInternalsBrowserProxyImpl
  };
});