// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!MetadataProvider} rawProvider
 * @constructor
 * @struct
 */
function MetadataModel(rawProvider) {
  /**
   * @private {!MetadataProvider}
   * @const
   */
  this.rawProvider_ = rawProvider;

  /**
   * @private {!MetadataProviderCache}
   * @const
   */
  this.cache_ = new MetadataProviderCache();

  /**
   * @private {!Array<!MetadataProviderCallbackRequest<T>>}
   * @const
   */
  this.callbackRequests_ = [];

  /**
   * @private {boolean}
   */
  this.useModificationByMeTime_ = false;
}

/**
 * @param {!VolumeManagerCommon.VolumeInfoProvider} volumeManager
 * @return {!MetadataModel}
 */
MetadataModel.create = function(volumeManager) {
  return new MetadataModel(
      new MultiMetadataProvider(
          new FileSystemMetadataProvider(),
          new ExternalMetadataProvider(),
          new ContentMetadataProvider(),
          volumeManager));
};

/**
 * @return {!MetadataProvider}
 */
MetadataModel.prototype.getProvider = function() {
  return this.rawProvider_;
};

/**
 * Obtains metadata for entries.
 * @param {!Array<!Entry>} entries Entries.
 * @param {!Array<string>} names Metadata property names to be obtained.
 * @return {!Promise<!Array<!MetadataItem>>}
 */
MetadataModel.prototype.get = function(entries, names) {
  names = this.rewriteRequestedNames_(names);

  this.rawProvider_.checkPropertyNames(names);

  // Check if the results are cached or not.
  if (this.cache_.hasFreshCache(entries, names))
    return Promise.resolve(this.getCache(entries, names));

  // The LRU cache may be cached out when the callback is completed.
  // To hold cached values, create snapshot of the cache for entries.
  var requestId = this.cache_.generateRequestId();
  var snapshot = this.cache_.createSnapshot(entries);
  var requests = snapshot.createRequests(entries, names);
  snapshot.startRequests(requestId, requests);
  this.cache_.startRequests(requestId, requests);

  // Register callback.
  var promise = new Promise(function(fulfill) {
    this.callbackRequests_.push(new MetadataProviderCallbackRequest(
        this, entries, names, snapshot, fulfill));
  }.bind(this));

  // If the requests are not empty, call the requests.
  if (requests.length) {
    this.rawProvider_.get(requests).then(function(list) {
      // Obtain requested entries and ensure all the requested properties are
      // contained in the result.
      var requestedEntries = [];
      for (var i = 0; i < requests.length; i++) {
        requestedEntries.push(requests[i].entry);
        for (var j = 0; j < requests[i].names.length; j++) {
          var name = requests[i].names[j];
          if (!(name in list[i]))
            list[i][name] = undefined;
        }
      }

      // Store cache.
      this.cache_.storeProperties(requestId, requestedEntries, list);

      // Invoke callbacks.
      var i = 0;
      while (i < this.callbackRequests_.length) {
        if (this.callbackRequests_[i].storeProperties(
            requestId, requestedEntries, list)) {
          // Callback was called.
          this.callbackRequests_.splice(i, 1);
        } else {
          i++;
        }
      }
    }.bind(this));
  }

  return promise;
};

/**
 * Rewrites requested metadata property names if needed.
 * @param {!Array<string>} names Metadata property names.
 * @return {!Array<string>} names Rewritten metadata property names.
 */
MetadataModel.prototype.rewriteRequestedNames_ = function(names) {
  // When modificationTime is requested, always request modificationByMeTime
  // regardless of this.useModificationByMeTime_ because the result of
  // this request can be cached and reused later.
  if (names.indexOf('modificationTime') >= 0 &&
      names.indexOf('modificationByMeTime') < 0) {
    names = names.slice();
    names.push('modificationByMeTime');
  }
  return names;
};

/**
 * Rewrites modificationDate of MetadataItems if needed.
 * @param {!Array<!MetadataItem>} items Items.
 * @return {!Array<!MetadataItem>} Rewritten items.
 */
MetadataModel.prototype.rewriteModificationDate = function(items) {
  if (!this.useModificationByMeTime_)
    return items;

  var newItems = [];
  for (var i = 0; i < items.length; ++i) {
    var item = items[i];
    if (item.modificationByMeTime &&
        item.modificationByMeTime !== item.modificationTime) {
      item = item.clone();
      item.modificationTime = item.modificationByMeTime;
    }
    newItems.push(item);
  }
  return newItems;
};

