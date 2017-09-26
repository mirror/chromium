// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the Cookies and Local Storage Data
 * section.
 */

/**
 * @enum {number}
 */
var LocalDataStatus = {
  NORMAL: 0,
  DISABLED: 1,
  SELECTED: 2,
};

/**
 * @typedef {{
 *   data: !Object,
 *   id: string,
 *   status: LocalDataStatus,
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
     * @param {number} begin Which element to start with.
     * @param {number} count How many list elements are displayed.
     * @return {!Promise<!LocalDataList>}
     */
    getDisplayList(begin, count) {}

    /**
     * @param {number} moveID Which elements to .
     * @param {number} beforeId Which elements to .
     */
    moveItemBefore(moveID, beforeId) {}

    /**
     * Operations using an |index| can produce unexpected results if the list
     * is being changed by another party. The primary use for this call is to
     * move to the top (index 0) or bottom (index -1) of the list. To explicitly
     * move item 'A' before item 'B', use moveItemBefore().
     * Call getDisplayList() to get an updated view.
     * @param {number} moveID Which elements to .
     * @param {number} index The item will occupy index N, all subsequent items
     *     move further down. A value < 0 or >= list length moves to the end.
     */
    moveItemTo(moveID, index) {}

    /**
     * Call getDisplayList() to get an updated view.
     */
    removeAll() {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {?Object} filter Host specific filtering.
     */
    removeByFilter(filter) {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {!Array<number>} idList Which elements to delete.
     */
    removeItems(idList) {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {number} id Which element to replace.
     * @param {!LocalDataItem} item Replacement data.
     */
    replaceItem(id, item) {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {?Object} ordering The effect of the ordering string is up to the
     *     host to interpret.
     */
    sortBy(ordering) {}

    /**
     * Call getDisplayList() to get an updated view.
     */
    selectAll() {}

    /**
     * Call getDisplayList() to get an updated view.
     * @param {?Object} filter Host specific filtering.
     */
    selectByFilter(filter) {}

    /**
     * The updates are applied in the order they appear. If the same item
     * appears more than once, the last entry prevails (though there's little
     * point in sending an item more than once).
     * Call getDisplayList() to get an updated view.
     * @param {!Array<!Array<string, boolean>>} selectionList
     */
    selectItems(selectionList) {}



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
     * Fetches all children of a given cookie.
     * @param {string} path The path to the parent cookie.
     * @return {!Promise<!Array<!CookieDataSummaryItem>>} Returns a cookie list
     *     for the given path.
     */
    loadCookieChildren(path) {}

    /**
     * Removes a given cookie.
     * @param {string} path The path to the parent cookie.
     */
    removeCookie(path) {}

    /**
     * Removes all cookies.
     * @return {!Promise<!CookieList>} Returns the up to date cookie list once
     *     deletion is complete (empty list).
     */
    removeAllCookies() {}
  }

  /**
   * @implements {settings.LocalDataBrowserProxy}
   */
  class LocalDataBrowserProxyImpl {
    /** @override */
    getDisplayList(begin, count) {
      return cr.sendWithPromise('localData.getDisplayList', begin, count);
    }

    /** @override */
    moveItemBefore(moveID, beforeId) {
      chrome.send('localData.moveItemBefore', [moveID, beforeId]);
    }

    /** @override */
    moveItemTo(moveID, index) {
      chrome.send('localData.moveItemTo', [moveID, index]);
    }

    /** @override */
    removeAll() {
      chrome.send('localData.removeAll');
    }

    /** @override */
    removeByFilter(filter) {
      chrome.send('localData.removeByFilter', [filter]);
    }

    /** @override */
    removeItems(idList) {
      chrome.send('localData.removeItem', [idList]);
    }

    /** @override */
    replaceItem(id, item) {
      chrome.send('localData.replaceItem', [id, item]);
    }

    /** @override */
    sortBy(ordering) {
      chrome.send('localData.sortBy', [ordering]);
    }

    /** @override */
    selectAll() {
      chrome.send('localData.selectAll');
    }

    /** @override */
    selectByFilter(filter) {
      chrome.send('localData.selectByFilter', [filter]);
    }

    /** @override */
    selectItems(selectionList) {
      chrome.send('localData.selectItems', [filter]);
    }


    /** @override */
    getCookieDetails(site) {
      return cr.sendWithPromise('getCookieDetails', [site]);
    }

    /** @override */
    reloadCookies() {
      return cr.sendWithPromise('reloadCookies');
    }

    /** @override */
    loadCookieChildren(path) {
      return cr.sendWithPromise('loadCookie', path);
    }

    /** @override */
    removeCookie(path) {
      chrome.send('removeCookie', [path]);
    }

    /** @override */
    removeAllCookies() {
      return cr.sendWithPromise('removeAllCookies');
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
