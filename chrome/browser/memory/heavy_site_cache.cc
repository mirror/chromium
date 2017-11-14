// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/heavy_site_cache.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace memory {

namespace {

constexpr char kTimestampKey[] = "timestamp";

// Returns false on error.
bool GetTimestamp(const ContentSettingPatternSource& source,
                  double* timestamp) {
  DCHECK(source.setting_value);
  DCHECK(timestamp);
  base::DictionaryValue* dict = nullptr;
  source.setting_value->GetAsDictionary(&dict);
  return dict && dict->GetDouble(kTimestampKey, timestamp);
}

}  // namespace

HeavySiteCache::HeavySiteCache(Profile* profile, size_t max_cache_size)
    : settings_map_(HostContentSettingsMapFactory::GetForProfile(profile)),
      max_cache_size_(max_cache_size) {
  // TODO(csharrison): Initialize cache_size_.
  DCHECK(settings_map_);
  ContentSettingsForOneType settings;
  settings_map_->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_HEAVY_MEMORY,
                                       std::string(), &settings);
  cache_size_ = settings.size();
  // TrimCache also does a verification check.
  TrimCache();
  CheckCacheConsistency();
}

HeavySiteCache::~HeavySiteCache() = default;

void HeavySiteCache::AddSite(const GURL& url) {
  double now = clock_->Now().ToDoubleT();
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetDouble(kTimestampKey, now);
  settings_map_->SetWebsiteSettingDefaultScope(
      url, GURL(), CONTENT_SETTINGS_TYPE_HEAVY_MEMORY, std::string(),
      std::move(dict));
  cache_size_++;
  if (cache_size_ > max_cache_size_)
    TrimCache();
  CheckCacheConsistency();
}

void HeavySiteCache::RemoveSite(const GURL& url) {
  settings_map_->SetWebsiteSettingDefaultScope(
      url, GURL(), CONTENT_SETTINGS_TYPE_HEAVY_MEMORY, std::string(), nullptr);
  cache_size_--;
  DCHECK_GE(cache_size_, 0u);
  CheckCacheConsistency();
}

bool HeavySiteCache::IsSiteHeavy(const GURL& url) const {
  // A site is heavy if there exists a website setting for it.
  return !!settings_map_->GetWebsiteSetting(
      url, GURL(), CONTENT_SETTINGS_TYPE_HEAVY_MEMORY, std::string(), nullptr);
}

void HeavySiteCache::TrimCache() {
  ContentSettingsForOneType settings;
  settings_map_->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_HEAVY_MEMORY,
                                       std::string(), &settings);
  DCHECK_EQ(cache_size_, settings.size());

  // If any of the data is corrupted or in a format we don't recognize, just
  // erase everything. Otherwise, sort the settings by their timestamp in
  // descending order.
  bool error = false;
  std::sort(settings.begin(), settings.end(),
            [&error](const ContentSettingPatternSource& lhs,
                     const ContentSettingPatternSource& rhs) {
              double lhs_timestamp = 0.;
              double rhs_timestamp = 0.;
              error |= !GetTimestamp(lhs, &lhs_timestamp);
              error |= !GetTimestamp(rhs, &rhs_timestamp);
              return lhs_timestamp > rhs_timestamp;
            });
  if (error) {
    settings_map_->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_HEAVY_MEMORY);
    cache_size_ = 0u;
    return;
  }

  // Remove the elements with the least timestamp (i.e. the ones added least
  // recently).
  while (cache_size_ > max_cache_size_) {
    DCHECK(settings.size());
    const ContentSettingPatternSource& pattern_to_remove = settings.back();
    settings.pop_back();

    settings_map_->SetWebsiteSettingCustomScope(
        pattern_to_remove.primary_pattern, pattern_to_remove.secondary_pattern,
        CONTENT_SETTINGS_TYPE_HEAVY_MEMORY, std::string(), nullptr);
    cache_size_--;
  }
}

void HeavySiteCache::CheckCacheConsistency() const {
#if DCHECK_IS_ON()
  ContentSettingsForOneType settings;
  settings_map_->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_HEAVY_MEMORY,
                                       std::string(), &settings);
  DCHECK_EQ(settings.size(), cache_size_);
  DCHECK_GE(max_cache_size_, cache_size_);
  for (const auto& it : settings) {
    DCHECK(it.secondary_pattern.MatchesAllHosts());
    double out_timestamp = 0.;
    DCHECK(GetTimestamp(it, &out_timestamp));
  }
#endif
}

}  // namespace memory
