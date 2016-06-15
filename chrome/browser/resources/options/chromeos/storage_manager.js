// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   totalSize: string,
 *   availableSize: string,
 *   usedSize: string,
 *   usedRatio: number
 * }}
 */
options.StorageSizeStat;

cr.define('options', function() {
  var Page = cr.ui.pageManager.Page;
  var PageManager = cr.ui.pageManager.PageManager;

  function StorageManager() {
    Page.call(this, 'storage',
              loadTimeData.getString('storageManagerPageTabTitle'),
              'storageManagerPage');
  }

  cr.addSingletonGetter(StorageManager);

  StorageManager.prototype = {
    __proto__: Page.prototype,

    /** @override */
    initializePage: function() {
      Page.prototype.initializePage.call(this);

      $('storage-manager-label-downloads').onclick = function() {
        chrome.send('openDownloads');
      };
      $('storage-manager-label-arc').onclick = function() {
        chrome.send('openArcStorage');
      };

      $('storage-confirm').onclick = function() {
        PageManager.closeOverlay();
      };
    },

    /** @override */
    didShowPage: function() {
      // Updating storage information can be expensive (e.g. computing directory
      // sizes recursively), so we delay this operation until the page is shown.
      chrome.send('updateStorageInfo');
    },

    /**
     * Updates the size information (total/used/available) of the internal
     * storage.
     * @param {!options.StorageSizeStat} sizeStat
     * @private
     */
    setSizeStat_: function(sizeStat) {
      $('storage-manager-size-capacity').textContent = sizeStat.totalSize;
      $('storage-manager-size-in-use').textContent = sizeStat.usedSize;
      $('storage-manager-size-available').textContent = sizeStat.availableSize;
    },

    /**
     * Updates the size Downloads directory.
     * @param {string} size Formatted string of the size of Downloads.
     * @private
     */
    setDownloadsSize_: function(size) {
      $('storage-manager-size-downloads').textContent = size;
    },

    /**
     * Updates the total size of Android apps and cache.
     * @param {string} size Formatted string of the size of Android apps and
     * cache.
     * @private
     */
    setArcSize_: function(size) {
      assert(!$('storage-manager-item-arc').hidden);
      $('storage-manager-size-arc').textContent = size;
    },

    /**
     * Shows the item "Android apps and cache" on the overlay UI.
     * @private
     */
    showArcItem_: function() {
      $('storage-manager-item-arc').hidden = false;
    },
  };

  // Forward public APIs to private implementations.
  cr.makePublic(StorageManager, [
    'setArcSize',
    'setDownloadsSize',
    'setSizeStat',
    'showArcItem',
  ]);

  return {
    StorageManager: StorageManager
  };
});
