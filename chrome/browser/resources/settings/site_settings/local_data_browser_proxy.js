// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the Cookies and Local Storage Data
 * section.
 */

/**
 * @typedef {{
 *   id: string,
 *   start: number,
 *   children: !Array<CookieDetails>,
 * }}
 */
var CookieList;

/**
 * @typedef {{
 *   data: !Object,
 *   id: string,
 * }}
 */
var LocalDataItem;

/**
 * @typedef {{
 *   filter: ?Object,
 *   items: !Array<LocalDataItem>,
 *   order: ?Object,
 *   start: number,
 *   total: number,
 * }}
 */
var LocalDataList;

cr.define('settings', function() {
  /** @interface */
  class LocalDataBrowserProxy {
    /**
     * @param {string} filter Search filter (use "" for none).
     * @return {!Promise}
     */
    filter(filter) {}

    /**
     * @param {number} begin Which element to start with.
     * @param {number} count How many list elements are displayed.
     * @return {!Promise<!LocalDataList>}
     */
    getDisplayList(begin, count) {}

    /**
     * Removes all local data (local storage, cookies, etc.).
     * @return {!Promise<!LocalDataList>} Likely an empty list, unless something
     *     added something between the delete and the response creation.
     */
    removeAll() {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {?Object} filter Host specific filtering.
     */
    removeByFilter(filter) {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {string} id Which element to delete.
     */
    removeItem(id) {}

    /**
     * Gets the cookie details for a particular site.
     * @param {string} site The name of the site.
     * @return {!Promise<!CookieList>}
     */
    getCookieDetails(site) {}

    /**
     * Reloads all cookies.
     * @return {!Promise<!CookieList>} Returns the full cookie
     *     list.
     */
    reloadCookies() {}

    /**
     * Removes a given cookie.
     * @param {string} path The path to the parent cookie.
     */
    removeCookie(path) {}
  }

  /**
   * @implements {settings.LocalDataBrowserProxy}
   */
  class LocalDataBrowserProxyImpl {
    /** @override */
    filter(filter) {
      return cr.sendWithPromise('localData.filter', filter);
    }

    /** @override */
    getDisplayList(begin, count) {
      return cr.sendWithPromise('localData.getDisplayList', begin, count);
    }

    /** @override */
    removeAll() {
      return cr.sendWithPromise('localData.removeAll');
    }

    /** @override */
    removeByFilter(filter) {
      chrome.send('localData.removeByFilter', [filter]);
    }

    /** @override */
    removeItem(id) {
      chrome.send('localData.removeItem', [id]);
    }

    /** @override */
    getCookieDetails(site) {
      return cr.sendWithPromise('getCookieDetails', site);
    }

    /** @override */
    reloadCookies() {
      return cr.sendWithPromise('localData.reload');
    }

    /** @override */
    removeCookie(path) {
      chrome.send('removeCookie', [path]);
    }
  }

  // The singleton instance_ is replaced with a test version of this wrapper
  // during testing.
  cr.addSingletonGetter(LocalDataBrowserProxyImpl);

  return {
    LocalDataBrowserProxy: LocalDataBrowserProxy,
    LocalDataBrowserProxyImpl: LocalDataBrowserProxyImpl,
  };
});
