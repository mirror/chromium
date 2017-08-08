// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/feature_cache.h"

#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

FeatureCache::FeatureCache() {}
FeatureCache::~FeatureCache() = default;

FeatureCache::FeatureNameSet FeatureCache::GetAvailableFeatures(
    ScriptContext* context) {
  const FeatureSet& features = GetFeaturesFromCache(context);
  FeatureNameSet names;
  for (const Feature* feature : features) {
    if (context->IsAnyFeatureAvailableToContext(
            *feature, CheckAliasStatus::NOT_ALLOWED)) {
      names.insert(feature->name());
    }
  }
  return names;
}

void FeatureCache::InvalidateExtension(const ExtensionId& extension_id) {
  for (auto iter = feature_cache_.begin(); iter != feature_cache_.end();) {
    if (iter->first.first == extension_id)
      iter = feature_cache_.erase(iter);
    else
      ++iter;
  }
}

const FeatureCache::FeatureSet& FeatureCache::GetFeaturesFromCache(
    ScriptContext* context) {
  CacheMapKey key(context->GetExtensionID(), context->context_type());

  auto iter = feature_cache_.find(key);
  if (iter != feature_cache_.end())
    return iter->second;

  FeatureSet features;
  const FeatureProvider* api_feature_provider =
      FeatureProvider::GetAPIFeatures();
  GURL empty_url;
  const Extension* extension = context->extension();
  for (const auto& map_entry : api_feature_provider->GetAllFeatures()) {
    // Exclude internal APIs.
    if (map_entry.second->IsInternal())
      continue;

    // Exclude child features (like events or specific functions).
    if (api_feature_provider->GetParent(map_entry.second.get()) != nullptr)
      continue;

    // Skip chrome.test if this isn't a test.
    if (map_entry.first == "test" &&
        !base::CommandLine::ForCurrentProcess()->HasSwitch(
            ::switches::kTestType)) {
      continue;
    }

    if (!ExtensionAPI::GetSharedInstance()->IsAnyFeatureAvailableToContext(
            *map_entry.second, extension, context->context_type(), empty_url,
            CheckAliasStatus::NOT_ALLOWED)) {
      continue;
    }

    features.insert(map_entry.second.get());
  }

  return feature_cache_.emplace(key, std::move(features)).first->second;
}

}  // namespace extensions
