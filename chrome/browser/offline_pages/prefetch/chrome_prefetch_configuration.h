// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_CHROME_PREFETCH_CONFIGURATION_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_CHROME_PREFETCH_CONFIGURATION_H_

#include "base/macros.h"
#include "components/offline_pages/core/prefetch/prefetch_configuration.h"

class PrefService;

namespace offline_pages {

// Chrome implementation of PrefetchConfiguration.
class ChromePrefetchConfiguration : public PrefetchConfiguration {
 public:
  explicit ChromePrefetchConfiguration(PrefService* pref_service);
  ~ChromePrefetchConfiguration() override = default;

  // PrefetchConfiguration implementation.
  bool IsPrefetchingEnabledBySettings() override;

 private:
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(OfflineMetricsCollectorImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_CHROME_PREFETCH_CONFIGURATION_H_
