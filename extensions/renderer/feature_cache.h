// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_FEATURE_CACHE_H_
#define EXTENSIONS_RENDERER_FEATURE_CACHE_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/macros.h"
#include "extensions/common/extension_id.h"
#include "extensions/common/features/feature.h"

namespace extensions {
class ScriptContext;

// Caches features available to different extensions in different contexts, and
// returns features available to a given context.
class FeatureCache {
 public:
  using FeatureSet = std::set<const Feature*>;
  using FeatureNameSet = std::set<std::string>;

  FeatureCache();
  ~FeatureCache();

  // Returns a set of names of features available to the given |context|.
  // Implementation Detail: This returns a set of names (instead of set of
  // features) to allow lexicographical iteration over available features.
  FeatureNameSet GetAvailableFeatures(ScriptContext* context);

  // Invalidates the cache for the specified extension.
  void InvalidateExtension(const ExtensionId& extension_id);

 private:
  // Note: We use a key of ExtensionId, Feature::Context to maximize cache hits.
  // Unfortunately, this won't always be perfectly accurate, since some features
  // may have other context-dependent restrictions (such as URLs), but caching
  // by extension id + context + url would result in significantly fewer hits.
  using CacheMapKey = std::pair<ExtensionId, Feature::Context>;
  using CacheMap = std::map<CacheMapKey, FeatureSet>;

  // Returns the features available to the given context from the cache.
  const FeatureSet& GetFeaturesFromCache(ScriptContext* context);

  CacheMap feature_cache_;

  DISALLOW_COPY_AND_ASSIGN(FeatureCache);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_FEATURE_CACHE_H_
