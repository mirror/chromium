// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEMORY_HEAVY_SITE_CACHE_H_
#define CHROME_BROWSER_MEMORY_HEAVY_SITE_CACHE_H_

#include <memory>
#include <utility>

#include "base/time/clock.h"
#include "components/keyed_service/core/keyed_service.h"

class HostContentSettingsMap;
class GURL;
class Profile;

namespace memory {

class HeavySiteCache : public KeyedService {
 public:
  HeavySiteCache(Profile* profile, size_t max_cache_size);
  ~HeavySiteCache() override;

  void AddSite(const GURL& url);
  void RemoveSite(const GURL& url);
  bool IsSiteHeavy(const GURL& url) const;

  void set_clock_for_testing(std::unique_ptr<base::Clock> clock) {
    clock_ = std::move(clock);
  }

 private:
  void TrimCache();

  // No-op if DCHECK is not enabled.
  void CheckCacheConsistency() const;

  // A clock is injected into this class so tests can set arbitrary timestamps
  // in website settings. This should never be nullptr.
  std::unique_ptr<base::Clock> clock_;

  HostContentSettingsMap* settings_map_ = nullptr;
  size_t max_cache_size_ = 0u;
  size_t cache_size_ = 0u;
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_HEAVY_SITE_CACHE_H_