/**
 * Obtains metadata cache for entries.
 * @param {!Array<!Entry>} entries Entries.
 * @param {!Array<string>} names Metadata property names to be obtained.
 * @return {!Array<!MetadataItem>}
 */
MetadataModel.prototype.getCache = function(entries, names) {
  names = this.rewriteRequestedNames_(names);

  // Check if the property name is correct or not.
  this.rawProvider_.checkPropertyNames(names);
  return this.rewriteModificationDate(this.cache_.get(entries, names));
};

/**
 * Clears old metadata for newly created entries.
 * @param {!Array<!Entry>} entries
 */
MetadataModel.prototype.notifyEntriesCreated = function(entries) {
  this.cache_.clear(util.entriesToURLs(entries));
};

/**
 * Clears metadata for deleted entries.
 * @param {!Array<string>} urls Note it is not an entry list because we cannot
 *     obtain entries after removing them from the file system.
 */
MetadataModel.prototype.notifyEntriesRemoved = function(urls) {
  this.cache_.clear(urls);
};

/**
 * Invalidates metadata for updated entries.
 * @param {!Array<!Entry>} entries
 */
MetadataModel.prototype.notifyEntriesChanged = function(entries) {
  this.cache_.invalidate(this.cache_.generateRequestId(), entries);
};

/**
 * Clears all cache.
 */
MetadataModel.prototype.clearAllCache = function() {
  this.cache_.clearAll();
};

/**
 * Adds event listener to internal cache object.
 * @param {string} type
 * @param {function(Event):undefined} callback
 */
MetadataModel.prototype.addEventListener = function(type, callback) {
  this.cache_.addEventListener(type, callback);
};

/**
 * Sets whether to use modificationByMeTime as "Last Modified" time.
 * @param {boolean} useModificationByMeTime
 */
MetadataModel.prototype.setUseModificationByMeTime = function(
    useModificationByMeTime) {
  this.useModificationByMeTime_ = useModificationByMeTime;
};

/**
 * @param {!Array<!Entry>} entries
 * @param {!Array<string>} names
 * @param {!MetadataCacheSet} cache
 * @param {function(!MetadataItem):undefined} fulfill
 * @constructor
 * @struct
 */
function MetadataProviderCallbackRequest(
    metadataModel, entries, names, cache, fulfill) {
  /**
   * @private {!MetadataModel}
   * @const
   */
  this.metadataModel_ = metadataModel;

  /**
   * @private {!Array<!Entry>}
   * @const
   */
  this.entries_ = entries;

  /**
   * @private {!Array<string>}
   * @const
   */
  this.names_ = names;

  /**
   * @private {!MetadataCacheSet}
   * @const
   */
  this.cache_ = cache;

  /**
   * @private {function(!MetadataItem):undefined}
   * @const
   */
  this.fulfill_ = fulfill;
}

/**
 * Stores properties to snapshot cache of the callback request.
 * If all the requested property are served, it invokes the callback.
 * @param {number} requestId
 * @param {!Array<!Entry>} entries
 * @param {!Array<!MetadataItem>} objects
 * @return {boolean} Whether the callback is invoked or not.
 */
MetadataProviderCallbackRequest.prototype.storeProperties = function(
    requestId, entries, objects) {
  this.cache_.storeProperties(requestId, entries, objects);
  if (this.cache_.hasFreshCache(this.entries_, this.names_)) {
    this.fulfill_(this.metadataModel_.rewriteModificationDate(
        this.cache_.get(this.entries_, this.names_)));
    return true;
  }
  return false;
};

/**
 * Helper wrapper for LRUCache.
 * @constructor
 * @extends {MetadataCacheSet}
 * @struct
 */
function MetadataProviderCache() {
  MetadataCacheSet.call(this, new MetadataCacheSetStorageForObject({}));

  /**
   * @private {number}
   */
  this.requestIdCounter_ = 0;
}

MetadataProviderCache.prototype.__proto__ = MetadataCacheSet.prototype;

/**
 * Generates a unique request ID every time when it is called.
 * @return {number}
 */
MetadataProviderCache.prototype.generateRequestId = function() {
  return this.requestIdCounter_++;
};
