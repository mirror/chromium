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
class Extension;
class ScriptContext;

class FeatureCache {
 public:
  using FeatureSet = std::set<const Feature*>;
  using FeatureNameSet = std::set<std::string>;

  FeatureCache();
  ~FeatureCache();

  FeatureNameSet GetAvailableFeatures(ScriptContext* context);

  void InvalidateExtension(const ExtensionId& extension_id);

 private:
  using CacheMapKey = std::pair<ExtensionId, Feature::Context>;
  using CacheMap = std::map<CacheMapKey, FeatureSet>;

  const FeatureSet& GetFeaturesFromCache(ScriptContext* context);

  CacheMap feature_cache_;
  DISALLOW_COPY_AND_ASSIGN(FeatureCache);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_FEATURE_CACHE_H_
