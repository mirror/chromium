// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var metadataProxy = {};

/**
 * Maximum number of entries whose metadata can be cached.
 * @const {number}
 * @private
 */
metadataProxy.MAX_CACHED_METADATA_ = 10000;

/**
 * @private {!LRUCache<!Metadata>}
 */
metadataProxy.cache_ = new LRUCache(metadataProxy.MAX_CACHED_METADATA_);

/**
 * Returns metadata for the given FileEntry. Uses cached metadata if possible.
 *
 * @param {!FileEntry} entry
 * @param {boolean=} opt_force_refresh_cache If true, get metadata from actual
 *     entry assuming that the cache entry is stale.
 * @return {!Promise<!Metadata>}
 */
metadataProxy.getEntryMetadata = function(entry, opt_force_refresh_cache) {
  var entryURL = entry.toURL();
  if (!opt_force_refresh_cache && metadataProxy.cache_.hasKey(entryURL)) {
    return Promise.resolve(metadataProxy.cache_.get(entryURL));
  } else {
    return new Promise(function(resolve, reject) {
      entry.getMetadata(function(metadata) {
        metadataProxy.cache_.put(entryURL, metadata);
        resolve(metadata);
      }, reject);
    });
  }
};
