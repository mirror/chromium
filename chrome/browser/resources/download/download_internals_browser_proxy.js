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

cr.define('downloadInternals', function() {
  /** @interface */
  function DownloadInternalsBrowserProxy() {}

  DownloadInternalsBrowserProxy.prototype = {
    /**
     * Gets the current status of the Download Service
     * @return {!Promise<ServiceStatus>} A promise firing when the service
     *     status is fetched.
     */
    getServiceState: function() {},
  };

  /**
   * @constructor
   * @implements {downloadInternals.DownloadInternalsBrowserProxy}
   */
  function DownloadInternalsBrowserProxyImpl() {}
  cr.addSingletonGetter(DownloadInternalsBrowserProxyImpl);


  DownloadInternalsBrowserProxyImpl.prototype = {
    /** @override */
    getServiceState: function() {
      return cr.sendWithPromise('getServiceState');
    }
  };

  return {
    DownloadInternalsBrowserProxy: DownloadInternalsBrowserProxy,
    DownloadInternalsBrowserProxyImpl: DownloadInternalsBrowserProxyImpl
  };
});